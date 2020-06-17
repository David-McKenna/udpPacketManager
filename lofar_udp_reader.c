#include "lofar_udp_misc.h"
#include "lofar_udp_reader.h"


/**
 * @brief      Parse LOFAR UDP headers to determine some metadata about the ports
 *
 * @param      meta    The lofar_udp_meta to initialise
 * @param[in]  header  The header data to process
 *
 * @return     0: Success, 1: Fatal error
 */
int lofar_udp_parse_headers(lofar_udp_meta *meta, const char header[MAX_NUM_PORTS][UDPHDRLEN]) {

	union char_unsigned_int tsseq;
	union char_short source;

	int bitMul;
	int baseLength;
	int bitModeMax= 4;

	// Process each input port
	for (int port = 0; port < meta->numPorts; port++) {
		// Data integrity checks
		if ((unsigned char) header[port][0] < UDPCURVER) {
			fprintf(stderr, "Input header on port %d appears malformed (RSP Version less than 3), exiting.\n", port);
			return 1;
		}

		tsseq.c[0] = header[port][8]; tsseq.c[1] = header[port][9]; tsseq.c[2] = header[port][10];  tsseq.c[3] = header[port][11];
		if (tsseq.ui <  LFREPOCH) {
			fprintf(stderr, "Input header on port %d appears malformed (data timestamp before 2008), exiting.\n", port);
			return 1;
		}

		tsseq .c[0] = header[port][12]; tsseq.c[1] = header[port][13]; tsseq.c[2] = header[port][14]; tsseq.c[3] = header[port][15];
		if (tsseq.ui > RSPMAXSEQ) {
			fprintf(stderr, "Input header on port %d appears malformed (sequence higher than 200MHz clock maximum, %d), exiting.\n", port, tsseq.ui);
			return 1;
		}

		if ((unsigned char) header[port][6] > UDPMAXBEAM) {
			fprintf(stderr, "Input header on port %d appears malformed (more than %d beamlets on a port, %d), exiting.\n", port, UDPMAXBEAM, header[port][6]);
			return 1;
		}

		if ((unsigned char) header[port][7] != 16) {
			fprintf(stderr, "Input header on port %d appears malformed (time slices are %d, not 16), exiting.\n", port, header[port][7]);
			return 1;
		}

		source.c[0] = header[port][1]; source.c[1] = header[port][2];
		if (((lofar_source_bytes*) &source)->padding0 != 0) {
			fprintf(stderr, "Input header on port %d appears malformed (padding bit (0) is set), exiting.\n", port);
			return 1;
		} else if (((lofar_source_bytes*) &source)->errorBit != 0) {
			fprintf(stderr, "Input header on port %d appears malformed (error bit is set), exiting.\n", port);
			return 1;
		} else if (((lofar_source_bytes*) &source)->bitMode == 3) {
			fprintf(stderr, "Input header on port %d appears malformed (BM of 3 doesn't exist), exiting.\n", port);
			return 1;
		} else if (((lofar_source_bytes*) &source)->padding1 > 1) {
			fprintf(stderr, "Input header on port %d appears malformed (padding bits (1) are set), exiting.\n", port);
			return 1;
		} else if (((lofar_source_bytes*) &source)->padding1 == 1) {
			fprintf(stderr, "Input header on port %d appears malformed (our replay packet warning bit is set), continuing with caution...\n", port);
		}


		// Determine the number of beamlets and bitmode on the port
		meta->portBeamlets[port] = (int) header[port][6];
		meta->portCumulativeBeamlets[port] = meta->totalBeamlets;
		meta->totalBeamlets += (int) header[port][6];
		switch (((lofar_source_bytes*) &source)->bitMode) {
			case 0:
				meta->inputBitMode[port] = 16;
				bitModeMax = 16;
				break;

			case 1:
				meta->inputBitMode[port] = 8;
				if (bitModeMax == 4) bitModeMax = 8;
				break;

			case 2:
				meta->inputBitMode[port] = 4;
				// default bitModeMax
				break;

			default:
				fprintf(stderr, "How did we get here? BM=3 should have been caught already...\n");
				return 1;
		}

		// Determine the size of the input array
		// 4-bit: half the size per sample
		// 16-bit: 2x the size per sample
		bitMul = 1 - 0.5 * (meta->inputBitMode[port] == 4) + 1 * (meta->inputBitMode[port] == 16); // 4bit = 0.5x, 16bit = 2x

		baseLength = (int) (meta->portBeamlets[port] * bitMul * UDPNTIMESLICE * UDPNPOL);
		meta->portPacketLength[port] = (int) ((UDPHDRLEN) + baseLength);
		
	}

	return 0;
}


//TODO:
/*
int lofar_udp_skip_to_packet_meta(lofar_udp_reader *reader, long currentPacket, long targetPacket) {
	if (reader->compressedReader) {
		// Standard search method
	} else {
		// fseek... then search
	}
}
*/

/**
 * @brief      If a target packet is set, search for it and align each port with
 *             the target packet as the first packet in the inputData arrays
 *
 * @param      reader  The lofar_udp_reader to process
 *
 * @return     0: Success, 1: Fatal error
 */
int lofar_udp_skip_to_packet(lofar_udp_reader *reader) {
	// This is going to be fun to document...
	long currentPacket, lastPacketOffset, guessPacket, packetDelta, scanning = 0, startOff, nextOff, endOff;
	int packetShift[MAX_NUM_PORTS], nchars, returnLen, returnVal = 0;

	// Initialise the offset shift needed
	for (int port = 0; port < reader->meta->numPorts; port++) packetShift[port] = 0;

	VERBOSE(printf("lofar_udp_skip_to_packet: starting scan...\n"););

	// Scanning across each port,
	for (int port = 0; port < reader->meta->numPorts; port++) {
		// Get the offset to the last packet in the inputData array on a given port
		lastPacketOffset = (reader->meta->packetsPerIteration - 1) * reader->meta->portPacketLength[port];

		// Find the first packet number, determine if our target occurs before the observation started
		currentPacket = lofar_get_packet_number(&(reader->meta->inputData[port][0]));
		if (currentPacket > reader->meta->lastPacket) {
			fprintf(stderr, "Requested packet prior to current observing block (req: %ld, 1st: %ld), exiting.\n", reader->meta->lastPacket, currentPacket);
			return 1;
		}

		VERBOSE(printf("lofar_udp_skip_to_packet: first packet %ld...\n", currentPacket););

		// Find the packet number of the last packet in the inputData array
		currentPacket = lofar_get_packet_number(&(reader->meta->inputData[port][lastPacketOffset]));
		
		VERBOSE(printf("lofar_udp_skip_to_packet: last packet %ld, delta %ld...\n", currentPacket, reader->meta->lastPacket - currentPacket););
		
		// Get the difference between the target packet and the current last packet
		packetDelta = reader->meta->lastPacket - currentPacket;
		// If the current last packet is too low, we'll need to load in more data. Perform this in a loop
		// 	until we are beyond the target packet (so the target packet is in the current array)
		// 	
		// Possible edge case: the packet is in the last slot on port < numPort for the current port, but is
		// 	in the next gulp on another prot due to packet loss. As a result, reading the next packet will cause
		// 	us to lose the target packet on the previous port, artifically pushing the target packet higher later on.
		// 	Far too much work to try fix it, but worth remembering.
		while (currentPacket < reader->meta->lastPacket) {
			VERBOSE(printf("lofar_udp_skip_to_packet: scan at %ld...\n", currentPacket););

			// Set an int so we can update the progress line later
			scanning = 1;

			// Read in a new block of data on all ports, error check
			returnVal = lofar_udp_reader_read_step(reader);
			if (returnVal > 0) return returnVal;

			// Get the new last packet
			currentPacket = lofar_get_packet_number(&(reader->meta->inputData[port][lastPacketOffset]));

			// Print a status update to the CLI
			printf("\rScanning to packet %ld (~%.02f%% complete, currently at packet %ld, %ld to go)", reader->meta->lastPacket, (float) 100.0 -  (float) (reader->meta->lastPacket - currentPacket) / (packetDelta) * 100.0, currentPacket, reader->meta->lastPacket - currentPacket);
			fflush(stdout);
		}
		if (scanning) printf("\33[2K\rReached target packet %ld on port %d.\n", reader->meta->lastPacket, port);
	}
	// Data is now all loaded such that every inputData array has or is past the target packet, now we need to interally align the arrays

	// For each port,
	for (int port = 0; port < reader->meta->numPorts; port++) {

		// Get the current packet, and guess the target packet by assuming no packet loss
		currentPacket = lofar_get_packet_number(&(reader->meta->inputData[port][0]));
		guessPacket = lofar_get_packet_number(&(reader->meta->inputData[port][(reader->meta->lastPacket - currentPacket) * reader->meta->portPacketLength[port]]));

		VERBOSE(printf("lofar_udp_skip_to_packet: searching within current array starting index %ld (max %ld)...\n", (reader->meta->lastPacket - currentPacket) * reader->meta->portPacketLength[port], reader->meta->packetsPerIteration * reader->meta->portPacketLength[port]););
		VERBOSE(printf("lofar_udp_skip_to_packet: meta search: currentGuess %ld, 0th packet %ld, target %ld...\n", guessPacket, currentPacket, reader->meta->lastPacket););

		// If noo packets are dropped, just shift across to the data
		if (guessPacket == reader->meta->lastPacket) {
			packetShift[port] = reader->meta->lastPacket - currentPacket;
			
		// The packet at the index is too high/low, we've had packet loss. Perform a binary-style search until we find the packet
		} else {

			// If the guess packet was too high, reset it to currentPacket so that starting offset is 0.
			// This was moved up 5 lines, please don't be a breaking change...
			if (guessPacket > reader->meta->lastPacket) guessPacket = currentPacket;

			// Starting offset: the different between the 0th packet and the guess packet
			startOff =  guessPacket - currentPacket;

			// End offset: the end of the array
			endOff = reader->meta->packetsPerIteration;

			// By default, we will be shifting by at least the starting offset
			packetShift[port] = nextOff = startOff;

			// Make a new guess at our starting offset
			guessPacket = lofar_get_packet_number(&(reader->meta->inputData[port][packetShift[port] * reader->meta->portPacketLength[port]]));

			// Iterate until we reach a target packet
			while(guessPacket != reader->meta->lastPacket) {
				VERBOSE(printf("lofar_udp_skip_to_packet: meta search: currentGuess %ld, lastGuess %ld, target %ld...\n", guessPacket, lastPacketOffset, reader->meta->lastPacket););
				
				// Update the offsrt, binary search iteration style
				nextOff = (startOff + endOff) / 2;

				// Catch a weird edge case (I have no idea how this was happening, or how to reproduce it anymore)
				if (nextOff > reader->meta->packetsPerIteration) {
					fprintf(stderr, "Error: Unable to converge on solution for first packet on port %d, exiting.\n", port);
					return 1;
				}

				// Find the packet number of the next guess
				guessPacket = lofar_get_packet_number(&(reader->meta->inputData[port][nextOff * reader->meta->portPacketLength[port]]));
				
				// Binary search updates
				if (guessPacket > reader->meta->lastPacket) {
					endOff = nextOff - 1;
				} else if (guessPacket < reader->meta->lastPacket)  {
					startOff = nextOff + 1;
				} else if (guessPacket == reader->meta->lastPacket) {
					continue;
				}

				// If we can't find the packet, shift the indices away and try find the next packet
				if (startOff > endOff) {
					reader->meta->lastPacket += 1;
					startOff -= 20;
					endOff += 20;
				}

			}
			// Set the offset to the final nextOff iteration (or first if we guessed right the first tiem)
			packetShift[port] = nextOff;
		}

		VERBOSE(printf("lofar_udp_skip_to_packet: exited loop, shifting data...\n"););

		// Shift the data on the port as needed
		returnVal = lofar_udp_shift_remainder_packets(reader->meta, packetShift, 0);
		if (returnVal > 0) return 1;

		// Find the amount of data needed, and read in new data to fill the gap left at the end of the array after the shift
		nchars = (reader->meta->packetsPerIteration - packetShift[port]) * reader->meta->portPacketLength[port];
		returnLen = lofar_udp_reader_nchars(reader, port, &(reader->meta->inputData[port][reader->meta->inputDataOffset[port]]), nchars);

		if (nchars != returnLen) {
			fprintf(stderr, "Unable to read enough data to fill first buffer, exiting.\n");
			return 1;
		}

		// Reset the packetShit for the port to 0
		packetShift[port] = 0;
	}

	// Success!
	return 0;
}

/**
 * @brief      Initialises a lofar_udp_reader object based on the inputs. Will
 *             perform the first read operation to align the first packet, but
 *             it will not be processed until lofar_udp_reader_step is called on
 *             the struct.
 *
 * @param      inputFiles        The input files to process
 * @param      meta              The lofar_udp_meta struct to initialise
 * @param[in]  compressedReader  Enable / disable zstandard decompression on the
 *                               input data
 *
 * @return     lofar_udp_reader ptr, or NULL on error
 */
lofar_udp_reader* lofar_udp_file_reader_setup(FILE **inputFiles, lofar_udp_meta *meta, const int compressedReader) {
	int returnVal;
	static lofar_udp_reader reader;

	// Initialise the reader struct as needed
	reader.compressedReader = compressedReader;
	reader.packetsPerIteration = meta->packetsPerIteration;
	reader.meta = meta;

	for (int port = 0; port < meta->numPorts; port++) {
		reader.fileRef[port] = inputFiles[port];
		if (compressedReader) {

			// Setup the stream
			reader.dstream[port] = ZSTD_createDStream();
			ZSTD_initDStream(reader.dstream[port]);

			// Setup the compressed data buffer/struct
			reader.readingTracker[port].size = ZSTD_DStreamInSize() * ZSTD_BUFFERMUL;
			reader.readingTracker[port].pos = reader.readingTracker[port].size;
			reader.inBuffer[port] = calloc(reader.readingTracker[port].size, sizeof(char));
			reader.readingTracker[port].src = reader.inBuffer[port];

			// Setup the decompressed data buffer/struct
			reader.decompressionTracker[port].size = ZSTD_DStreamOutSize() * ZSTD_BUFFERMUL;
			reader.decompressionTracker[port].pos = reader.decompressionTracker[port].size; // Initialisation for our step-by-step reader
			reader.outBuffer[port] = calloc(reader.decompressionTracker[port].size, sizeof(char));
			reader.decompressionTracker[port].dst = reader.outBuffer[port];

			// Change the offset to the end of the array so we can read in a new block after initialisation
			reader.outPtrOffset[port] = reader.decompressionTracker[port].size;
		}
	}

	// Gulp the first set of raw data
	returnVal = lofar_udp_reader_read_step(&reader);
	if (returnVal > 0) return NULL;
	reader.meta->inputDataReady = 0;

	VERBOSE(if (meta->VERBOSE) printf("reader_setup: First packet %ld\n", meta->lastPacket));

	// If we have been given a starting packet, search for it and align the data to it
	if (reader.meta->lastPacket > LFREPOCH) {
		returnVal = lofar_udp_skip_to_packet(&reader);
		if (returnVal > 0) return NULL;
	}

	VERBOSE(if (meta->VERBOSE) printf("reader_setup: Skipped, aligning to %ld\n", meta->lastPacket));

	// Align the first packet on each port, previous search may still have a 1/2 packet delta if there was packet loss
	returnVal = lofar_udp_get_first_packet_alignment(&reader);
	if (returnVal > 0) return NULL;

	reader.meta->inputDataReady = 1;
	return &reader;
}

/**
 * @brief      Re-use a reader on the same input files but targeting a later
 *             timestamp
 *
 * @param      reader          The lofar_udp_reader to re-initialize
 * @param[in]  startingPacket  The starting packet numberfor the next iteration
 *                             (LOFAR pakcet number, not an offset)
 * @param[in]  packetsReadMax  The maxmum number of packets to read in during
 *                             this lifetime of the reader
 *
 * @return     int: 0: Success, < 0: Some data issues, but tolerable: > 1: Fatal
 *             errors
 */
int lofar_udp_file_reader_reuse(lofar_udp_reader *reader, const long startingPacket, const long packetsReadMax) {

	int returnVal = 0;
	// Reset the maximum packets to LONG_MAX if set to an unreasonabl value
	long localMaxPackets = packetsReadMax;
	if (packetsReadMax < 0) localMaxPackets = LONG_MAX;

	// Reset old variables
	reader->meta->packetsPerIteration = reader->packetsPerIteration;
	reader->meta->packetsRead = 0;
	reader->meta->packetsReadMax = localMaxPackets;
	reader->meta->lastPacket = startingPacket;

	for (int port = 0; port < reader->meta->numPorts; port++) {
		reader->meta->inputDataOffset[port] = 0;
		reader->meta->portLastDroppedPackets[port] = 0;
		// reader->meta->portTotalDroppedPackets[port] = 0; // -> maybe keep this for an overall view?
	}

	VERBOSE(if (reader->meta->VERBOSE) printf("reader_setup: First packet %ld\n", reader->meta->lastPacket));

	// Setup to search for the next starting packet
	reader->meta->inputDataReady = 0;
	if (reader->meta->lastPacket > LFREPOCH) {
		returnVal = lofar_udp_skip_to_packet(reader);
		if (returnVal > 0) return returnVal;
	}

	// Align the first packet of each port, previous search may still have a 1/2 packet delta if there was packet loss
	returnVal = lofar_udp_get_first_packet_alignment(reader);
	if (returnVal > 0) return returnVal;

	// Set the reader status as ready to process, but not read in, on next step
	reader->meta->inputDataReady = 1;
	reader->meta->outputDataReady = 0;
	return returnVal;
}


/**
 * @brief      Temporarily read in num bytes from a zstandard compressed file
 *
 * @param      outbuf     The outbuf
 * @param[in]  size       The size
 * @param[in]  num        The number
 * @param      inputFile  The input file
 * @param[in]  resetSeek  The reset seek
 *
 * @return     int 0: ZSTD error, 1: File error, other: data read length
 */
int fread_temp_ZSTD(void *outbuf, const size_t size, int num, FILE* inputFile, const int resetSeek) {
	
	// Build the decompression stream
	ZSTD_DStream* dstreamTmp = ZSTD_createDStream();
	const size_t minRead = ZSTD_DStreamInSize();
	const size_t minOut = ZSTD_DStreamOutSize();
	ZSTD_initDStream(dstreamTmp);

	// Allocate the buffer memory, build the buffer structs
	char* inBuff = calloc(minRead, sizeof(char));
	char* outBuff = calloc(minOut , sizeof(char));

	ZSTD_inBuffer tmpRead = {inBuff, minRead * sizeof(char), 0};
	ZSTD_outBuffer tmpDecom = {outBuff, minOut * sizeof(char), 0};

	// Read in the compressed data
	int readLen = fread(inBuff, sizeof(char), minRead, inputFile);
	if (readLen != (int) minRead) {
		fprintf(stderr, "Unable to read in header from file; exiting.\n");
		return 1;
	}

	// Move the read head back to the start of the packer
	if (resetSeek) fseek(inputFile, -minRead, SEEK_CUR);

	// Decompressed the data, check for errors
	size_t output = ZSTD_decompressStream(dstreamTmp, &tmpDecom , &tmpRead);
	VERBOSE(printf("Header decompression code: %ld, %s\n", output, ZSTD_getErrorName(output)));
	if (ZSTD_isError(output)) {
		printf("ZSTD enountered an error while doing temp read (code %ld, %s), returning 0 data.\n", output, ZSTD_getErrorName(output));
		return 0;
	}
	// Ensure we got enough data
	readLen = tmpDecom.pos;
	if (readLen > num) readLen = num;

	// Copy the output and cleanup
	memcpy(outbuf, outBuff, size * num); 
	ZSTD_freeDStream(dstreamTmp);
	free(inBuff);
	free(outBuff);

	return readLen;

}




/**
 * @brief      Set the processing function. output length per packet based on
 *             the processing mode given by the metadata,
 *
 * @param      meta  The lofar_udp_meta to read from and update
 *
 * @return     0: Success, 1: Unknown processing mode supplied
 */
int lofar_udp_setup_processing(lofar_udp_meta *meta) {
	int hdrOffset = -1 * UDPHDRLEN; // Default: no header, offset by -16
	int equalIO = 0; // If packet length in + out should match
	float mulFactor = 1.0; // Scale packet length linearly
	int workingData = 0;

	// Mass switch statement for each of the different processing modes
	// TODO: will need heavy update after funswitch_loops change
	// TODO: try isolate 8x/16x decimation issue
	// TODO: Account for bit level inputs (4x / 16x)
	switch (meta->processingMode) {
		case 0:
			meta->processFunc = &lofar_udp_raw_udp_copy;
			meta->numOutputs = meta->numPorts;
			hdrOffset = 0; // include header
			equalIO = 1;
			break;
		case 1:
			meta->processFunc = &lofar_udp_raw_udp_copy_nohdr;
			meta->numOutputs = meta->numPorts;
			equalIO = 1;
			break;

		case 2:
			meta->processFunc = &lofar_udp_raw_udp_copy_split_pols;
			meta->numOutputs = UDPNPOL;

			break;


		case 10:
			meta->processFunc = &lofar_udp_raw_udp_reorder;
			meta->numOutputs = 1;
			break;
		case 11:
			meta->processFunc = &lofar_udp_raw_udp_reorder_split_pols;
			meta->numOutputs = UDPNPOL;
			break;


		case 20:
			meta->processFunc = &lofar_udp_raw_udp_reversed;
			meta->numOutputs = 1;
			break;
		case 21:
			meta->processFunc = &lofar_udp_raw_udp_reversed_split_pols;
			meta->numOutputs = UDPNPOL;
			break;

		case 100:
			meta->processFunc = &lofar_udp_raw_udp_stokesI;
			meta->numOutputs = 1;
			mulFactor = sizeof(float) / (4 * sizeof(char));
			break;
		case 101:
			meta->processFunc = &lofar_udp_raw_udp_stokesV;
			meta->numOutputs = 1;
			mulFactor = sizeof(float) / (4 * sizeof(char));
			break;


		case 120:
			meta->processFunc = &lofar_udp_raw_udp_stokesI_sum16;
			meta->numOutputs = 1;
			mulFactor = (float) (sizeof(float) / (4 * sizeof(char))) / 16.0;
			break;

		default:
			fprintf(stderr, "Unknown processing mode %d, exiting...\n", meta->processingMode);
			return 1;


	}

	if (equalIO) {
		for (int port = 0; port < meta->numPorts; port++) {
			meta->packetOutputLength[port] = hdrOffset + meta->portPacketLength[port];
		}
	} else {
		for (int port = 0; port < meta->numPorts; port++) workingData += hdrOffset + meta->portPacketLength[port];
		workingData =  (int) workingData * mulFactor;
		workingData /= meta->numOutputs;

		for (int out = 0; out < meta->numOutputs; out++ ) { 
			meta->packetOutputLength[out] = workingData;
		}
	}

	return 0;
}

/**
 * @brief      Set up a lofar_udp_reader and assoaicted lofar_udp_meta using a
 *             set of input files and pre-set control/metadata parameters
 *
 * @param      inputFiles            An array of input files (max MAX_NUM_PORTS)
 * @param[in]  numPorts              The number ports to process (max MAX_NUM_PORTS)
 * @param[in]  replayDroppedPackets  1: Enable 0: Disable replaying the last packet (otherwise pad with 0s)
 * @param[in]  processingMode        The processing mode
 * @param[in]  verbose               1: Enable verbosity, 0: Disable verbosity
 *                                   (only if enabled at compile time)
 * @param[in]  packetsPerIteration   The packets per read/process iteration
 * @param[in]  startingPacket        The starting packet number (LOFAR packet number, not an offset)
 * @param[in]  packetsReadMax        The maximum number of packets to read over the lifetime of the object
 * @param[in]  compressedReader      Enable / disable zstandard decompression on the input data
 *
 * @return     lofar_udp_reader ptr, or NULL on error
 */
lofar_udp_reader* lofar_udp_meta_file_reader_setup(FILE **inputFiles, const int numPorts, const int replayDroppedPackets, const int processingMode, const int verbose, const long packetsPerIteration, const long startingPacket, const long packetsReadMax, const int compressedReader) {
	
	if (numPorts > MAX_NUM_PORTS) {
		fprintf(stderr, "ERROR: You requested %d ports, but LOFAR can only produce %d, exiting.\n", numPorts, MAX_NUM_PORTS);
		return NULL;
	}
	if (packetsPerIteration < 1) {
		fprintf(stderr, "ERROR: Packets per iteration indicates no work will be performed (%ld per iteration), exiting.\n", packetsPerIteration);
		return NULL;
	}

	// Setup the metadata struct and a few variables we'll need
	static lofar_udp_meta meta = { .processingMode = 0, .packetsRead = 0, .inputDataReady = 0, .outputDataReady = 0 }; // .outputBitMode
	char inputHeaders[MAX_NUM_PORTS][UDPHDRLEN];
	int readlen;
	long localMaxPackets = packetsReadMax;

	// Reset the maximum packets to LONG_MAX if set to an unreasonable value
	if (packetsReadMax < 0) localMaxPackets = LONG_MAX;

	// Set the simple metadata defaults
	meta.numPorts = numPorts;
	meta.replayDroppedPackets = replayDroppedPackets;
	meta.processingMode = processingMode;
	meta.packetsPerIteration = packetsPerIteration;
	meta.packetsReadMax = localMaxPackets;
	meta.lastPacket = startingPacket;
	
	VERBOSE(meta.VERBOSE = verbose);
	#ifndef ALLOW_VERBOSE
	if (verbose) fprintf(stderr, "Warning: verbosity was disabled at compile time, but you requested it. Continuing...\n");
	#endif


	// Scan in the first header on each port
	for (int port = 0; port < numPorts; port++) {
		
		if (compressedReader)  {
			readlen = fread_temp_ZSTD(&(inputHeaders[port]), sizeof(char), 16, inputFiles[port], 1);
		} else  {
			readlen = fread(&(inputHeaders[port]), sizeof(char), 16, inputFiles[port]);
			fseek(inputFiles[port], -16, SEEK_CUR);
		}

		if (readlen != 16) {
			fprintf(stderr, "Unable to read header on port %d, exiting.\n", port);
			return NULL;
		}

	}


	// Parse the input file headers to get packet metadata on each port
	if (lofar_udp_parse_headers(&meta, inputHeaders)) {
		fprintf(stderr, "Unable to setup meadata using given headers; exiting.\n");
		return NULL;
	}

	if (lofar_udp_setup_processing(&meta)) {
		fprintf(stderr, "Unable to setup processing mode %d, exiting.\n", processingMode);
		return NULL;
	}


	// Allocate the memory needed to store the raw / reprocessed data, initlaise the variables that are stored on a per-port basis.
	for (int port = 0; port < meta.numPorts; port++) {
		// Ofset input by 2 for a zero/buffer packet on boundary
		meta.inputData[port] = calloc(meta.portPacketLength[port] * (meta.packetsPerIteration + 2), sizeof(char)) + (meta.portPacketLength[port] * 2);

		// Initalise these arrays while we're looping
		meta.inputDataOffset[port] = 0;
		meta.portLastDroppedPackets[port] = 0;
		meta.portTotalDroppedPackets[port] = 0;
	}

	for (int out = 0; out < meta.numOutputs; out++) {
		meta.outputData[out] = calloc(meta.packetOutputLength[out] * meta.packetsPerIteration, sizeof(char));
	}



	VERBOSE(if (meta.VERBOSE) {
		printf("Meta debug:\ntotalBeamlets %d, numPorts %d, replayDroppedPackets %d, processingMode %d, outputBitMode %d, packetsPerIteration %ld, packetsRead %ld, packetsReadMax %ld, lastPacket %ld, \n",
				meta.totalBeamlets, meta.numPorts, meta.replayDroppedPackets, meta.processingMode, 0, meta.packetsPerIteration, meta.packetsRead, meta.packetsReadMax, meta.lastPacket);

		for (int i = 0; i < meta.numPorts; i++) {
			printf("Port %d: inputDataOffset %ld, portBeamlets %d, inputBitMode %d, portPacketLength %d, packetOutputLength %d, portLastDroppedPackets %d, portTotalDroppedPackets %d\n", i, 
				meta.inputDataOffset[i], meta.portBeamlets[i], meta.inputBitMode[i], meta.portPacketLength[i], meta.packetOutputLength[i], meta.portLastDroppedPackets[i], meta.portTotalDroppedPackets[i]);

		for (int i = 0; i < meta.numOutputs; i++) printf("Output %d, packetLength %d, numOut %d", i, meta.packetOutputLength[i], meta.numOutputs);
	}});


	// Form a reader using the given metadata and input files
	return lofar_udp_file_reader_setup(inputFiles, &meta, compressedReader);
}


/**
 * @brief      Close input files, free alloc'd memory, free zstd decompression
 *             streams once we are finished.
 *
 * @param[in]  reader  The lofar_udp_reader struct to cleanup
 *
 * @return     int: 0: Success, other: ???
 */
int lofar_udp_reader_cleanup(const lofar_udp_reader *reader) {

	// Cleanup the malloc/calloc'd memory addresses, close the input files.
	// TODO: detach output files into it's own loop
	for (int i = 0; i < reader->meta->numOutputs; i++) free(reader->meta->outputData[i]);

	for (int i = 0; i < reader->meta->numPorts; i++) {
		// Free input data pointer (from the correct offset)
		VERBOSE(if(reader->meta->VERBOSE) printf("Freeing inputData, outputData, closing file on port %d\n", i););
		free(reader->meta->inputData[i] - 2 * reader->meta->portPacketLength[i]);
		// Close the input file
		fclose(reader->fileRef[i]);

		if (reader->compressedReader) {
			// Free the decomression stream
			VERBOSE(if(reader->meta->VERBOSE) printf("Freeing decompression buffers and ZSTD stream on port %d\n", i););
			ZSTD_freeDStream(reader->dstream[i]);
			// Free the decompression input/output buffers
			free(reader->inBuffer[i]);
			free(reader->outBuffer[i]);
		}

	}

	return 0;
}


/**
 * @brief      Read a set amount of data to a given pointer on a given port.
 *             Supports standard files and decompressed files when
 *             reader->compressedReader == 1
 *
 * @param      reader       The lofar_udp_reader struct to process
 * @param[in]  port         The port (file) to read data from
 * @param      targetArray  The storage array
 * @param[in]  nchars       The number of chars (bytes) to read in
 *
 * @return     long: bytes read
 */
long lofar_udp_reader_nchars(lofar_udp_reader *reader, const int port, char *targetArray, const long nchars) {
	// Return if we have nothing to do
	if (nchars < 0) return -1;

	if (reader->compressedReader) {
		// Compressed file: Perform streaming decompression on a zstandard compressed file
		VERBOSE(if (reader->meta->VERBOSE) printf("reader_nchars: Entering read request (compressed): %d, %ld\n", port, nchars));

		long dataRead = 0;
		int compDataRead = 0, byteDelta = 0, returnVal = 0;
		// ZSTD decompress + pass along data to normal input channel

		// Loop until we hit an exit criteria (EOF / zstd error / nchars)
		while(1) {

			// Check decompression buffer for remanants of the last read operation
			if (reader->decompressionTracker[port].pos != 0) {
				VERBOSE(if (reader->meta->VERBOSE) printf("reader_nchars: Compressed data in cache: %d, %ld\n", port, reader->decompressionTracker[port].pos));
				VERBOSE(if (reader->meta->VERBOSE) printf("reader_nchars: Compressed %d %ld/%ld: %ld %ld %ld %ld %ld\n", port, dataRead, nchars, reader->decompressionTracker[port].pos, reader->decompressionTracker[port].size, reader->readingTracker[port].pos, reader->readingTracker[port].size, reader->outPtrOffset[port]));

				// If the amount of requested data is greater than the amount of data in the buffer
				if ((unsigned int) nchars > (reader->decompressionTracker[port].pos - reader->outPtrOffset[port])) {
					// Read the remainder of the buffer
					byteDelta = reader->decompressionTracker[port].pos - reader->outPtrOffset[port];
				} else {
					// Read only what we need
					byteDelta = nchars;
				}

				// Copy over the required data, update our progress and offsets
				VERBOSE(if (reader->meta->VERBOSE) printf("reader_nchars: Cache copy: %d, %ld->%ld\n", port, dataRead, reader->decompressionTracker[port].pos));
				memcpy(&(targetArray[dataRead]), &(reader->outBuffer[port][reader->outPtrOffset[port]]), byteDelta);
				dataRead += byteDelta;
				reader->outPtrOffset[port] += byteDelta;
				if (reader->outPtrOffset[port] == reader->decompressionTracker[port].pos) reader->outPtrOffset[port] = reader->decompressionTracker[port].pos = 0;
				VERBOSE(if (reader->meta->VERBOSE) printf("reader_nchars: Compressed data cache ptrs: %d, %ld, %ld\n", port, reader->decompressionTracker[port].pos, reader->outPtrOffset[port]));
				
				// Return if we have all the data we need
				if (dataRead == nchars) return dataRead;
			}

			// Check the decompression stream for compressed data
			if (reader->readingTracker[port].pos != reader->readingTracker[port].size) {
				// Loop across while decompressing the data (zstd decompressed in frame iterations, so it may take a few iterations)
				while (reader->readingTracker[port].pos < reader->readingTracker[port].size) {

					// zstd streaming decompression + check for errors
					returnVal = ZSTD_decompressStream(reader->dstream[port], &(reader->decompressionTracker[port]), &(reader->readingTracker[port]));
					if (ZSTD_isError(returnVal)) {
						fprintf(stderr, "ZSTD encountered an error decompressing a frame (code %d, %s), exiting data read early.\n", returnVal, ZSTD_getErrorName(returnVal));
						return dataRead;
					}

					// Determine how much data we need to copy from the buffer
					if ((dataRead + reader->decompressionTracker[port].pos) > (unsigned int) nchars) byteDelta = nchars - dataRead;
					else {
						byteDelta = reader->decompressionTracker[port].pos;
						// Update the decompression tracker to signify that we have used all the data
						reader->decompressionTracker[port].pos = 0;
					}

					// Copy the data to the output array, update the decompressed buffer pointer and pos pointer
					memcpy(&(targetArray[dataRead]), reader->outBuffer[port], byteDelta);
					reader->outPtrOffset[port] = byteDelta;

					// Update the total data read + check if we have reached our goal
					dataRead += byteDelta;
					if (dataRead == nchars) return dataRead;


				}
			}

			// Check if the input buffer has been fully used
			if (reader->readingTracker[port].pos == reader->readingTracker[port].size) {
				// Read in compressed data
				compDataRead = fread(&(reader->inBuffer[port][0]), sizeof(char), reader->readingTracker[port].size, reader->fileRef[port]);

				// If we loop back around after hitting EOF, exit
				if (compDataRead == 0)  {
					fprintf(stderr, "Unable to read sufficient data from compressed archive for port %d.\n", port);
					return dataRead;
				}

				// If we don't read in enough data (also EOF)
				if ((unsigned int)  compDataRead != reader->readingTracker[port].size) {
					reader->readingTracker[port].size = compDataRead;
				}

				// Reset the input tracket point to signal new data
				reader->readingTracker[port].pos = 0;
			}

			// Repeat
		}

		// Should be unreachable, error if reached.
		return 0;
	} else {
		// Decompressed file: just read the data lol
		VERBOSE(if (reader->meta->VERBOSE) printf("reader_nchars: Entering read request: %d, %ld\n", port, nchars));

		return fread(targetArray, sizeof(char), nchars, reader->fileRef[port]);
	}
}


/**
 * @brief      Attempt to fill the reader->meta->inputData buffers with new
 *             data. Performs a shift on the last N packets of a given port if
 *             there was packet loss on the last iteration (so the packets were
 *             not used, allowing us to use them on the next iteration)
 *
 * @param      reader  The input lofar_udp_reader struct to process
 *
 * @return     int: 0: Success, -2: Final iteration, reached packet cap, -3:
 *             Final iteration, received less data than requested (EOF)
 */
int lofar_udp_reader_read_step(lofar_udp_reader *reader) {
	int returnVal = 0;
	int checkReturnValue = 0;

	// Make sure we have work to perform
	if (reader->meta->packetsPerIteration == 0) {
		fprintf(stderr, "Last packets per iteration was 0, there is no work to perform, exiting...\n");
		return 1;
	}

	// Reset the packets per iteration to the intended length (can be lowered due to out of order packets)
	reader->meta->packetsPerIteration = reader->packetsPerIteration;

	// Ensure we aren't passed the read length cap
	if (reader->meta->packetsRead >= (reader->meta->packetsReadMax - reader->meta->packetsPerIteration)) {
		VERBOSE(if(reader->meta->VERBOSE) printf("Processing final read (%ld packets) before reaching maximum packet cap.\n", reader->meta->packetsPerIteration));
		returnVal = -2;
	}

	// If packets were dropped, shift the remaining packets back to the start of the array
	if ((checkReturnValue = lofar_udp_shift_remainder_packets(reader->meta, reader->meta->portLastDroppedPackets, 1)) > 0) return 1;
	//else if (checkReturnValue < 0) if(lofar_udp_realign_data(reader) > 0) return 1;
	
	// Read in the required new data
	omp_set_num_threads(OMP_THREADS);
	#pragma omp parallel for 
	for (int port = 0; port < reader->meta->numPorts; port++) {
		long charsToRead, charsRead, packetPerIter;
		
		// Determine how much data is needed and read-in to the offset after any leftover packets
		charsToRead = (reader->meta->packetsPerIteration - reader->meta->portLastDroppedPackets[port]) * reader->meta->portPacketLength[port];
		charsRead = lofar_udp_reader_nchars(reader, port, &(reader->meta->inputData[port][reader->meta->inputDataOffset[port]]), charsToRead);

		// Raise a warning if we received less data than requested (EOF/file error)
		if (charsRead < charsToRead) {

			packetPerIter = (charsRead / reader->meta->portPacketLength[port]) + reader->meta->portLastDroppedPackets[port];
			#pragma omp critical (packetCheck) 
			{
			if(packetPerIter < reader->meta->packetsPerIteration) {
				reader->meta->packetsPerIteration = packetPerIter;
				fprintf(stderr, "Received less data from file on port %d than expected, may be nearing end of file.\nReducing packetsPerIteration to %ld, to account for the limited amount of input data.\n", port, reader->meta->packetsPerIteration);

			}
			#ifdef __SLOWDOWN
			sleep(5);
			#endif

			returnVal = -3;
			}
		}
	}

	// Mark the input data are ready to be processed
	reader->meta->inputDataReady = 1;
	return returnVal;
}


/**
 * @brief      Perform a read/process step, without any timing.
 *
 * @param      reader  The input lofar_udp_reader struct to process
 * @param      timing  A length of 2 double array to store (I/O, Memops) timing
 *                     data
 *
 * @return     int: 0: Success, < 0: Some data issues, but tolerable: > 1: Fatal
 *             errors
 */
int lofar_udp_reader_step_timed(lofar_udp_reader *reader, double timing[2]) {
	int readReturnVal = 0, stepReturnVal = 0;
	struct timespec tick0, tick1, tock0, tock1;
	const int time = !(timing[0] == -1.0);

	// Start the reader clock
	if (time) clock_gettime(CLOCK_MONOTONIC_RAW, &tick0);

	VERBOSE(if (reader->meta->VERBOSE) printf("reader_step ready: %d, %d\n", reader->meta->inputDataReady, reader->meta->outputDataReady));
	// Check if the data states are ready for a new gulp
	if (reader->meta->inputDataReady != 1 && reader->meta->outputDataReady != 0) {
		if ((readReturnVal = lofar_udp_reader_read_step(reader)) > 0) return readReturnVal;
		reader->meta->leadingPacket = reader->meta->lastPacket + 1;
		reader->meta->outputDataReady = 0;
	}


	// End the I/O timer. start the processing timer
	if (time) { 
		clock_gettime(CLOCK_MONOTONIC_RAW, &tock0);
		clock_gettime(CLOCK_MONOTONIC_RAW, &tick1);
	}


	VERBOSE(if (reader->meta->VERBOSE) printf("reader_step ready2: %d, %d\n", reader->meta->inputDataReady, reader->meta->outputDataReady));
	// Make sure there is a new input data set before running
	// On the setup iteration, the output data is marked as ready to prevent this occurring until the first read step is called
	if (reader->meta->outputDataReady != 1) {
		if ((stepReturnVal = (reader->meta->processFunc)(reader->meta)) > 0) return stepReturnVal;
		reader->meta->packetsRead += reader->meta->packetsPerIteration;
		reader->meta->inputDataReady = 0;
	}


	// Stop the processing timer, save off the values to the provided array
	if (time) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &tock1);
		timing[0] = TICKTOCK(tick0, tock0);
		timing[1] = TICKTOCK(tick1, tock1);
	}

	// Reurn the "worst" calue.
	if (readReturnVal < stepReturnVal) return readReturnVal;
	return stepReturnVal;
}

/**
 * @brief      Perform a read/process step, without any timing.
 *
 * @param      reader  The input lofar_udp_reader struct to process
 *
 * @return     int: 0: Success, < 0: Some data issues, but tolerable: > 1: Fatal errors
 */
int lofar_udp_reader_step(lofar_udp_reader *reader) {
	double fakeTiming[2] = {-1.0, 0};

	return lofar_udp_reader_step_timed(reader, fakeTiming);
}


/**
 * @brief      Align each of the ports so that they start on the same packet
 *             number. This works on the assumption the packet different is less
 *             than the input array length. This function will read in more data
 *             to ensure the entire array is full after the shift process.
 *
 * @param      reader  The input lofar_udp_reader struct to process
 *
 * @return     0: Success, 1: Fatal error
 */
int lofar_udp_get_first_packet_alignment(lofar_udp_reader *reader) {

	// Align the data to the same packet on each port
	int returnVal = lofar_udp_get_first_packet_alignment_meta(reader->meta);
	int shiftPackets[MAX_NUM_PORTS], returnLen;
	long nchars;

	VERBOSE(if (reader->meta->VERBOSE) printf("first_pkt_align: returnVal %d\n", returnVal));
	
	// Update the data status if we haven't had a majour error
	if (returnVal != 1) reader->meta->inputDataReady = 1;
	
	// If the ports were not already aligned, read in new data.
	if (returnVal < 0) {
		VERBOSE(if (reader->meta->VERBOSE) printf("Entering fixup loop\n"));

		// Calculate the offset for each port
		for (int port = 0; port < reader->meta->numPorts; port++) shiftPackets[port] = reader->meta->packetsPerIteration - reader->meta->portLastDroppedPackets[port];

		// Perform the shift operation, return if no shift occured
		returnVal = lofar_udp_shift_remainder_packets(reader->meta, shiftPackets, 0);
		VERBOSE(if (reader->meta->VERBOSE) printf("Packets shifted, returnVal %d\n", returnVal));
		if (returnVal > 0) return returnVal;

		// Read in new data to the ports that required it, could probably `pragma omp parallel for` this
		for (int port = 0; port < reader->meta->numPorts; port++) {
			VERBOSE(if (reader->meta->VERBOSE) printf("Entering post-fixup loop port %d, dropped %d\n", port, reader->meta->portLastDroppedPackets[port]));
			
			// Check if a port needs to be topped up
			if (reader->meta->portLastDroppedPackets[port] > 0) {
				// Determine how much data is needed and read it in
				nchars = reader->meta->portLastDroppedPackets[port] * reader->meta->portPacketLength[port];
				returnLen = lofar_udp_reader_nchars(reader, port, &(reader->meta->inputData[port][reader->meta->inputDataOffset[port]]), nchars);
				
				// Make sure we got the right amount of data back
				if (returnLen != nchars) {
					fprintf(stderr, "Unable to read in enough data for first packet alignment at given data length, exiting.\n");
					reader->meta->inputDataReady = 0;
					return 1;
				}
			
			}

			// Reset the data offset to 0
			reader->meta->inputDataOffset[port] = 0;
		}
		VERBOSE(if (reader->meta->VERBOSE) printf("Exitint fixup loop\n"));
	} else return returnVal;

	return 0;
}


/**
 * @brief      Align each of the ports so that they start on the same packet
 *             number. This works on the assumption the packet different is less
 *             than the input array length
 *
 * @param      meta  The input lofar_udp_meta struct to process
 *
 * @return     int 0: Success, -1: Some port shifting required, 1: Unable to
 *             align data
 */
int lofar_udp_get_first_packet_alignment_meta(lofar_udp_meta *meta) {

	// If we only have 1 ports, just return.
	//if (meta->numPorts < 2) return 0;
	int returnValue = 0;

	long portStartingPacket[MAX_NUM_PORTS];
	long maxPacketNumber = -1;
	long maxIndex[MAX_NUM_PORTS];

	long portDelta, trueDelta = 0, currentPacketNumber;

	// Determine the maximum port packet number, update the variables as neded
	for (int port = 0; port < meta->numPorts; port++) {
			// Initialise/Reset the dropped packet counters
			meta->portLastDroppedPackets[port] = 0;
			meta->portTotalDroppedPackets[port] = 0;
			maxIndex[port] = meta->packetsPerIteration * meta->portPacketLength[port];
			portStartingPacket[port] = lofar_get_packet_number(meta->inputData[port]);

			// Check if the maxPacketNumber needs to be updated
			if (portStartingPacket[port] > maxPacketNumber) maxPacketNumber = portStartingPacket[port]; 

	}

	// Across each port,
	for (int port = 0; port < meta->numPorts; port++) {
		// Find the amount of packets from the maximum
		portDelta = (int) (maxPacketNumber - portStartingPacket[port]);
		VERBOSE(if (meta->VERBOSE) printf("first_pkt_meta: port %d port delta %ld\n", port, portDelta));


		// Check if a port doesn't have the expected packet number
		if (portDelta > 0) {
			// Change the return int to -1, count the number of packets that need to be shifted as a result
			returnValue = -1;
			trueDelta = 0;

			// Iterate up to the expected packet loss
			for (int i = meta->portLastDroppedPackets[port]; i < portDelta; i++) {

				// Check if i is beyond the number of gulp'd packets, update the returnValue, break
				if (i > maxIndex[port]) {
					returnValue = 1;
					break;
				}

				// Check the current packet number relative to the tagret number
				currentPacketNumber = lofar_get_packet_number(&(meta->inputData[port][meta->portPacketLength[port] * i]));
				if (currentPacketNumber < maxPacketNumber) {
					// If less than, count a packet that needs to be skipped
					trueDelta += 1;
				} else if (currentPacketNumber == maxPacketNumber) {
					// If equal to, count the packet and break the loop
					trueDelta += 1;
					continue;
				} else { // currentPacketNumber > maxPacketNumber
					// If we skip the target packet, set a new target and break the loop, reset port to -1 (0 on ++)
					maxPacketNumber = currentPacketNumber;
					port = -1;
					break;
				}

			}

			// Return if we broke because we ran out of data
			if (returnValue == 1) return returnValue;
			// Update the number of 'dropped' packets
			meta->portLastDroppedPackets[port] += trueDelta;
		}
	}

	// Set the lastPacket variable
	meta->lastPacket = maxPacketNumber -1;
	meta->leadingPacket = maxPacketNumber;

	return returnValue;
}


// Abandoned attempt to search for a header if data becomes corrupted / misaligned. Did not work.
/*
int lofar_udp_realign_data(lofar_udp_reader *reader) {
	int *droppedPackets = reader->meta->portLastDroppedPackets;

	char referenceData[UDPHDRLEN];
	int idx, status = 0, offset = 1;
	int targetDropped = 0;
	int returnVal = 1;
	for (int port = 0; port < reader->meta->numPorts; port++) {
		if (reader->meta->portLastDroppedPackets[port] == targetDropped) {
			// Get the timestamp and sequence from the last packet on a good port
			for (int i = 0; i < UDPHDRLEN; i++) referenceData[i] = reader->meta->inputData[port][-1 * reader->meta->portPacketLength[port] + i];
			break;

		}

		if (targetDropped == reader->meta->packetsPerIteration) {
			fprintf(stderr, "Data integrity comprimised on all ports, exiting.\n");
			return 1;
		}
		if (port == reader->meta->numPorts - 1) targetDropped++;
	}

	// Assumption: same clock used on both ports
	long referencePacket = lofar_get_packet_number(referenceData);
	long workingPkt;
	long workingBlockLength;
	for (int port = 0; port < reader->meta->numPorts; port++) {
		if (droppedPackets[port] < 0) {
			workingBlockLength = reader->meta->packetsPerIteration * reader->meta->portPacketLength[port];
			fprintf(stderr, "Data integrity issue on port %d, attempting to realign data...\n", port);
			// Get data from the last packet on the previous port iteration
			for (int i = 0; i < 8; i++) referenceData[i] = reader->meta->inputData[port][-1 * reader->meta->portPacketLength[port] + i];

			idx = (reader->meta->packetsPerIteration) * reader->meta->portPacketLength[port] - 16;
			while (returnVal) {
				idx -= offset;

				if (offset < 0) {
					fprintf(stderr, "Unable to realign data on port %d, exiting.\n", port);
					return 1;
				} else if (offset > workingBlockLength) {
					fprintf(stderr, "Reading in new block of data for search on port %d...\n", port);
					returnVal = lofar_udp_reader_nchars(reader, port, reader->meta->inputData[port], workingBlockLength);
					if (returnVal != reader->meta->packetsPerIteration * reader->meta->portPacketLength[port]) {
						fprintf(stderr, "Reached end of file before aligning data, exiting...\n");
						return 1;
					}
					idx = workingBlockLength;
				}

				if (status == 0) {
					if (memcmp(referenceData, &(reader->meta->inputData[port][idx]), 8)) {
						status = 1;
						offset = 0;
					} else continue;
				} else {
					workingPkt = lofar_get_packet_number(&(reader->meta->inputData[port][idx]));
					printf("Scanning: %ld, %ld\n", workingPkt, referencePacket);
					// If the packet difference is significant, assume we matched on noise, start search again.
					if (workingPkt > referencePacket + 200 * reader->meta->packetsPerIteration) {
						status = 0;
						offset = 1;

					} else if (workingPkt == referencePacket) {
						fprintf(stderr, "Realigned data on port %d.\n", port);
						memcpy(&(reader->meta->inputData[port][-1 * reader->meta->portPacketLength[port]]), &(reader->meta->inputData[port][idx]), reader->meta->portPacketLength[port] * reader->meta->packetsPerIteration - idx);
						returnVal = 0;
						reader->meta->inputDataOffset[port] = reader->meta->portPacketLength[port] * (reader->meta->packetsPerIteration - 1) - idx;

					} else if (workingPkt > referencePacket) {
						if (status == 3) {
							referencePacket += 1;
						}
						status = 2;
						offset = reader->meta->portPacketLength[port];
						continue;
					} else { //if (workingPkt < reference)
						if (status == 2) {
							referencePacket += 1;
						}
						status = 3;
						offset = -1 *  reader->meta->portPacketLength[port];
						continue;
					}
				}
			}

		}
	}

	return returnVal;
}
*/



/**
 * @brief      Shift the last N packets of data in a given port's array back to
 *             the start; mainly used to handle the case of packet loss without
 *             re-reading the data (read: zstd streaming is hell)
 *
 * @param      meta           The input lofar_udp_meta struct to process
 * @param      shiftPackets   [PORTS] int array of number of packets to shift
 *                            from the tail of each port by
 * @param[in]  handlePadding  Allow the function to handle copying the last
 *                            packet to the padding offset
 *
 * @return     int: 0: Success, -1: Negative shift requested, out
 *             of order data on last gulp,
 */
int lofar_udp_shift_remainder_packets(lofar_udp_meta *meta, const int shiftPackets[], const int handlePadding) {

	int returnVal = 0;
	int packetShift, destOffset, portPacketLength;
	long byteShift, sourceOffset;

	char *inputData;
	int totalShift = 0;
	

	// Check if we have any work to do, reset offsets, exit if no work requested
	for (int port = 0; port < meta->numPorts; port++) {
		meta->inputDataOffset[port] = 0;
		totalShift += shiftPackets[port];
	}

	if (totalShift < 1) return 0;


	// Shift the data on each port
	for (int port = 0; port < meta->numPorts; port++) {
		inputData = meta->inputData[port];
		packetShift = shiftPackets[port];


		VERBOSE(if (meta->VERBOSE) printf("shift_remainder: Port %d packet shift %d padding %d\n", port, packetShift, handlePadding));

 		// If we have packet shift or want to copy the final packet for future refernece
		if (packetShift > 0 || handlePadding == 1) {

			// Negative shift -> negative packet loss -> out of order data -> do nothing, we'll  drop a few packets and resume work
			if (packetShift < 0) {
				fprintf(stderr, "Requested shift on port %d is negative (%d);", port, packetShift);
				if (packetShift <-5) fprintf(stderr, " this is an indication of data integrity issues. Be careful with outputs from this dataset.\n");
				else fprintf(stderr, " attempting to continue...\n");
				returnVal = -1;
				meta->inputDataOffset[port] = 0;
				if (handlePadding == 0) continue;
				packetShift = 0;
			}

			portPacketLength = meta->portPacketLength[port];

			// Calcualte the origin + destination indices, and the amount of data to shift
			sourceOffset = meta->portPacketLength[port] * (meta->packetsPerIteration - packetShift - handlePadding); // Verify -1...
			destOffset = -1 * portPacketLength * handlePadding;
			byteShift = sizeof(char) * (packetShift + handlePadding) * portPacketLength;

			VERBOSE(if (meta->VERBOSE) printf("SO: %ld, DO: %d, BS: %ld IDO: %ld\n", sourceOffset, destOffset, byteShift, destOffset + byteShift));

			// Mmemove the data as needed (memcpy can't act on the same array)
			memmove(&(inputData[destOffset]), &(inputData[sourceOffset]), byteShift);

			// Reset the 0-valued buffer to wipe any time/sequence data remaining
			if (!meta->replayDroppedPackets) {
				sourceOffset = -2 * portPacketLength;
				for (int i = 0; i < portPacketLength; i++) inputData[sourceOffset + i] = 0;
			}
			// Update the inputDataOffset variable to point to the new  'head' of the array, including the new offset
			meta->inputDataOffset[port] = destOffset + byteShift;
			VERBOSE( if(meta->VERBOSE) printf("shift_remainder: Final data offset %d: %ld\n", port, meta->inputDataOffset[port]));
		}

		VERBOSE( if (meta->VERBOSE) printf("shift_remainder: Port %d end offset: %ld\n", port, meta->inputDataOffset[port]););
	}

	return returnVal;
}


// The next N lines are an abomination; it should be possible to use -funswitch-loops to reduce this to 3/4 copies of the same function rather than 10+




/*
 * @brief      Keep the same UDP hdrd/data/port structure, but pad for missing
 *             packets.
 *
 * @param      meta  The input lofar_udp_meta struct to process
 *
 * @return     int: 0: Success, 1: Large amount of out-of-ordered data, -1: Some
 *             packet loss
 */
int lofar_udp_raw_udp_copy(lofar_udp_meta *meta) {
	// Setup return variable
	int packetLoss = 0;
	// Setup working variables
	
	VERBOSE(const int verbose = meta->VERBOSE);
	
	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	// For each port of data provided,
	omp_set_num_threads(OMP_THREADS);
	#pragma omp parallel for 
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (verbose) printf("Port: %d on thread %d\n", port, omp_get_thread_num()));

		int nextSequence;
		long lastPortPacket, currentPortPacket, inputPacketOffset, outputPacketOffset, lastInputPacketOffset, iWork, iLoop;
		// Reset the dropped packets counter
		meta->portLastDroppedPackets[port] = 0;
		int currentPacketsDropped = 0;

		// Reset last packet, reference data on the current port
		lastPortPacket = meta->lastPacket;
		char  *inputPortData = meta->inputData[port];
		char  *outputData = meta->outputData[port];

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int portPacketLength = meta->portPacketLength[port];
		lastInputPacketOffset = (-2 + meta->replayDroppedPackets) * portPacketLength; 	// We request at least 2 packets are malloc'd before the array head pointer, so no SEGFAULTs here
														// -2 * PPL = 0s -1 * PPL = last processed packet -- used for calcuating offset in dropped case if 
														// 	we have packet loss on the boundary.
		VERBOSE(if (verbose) printf("LPP: %ld, PPL: %d, LIPO: %ld\n", lastPortPacket, portPacketLength, lastInputPacketOffset));
		// iWork -- input data offset calculations (account for dropped packets)
		// iLoop -- output data offsets
		// These are the same if there is no packet loss.
		// 
		// Reset iWork, inputPacketOffset, read in the first packet's number.
		iWork = 0, inputPacketOffset = 0;
		currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
		VERBOSE(if (verbose) printf("Packet 0, port %d: %ld\n", port, currentPortPacket));
		
		for (iLoop = 0; iLoop < packetsPerIteration; iLoop++) {
			//VERBOSE(if (verbose) printf("Loop %ld, Work %ld, packet %ld, target %ld\n", iLoop, iWork, currentPortPacket, lastPortPacket + 1));

			// Check for packet loss by ensuring we have sequential packet numbers
			if (currentPortPacket != (lastPortPacket + 1)) {
				// Check if a packet is out of order; if so, drop the packet
				// TODO: Better future option: check if the packet is in this block, copy and overwrite padded packed instead
				VERBOSE(if (verbose) printf("Packet %ld is not the expected packet, %ld.\n", currentPortPacket, lastPortPacket + 1));
				if (currentPortPacket < lastPortPacket) {
					
					VERBOSE(if (verbose) printf("Packet %ld on port %d is out of order; dropping.\n", currentPortPacket, port));
					// Dropped packet -> index not processed -> effectively an 'added' packet, decrement the dropped packet count
					// 	so that we don't include an extra packet in shift operations
					currentPacketsDropped -= 1;

					iWork++;
					inputPacketOffset = iWork * portPacketLength;
					currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					continue;

				}

				// Packet dropped:
				//  Trip the return int
				//	Increment the port counter for this operation
				//	Increment the last packet offset so it can be used again next time, including the new offset
				packetLoss = -1;
				currentPacketsDropped += 1;
				lastPortPacket += 1;

				if (replayDroppedPackets) {
					// If we are replaying the last packet, change the array index to the last good packet index
					inputPacketOffset = lastInputPacketOffset;
				} else {
					// Array should be 0 padded at the start; copy data from there.
					inputPacketOffset = -2 * portPacketLength;
					memcpy(&(inputPortData[inputPacketOffset + 8]), &(inputPortData[lastInputPacketOffset + 8]), 8);
				}

				// Increment the 'sequence' component of the header so that the packet number appears right in future packet reads
				nextSequence = lofar_get_next_packet_sequence(&(inputPortData[lastInputPacketOffset]));
				memcpy(&(inputPortData[inputPacketOffset + 12]), &nextSequence, 4);
				// The last bit of the 'source' short isn't used; leave a signature that this packet was modified
				// Mask off the rest of the byte before adding encase we've used this packet before
				inputPortData[inputPacketOffset + 2] = (inputPortData[inputPacketOffset + 2] & 127) + 128;
				
				//VERBOSE(if (verbose) printf("Packet %ld on port %d is missing; padding.\n", lastPortPacket, port));

			} else {
				//VERBOSE(if (verbose) printf("Packet %ld is the expected packet.\n", currentPortPacket));

				// We have a sequential packet, therefore:
				//		Update the last legitimate packet number and array offset index
				//		Increment iWork (input data packet index) and determine the new input offset
				//		Get the next packet number for the next loop
				lastInputPacketOffset = inputPacketOffset;
				lastPortPacket = currentPortPacket;

				iWork++;
				inputPacketOffset = iWork * portPacketLength;

				currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));

			}

			// Move the packet (or substitute) to the output data
			outputPacketOffset = iLoop * portPacketLength;
			//VERBOSE(if (verbose) printf("Copying %d data from %ld to %ld\n", portPacketLength, lastInputPacketOffset, outputPacketOffset));
			memcpy(&(outputData[outputPacketOffset]), &(inputPortData[lastInputPacketOffset]), portPacketLength);
			


		}
		// Update the overall dropped packet count for this port
		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));
	}

	for (int port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < (-0.001 * (float) packetsPerIteration)) {
			fprintf(stderr, "A large number of packets were out of order on port %d; this normally indicates a data integrity issue, exiting...", port);
			return 1;
		}
	}
	// Update the last packet variable
	meta->lastPacket += packetsPerIteration;
	VERBOSE(if (verbose) printf("Exiting operation, last packet was %ld\n", meta->lastPacket));

	// Update the input/output states
	meta->inputDataReady = 0;
	meta->outputDataReady = 1;
	return packetLoss;
}


/**
 * @brief      { function_description }
 *
 * @param      meta  The meta
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_raw_udp_copy_nohdr(lofar_udp_meta *meta) {
	// Setup return variable
	int packetLoss = 0;

	// Setup working variables
	
	VERBOSE(const int verbose = meta->VERBOSE);
	

	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	// For each port of data provided,
	omp_set_num_threads(OMP_THREADS);
	#pragma omp parallel for 
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (verbose) printf("Port: %d on thread %d\n", port, omp_get_thread_num()));

		long lastPortPacket, currentPortPacket, inputPacketOffset, outputPacketOffset, lastInputPacketOffset, iWork, iLoop;
		// Reset the dropped packets counter
		meta->portLastDroppedPackets[port] = 0;
		int currentPacketsDropped = 0;

		// Reset last packet, reference data on the current port
		lastPortPacket = meta->lastPacket;
		char  *inputPortData = meta->inputData[port];
		char  *outputData = meta->outputData[port];

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int portPacketLength = meta->portPacketLength[port];
		const int headerlessPortPacketLength = meta->portPacketLength[port] - UDPHDRLEN;
		lastInputPacketOffset = (-2 + meta->replayDroppedPackets) * headerlessPortPacketLength; 	// We request at least 2 packets are malloc'd before the array head pointer, so no SEGFAULTs here
														// -2 * PPL = 0s -1 * PPL = last processed packet -- used for calcuating offset in dropped case if 
														// 	we have packet loss on the boundary.
		VERBOSE(if (verbose) printf("LPP: %ld, PPL: %d, LIPO: %ld\n", lastPortPacket, portPacketLength, lastInputPacketOffset));
		// iWork -- input data offset calculations (account for dropped packets)
		// iLoop -- output data offsets
		// These are the same if there is no packet loss.
		// 
		// Reset iWork, inputPacketOffset, read in the first packet's number.
		iWork = 0, inputPacketOffset = 0;
		currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
		VERBOSE(if (verbose) printf("Packet 0: %ld\n", currentPortPacket));
		
		for (iLoop = 0; iLoop < packetsPerIteration; iLoop++) {
			VERBOSE(if (verbose) printf("Loop %ld, Work %ld, packet %ld, target %ld\n", iLoop, iWork, currentPortPacket, lastPortPacket + 1));

			// Check for packet loss by ensuring we have sequential packet numbers
			if (currentPortPacket != (lastPortPacket + 1)) {
				// Check if a packet is out of order; if so, drop the packet
				// TODO: Better future option: check if the packet is in this block, copy and overwrite padded packed instead
				VERBOSE(if (verbose) printf("Packet %ld is not the expected packet, %ld.\n", currentPortPacket, lastPortPacket + 1));
				if (currentPortPacket < lastPortPacket) {
					
					VERBOSE(if (verbose) printf("Packet %ld on port %d is out of order; dropping.\n", currentPortPacket, port));
					// Dropped packet -> index not processed -> effectively an 'added' packet, decrement the dropped packet count
					// 	so that we don't include an extra packet in shift operations
					currentPacketsDropped -= 1;

					iWork++;
					inputPacketOffset = iWork * portPacketLength;
					currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					continue;

				}

				// Packet dropped:
				//  Trip the return int
				//	Increment the port counter for this operation
				//	Increment the last packet offset so it can be used again next time, including the new offset
				packetLoss = -1;
				currentPacketsDropped += 1;
				lastPortPacket += 1;

				if (replayDroppedPackets) {
					// If we are replaying the last packet, change the array index to the last good packet index
					inputPacketOffset = lastInputPacketOffset;
				} else {
					// Array should be 0 padded at the start; copy data from there.
					inputPacketOffset = -2 * portPacketLength;
				}

						
				VERBOSE(if (verbose) printf("Packet %ld on port %d is missing; padding.\n", lastPortPacket + 1, port));

			} else {
				VERBOSE(if (verbose) printf("Packet %ld is the expected packet.\n", currentPortPacket));

				// We have a sequential packet, therefore:
				//		Update the last legitimate packet number and array offset index
				//		Increment iWork (input data packet index) and determine the new input offset
				//		Get the next packet number for the next loop
				lastInputPacketOffset = inputPacketOffset + UDPHDRLEN;
				lastPortPacket = currentPortPacket;

				iWork++;
				inputPacketOffset = iWork * portPacketLength;

				currentPortPacket  = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));

			}


			outputPacketOffset = iLoop * headerlessPortPacketLength;
			VERBOSE(if (verbose) printf("Copying %d data from %ld to %ld\n", portPacketLength, lastInputPacketOffset, outputPacketOffset));
			memcpy(&(outputData[outputPacketOffset]), &(inputPortData[lastInputPacketOffset]), headerlessPortPacketLength);



		}
		// Update the overall dropped packet count for this port
		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));
	}

	for (int port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < (-0.001 * (float) packetsPerIteration)) {
			fprintf(stderr, "A large number of packets were out of order on port %d; this normally indicates a data integrity issue, exiting...", port);
			return 1;
		}
	}

	// Update the last packet variable
	meta->lastPacket += packetsPerIteration;
	VERBOSE(if (verbose) printf("Exiting operation, last packet was %ld\n", meta->lastPacket));

	// Update the input/output states
	meta->inputDataReady = 0;
	meta->outputDataReady = 1;
	return packetLoss;
}

/**
 * @brief      { function_description }
 *
 * @param      meta  The meta
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_raw_udp_copy_split_pols(lofar_udp_meta *meta) {
	// Setup return variable

	// Setup working variables
	
	VERBOSE(const int verbose = meta->VERBOSE);
	
	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	int packetLoss = 0;

	// For each port of data provided,
	omp_set_num_threads(OMP_THREADS);
	#pragma omp parallel for 
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (verbose) printf("Port: %d on thread %d\n", port, omp_get_thread_num()));

		long lastPortPacket, currentPortPacket, inputPacketOffset, outputPacketOffset, lastInputPacketOffset, iWork, iLoop, tsInOffset, tsOutOffset;
		// Reset the dropped packets counter
		meta->portLastDroppedPackets[port] = 0;
		int currentPacketsDropped = 0;

		// Reset last packet, reference data on the current port
		lastPortPacket = meta->lastPacket;
		char  *inputPortData = meta->inputData[port];
		char  **outputData = meta->outputData;

		const int portBeamlets = meta->portBeamlets[port];
		const int cumulativeBeamlets = meta->portCumulativeBeamlets[port];

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int portPacketLength = meta->portPacketLength[port];
		const int packetOutputLength = meta->packetOutputLength[0];
		lastInputPacketOffset = (-2 + meta->replayDroppedPackets) * portPacketLength; 	// We request at least 2 packets are malloc'd before the array head pointer, so no SEGFAULTs here
														// -2 * PPL = 0s -1 * PPL = last processed packet -- used for calcuating offset in dropped case if 
														// 	we have packet loss on the boundary.
		VERBOSE(if (verbose) printf("LPP: %ld, PPL: %d, LIPO: %ld\n", lastPortPacket, portPacketLength, lastInputPacketOffset));
		// iWork -- input data offset calculations (account for dropped packets)
		// iLoop -- output data offsets
		// These are the same if there is no packet loss.
		// 
		// Reset iWork, inputPacketOffset, read in the first packet's number.
		iWork = 0, inputPacketOffset = 0;
		currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
		VERBOSE(if (verbose) printf("Packet 0: %ld\n", currentPortPacket));
		
		for (iLoop = 0; iLoop < packetsPerIteration; iLoop++) {
			VERBOSE(if (verbose) printf("Loop %ld, Work %ld, packet %ld, target %ld\n", iLoop, iWork, currentPortPacket, lastPortPacket + 1));

			// Check for packet loss by ensuring we have sequential packet numbers
			if (currentPortPacket != (lastPortPacket + 1)) {
				// Check if a packet is out of order; if so, drop the packet
				// TODO: Better future option: check if the packet is in this block, copy and overwrite padded packed instead
				VERBOSE(if (verbose) printf("Packet %ld is not the expected packet, %ld.\n", currentPortPacket, lastPortPacket + 1));
				if (currentPortPacket < lastPortPacket) {
					
					VERBOSE(if(verbose) printf("Packet %ld on port %d is out of order (expected %ld); dropping.\n", currentPortPacket, port, lastPortPacket + 1));
					// Dropped packet -> index not processed -> effectively an 'added' packet, decrement the dropped packet count
					// 	so that we don't include an extra packet in shift operations
					currentPacketsDropped -= 1;

					iWork++;
					inputPacketOffset = iWork * portPacketLength;
					currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					continue;

				}

				// Packet dropped:
				//  Trip the return int
				//	Increment the port counter for this operation
				//	Increment the last packet offset so it can be used again next time, including the new offset
				packetLoss = -1;
				currentPacketsDropped += 1;
				lastPortPacket += 1;

				if (replayDroppedPackets) {
					// If we are replaying the last packet, change the array index to the last good packet index
					inputPacketOffset = lastInputPacketOffset;
				} else {
					// Array should be 0 padded at the start; copy data from there.
					inputPacketOffset = -2 * portPacketLength;
				}

				VERBOSE(if (verbose) printf("Packet %ld on port %d is missing; padding.\n", lastPortPacket + 1, port));

			} else {
				VERBOSE(if (verbose) printf("Packet %ld is the expected packet.\n", currentPortPacket));

				// We have a sequential packet, therefore:
				//		Update the last legitimate packet number and array offset index
				//		Increment iWork (input data packet index) and determine the new input offset
				//		Get the next packet number for the next loop
				lastInputPacketOffset = inputPacketOffset + UDPHDRLEN;
				lastPortPacket = currentPortPacket;

				iWork++;
				inputPacketOffset = iWork * portPacketLength;


				// Speedup: add 4 to the sequence, check if accurate. Doesn't work at rollover.
				if  (((char_unsigned_int*) &(inputPortData[inputPacketOffset + 12]))->ui  == (((char_unsigned_int*) &(inputPortData[lastInputPacketOffset -4])))->ui + 16)  {
					currentPortPacket += 1;
				} else currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));

			}

			outputPacketOffset = iLoop * packetOutputLength;

			//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
			#pragma GCC unroll 61
			for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
				tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL;
				tsOutOffset = outputPacketOffset + (beamlet + cumulativeBeamlets) * UDPNTIMESLICE;

				#pragma GCC unroll 16 //UDPNTIMESLICE not defined at compile?
				for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
					outputData[0][tsOutOffset] = inputPortData[tsInOffset]; // Xr
					outputData[1][tsOutOffset] = inputPortData[tsInOffset + 1]; // Xi
					outputData[2][tsOutOffset] = inputPortData[tsInOffset + 2]; // Yr
					outputData[3][tsOutOffset] = inputPortData[tsInOffset + 3]; // Yi

					tsInOffset += 4;
					tsOutOffset += 1;
				}


			}


		}
		// Update the overall dropped packet count for this port

		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));
	}

	for (int port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < (-0.001 * (float) packetsPerIteration)) {
			fprintf(stderr, "A large number of packets were out of order on port %d; this normally indicates a data integrity issue, exiting...", port);
			return 1;
		}
	}

	for (int port =0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < 0 && meta->replayDroppedPackets) {
			// TODO: pad the end of the arrays if the data doesn't fill it.
			// Or just re-loop with fresh data and report less data back?
			// Maybe rearchitect a bit, overread the amount of data needed, only return what's needed.
		}
	}

	// Update the last packet variable
	meta->lastPacket += packetsPerIteration;
	VERBOSE(if (verbose) printf("Exiting operation, last packet was %ld\n", meta->lastPacket));

	// Update the input/output states
	meta->inputDataReady = 0;
	meta->outputDataReady = 1;
	return packetLoss;
}


/**
 * @brief      { function_description }
 *
 * @param      meta  The meta
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_raw_udp_reorder(lofar_udp_meta *meta) {
	// Setup return variable

	// Setup working variables
	
	VERBOSE(const int verbose = meta->VERBOSE);
	
	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	int packetLoss = 0;

	// For each port of data provided,
	omp_set_num_threads(OMP_THREADS);
	#pragma omp parallel for 
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (verbose) printf("Port: %d on thread %d\n", port, omp_get_thread_num()));

		long lastPortPacket, currentPortPacket, inputPacketOffset, outputPacketOffset, lastInputPacketOffset, iWork, iLoop, tsInOffset, tsOutOffset;
		// Reset the dropped packets counter
		meta->portLastDroppedPackets[port] = 0;
		int currentPacketsDropped = 0;

		// Reset last packet, reference data on the current port
		lastPortPacket = meta->lastPacket;
		char  *inputPortData = meta->inputData[port];
		char  *outputData = meta->outputData[0];

		const int portBeamlets = meta->portBeamlets[port];
		const int cumulativeBeamlets = meta->portCumulativeBeamlets[port];
		const int totalBeamlets = meta->totalBeamlets;

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int portPacketLength = meta->portPacketLength[port];
		const int packetOutputLength = meta->packetOutputLength[0];
		lastInputPacketOffset = (-2 + meta->replayDroppedPackets) * portPacketLength; 	// We request at least 2 packets are malloc'd before the array head pointer, so no SEGFAULTs here
														// -2 * PPL = 0s -1 * PPL = last processed packet -- used for calcuating offset in dropped case if 
														// 	we have packet loss on the boundary.
		VERBOSE(if (verbose) printf("LPP: %ld, PPL: %d, LIPO: %ld\n", lastPortPacket, portPacketLength, lastInputPacketOffset));
		// iWork -- input data offset calculations (account for dropped packets)
		// iLoop -- output data offsets
		// These are the same if there is no packet loss.
		// 
		// Reset iWork, inputPacketOffset, read in the first packet's number.
		iWork = 0, inputPacketOffset = 0;
		currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
		VERBOSE(if (verbose) printf("Packet 0: %ld\n", currentPortPacket));
		
		for (iLoop = 0; iLoop < packetsPerIteration; iLoop++) {
			VERBOSE(if (verbose) printf("Loop %ld, Work %ld, packet %ld, target %ld\n", iLoop, iWork, currentPortPacket, lastPortPacket + 1));

			// Check for packet loss by ensuring we have sequential packet numbers
			if (currentPortPacket != (lastPortPacket + 1)) {
				// Check if a packet is out of order; if so, drop the packet
				// TODO: Better future option: check if the packet is in this block, copy and overwrite padded packed instead
				VERBOSE(if (verbose) printf("Packet %ld is not the expected packet, %ld.\n", currentPortPacket, lastPortPacket + 1));
				if (currentPortPacket < lastPortPacket) {
					
					VERBOSE(if(verbose) printf("Packet %ld on port %d is out of order (expected %ld); dropping.\n", currentPortPacket, port, lastPortPacket + 1));
					// Dropped packet -> index not processed -> effectively an 'added' packet, decrement the dropped packet count
					// 	so that we don't include an extra packet in shift operations
					currentPacketsDropped -= 1;

					iWork++;
					inputPacketOffset = iWork * portPacketLength;
					currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					continue;

				}

				// Packet dropped:
				//  Trip the return int
				//	Increment the port counter for this operation
				//	Increment the last packet offset so it can be used again next time, including the new offset
				packetLoss = -1;
				currentPacketsDropped += 1;
				lastPortPacket += 1;

				if (replayDroppedPackets) {
					// If we are replaying the last packet, change the array index to the last good packet index
					inputPacketOffset = lastInputPacketOffset;
				} else {
					// Array should be 0 padded at the start; copy data from there.
					inputPacketOffset = -2 * portPacketLength;
				}

				VERBOSE(if (verbose) printf("Packet %ld on port %d is missing; padding.\n", lastPortPacket + 1, port));

			} else {
				VERBOSE(if (verbose) printf("Packet %ld is the expected packet.\n", currentPortPacket));

				// We have a sequential packet, therefore:
				//		Update the last legitimate packet number and array offset index
				//		Increment iWork (input data packet index) and determine the new input offset
				//		Get the next packet number for the next loop
				lastInputPacketOffset = inputPacketOffset + UDPHDRLEN;
				lastPortPacket = currentPortPacket;

				iWork++;
				inputPacketOffset = iWork * portPacketLength;


				// Speedup: add 4 to the sequence, check if accurate. Doesn't work at rollover.
				if  (((char_unsigned_int*) &(inputPortData[inputPacketOffset + 12]))->ui  == (((char_unsigned_int*) &(inputPortData[lastInputPacketOffset -4])))->ui + 16)  {
					currentPortPacket += 1;
				} else currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));

			}


			outputPacketOffset = iLoop * packetOutputLength;

			//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
			#pragma GCC unroll 61
			for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
				tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL;
				tsOutOffset = outputPacketOffset + (beamlet + cumulativeBeamlets) * UDPNPOL;

				#pragma GCC unroll 16 //UDPNTIMESLICE not defined at compile?
				for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
					outputData[tsOutOffset] = inputPortData[tsInOffset]; // Xr
					outputData[tsOutOffset + 1] = inputPortData[tsInOffset + 1]; // Xi
					outputData[tsOutOffset + 2] = inputPortData[tsInOffset + 2]; // Yr
					outputData[tsOutOffset + 3] = inputPortData[tsInOffset + 3]; // Yi

					tsInOffset += 4;
					tsOutOffset += totalBeamlets * UDPNPOL;
				}

			}


		}
		// Update the overall dropped packet count for this port

		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));
	}

	for (int port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < (-0.001 * (float) packetsPerIteration)) {
			fprintf(stderr, "A large number of packets were out of order on port %d; this normally indicates a data integrity issue, exiting...", port);
			return 1;
		}
	}

	for (int port =0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < 0 && meta->replayDroppedPackets) {
			// TODO: pad the end of the arrays if the data doesn't fill it.
			// Or just re-loop with fresh data and report less data back?
			// Maybe rearchitect a bit, overread the amount of data needed, only return what's needed.
		}
	}

	// Update the last packet variable
	meta->lastPacket += packetsPerIteration;
	VERBOSE(if (verbose) printf("Exiting operation, last packet was %ld\n", meta->lastPacket));

	// Update the input/output states
	meta->inputDataReady = 0;
	meta->outputDataReady = 1;

	return packetLoss;
}


/**
 * @brief      { function_description }
 *
 * @param      meta  The meta
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_raw_udp_reorder_split_pols(lofar_udp_meta *meta) {
	// Setup return variable

	// Setup working variables
	
	VERBOSE(const int verbose = meta->VERBOSE);
	
	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	int packetLoss = 0;

	// For each port of data provided,
	omp_set_num_threads(OMP_THREADS);
	#pragma omp parallel for 
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (verbose) printf("Port: %d on thread %d\n", port, omp_get_thread_num()));

		long lastPortPacket, currentPortPacket, inputPacketOffset, outputPacketOffset, lastInputPacketOffset, iWork, iLoop, tsInOffset, tsOutOffset;
		// Reset the dropped packets counter
		meta->portLastDroppedPackets[port] = 0;
		int currentPacketsDropped = 0;

		// Reset last packet, reference data on the current port
		lastPortPacket = meta->lastPacket;
		char  *inputPortData = meta->inputData[port];
		char  **outputData = meta->outputData;

		const int portBeamlets = meta->portBeamlets[port];
		const int cumulativeBeamlets = meta->portCumulativeBeamlets[port];
		const int totalBeamlets = meta->totalBeamlets;

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int portPacketLength = meta->portPacketLength[port];
		const int packetOutputLength = meta->packetOutputLength[0];
		lastInputPacketOffset = (-2 + meta->replayDroppedPackets) * portPacketLength; 	// We request at least 2 packets are malloc'd before the array head pointer, so no SEGFAULTs here
														// -2 * PPL = 0s -1 * PPL = last processed packet -- used for calcuating offset in dropped case if 
														// 	we have packet loss on the boundary.
		VERBOSE(if (verbose) printf("LPP: %ld, PPL: %d, LIPO: %ld\n", lastPortPacket, portPacketLength, lastInputPacketOffset));
		// iWork -- input data offset calculations (account for dropped packets)
		// iLoop -- output data offsets
		// These are the same if there is no packet loss.
		// 
		// Reset iWork, inputPacketOffset, read in the first packet's number.
		iWork = 0, inputPacketOffset = 0;
		currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
		VERBOSE(if (verbose) printf("Packet 0: %ld\n", currentPortPacket));
		
		for (iLoop = 0; iLoop < packetsPerIteration; iLoop++) {
			VERBOSE(if (verbose) printf("Loop %ld, Work %ld, packet %ld, target %ld\n", iLoop, iWork, currentPortPacket, lastPortPacket + 1));

			// Check for packet loss by ensuring we have sequential packet numbers
			if (currentPortPacket != (lastPortPacket + 1)) {
				// Check if a packet is out of order; if so, drop the packet
				// TODO: Better future option: check if the packet is in this block, copy and overwrite padded packed instead
				VERBOSE(if (verbose) printf("Packet %ld is not the expected packet, %ld.\n", currentPortPacket, lastPortPacket + 1));
				if (currentPortPacket < lastPortPacket) {
					
					VERBOSE(if(verbose) printf("Packet %ld on port %d is out of order (expected %ld); dropping.\n", currentPortPacket, port, lastPortPacket + 1));
					// Dropped packet -> index not processed -> effectively an 'added' packet, decrement the dropped packet count
					// 	so that we don't include an extra packet in shift operations
					currentPacketsDropped -= 1;

					iWork++;
					inputPacketOffset = iWork * portPacketLength;
					currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					continue;

				}

				// Packet dropped:
				//  Trip the return int
				//	Increment the port counter for this operation
				//	Increment the last packet offset so it can be used again next time, including the new offset
				packetLoss = -1;
				currentPacketsDropped += 1;
				lastPortPacket += 1;

				if (replayDroppedPackets) {
					// If we are replaying the last packet, change the array index to the last good packet index
					inputPacketOffset = lastInputPacketOffset;
				} else {
					// Array should be 0 padded at the start; copy data from there.
					inputPacketOffset = -2 * portPacketLength;
				}

				VERBOSE(if (verbose) printf("Packet %ld on port %d is missing; padding.\n", lastPortPacket + 1, port));

			} else {
				VERBOSE(if (verbose) printf("Packet %ld is the expected packet.\n", currentPortPacket));

				// We have a sequential packet, therefore:
				//		Update the last legitimate packet number and array offset index
				//		Increment iWork (input data packet index) and determine the new input offset
				//		Get the next packet number for the next loop
				lastInputPacketOffset = inputPacketOffset + UDPHDRLEN;
				lastPortPacket = currentPortPacket;

				iWork++;
				inputPacketOffset = iWork * portPacketLength;


				// Speedup: add 4 to the sequence, check if accurate. Doesn't work at rollover.
				if  (((char_unsigned_int*) &(inputPortData[inputPacketOffset + 12]))->ui  == (((char_unsigned_int*) &(inputPortData[lastInputPacketOffset -4])))->ui + 16)  {
					currentPortPacket += 1;
				} else currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));

			}

			outputPacketOffset = iLoop * packetOutputLength;

			//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
			#pragma GCC unroll 61
			for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
				tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL;
				tsOutOffset = outputPacketOffset + beamlet + cumulativeBeamlets;

				#pragma GCC unroll 16 //UDPNTIMESLICE not defined at compile?
				for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
					outputData[0][tsOutOffset] = inputPortData[tsInOffset]; // Xr
					outputData[1][tsOutOffset] = inputPortData[tsInOffset + 1]; // Xi
					outputData[2][tsOutOffset] = inputPortData[tsInOffset + 2]; // Yr
					outputData[3][tsOutOffset] = inputPortData[tsInOffset + 3]; // Yi

					tsInOffset += 4;
					tsOutOffset += totalBeamlets;
				}


			}


		}
		// Update the overall dropped packet count for this port

		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));
	}

	for (int port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < (-0.001 * (float) packetsPerIteration)) {
			fprintf(stderr, "A large number of packets were out of order on port %d; this normally indicates a data integrity issue, exiting...", port);
			return 1;
		}
	}

	for (int port =0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < 0 && meta->replayDroppedPackets) {
			// TODO: pad the end of the arrays if the data doesn't fill it.
			// Or just re-loop with fresh data and report less data back?
			// Maybe rearchitect a bit, overread the amount of data needed, only return what's needed.
		}
	}

	// Update the last packet variable
	meta->lastPacket += packetsPerIteration;
	VERBOSE(if (verbose) printf("Exiting operation, last packet was %ld\n", meta->lastPacket));

	// Update the input/output states
	meta->inputDataReady = 0;
	meta->outputDataReady = 1;
	return packetLoss;
}


/**
 * @brief      { function_description }
 *
 * @param      meta  The meta
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_raw_udp_reversed(lofar_udp_meta *meta) {
	// Setup return variable

	// Setup working variables
	
	VERBOSE(const int verbose = meta->VERBOSE);
	
	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	int packetLoss = 0;

	// For each port of data provided,
	omp_set_num_threads(OMP_THREADS);
	#pragma omp parallel for 
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (verbose) printf("Port: %d on thread %d\n", port, omp_get_thread_num()));

		long lastPortPacket, currentPortPacket, inputPacketOffset, outputPacketOffset, lastInputPacketOffset, iWork, iLoop, tsInOffset, tsOutOffset;
		// Reset the dropped packets counter
		meta->portLastDroppedPackets[port] = 0;
		int currentPacketsDropped = 0;

		// Reset last packet, reference data on the current port
		lastPortPacket = meta->lastPacket;
		char  *inputPortData = meta->inputData[port];
		char  *outputData = meta->outputData[0];

		const int portBeamlets = meta->portBeamlets[port];
		const int cumulativeBeamlets = meta->portCumulativeBeamlets[port];
		const int totalBeamlets = meta->totalBeamlets;

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int portPacketLength = meta->portPacketLength[port];
		const int packetOutputLength = meta->packetOutputLength[0];
		lastInputPacketOffset = (-2 + meta->replayDroppedPackets) * portPacketLength; 	// We request at least 2 packets are malloc'd before the array head pointer, so no SEGFAULTs here
														// -2 * PPL = 0s -1 * PPL = last processed packet -- used for calcuating offset in dropped case if 
														// 	we have packet loss on the boundary.
		VERBOSE(if (verbose) printf("LPP: %ld, PPL: %d, LIPO: %ld\n", lastPortPacket, portPacketLength, lastInputPacketOffset));
		// iWork -- input data offset calculations (account for dropped packets)
		// iLoop -- output data offsets
		// These are the same if there is no packet loss.
		// 
		// Reset iWork, inputPacketOffset, read in the first packet's number.
		iWork = 0, inputPacketOffset = 0;
		currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
		VERBOSE(if (verbose) printf("Packet 0: %ld\n", currentPortPacket));
		
		for (iLoop = 0; iLoop < packetsPerIteration; iLoop++) {
			VERBOSE(if (verbose) printf("Loop %ld, Work %ld, packet %ld, target %ld\n", iLoop, iWork, currentPortPacket, lastPortPacket + 1));

			// Check for packet loss by ensuring we have sequential packet numbers
			if (currentPortPacket != (lastPortPacket + 1)) {
				// Check if a packet is out of order; if so, drop the packet
				// TODO: Better future option: check if the packet is in this block, copy and overwrite padded packed instead
				VERBOSE(if (verbose) printf("Packet %ld is not the expected packet, %ld.\n", currentPortPacket, lastPortPacket + 1));
				if (currentPortPacket < lastPortPacket) {
					
					VERBOSE(if(verbose) printf("Packet %ld on port %d is out of order (expected %ld); dropping.\n", currentPortPacket, port, lastPortPacket + 1));
					// Dropped packet -> index not processed -> effectively an 'added' packet, decrement the dropped packet count
					// 	so that we don't include an extra packet in shift operations
					currentPacketsDropped -= 1;

					iWork++;
					inputPacketOffset = iWork * portPacketLength;
					currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					continue;

				}

				// Packet dropped:
				//  Trip the return int
				//	Increment the port counter for this operation
				//	Increment the last packet offset so it can be used again next time, including the new offset
				packetLoss = -1;
				currentPacketsDropped += 1;
				lastPortPacket += 1;

				if (replayDroppedPackets) {
					// If we are replaying the last packet, change the array index to the last good packet index
					inputPacketOffset = lastInputPacketOffset;
				} else {
					// Array should be 0 padded at the start; copy data from there.
					inputPacketOffset = -2 * portPacketLength;
				}

				VERBOSE(if (verbose) printf("Packet %ld on port %d is missing; padding.\n", lastPortPacket + 1, port));

			} else {
				VERBOSE(if (verbose) printf("Packet %ld is the expected packet.\n", currentPortPacket));

				// We have a sequential packet, therefore:
				//		Update the last legitimate packet number and array offset index
				//		Increment iWork (input data packet index) and determine the new input offset
				//		Get the next packet number for the next loop
				lastInputPacketOffset = inputPacketOffset + UDPHDRLEN;
				lastPortPacket = currentPortPacket;

				iWork++;
				inputPacketOffset = iWork * portPacketLength;


				// Speedup: add 4 to the sequence, check if accurate. Doesn't work at rollover.
				if  (((char_unsigned_int*) &(inputPortData[inputPacketOffset + 12]))->ui  == (((char_unsigned_int*) &(inputPortData[lastInputPacketOffset -4])))->ui + 16)  {
					currentPortPacket += 1;
				} else currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));

			}

			outputPacketOffset = iLoop * packetOutputLength;

			//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
			#pragma GCC unroll 61
			for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
				tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL;
				tsOutOffset = outputPacketOffset + (totalBeamlets - (beamlet + cumulativeBeamlets)) * UDPNPOL;

				#pragma GCC unroll 16 //UDPNTIMESLICE not defined at compile?
				for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
					outputData[tsOutOffset] = inputPortData[tsInOffset]; // Xr
					outputData[tsOutOffset + 1] = inputPortData[tsInOffset + 1]; // Xi
					outputData[tsOutOffset + 2] = inputPortData[tsInOffset + 2]; // Yr
					outputData[tsOutOffset + 3] = inputPortData[tsInOffset + 3]; // Yi

					tsInOffset += 4;
					tsOutOffset += totalBeamlets * UDPNPOL;
				}


			}


		}
		// Update the overall dropped packet count for this port

		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));
	}

	for (int port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < (-0.001 * (float) packetsPerIteration)) {
			fprintf(stderr, "A large number of packets were out of order on port %d; this normally indicates a data integrity issue, exiting...", port);
			return 1;
		}
	}

	for (int port =0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < 0 && meta->replayDroppedPackets) {
			// TODO: pad the end of the arrays if the data doesn't fill it.
			// Or just re-loop with fresh data and report less data back?
			// Maybe rearchitect a bit, overread the amount of data needed, only return what's needed.
		}
	}

	// Update the last packet variable
	meta->lastPacket += packetsPerIteration;
	VERBOSE(if (verbose) printf("Exiting operation, last packet was %ld\n", meta->lastPacket));

	// Update the input/output states
	meta->inputDataReady = 0;
	meta->outputDataReady = 1;
	return packetLoss;
}


/**
 * @brief      { function_description }
 *
 * @param      meta  The meta
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_raw_udp_reversed_split_pols(lofar_udp_meta *meta) {
	// Setup return variable

	// Setup working variables
	
	VERBOSE(const int verbose = meta->VERBOSE);
	
	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	int packetLoss = 0;

	// For each port of data provided,
	omp_set_num_threads(OMP_THREADS);
	#pragma omp parallel for 
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (verbose) printf("Port: %d on thread %d\n", port, omp_get_thread_num()));

		long lastPortPacket, currentPortPacket, inputPacketOffset, outputPacketOffset, lastInputPacketOffset, iWork, iLoop, tsInOffset, tsOutOffset;
		// Reset the dropped packets counter
		meta->portLastDroppedPackets[port] = 0;
		int currentPacketsDropped = 0;

		// Reset last packet, reference data on the current port
		lastPortPacket = meta->lastPacket;
		char  *inputPortData = meta->inputData[port];
		char  **outputData = meta->outputData;

		const int portBeamlets = meta->portBeamlets[port];
		const int cumulativeBeamlets = meta->portCumulativeBeamlets[port];
		const int totalBeamlets = meta->totalBeamlets;

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int portPacketLength = meta->portPacketLength[port];
		const int packetOutputLength = meta->packetOutputLength[0];
		lastInputPacketOffset = (-2 + meta->replayDroppedPackets) * portPacketLength; 	// We request at least 2 packets are malloc'd before the array head pointer, so no SEGFAULTs here
														// -2 * PPL = 0s -1 * PPL = last processed packet -- used for calcuating offset in dropped case if 
														// 	we have packet loss on the boundary.
		VERBOSE(if (verbose) printf("LPP: %ld, PPL: %d, LIPO: %ld\n", lastPortPacket, portPacketLength, lastInputPacketOffset));
		// iWork -- input data offset calculations (account for dropped packets)
		// iLoop -- output data offsets
		// These are the same if there is no packet loss.
		// 
		// Reset iWork, inputPacketOffset, read in the first packet's number.
		iWork = 0, inputPacketOffset = 0;
		currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
		VERBOSE(if (verbose) printf("Packet 0: %ld\n", currentPortPacket));
		
		for (iLoop = 0; iLoop < packetsPerIteration; iLoop++) {
			VERBOSE(if (verbose) printf("Loop %ld, Work %ld, packet %ld, target %ld\n", iLoop, iWork, currentPortPacket, lastPortPacket + 1));

			// Check for packet loss by ensuring we have sequential packet numbers
			if (currentPortPacket != (lastPortPacket + 1)) {
				// Check if a packet is out of order; if so, drop the packet
				// TODO: Better future option: check if the packet is in this block, copy and overwrite padded packed instead
				VERBOSE(if (verbose) printf("Packet %ld is not the expected packet, %ld.\n", currentPortPacket, lastPortPacket + 1));
				if (currentPortPacket < lastPortPacket) {
					
					VERBOSE(if(verbose) printf("Packet %ld on port %d is out of order (expected %ld); dropping.\n", currentPortPacket, port, lastPortPacket + 1));
					// Dropped packet -> index not processed -> effectively an 'added' packet, decrement the dropped packet count
					// 	so that we don't include an extra packet in shift operations
					currentPacketsDropped -= 1;

					iWork++;
					inputPacketOffset = iWork * portPacketLength;
					currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					continue;

				}

				// Packet dropped:
				//  Trip the return int
				//	Increment the port counter for this operation
				//	Increment the last packet offset so it can be used again next time, including the new offset
				packetLoss = -1;
				currentPacketsDropped += 1;
				lastPortPacket += 1;

				if (replayDroppedPackets) {
					// If we are replaying the last packet, change the array index to the last good packet index
					inputPacketOffset = lastInputPacketOffset;
				} else {
					// Array should be 0 padded at the start; copy data from there.
					inputPacketOffset = -2 * portPacketLength;
				}

				VERBOSE(if (verbose) printf("Packet %ld on port %d is missing; padding.\n", lastPortPacket + 1, port));

			} else {
				VERBOSE(if (verbose) printf("Packet %ld is the expected packet.\n", currentPortPacket));

				// We have a sequential packet, therefore:
				//		Update the last legitimate packet number and array offset index
				//		Increment iWork (input data packet index) and determine the new input offset
				//		Get the next packet number for the next loop
				lastInputPacketOffset = inputPacketOffset + UDPHDRLEN;
				lastPortPacket = currentPortPacket;

				iWork++;
				inputPacketOffset = iWork * portPacketLength;


				// Speedup: add 4 to the sequence, check if accurate. Doesn't work at rollover.
				if  (((char_unsigned_int*) &(inputPortData[inputPacketOffset + 12]))->ui  == (((char_unsigned_int*) &(inputPortData[lastInputPacketOffset -4])))->ui + 16)  {
					currentPortPacket += 1;
				} else currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));

			}

			outputPacketOffset = iLoop * packetOutputLength;

			//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
			#pragma GCC unroll 61
			for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
				tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL;
				tsOutOffset = outputPacketOffset + (totalBeamlets - beamlet - cumulativeBeamlets);
				#pragma GCC unroll 16 //UDPNTIMESLICE not defined at compile?
				for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
					outputData[0][tsOutOffset] = inputPortData[tsInOffset]; // Xr
					outputData[1][tsOutOffset] = inputPortData[tsInOffset + 1]; // Xi
					outputData[2][tsOutOffset] = inputPortData[tsInOffset + 2]; // Yr
					outputData[3][tsOutOffset] = inputPortData[tsInOffset + 3]; // Yi

					tsInOffset += 4;
					tsOutOffset += totalBeamlets;
				}


			}


		}
		// Update the overall dropped packet count for this port

		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));
	}

	for (int port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < (-0.001 * (float) packetsPerIteration)) {
			fprintf(stderr, "A large number of packets were out of order on port %d; this normally indicates a data integrity issue, exiting...", port);
			return 1;
		}
	}

	for (int port =0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < 0 && meta->replayDroppedPackets) {
			// TODO: pad the end of the arrays if the data doesn't fill it.
			// Or just re-loop with fresh data and report less data back?
			// Maybe rearchitect a bit, overread the amount of data needed, only return what's needed.
		}
	}

	// Update the last packet variable
	meta->lastPacket += packetsPerIteration;
	VERBOSE(if (verbose) printf("Exiting operation, last packet was %ld\n", meta->lastPacket));

	// Update the input/output states
	meta->inputDataReady = 0;
	meta->outputDataReady = 1;

	return packetLoss;
}

/**
 * @brief      { function_description }
 *
 * @param[in]  Xr    { parameter_description }
 * @param[in]  Xi    { parameter_description }
 * @param[in]  Yr    { parameter_description }
 * @param[in]  Yi    { parameter_description }
 *
 * @return     { description_of_the_return_value }
 */
#pragma omp declare simd
float stokesI(float Xr, float Xi, float Yr, float Yi) {
	return  (1.0 * (Xr * Xr) + 1.0 * (Xi * Xi) + 1.0 * (Yr * Yr) + 1.0 * (Yi * Yi));
}

/**
 * @brief      { function_description }
 *
 * @param      meta  The meta
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_raw_udp_stokesI(lofar_udp_meta *meta) {
	// Setup return variable

	// Setup working variables
	
	VERBOSE(const int verbose = meta->VERBOSE);
	
	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	int packetLoss = 0;

	// For each port of data provided,
	omp_set_num_threads(OMP_THREADS);
	#pragma omp parallel for 
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (verbose) printf("Port: %d on thread %d\n", port, omp_get_thread_num()));

		long lastPortPacket, currentPortPacket, inputPacketOffset, outputPacketOffset, lastInputPacketOffset, iWork, iLoop, tsInOffset, tsOutOffset;
		// Reset the dropped packets counter
		meta->portLastDroppedPackets[port] = 0;
		int currentPacketsDropped = 0;

		// Reset last packet, reference data on the current port
		lastPortPacket = meta->lastPacket;
		char  *inputPortData = meta->inputData[port];
		float  *outputData = (float*) meta->outputData[0];

		const int portBeamlets = meta->portBeamlets[port];
		const int cumulativeBeamlets = meta->portCumulativeBeamlets[port];
		const int totalBeamlets = meta->totalBeamlets;

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int portPacketLength = meta->portPacketLength[port];
		const int packetOutputLength = meta->packetOutputLength[0];
		lastInputPacketOffset = (-2 + meta->replayDroppedPackets) * portPacketLength; 	// We request at least 2 packets are malloc'd before the array head pointer, so no SEGFAULTs here
														// -2 * PPL = 0s -1 * PPL = last processed packet -- used for calcuating offset in dropped case if 
														// 	we have packet loss on the boundary.
		VERBOSE(if (verbose) printf("LPP: %ld, PPL: %d, LIPO: %ld\n", lastPortPacket, portPacketLength, lastInputPacketOffset));
		// iWork -- input data offset calculations (account for dropped packets)
		// iLoop -- output data offsets
		// These are the same if there is no packet loss.
		// 
		// Reset iWork, inputPacketOffset, read in the first packet's number.
		iWork = 0, inputPacketOffset = 0;
		currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
		VERBOSE(if (verbose) printf("Packet 0: %ld\n", currentPortPacket));
		
		for (iLoop = 0; iLoop < packetsPerIteration; iLoop++) {
			VERBOSE(if (verbose) printf("Loop %ld, Work %ld, packet %ld, target %ld\n", iLoop, iWork, currentPortPacket, lastPortPacket + 1));

			// Check for packet loss by ensuring we have sequential packet numbers
			if (currentPortPacket != (lastPortPacket + 1)) {
				// Check if a packet is out of order; if so, drop the packet
				// TODO: Better future option: check if the packet is in this block, copy and overwrite padded packed instead
				VERBOSE(if (verbose) printf("Packet %ld is not the expected packet, %ld.\n", currentPortPacket, lastPortPacket + 1));
				if (currentPortPacket < lastPortPacket) {
					
					VERBOSE(if(verbose) printf("Packet %ld on port %d is out of order (expected %ld); dropping.\n", currentPortPacket, port, lastPortPacket + 1));
					// Dropped packet -> index not processed -> effectively an 'added' packet, decrement the dropped packet count
					// 	so that we don't include an extra packet in shift operations
					currentPacketsDropped -= 1;

					iWork++;
					inputPacketOffset = iWork * portPacketLength;
					currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					continue;

				}

				// Packet dropped:
				//  Trip the return int
				//	Increment the port counter for this operation
				//	Increment the last packet offset so it can be used again next time, including the new offset
				packetLoss = -1;
				currentPacketsDropped += 1;
				lastPortPacket += 1;

				if (replayDroppedPackets) {
					// If we are replaying the last packet, change the array index to the last good packet index
					inputPacketOffset = lastInputPacketOffset;
				} else {
					// Array should be 0 padded at the start; copy data from there.
					inputPacketOffset = -2 * portPacketLength;
				}

				VERBOSE(if (verbose) printf("Packet %ld on port %d is missing; padding.\n", lastPortPacket + 1, port));

			} else {
				VERBOSE(if (verbose) printf("Packet %ld is the expected packet.\n", currentPortPacket));

				// We have a sequential packet, therefore:
				//		Update the last legitimate packet number and array offset index
				//		Increment iWork (input data packet index) and determine the new input offset
				//		Get the next packet number for the next loop
				lastInputPacketOffset = inputPacketOffset + UDPHDRLEN;
				lastPortPacket = currentPortPacket;

				iWork++;
				inputPacketOffset = iWork * portPacketLength;


				// Speedup: add 4 to the sequence, check if accurate. Doesn't work at rollover.
				if  (((char_unsigned_int*) &(inputPortData[inputPacketOffset + 12]))->ui  == (((char_unsigned_int*) &(inputPortData[lastInputPacketOffset -4])))->ui + 16)  {
					currentPortPacket += 1;
				} else currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));

			}

			outputPacketOffset = iLoop * packetOutputLength / (sizeof(float) / sizeof(char));

			//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
			#pragma GCC unroll 61
			for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
				tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL;
				tsOutOffset = outputPacketOffset + (totalBeamlets - beamlet - cumulativeBeamlets);

				//#pragma omp simd 
				#pragma GCC unroll 16 //UDPNTIMESLICE not defined at compile?
				for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
					outputData[tsOutOffset] = stokesI((signed char) inputPortData[tsInOffset], (signed char) inputPortData[tsInOffset + 1], (signed char) inputPortData[tsInOffset + 2], (signed char) inputPortData[tsInOffset + 3]);

					tsInOffset += 4;
					tsOutOffset += totalBeamlets;
				}


			}


		}
		// Update the overall dropped packet count for this port

		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));
	}

	for (int port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < (-0.001 * (float) packetsPerIteration)) {
			fprintf(stderr, "A large number of packets were out of order on port %d; this normally indicates a data integrity issue, exiting...", port);
			return 1;
		}
	}

	for (int port =0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < 0 && meta->replayDroppedPackets) {
			// TODO: pad the end of the arrays if the data doesn't fill it.
			// Or just re-loop with fresh data and report less data back?
			// Maybe rearchitect a bit, overread the amount of data needed, only return what's needed.
		}
	}

	// Update the last packet variable
	meta->lastPacket += packetsPerIteration;
	VERBOSE(if (verbose) printf("Exiting operation, last packet was %ld\n", meta->lastPacket));

	// Update the input/output states
	meta->inputDataReady = 0;
	meta->outputDataReady = 1;

	return packetLoss;
}


/**
 * @brief      { function_description }
 *
 * @param      meta  The meta
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_raw_udp_stokesI_sum16(lofar_udp_meta *meta) {
	// Setup return variable

	// Setup working variables
	
	VERBOSE(const int verbose = meta->VERBOSE);
	
	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	int packetLoss = 0;

	// For each port of data provided,
	omp_set_num_threads(OMP_THREADS);
	#pragma omp parallel for 
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (verbose) printf("Port: %d on thread %d\n", port, omp_get_thread_num()));

		long lastPortPacket, currentPortPacket, inputPacketOffset, outputPacketOffset, lastInputPacketOffset, iWork, iLoop, tsInOffset, tsOutOffset;
		// Reset the dropped packets counter
		meta->portLastDroppedPackets[port] = 0;
		int currentPacketsDropped = 0;

		// Reset last packet, reference data on the current port
		lastPortPacket = meta->lastPacket;
		char  *inputPortData = meta->inputData[port];
		float  *outputData = (float*) meta->outputData[0];
		float tempVal;

		const int portBeamlets = meta->portBeamlets[port];
		const int cumulativeBeamlets = meta->portCumulativeBeamlets[port];
		const int totalBeamlets = meta->totalBeamlets;

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int portPacketLength = meta->portPacketLength[port];
		const int packetOutputLength = meta->packetOutputLength[0];
		lastInputPacketOffset = (-2 + meta->replayDroppedPackets) * portPacketLength; 	// We request at least 2 packets are malloc'd before the array head pointer, so no SEGFAULTs here
														// -2 * PPL = 0s -1 * PPL = last processed packet -- used for calcuating offset in dropped case if 
														// 	we have packet loss on the boundary.
		VERBOSE(if (verbose) printf("LPP: %ld, PPL: %d, LIPO: %ld\n", lastPortPacket, portPacketLength, lastInputPacketOffset));
		// iWork -- input data offset calculations (account for dropped packets)
		// iLoop -- output data offsets
		// These are the same if there is no packet loss.
		// 
		// Reset iWork, inputPacketOffset, read in the first packet's number.
		iWork = 0, inputPacketOffset = 0;
		currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
		VERBOSE(if (verbose) printf("Packet 0: %ld\n", currentPortPacket));
		
		for (iLoop = 0; iLoop < packetsPerIteration; iLoop++) {
			VERBOSE(if (verbose) printf("Loop %ld, Work %ld, packet %ld, target %ld\n", iLoop, iWork, currentPortPacket, lastPortPacket + 1));

			// Check for packet loss by ensuring we have sequential packet numbers
			if (currentPortPacket != (lastPortPacket + 1)) {
				// Check if a packet is out of order; if so, drop the packet
				// TODO: Better future option: check if the packet is in this block, copy and overwrite padded packed instead
				VERBOSE(if (verbose) printf("Packet %ld is not the expected packet, %ld.\n", currentPortPacket, lastPortPacket + 1));
				if (currentPortPacket < lastPortPacket) {
					
					VERBOSE(if(verbose) printf("Packet %ld on port %d is out of order (expected %ld); dropping.\n", currentPortPacket, port, lastPortPacket + 1));
					// Dropped packet -> index not processed -> effectively an 'added' packet, decrement the dropped packet count
					// 	so that we don't include an extra packet in shift operations
					currentPacketsDropped -= 1;

					iWork++;
					inputPacketOffset = iWork * portPacketLength;
					currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					continue;

				}

				// Packet dropped:
				//  Trip the return int
				//	Increment the port counter for this operation
				//	Increment the last packet offset so it can be used again next time, including the new offset
				packetLoss = -1;
				currentPacketsDropped += 1;
				lastPortPacket += 1;

				if (replayDroppedPackets) {
					// If we are replaying the last packet, change the array index to the last good packet index
					inputPacketOffset = lastInputPacketOffset;
				} else {
					// Array should be 0 padded at the start; copy data from there.
					inputPacketOffset = -2 * portPacketLength;
				}

				VERBOSE(if (verbose) printf("Packet %ld on port %d is missing; padding.\n", lastPortPacket + 1, port));

			} else {
				VERBOSE(if (verbose) printf("Packet %ld is the expected packet.\n", currentPortPacket));

				// We have a sequential packet, therefore:
				//		Update the last legitimate packet number and array offset index
				//		Increment iWork (input data packet index) and determine the new input offset
				//		Get the next packet number for the next loop
				lastInputPacketOffset = inputPacketOffset + UDPHDRLEN;
				lastPortPacket = currentPortPacket;

				iWork++;
				inputPacketOffset = iWork * portPacketLength;


				// Speedup: add 4 to the sequence, check if accurate. Doesn't work at rollover.
				if  (((char_unsigned_int*) &(inputPortData[inputPacketOffset + 12]))->ui  == (((char_unsigned_int*) &(inputPortData[lastInputPacketOffset -4])))->ui + 16)  {
					currentPortPacket += 1;
				} else currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));

			}

			outputPacketOffset = iLoop * packetOutputLength / (sizeof(float) / sizeof(char));

			//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
			#pragma GCC unroll 61
			for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
				tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL;
				tsOutOffset = outputPacketOffset + (totalBeamlets - beamlet - cumulativeBeamlets);
				tempVal = 0.0;

				//#pragma omp simd 
				#pragma GCC unroll 16 //UDPNTIMESLICE not defined at compile?
				for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
					tempVal += stokesI((signed char) inputPortData[tsInOffset], (signed char) inputPortData[tsInOffset + 1], (signed char) inputPortData[tsInOffset + 2], (signed char) inputPortData[tsInOffset + 3]);

					tsInOffset += 4;
				}

				outputData[tsOutOffset] = tempVal;


			}


		}
		// Update the overall dropped packet count for this port

		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));
	}

	for (int port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < (-0.001 * (float) packetsPerIteration)) {
			fprintf(stderr, "A large number of packets were out of order on port %d; this normally indicates a data integrity issue, exiting...", port);
			return 1;
		}
	}

	for (int port =0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < 0 && meta->replayDroppedPackets) {
			// TODO: pad the end of the arrays if the data doesn't fill it.
			// Or just re-loop with fresh data and report less data back?
			// Maybe rearchitect a bit, overread the amount of data needed, only return what's needed.
		}
	}

	// Update the last packet variable
	meta->lastPacket += packetsPerIteration;
	VERBOSE(if (verbose) printf("Exiting operation, last packet was %ld\n", meta->lastPacket));

	// Update the input/output states
	meta->inputDataReady = 0;
	meta->outputDataReady = 1;

	return packetLoss;
}



/**
 * @brief      { function_description }
 *
 * @param[in]  Xr    { parameter_description }
 * @param[in]  Xi    { parameter_description }
 * @param[in]  Yr    { parameter_description }
 * @param[in]  Yi    { parameter_description }
 *
 * @return     { description_of_the_return_value }
 */
#pragma omp declare simd
float stokesV(float Xr, float Xi, float Yr, float Yi) {
	return 2.0 * ((Xr * Yi) + (-1.0 * Xi * Yr));
}

/**
 * @brief      { function_description }
 *
 * @param      meta  The meta
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_raw_udp_stokesV(lofar_udp_meta *meta) {
	// Setup return variable

	// Setup working variables
	
	VERBOSE(const int verbose = meta->VERBOSE);
	
	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	int packetLoss = 0;

	// For each port of data provided,
	omp_set_num_threads(OMP_THREADS);
	#pragma omp parallel for 
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (verbose) printf("Port: %d on thread %d\n", port, omp_get_thread_num()));

		long lastPortPacket, currentPortPacket, inputPacketOffset, outputPacketOffset, lastInputPacketOffset, iWork, iLoop, tsInOffset, tsOutOffset;
		// Reset the dropped packets counter
		meta->portLastDroppedPackets[port] = 0;
		int currentPacketsDropped = 0;

		// Reset last packet, reference data on the current port
		lastPortPacket = meta->lastPacket;
		char  *inputPortData = meta->inputData[port];
		float *outputData = (float*) (meta->outputData[0]);

		const int portBeamlets = meta->portBeamlets[port];
		const int cumulativeBeamlets = meta->portCumulativeBeamlets[port];
		const int totalBeamlets = meta->totalBeamlets;

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int portPacketLength = meta->portPacketLength[port];
		const int packetOutputLength = meta->packetOutputLength[0];
		lastInputPacketOffset = (-2 + meta->replayDroppedPackets) * portPacketLength; 	// We request at least 2 packets are malloc'd before the array head pointer, so no SEGFAULTs here
														// -2 * PPL = 0s -1 * PPL = last processed packet -- used for calcuating offset in dropped case if 
														// 	we have packet loss on the boundary.
		VERBOSE(if (verbose) printf("LPP: %ld, PPL: %d, LIPO: %ld\n", lastPortPacket, portPacketLength, lastInputPacketOffset));
		// iWork -- input data offset calculations (account for dropped packets)
		// iLoop -- output data offsets
		// These are the same if there is no packet loss.
		// 
		// Reset iWork, inputPacketOffset, read in the first packet's number.
		iWork = 0, inputPacketOffset = 0;
		currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
		VERBOSE(if (verbose) printf("Packet 0: %ld\n", currentPortPacket));
		
		for (iLoop = 0; iLoop < packetsPerIteration; iLoop++) {
			VERBOSE(if (verbose) printf("Loop %ld, Work %ld, packet %ld, target %ld\n", iLoop, iWork, currentPortPacket, lastPortPacket + 1));

			// Check for packet loss by ensuring we have sequential packet numbers
			if (currentPortPacket != (lastPortPacket + 1)) {
				// Check if a packet is out of order; if so, drop the packet
				// TODO: Better future option: check if the packet is in this block, copy and overwrite padded packed instead
				VERBOSE(if (verbose) printf("Packet %ld is not the expected packet, %ld.\n", currentPortPacket, lastPortPacket + 1));
				if (currentPortPacket < lastPortPacket) {
					
					VERBOSE(if(verbose) printf("Packet %ld on port %d is out of order (expected %ld); dropping.\n", currentPortPacket, port, lastPortPacket + 1));
					// Dropped packet -> index not processed -> effectively an 'added' packet, decrement the dropped packet count
					// 	so that we don't include an extra packet in shift operations
					currentPacketsDropped -= 1;

					iWork++;
					inputPacketOffset = iWork * portPacketLength;
					currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					continue;

				}

				// Packet dropped:
				//  Trip the return int
				//	Increment the port counter for this operation
				//	Increment the last packet offset so it can be used again next time, including the new offset
				packetLoss = -1;
				currentPacketsDropped += 1;
				lastPortPacket += 1;

				if (replayDroppedPackets) {
					// If we are replaying the last packet, change the array index to the last good packet index
					inputPacketOffset = lastInputPacketOffset;
				} else {
					// Array should be 0 padded at the start; copy data from there.
					inputPacketOffset = -2 * portPacketLength;
				}

				VERBOSE(if (verbose) printf("Packet %ld on port %d is missing; padding.\n", lastPortPacket + 1, port));

			} else {
				VERBOSE(if (verbose) printf("Packet %ld is the expected packet.\n", currentPortPacket));

				// We have a sequential packet, therefore:
				//		Update the last legitimate packet number and array offset index
				//		Increment iWork (input data packet index) and determine the new input offset
				//		Get the next packet number for the next loop
				lastInputPacketOffset = inputPacketOffset + UDPHDRLEN;
				lastPortPacket = currentPortPacket;

				iWork++;
				inputPacketOffset = iWork * portPacketLength;


				// Speedup: add 4 to the sequence, check if accurate. Doesn't work at rollover.
				if  (((char_unsigned_int*) &(inputPortData[inputPacketOffset + 12]))->ui  == (((char_unsigned_int*) &(inputPortData[lastInputPacketOffset -4])))->ui + 16)  {
					currentPortPacket += 1;
				} else currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));

			}

			outputPacketOffset = iLoop * packetOutputLength / (sizeof(float) / sizeof(char));

			//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
			#pragma GCC unroll 61
			for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
				tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL;
				tsOutOffset = outputPacketOffset + (totalBeamlets - beamlet - cumulativeBeamlets);

				//#pragma omp simd
				#pragma GCC unroll 16 //UDPNTIMESLICE not defined at compile?
				for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
					outputData[tsOutOffset] = stokesV((signed char) inputPortData[tsInOffset], (signed char) inputPortData[tsInOffset + 1], (signed char) inputPortData[tsInOffset + 2], (signed char) inputPortData[tsInOffset + 3]);

					tsInOffset += 4;
					tsOutOffset += totalBeamlets;
				}


			}


		}
		// Update the overall dropped packet count for this port

		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));
	}

	for (int port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < (-0.001 * (float) packetsPerIteration)) {
			fprintf(stderr, "A large number of packets were out of order on port %d; this normally indicates a data integrity issue, exiting...", port);
			return 1;
		}
	}

	for (int port =0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < 0 && meta->replayDroppedPackets) {
			// TODO: pad the end of the arrays if the data doesn't fill it.
			// Or just re-loop with fresh data and report less data back?
			// Maybe rearchitect a bit, overread the amount of data needed, only return what's needed.
		}
	}

	// Update the last packet variable
	meta->lastPacket += packetsPerIteration;
	VERBOSE(if (verbose) printf("Exiting operation, last packet was %ld\n", meta->lastPacket));

	// Update the input/output states
	meta->inputDataReady = 0;
	meta->outputDataReady = 1;

	return packetLoss;
}

/* LUT for 4-bit data */
const char bitmodeConversion[256][2] = {
		{ 0 , 0 }, { 0 , 1 }, { 0 , 2 }, { 0 , 3 }, { 0 , 4 }, 
		{ 0 , 5 }, { 0 , 6 }, { 0 , 7 }, { 0 , -8 }, { 0 , -7 }, 
		{ 0 , -6 }, { 0 , -5 }, { 0 , -4 }, { 0 , -3 }, { 0 , -2 }, 
		{ 0 , -1 }, { 1 , 0 }, { 1 , 1 }, { 1 , 2 }, { 1 , 3 }, 
		{ 1 , 4 }, { 1 , 5 }, { 1 , 6 }, { 1 , 7 }, { 1 , -8 }, 
		{ 1 , -7 }, { 1 , -6 }, { 1 , -5 }, { 1 , -4 }, { 1 , -3 }, 
		{ 1 , -2 }, { 1 , -1 }, { 2 , 0 }, { 2 , 1 }, { 2 , 2 }, 
		{ 2 , 3 }, { 2 , 4 }, { 2 , 5 }, { 2 , 6 }, { 2 , 7 }, 
		{ 2 , -8 }, { 2 , -7 }, { 2 , -6 }, { 2 , -5 }, { 2 , -4 }, 
		{ 2 , -3 }, { 2 , -2 }, { 2 , -1 }, { 3 , 0 }, { 3 , 1 }, 
		{ 3 , 2 }, { 3 , 3 }, { 3 , 4 }, { 3 , 5 }, { 3 , 6 }, 
		{ 3 , 7 }, { 3 , -8 }, { 3 , -7 }, { 3 , -6 }, { 3 , -5 }, 
		{ 3 , -4 }, { 3 , -3 }, { 3 , -2 }, { 3 , -1 }, { 4 , 0 }, 
		{ 4 , 1 }, { 4 , 2 }, { 4 , 3 }, { 4 , 4 }, { 4 , 5 }, 
		{ 4 , 6 }, { 4 , 7 }, { 4 , -8 }, { 4 , -7 }, { 4 , -6 }, 
		{ 4 , -5 }, { 4 , -4 }, { 4 , -3 }, { 4 , -2 }, { 4 , -1 }, 
		{ 5 , 0 }, { 5 , 1 }, { 5 , 2 }, { 5 , 3 }, { 5 , 4 }, 
		{ 5 , 5 }, { 5 , 6 }, { 5 , 7 }, { 5 , -8 }, { 5 , -7 }, 
		{ 5 , -6 }, { 5 , -5 }, { 5 , -4 }, { 5 , -3 }, { 5 , -2 }, 
		{ 5 , -1 }, { 6 , 0 }, { 6 , 1 }, { 6 , 2 }, { 6 , 3 }, 
		{ 6 , 4 }, { 6 , 5 }, { 6 , 6 }, { 6 , 7 }, { 6 , -8 }, 
		{ 6 , -7 }, { 6 , -6 }, { 6 , -5 }, { 6 , -4 }, { 6 , -3 }, 
		{ 6 , -2 }, { 6 , -1 }, { 7 , 0 }, { 7 , 1 }, { 7 , 2 }, 
		{ 7 , 3 }, { 7 , 4 }, { 7 , 5 }, { 7 , 6 }, { 7 , 7 }, 
		{ 7 , -8 }, { 7 , -7 }, { 7 , -6 }, { 7 , -5 }, { 7 , -4 }, 
		{ 7 , -3 }, { 7 , -2 }, { 7 , -1 }, { -8 , 0 }, { -8 , 1 }, 
		{ -8 , 2 }, { -8 , 3 }, { -8 , 4 }, { -8 , 5 }, { -8 , 6 }, 
		{ -8 , 7 }, { -8 , -8 }, { -8 , -7 }, { -8 , -6 }, { -8 , -5 }, 
		{ -8 , -4 }, { -8 , -3 }, { -8 , -2 }, { -8 , -1 }, { -7 , 0 }, 
		{ -7 , 1 }, { -7 , 2 }, { -7 , 3 }, { -7 , 4 }, { -7 , 5 }, 
		{ -7 , 6 }, { -7 , 7 }, { -7 , -8 }, { -7 , -7 }, { -7 , -6 }, 
		{ -7 , -5 }, { -7 , -4 }, { -7 , -3 }, { -7 , -2 }, { -7 , -1 }, 
		{ -6 , 0 }, { -6 , 1 }, { -6 , 2 }, { -6 , 3 }, { -6 , 4 }, 
		{ -6 , 5 }, { -6 , 6 }, { -6 , 7 }, { -6 , -8 }, { -6 , -7 }, 
		{ -6 , -6 }, { -6 , -5 }, { -6 , -4 }, { -6 , -3 }, { -6 , -2 }, 
		{ -6 , -1 }, { -5 , 0 }, { -5 , 1 }, { -5 , 2 }, { -5 , 3 }, 
		{ -5 , 4 }, { -5 , 5 }, { -5 , 6 }, { -5 , 7 }, { -5 , -8 }, 
		{ -5 , -7 }, { -5 , -6 }, { -5 , -5 }, { -5 , -4 }, { -5 , -3 }, 
		{ -5 , -2 }, { -5 , -1 }, { -4 , 0 }, { -4 , 1 }, { -4 , 2 }, 
		{ -4 , 3 }, { -4 , 4 }, { -4 , 5 }, { -4 , 6 }, { -4 , 7 }, 
		{ -4 , -8 }, { -4 , -7 }, { -4 , -6 }, { -4 , -5 }, { -4 , -4 }, 
		{ -4 , -3 }, { -4 , -2 }, { -4 , -1 }, { -3 , 0 }, { -3 , 1 }, 
		{ -3 , 2 }, { -3 , 3 }, { -3 , 4 }, { -3 , 5 }, { -3 , 6 }, 
		{ -3 , 7 }, { -3 , -8 }, { -3 , -7 }, { -3 , -6 }, { -3 , -5 }, 
		{ -3 , -4 }, { -3 , -3 }, { -3 , -2 }, { -3 , -1 }, { -2 , 0 }, 
		{ -2 , 1 }, { -2 , 2 }, { -2 , 3 }, { -2 , 4 }, { -2 , 5 }, 
		{ -2 , 6 }, { -2 , 7 }, { -2 , -8 }, { -2 , -7 }, { -2 , -6 }, 
		{ -2 , -5 }, { -2 , -4 }, { -2 , -3 }, { -2 , -2 }, { -2 , -1 }, 
		{ -1 , 0 }, { -1 , 1 }, { -1 , 2 }, { -1 , 3 }, { -1 , 4 }, 
		{ -1 , 5 }, { -1 , 6 }, { -1 , 7 }, { -1 , -8 }, { -1 , -7 },
		{ -1 , -6 }, { -1 , -5 }, { -1 , -4 }, { -1 , -3 }, { -1 , -2 }, 
		{ -1 , -1 }

};