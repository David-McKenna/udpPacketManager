#include "lofar_udp_misc.h"
#include "lofar_udp_reader.h"
#include "lofar_udp_backends.hpp"


// Define a set of default structs

// Calibration default
// While in here we default to enbling calibration, the overall default is read from
// the lofar_udp_config and overwrites the value in here.
lofar_udp_calibration lofar_udp_calibration_default = {
	.calibrationStepsGenerated = 0,
	.calibrationFifo = "/tmp/udp_calibation_pipe",
	.calibrationSubbands = "HBA,12:499",
	.calibrationDuration = 3600.0,
	.calibrationPointing = { 0.0, 0.7853982 },
	.calibrationPointingBasis = "AZELGO"
};

// Configuration default
lofar_udp_config lofar_udp_config_default = {
	.inputFiles = NULL,
	.numPorts = 4,
	.replayDroppedPackets = 0,
	.processingMode = 0,
	.verbose = 0,
	.packetsPerIteration = 65536,
	.startingPacket = -1,
	.packetsReadMax = -1,
	.compressedReader = 0,
	.beamletLimits = { 0, 0 },
	.calibrateData = 0,
	.calibrationConfiguration = &lofar_udp_calibration_default
};


// Reader / meta with NULL-initialised values to help the cleanup function
lofar_udp_reader lofar_udp_reader_default = {
	.dstream = { NULL },
	.inBuffer = { NULL }
};


lofar_udp_meta lofar_udp_meta_default = {
	.inputData = { NULL },
	.outputData = { NULL },
	.packetsRead = 0,
	.inputDataReady = 0,
	.outputDataReady = 0,
	.jonesMatrices = NULL,
	.calibrationStep = 0
};

/**
 * @brief      Parse LOFAR UDP headers to determine some metadata about the
 *             ports
 *
 * @param      meta           The lofar_udp_meta to initialise
 * @param[in]  header         The header data to process
 * @param[in]  beamletLimits  The upper/lower beamlets limits
 *
 * @return     0: Success, 1: Fatal error
 */
int lofar_udp_parse_headers(lofar_udp_meta *meta, char header[MAX_NUM_PORTS][UDPHDRLEN], const int beamletLimits[2]) {

	lofar_source_bytes *source;

	float bitMul;
	int cacheBitMode = 0;
	meta->totalRawBeamlets = 0;
	meta->totalProcBeamlets = 0;


	// Process each input port
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE( if(meta->VERBOSE) printf("Port %d/%d\n", port, meta->numPorts - 1););
		// Data integrity checks
		if ((unsigned char) header[port][0] < UDPCURVER) {
			fprintf(stderr, "Input header on port %d appears malformed (RSP Version less than 3), exiting.\n", port);
			return 1;
		}

		if (*((unsigned int *) &(header[port][8])) <  LFREPOCH) {
			fprintf(stderr, "Input header on port %d appears malformed (data timestamp before 2008), exiting.\n", port);
			return 1;
		}

		if (*((unsigned int *) &(header[port][12])) > RSPMAXSEQ) {
			fprintf(stderr, "Input header on port %d appears malformed (sequence higher than 200MHz clock maximum, %d), exiting.\n", port, *((unsigned int *) &(header[port][12])));
			return 1;
		}

		if ((unsigned char) header[port][6] > UDPMAXBEAM) {
			fprintf(stderr, "Input header on port %d appears malformed (more than %d beamlets on a port, %d), exiting.\n", port, UDPMAXBEAM, header[port][6]);
			return 1;
		}

		if ((unsigned char) header[port][7] != UDPNTIMESLICE) {
			fprintf(stderr, "Input header on port %d appears malformed (time slices are %d, not UDPNTIMESLICE), exiting.\n", port, header[port][7]);
			return 1;
		}

		source = (lofar_source_bytes*) &(header[port][1]);
		if (source->padding0 != 0) {
			fprintf(stderr, "Input header on port %d appears malformed (padding bit (0) is set), exiting.\n", port);
			return 1;
		} else if (source->errorBit != 0) {
			fprintf(stderr, "Input header on port %d appears malformed (error bit is set), exiting.\n", port);
			return 1;
		} else if (source->bitMode == 3) {
			fprintf(stderr, "Input header on port %d appears malformed (BM of 3 doesn't exist), exiting.\n", port);
			return 1;
		} else if (source->padding1 > 1) {
			fprintf(stderr, "Input header on port %d appears malformed (padding bits (1) are set), exiting.\n", port);
			return 1;
		} else if (source->padding1 == 1) {
			fprintf(stderr, "Input header on port %d appears malformed (our replay packet warning bit is set), continuing with caution...\n", port);
		}

		if (port != 0 && meta->clockBit != source->clockBit) {
			fprintf(stderr, "ERROR: Input files contain a mixutre of 200MHz clock and 160MHz clock (port %d differs), please process these observaiton separately. Exiting.\n", port);
			return 1;
		} else {
			meta->clockBit = source->clockBit;
		}

		// Extract the station ID
		// Divide by 32 to convert from (my current guess based on SE607 / IE613 codes) RSP IDs to station codes
		meta->stationID = *((short*) &(header[port][4])) / 32;

		// Determine the number of beamlets on the port
		VERBOSE(printf("port %d, bitMode %d, beamlets %d (%u)\n", port, source->bitMode, (int) ((unsigned char) header[port][6]), (unsigned char) header[port][6]););
		meta->portRawBeamlets[port] = (int) ((unsigned char) header[port][6]);

		// Assume we are processing all beamlets by default
		meta->upperBeamlets[port] = meta->portRawBeamlets[port];

		// Update the cumulative beamlets, total raw beamlets
		// Ordering is intentional so that we get the number of beamlets before this port.
		meta->portRawCumulativeBeamlets[port] = meta->totalRawBeamlets;
		meta->portCumulativeBeamlets[port] = meta->totalProcBeamlets;
		meta->totalRawBeamlets += meta->portRawBeamlets[port];


		// Set the  upper, lower limit of beamlets as needed
		
		// Check the upper limit first, so that we can modify upperBeamlets if needed.
		if (beamletLimits[1] != 0 && (beamletLimits[1] < ((port + 1) * meta->portRawBeamlets[port])) && (beamletLimits[1] >= port * meta->portRawBeamlets[port])) {
			meta->upperBeamlets[port] = beamletLimits[1] - meta->totalRawBeamlets;
		}


		// Check the lower beamlet, and then update the total beamlets counters as needed
		if (beamletLimits[0] != 0 && (beamletLimits[0] < ((port + 1) * meta->portRawBeamlets[port])) && (beamletLimits[0] >= port * meta->portRawBeamlets[port])) {
			meta->baseBeamlets[port] = beamletLimits[0] - meta->totalRawBeamlets;

			// Lower the total count of beamlets, while not modifiyng
			// 	upperBeamlets
			meta->totalProcBeamlets += meta->upperBeamlets[port] - beamletLimits[0];
		} else {
			meta->baseBeamlets[port] = 0;
			meta->totalProcBeamlets += meta->upperBeamlets[port];
		}

		// Check the bitmode on the port
		switch (source->bitMode) {
			case 0:
				meta->inputBitMode = 16;
				break;

			case 1:
				meta->inputBitMode = 8;
				break;

			case 2:
				meta->inputBitMode = 4;
				break;

			default:
				fprintf(stderr, "How did we get here? BM=3 should have been caught already...\n");
				return 1;
		}

		// Ensure that the bitmode does not change between ports; we assume it is constant elsewhere.
		// Fairly certain the RSPs fall over if you try to do multiple bit modes anyway.
		if (port == 0) {
			cacheBitMode = meta->inputBitMode;
		} else {
			if (cacheBitMode != meta->inputBitMode) {
				fprintf(stderr, "Multiple input bit sizes detected; please parse these ports separately (port 0: %d, port %d: %d). Exiting.\n", cacheBitMode, port, meta->inputBitMode);
				return 1;
			}
		}



		// Determine the size of the input array
		// 4-bit: half the size per sample
		// 16-bit: 2x the size per sample
		bitMul = 1 - 0.5 * (meta->inputBitMode == 4) + 1 * (meta->inputBitMode == 16); // 4bit = 0.5x, 16bit = 2x
		meta->portPacketLength[port] = (int) ((UDPHDRLEN) + (meta->portRawBeamlets[port] * bitMul * UDPNTIMESLICE * UDPNPOL));
		
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

			// Account for packet ddsync between ports
 			for (int portInner = 0; portInner < reader->meta->numPorts; portInner++) {
				reader->meta->portLastDroppedPackets[portInner] = lofar_get_packet_number(&(reader->meta->inputData[portInner][lastPacketOffset])) - (currentPacket + reader->meta->packetsPerIteration);
				VERBOSE(if (reader->meta->portLastDroppedPackets[portInner]) {
					printf("%d: %d packets lost.\n", portInner, reader->meta->portLastDroppedPackets[portInner]);
				});

				if (reader->meta->portLastDroppedPackets[portInner] > reader->meta->packetsPerIteration) {
					fprintf(stderr, "\n\nWARNING: Large amount of packets dropped on port %d during scan iteration (%d lost), attempting to continue...\n", port, reader->meta->portLastDroppedPackets[portInner]);
					returnVal = -2;
				}
			}

			// Get the new last packet
			if (returnVal == -2) {
				// If we have a large amount of packet loss, update directly
				currentPacket = lofar_get_packet_number(&(reader->meta->inputData[0][lastPacketOffset]));
			} else {
				// Otherwise just add the number of iterations
				currentPacket += reader->meta->packetsPerIteration;			
			}

			// Print a status update to the CLI
			printf("\rScanning to packet %ld (~%.02f%% complete, currently at packet %ld on port %d, %ld to go)", reader->meta->lastPacket, (float) 100.0 -  (float) (reader->meta->lastPacket - currentPacket) / (packetDelta) * 100.0, currentPacket, port, reader->meta->lastPacket - currentPacket);
			fflush(stdout);
		}

		if (lofar_get_packet_number(&(reader->meta->inputData[port][0])) > reader->meta->lastPacket) {
			fprintf(stderr, "Port %d has scanned beyond target packet %ld (to start at %ld), exiting.\n", port, reader->meta->lastPacket, lofar_get_packet_number(&(reader->meta->inputData[port][lastPacketOffset])));
			return 1;
		}

		if (scanning) printf("\33[2K\rReached target packet %ld on port %d.\n", reader->meta->lastPacket, port);
	}
	// Data is now all loaded such that every inputData array has or is past the target packet, now we need to interally align the arrays

	// For each port,
	for (int port = 0; port < reader->meta->numPorts; port++) {

		// Re-initialise the offset shift needed on each port
		for (int port = 0; port < reader->meta->numPorts; port++) packetShift[port] = 0;

		// Get the current packet, and guess the target packet by assuming no packet loss
		currentPacket = lofar_get_packet_number(&(reader->meta->inputData[port][0]));
		guessPacket = lofar_get_packet_number(&(reader->meta->inputData[port][(reader->meta->lastPacket - currentPacket) * reader->meta->portPacketLength[port]]));

		VERBOSE(printf("lofar_udp_skip_to_packet: searching within current array starting index %ld (max %ld)...\n", (reader->meta->lastPacket - currentPacket) * reader->meta->portPacketLength[port], reader->meta->packetsPerIteration * reader->meta->portPacketLength[port]););
		VERBOSE(printf("lofar_udp_skip_to_packet: meta search: currentGuess %ld, 0th packet %ld, target %ld...\n", guessPacket, currentPacket, reader->meta->lastPacket););

		// If noo packets are dropped, just shift across to the data
		if (guessPacket == reader->meta->lastPacket) {
			packetShift[port] = reader->meta->packetsPerIteration - (reader->meta->lastPacket - currentPacket);
			
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
			packetShift[port] = reader->meta->packetsPerIteration - nextOff;
		}

		VERBOSE(printf("lofar_udp_skip_to_packet: exited loop, shifting data...\n"););

		// Shift the data on the port as needed
		returnVal = lofar_udp_shift_remainder_packets(reader, packetShift, 0);
		if (returnVal > 0) return 1;

		// Find the amount of data needed, and read in new data to fill the gap left at the end of the array after the shift
		nchars = (reader->meta->packetsPerIteration - packetShift[port]) * reader->meta->portPacketLength[port];
		returnLen = lofar_udp_reader_nchars(reader, port, &(reader->meta->inputData[port][reader->meta->inputDataOffset[port]]), nchars, reader->meta->inputDataOffset[port]);

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
lofar_udp_reader* lofar_udp_file_reader_setup(FILE **inputFiles, lofar_udp_meta *meta, const int compressedReader, lofar_udp_calibration *calibration) {
	int returnVal, bufferSize;
	static lofar_udp_reader reader;
	reader = lofar_udp_reader_default;

	// Initialise the reader struct as needed
	reader.compressedReader = compressedReader;
	reader.packetsPerIteration = meta->packetsPerIteration;
	reader.meta = meta;
	reader.calibration = calibration;

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
			bufferSize = meta->packetsPerIteration * meta->portPacketLength[port];
			VERBOSE(if (meta->VERBOSE) printf("reader_setup: expending decompression buffer by %ld bytes\n", bufferSize % ZSTD_DStreamOutSize()));
			bufferSize += bufferSize % ZSTD_DStreamOutSize();
			reader.decompressionTracker[port].size = bufferSize;
			reader.decompressionTracker[port].pos = 0; // Initialisation for our step-by-step reader
			reader.decompressionTracker[port].dst = reader.meta->inputData[port];
		}
	}

	// Gulp the first set of raw data
	returnVal = lofar_udp_reader_read_step(&reader);
	if (returnVal > 0) {
		lofar_udp_reader_cleanup_f(&reader, 0);
		return NULL;
	}
	reader.meta->inputDataReady = 0;

	VERBOSE(if (meta->VERBOSE) printf("reader_setup: First packet %ld\n", meta->lastPacket));

	// If we have been given a starting packet, search for it and align the data to it
	if (reader.meta->lastPacket > LFREPOCH) {
		returnVal = lofar_udp_skip_to_packet(&reader);
		if (returnVal > 0) {
			lofar_udp_reader_cleanup_f(&reader, 0);
			return NULL;
		}
	}

	VERBOSE(if (meta->VERBOSE) printf("reader_setup: Skipped, aligning to %ld\n", meta->lastPacket));

	// Align the first packet on each port, previous search may still have a 1/2 packet delta if there was packet loss
	returnVal = lofar_udp_get_first_packet_alignment(&reader);
	if (returnVal > 0) {
		lofar_udp_reader_cleanup_f(&reader, 0);
		return NULL;
	}

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

	// If we only had a partial read during the last iteration, finish filling the buffer
	if (reader->packetsPerIteration != reader->meta->packetsPerIteration) {
		#pragma omp parallel for
		for (int port = 0; port < reader->meta->numPorts; port++) {
			lofar_udp_reader_nchars(reader, port, &(reader->meta->inputData[port][reader->decompressionTracker[port].pos]), reader->packetsPerIteration * reader->meta->portPacketLength[port] - reader->decompressionTracker[port].pos, reader->decompressionTracker[port].pos);
		}
	}

	// Reset old variables
	reader->meta->packetsPerIteration = reader->packetsPerIteration;
	reader->meta->packetsRead = 0;
	reader->meta->packetsReadMax = startingPacket - reader->meta->lastPacket + 2 * reader->packetsPerIteration;
	reader->meta->lastPacket = startingPacket;
	reader->meta->calibrationStep = reader->calibration->calibrationStepsGenerated;

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
	reader->meta->packetsReadMax = localMaxPackets;
	reader->meta->inputDataReady = 1;
	reader->meta->outputDataReady = 0;
	return returnVal;
}


/**
 * @brief      Temporarily read in num bytes from a zstandard compressed file
 *
 * @param      outbuf     The output buffer pointer
 * @param[in]  size       The size of words ot read
 * @param[in]  num        The number of words to read
 * @param      inputFile  The input file
 * @param[in]  resetSeek  Do (1) / Don't (0) reset back to the original location
 *                        in FILE* after performing a read operation
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
		free(inBuff);
		free(outBuff);
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


	// Sanity check the processing mode
	switch (meta->processingMode) {
		case 0 ... 1:
			if (meta->calibrateData) {
				fprintf(stderr, "WARNING: Modes 0 and 1 cannot be calibrated, disabling calibration and continuing.\n");
				meta->calibrateData = 0;
			}
		case 2:
		case 10 ... 11:
		case 20 ... 21:
		case 30 ... 32:
		case 100 ... 104:
		case 110 ... 114:
		case 120 ... 124:
		case 130 ... 134:
		case 150 ... 154:
		case 160 ... 164:
			break;

		default:
			fprintf(stderr, "Unknown processing mode %d, exiting...\n", meta->processingMode);
			return 1;
	}

	// Define the output size per packet
	// Assume we are doing a copy operation by default
	meta->outputBitMode = meta->inputBitMode;

	// 4-bit data will (nearly) always be expanded to 8-bit chars
	if (meta->outputBitMode == 4) {
		meta->outputBitMode = 8;
	}

	switch (meta->processingMode) {
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
		#pragma GCC diagnostic push
		case 0:
			hdrOffset = 0; // include header
		#pragma GCC diagnostic pop
		case 1:
			meta->numOutputs = meta->numPorts;
			equalIO = 1;
			
			// These are the only processing modes that do not require 4-bit
			// data to be extracted. EqualIO will handle our sizes anyway
			if (meta->inputBitMode == 4) {
				meta->outputBitMode = 4;
			}
			break;

		case 2:
		case 11:
		case 21:
		case 31:
			meta->numOutputs = UDPNPOL;
			break;


		case 10:
		case 20:
		case 30:
			meta->numOutputs = 1;
			break;

		case 32:
			meta->numOutputs = 2;
			break;

		// Base Stokes Methods
		case 100:
		case 110:
		case 120:
		case 130:
			meta->numOutputs = 1;
			// 4 input words -> 1 larger word
			mulFactor = 1.0 / 4.0;
			meta->outputBitMode = 32;
			break;

		case 150:
			meta->numOutputs = 4;
			// 4 input words -> 1 larger word x 4
			// => Equivilent
			meta->outputBitMode = 32;
			break;

		case 160:
			meta->numOutputs = 2;
			// 4 input words -> 2 larger word x 4
			mulFactor = 2.0 / 4.0;
			meta->outputBitMode = 32;
			break;

		case 101:
		case 111:
		case 121:
		case 131:
		case 102:
		case 112:
		case 122:
		case 132:
		case 103:
		case 113:
		case 123:
		case 133:
		case 104:
		case 114:
		case 124:
		case 134:
			meta->numOutputs = 1;
			// Bit shift based on processing mode, 2^(mode % 10) * 4, 4 as in modes 100..130
			mulFactor = 1.0 / (float)  (1 << ((meta->processingMode % 10) + 2));
			meta->outputBitMode = 32;
			break;

		case 151:
		case 152:
		case 153:
		case 154:
			meta->numOutputs = 4;
			// Bit shift based on processing mode, 2^(mode % 10)
			mulFactor = 1.0 / (float)  (1 << (meta->processingMode % 10));
			meta->outputBitMode = 32;
			break;

		case 161:
		case 162:
		case 163:
		case 164:
			meta->numOutputs = 2;
			// Bit shift based on processing mode, 2^(mode % 10) * 2, 2 as in mode 160
			mulFactor = 1.0 / (float)  (1 << ((meta->processingMode % 10) + 1));
			meta->outputBitMode = 32;
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
		// Calculate the number of output char-sized elements
		workingData = (meta->numPorts * (hdrOffset + UDPHDRLEN)) + meta->totalProcBeamlets * UDPNPOL * ((float) meta->inputBitMode / 8.0) * UDPNTIMESLICE;

		// Adjust for scaling factor, output bitmode and number of outputs
		workingData = (int) (workingData * ((float) meta->outputBitMode / (float) meta->inputBitMode) * mulFactor);
		workingData /= meta->numOutputs;

		for (int out = 0; out < meta->numOutputs; out++ ) { 
			meta->packetOutputLength[out] = workingData;
		}
	}

	return 0;
}



/**
 * @brief      Old API access
 *
 * @param      inputFiles            An array of input files (max MAX_NUM_PORTS)
 * @param[in]  numPorts              The number ports to process (max
 *                                   MAX_NUM_PORTS)
 * @param[in]  replayDroppedPackets  1: Enable 0: Disable replaying the last
 *                                   packet (otherwise pad with 0s)
 * @param[in]  processingMode        The processing mode
 * @param[in]  verbose               1: Enable verbosity, 0: Disable verbosity
 *                                   (only if enabled at compile time)
 * @param[in]  packetsPerIteration   The packets per read/process iteration
 * @param[in]  startingPacket        The starting packet number (LOFAR packet
 *                                   number, not an offset)
 * @param[in]  packetsReadMax        The maximum number of packets to read over
 *                                   the lifetime of the object
 * @param[in]  compressedReader      Enable / disable zstandard decompression on
 *                                   the input data
 *
 * @return     lofar_udp_reader ptr, or NULL on error
 */
lofar_udp_reader* lofar_udp_meta_file_reader_setup(FILE **inputFiles, const int numPorts, const int replayDroppedPackets, const int processingMode, const int verbose, const long packetsPerIteration, const long startingPacket, const long packetsReadMax, const int compressedReader) {
	static lofar_udp_config config;
	// GCC is fine with assignment at definition, icc is not.
	config = lofar_udp_config_default;
	config.inputFiles = inputFiles;
	config.numPorts = numPorts;
	config.replayDroppedPackets = replayDroppedPackets;
	config.processingMode = processingMode;
	config.verbose = verbose;
	config.packetsPerIteration = packetsPerIteration;
	config.startingPacket = startingPacket;
	config.packetsReadMax = packetsReadMax;
	config.compressedReader = compressedReader;

	// Old call did not support beamlet limits, don't update.
	config.beamletLimits[0] = 0;
	config.beamletLimits[1] = 0;
	
	return lofar_udp_meta_file_reader_setup_struct(&config);
}

/**
 * @brief      Set up a lofar_udp_reader and assoaicted lofar_udp_meta using a
 *             set of input files and pre-set control/metadata parameters
 *
 * @param      config  The configuration struct, detailed options above
 *
 * @return     lofar_udp_reader ptr, or NULL on error
 */
lofar_udp_reader* lofar_udp_meta_file_reader_setup_struct(lofar_udp_config *config) {
	
	if (config->numPorts > MAX_NUM_PORTS) {
		fprintf(stderr, "ERROR: You requested %d ports, but LOFAR can only produce %d, exiting.\n", config->numPorts, MAX_NUM_PORTS);
		return NULL;
	}
	if (config->packetsPerIteration < 1) {
		fprintf(stderr, "ERROR: Packets per iteration indicates no work will be performed (%ld per iteration), exiting.\n", config->packetsPerIteration);
		return NULL;
	}
	if (config->beamletLimits[0] > 0 && config->beamletLimits[1] > 0) {
		if (config->beamletLimits[0] > config->beamletLimits[1]) {
			fprintf(stderr, "ERROR: Upper beamlet limit is lower than the lower beamlet limit. Please fix your ordering (%d, %d), exiting.\n", config->beamletLimits[0], config->beamletLimits[1]);
			return NULL;
		}

		if (config->processingMode < 2) {
			fprintf(stderr, "ERROR: Processing modes 0 and 1 do not support setting beamlet limits, exiting.\n");
			return NULL;
		}
	}

	// Setup the metadata struct and a few variables we'll need
	static lofar_udp_meta meta;
	meta = lofar_udp_meta_default;
	char inputHeaders[MAX_NUM_PORTS][UDPHDRLEN];
	int readlen, bufferSize;
	long localMaxPackets = config->packetsReadMax;

	// Reset the maximum packets to LONG_MAX if set to an unreasonable value
	if (config->packetsReadMax < 0) localMaxPackets = LONG_MAX;

	// Set the simple metadata defaults
	meta.numPorts = config->numPorts;
	meta.replayDroppedPackets = config->replayDroppedPackets;
	meta.processingMode = config->processingMode;
	meta.packetsPerIteration = config->packetsPerIteration;
	meta.packetsReadMax = localMaxPackets;
	meta.lastPacket = config->startingPacket;
	meta.calibrateData = config->calibrateData;
	
	VERBOSE(meta.VERBOSE = config->verbose);
	#ifndef ALLOW_VERBOSE
	if (config->verbose) fprintf(stderr, "Warning: verbosity was disabled at compile time, but you requested it. Continuing...\n");
	#endif


	// Scan in the first header on each port
	for (int port = 0; port < meta.numPorts; port++) {
		
		if (config->compressedReader)  {
			readlen = fread_temp_ZSTD(&(inputHeaders[port][0]), sizeof(char), 16, config->inputFiles[port], 1);
		} else  {
			readlen = fread(&(inputHeaders[port]), sizeof(char), 16, config->inputFiles[port]);
			fseek(config->inputFiles[port], -16, SEEK_CUR);
		}

		if (readlen != 16) {
			fprintf(stderr, "Unable to read header on port %d, exiting.\n", port);
			return NULL;
		}

	}


	// Parse the input file headers to get packet metadata on each port
	// We may repeat this step if the selected beamlets cause us to drop a port of dataa
	int updateBeamlets = (config->beamletLimits[0] > 0 || config->beamletLimits[1] > 0);
	int beamletLimits[2] = { 0, 0 };
	while (updateBeamlets != -1) {
		VERBOSE(if (meta.VERBOSE) printf("Handle headers: %d\n", updateBeamlets););
		// Standard setup
		if (lofar_udp_parse_headers(&meta, inputHeaders, beamletLimits) > 0) {
			fprintf(stderr, "Unable to setup meadata using given headers; exiting.\n");
			return NULL;

		// If we are only parsing a subset of beamlets
		} else if (updateBeamlets) {
			VERBOSE(if (meta.VERBOSE) printf("Handle headers chain: %d\n", updateBeamlets););
			int lowerPort = 0;
			int upperPort = meta.numPorts - 1;

			// Iterate over the given ports
			for (int port = 0; port < meta.numPorts; port++) {
				// Check if the lower limit is on the given port
				if (config->beamletLimits[0] > 0) {
					if ((meta.portRawCumulativeBeamlets[port] <= config->beamletLimits[0]) && ((meta.portRawCumulativeBeamlets[port] + meta.portRawBeamlets[port]) > config->beamletLimits[0] )) {
						VERBOSE(if (meta.VERBOSE) printf("Lower beamlet %d found on port %d\n", config->beamletLimits[0], port););
						lowerPort = port;
					}
				}

				// Check if the upper limit is on the given port
				if (config->beamletLimits[1] > 0) {
					if ((meta.portRawCumulativeBeamlets[port] < config->beamletLimits[1]) && ((meta.portRawCumulativeBeamlets[port] + meta.portRawBeamlets[port]) >= config->beamletLimits[1] )) {
						VERBOSE(if (meta.VERBOSE) printf("Upper beamlet %d found on port %d\n", config->beamletLimits[1], port););
						upperPort = port;
					}
				}
			}

			// Sanity check before progressing
			if (lowerPort > upperPort) {
				fprintf(stderr, "ERROR: Upon updating beamletLimits, we found the upper beamlet is in a port higher than the lower port (%d, %d), exiting.\n", upperPort, lowerPort);
				return NULL;
			}

			if (lowerPort != 0) {
				// Shift input files down
				for (int port = lowerPort; port <= upperPort; port++) {
					config->inputFiles[port - lowerPort] = config->inputFiles[port];
					// GCC 10 has a warning abot this line. Why?
					memcpy(&(inputHeaders[port - lowerPort][0]), &(inputHeaders[port][0]), UDPHDRLEN);
				}

				// Close unneeded files
				for (int port = upperPort + 1; port < config->numPorts; port++) {
					fclose(config->inputFiles[port]);
				}

				// Update beamlet limits to be relative to the new ports
				config->beamletLimits[0] -= meta.portRawCumulativeBeamlets[lowerPort];
				config->beamletLimits[1] -= meta.portRawCumulativeBeamlets[lowerPort];

			}

			// If we are dropping any ports, update numPorts
			if ((lowerPort != 0) || ((upperPort + 1) != config->numPorts)) {
				meta.numPorts = (upperPort + 1) - lowerPort;
			}

			VERBOSE(if (meta.VERBOSE) printf("New numPorts: %d\n", meta.numPorts););

			// Update updateBeamlets so that we can start the loop again, but not enter this code block.
			updateBeamlets = 0;
			// Update the limits to our current limits.
			beamletLimits[0] = config->beamletLimits[0];
			beamletLimits[1] = config->beamletLimits[1];
		} else {
			VERBOSE(if (meta.VERBOSE) printf("Handle headers: %d\n", updateBeamlets););
			updateBeamlets = -1;
		}
	}


	if (lofar_udp_setup_processing(&meta)) {
		fprintf(stderr, "Unable to setup processing mode %d, exiting.\n", config->processingMode);
		return NULL;
	}


	// Allocate the memory needed to store the raw / reprocessed data, initlaise the variables that are stored on a per-port basis.
	for (int port = 0; port < meta.numPorts; port++) {
		// Ofset input by 2 for a zero/buffer packet on boundary
		// If we have a compressed reader, align the length with the ZSTD buffer sizes
		bufferSize = (meta.portPacketLength[port] * (meta.packetsPerIteration)) % ZSTD_DStreamOutSize();
		meta.inputData[port] = calloc(meta.portPacketLength[port] * (meta.packetsPerIteration + 2) + bufferSize * config->compressedReader, sizeof(char)) + (meta.portPacketLength[port] * 2);
		VERBOSE(if(meta.VERBOSE) printf("calloc at %p for %ld +(%d) bytes\n", meta.inputData[port] - (meta.portPacketLength[port] * 2), meta.portPacketLength[port] * (meta.packetsPerIteration + 2) + bufferSize * config->compressedReader - meta.portPacketLength[port] * 2, meta.portPacketLength[port] * 2););

		// Initalise these arrays while we're looping
		meta.inputDataOffset[port] = 0;
		meta.portLastDroppedPackets[port] = 0;
		meta.portTotalDroppedPackets[port] = 0;
	}

	for (int out = 0; out < meta.numOutputs; out++) {
		meta.outputData[out] = calloc(meta.packetOutputLength[out] * meta.packetsPerIteration, sizeof(char));
		VERBOSE(if(meta.VERBOSE) printf("calloc at %p for %ld bytes\n", meta.outputData[out], meta.packetOutputLength[out] * meta.packetsPerIteration););
	}



	VERBOSE(if (meta.VERBOSE) {
		printf("Meta debug:\ntotalBeamlets %d, numPorts %d, replayDroppedPackets %d, processingMode %d, outputBitMode %d, packetsPerIteration %ld, packetsRead %ld, packetsReadMax %ld, lastPacket %ld, \n",
				meta.totalRawBeamlets, meta.numPorts, meta.replayDroppedPackets, meta.processingMode, meta.outputBitMode, meta.packetsPerIteration, meta.packetsRead, meta.packetsReadMax, meta.lastPacket);

		for (int i = 0; i < meta.numPorts; i++) {
			printf("Port %d: inputDataOffset %ld, portBeamlets %d, cumulativeBeamlets %d, inputBitMode %d, portPacketLength %d, packetOutputLength %d, portLastDroppedPackets %d, portTotalDroppedPackets %d\n", i, 
				meta.inputDataOffset[i], meta.portRawBeamlets[i], meta.portCumulativeBeamlets[i], meta.inputBitMode, meta.portPacketLength[i], meta.packetOutputLength[i], meta.portLastDroppedPackets[i], meta.portTotalDroppedPackets[i]);

		for (int i = 0; i < meta.numOutputs; i++) printf("Output %d, packetLength %d, numOut %d\n", i, meta.packetOutputLength[i], meta.numOutputs);
	}});


	// Form a reader using the given metadata and input files
	return lofar_udp_file_reader_setup(config->inputFiles, &meta, config->compressedReader, config->calibrationConfiguration);
}


/**
 * @brief      Close input files, free alloc'd memory, free zstd decompression
 *             streams once we are finished.
 *
 * @param[in]  reader  The lofar_udp_reader struct to cleanup
 *
 * @return     int: 0: Success, other: ???
 */
int lofar_udp_reader_cleanup(lofar_udp_reader *reader) {
	return lofar_udp_reader_cleanup_f(reader, 1);
}


/**
 * @brief      Optionally lose input files, free alloc'd memory, free zstd
 *             decompression streams once we are finished.
 *
 * @param[in]  reader      The lofar_udp_reader struct to cleanup
 * @param[in]  closeFiles  bool: close input files (1) or don't (0)
 *
 * @return     int: 0: Success, other: ???
 */
int lofar_udp_reader_cleanup_f(lofar_udp_reader *reader, const int closeFiles) {

	// Cleanup the malloc/calloc'd memory addresses, close the input files.
	for (int i = 0; i < reader->meta->numOutputs; i++) {
		if (reader->meta->outputData[i] != NULL) {
			free(reader->meta->outputData[i]);
		}
	}

	for (int i = 0; i < reader->meta->numPorts; i++) {
		// Free input data pointer (from the correct offset)
		if (reader->meta->inputData[i] != NULL) {
			VERBOSE(if(reader->meta->VERBOSE) printf("On port: %d freeing inputData at %p\n", i, reader->meta->inputData[i] - 2 * reader->meta->portPacketLength[i]););
			free(reader->meta->inputData[i] - 2 * reader->meta->portPacketLength[i]);
			reader->meta->inputData[i] = NULL;
		}

		// Close the input file
		if (reader->fileRef[i] != NULL && closeFiles) {
			VERBOSE(if(reader->meta->VERBOSE) printf("On port: %d closing file\n", i))
			fclose(reader->fileRef[i]);
			reader->fileRef[i] = NULL;
		}

		if (reader->compressedReader) {
			// Free the decomression stream
			if (reader->dstream[i] != NULL) {
				VERBOSE(if(reader->meta->VERBOSE) printf("Freeing decompression buffers and ZSTD stream on port %d\n", i););
				ZSTD_freeDStream(reader->dstream[i]);
				reader->dstream[i] = NULL;
			}

			if (reader->inBuffer[i] != NULL) {
				// Free the decompression input buffer
				free(reader->inBuffer[i]);
				reader->inBuffer[i] = NULL;
			}
		}
	}

	return 0;
}



/**
 * @brief      Generate the inverted Jones matrix to calibrate the observed data
 *
 * @param      reader  The lofar_udp_reader
 *
 * @return     int: 0: Success, 1: Failure
 */
int lofar_udp_reader_calibration(lofar_udp_reader *reader) {
	FILE *fifo;
	int numBeamlets, numTimesamples, returnVal;
	// Ensure we are meant to be calibrating data
	if (!reader->meta->calibrateData) {
		fprintf(stderr, "ERROR: Requested calibration while calibration is disabled. Exiting.\n");
		return 1;
	}


	// Make a FIFO pipe to communicate with dreamBeam
	printf("Making fifio\n");
	if (access(reader->calibration->calibrationFifo, F_OK) != -1) {
		if (remove(reader->calibration->calibrationFifo) != 0) {
			fprintf(stderr, "ERROR: Unable to cleanup old claibration FIFO (%d). Exiting.\n", errno);
			return 1;
		}
	}

	returnVal = mkfifo(reader->calibration->calibrationFifo, 0666);

	if (returnVal < 0) {
		fprintf(stderr, "ERROR: Unable to create FIFO pipe at %s (%d). Exiting.\n", reader->calibration->calibrationFifo, errno);
		return 1;
	}

	// Call dreamBeam to generate calibration
	// dreamBeamJonesGenerator.py --stn STNID --sub ANT,SBL:SBU --time TIME --dur DUR --int INT --pnt P0,P1,BASIS --pipe /tmp/pipe 
	char stationID[16] = "", unixTime[16] = "", duration[16] = "", integration[16] = "", pointing[512] = "";
	

	lofar_get_station_name(reader->meta->stationID, &(stationID[0]));
	sprintf(unixTime, "%d", *((int*) &(reader->meta->inputData[UDPHDROFF + 0][8])));
	sprintf(duration, "%15.4f", reader->calibration->calibrationDuration);
	sprintf(integration, "%15.10f", (float) (reader->packetsPerIteration * UDPNTIMESLICE) * (float) (clock200MHzSample * reader->meta->clockBit + clock160MHzSample * (1 - reader->meta->clockBit)));
	sprintf(pointing, "%f,%f,%s", reader->calibration->calibrationPointing[0], reader->calibration->calibrationPointing[1], reader->calibration->calibrationPointingBasis);
	printf("Calling dreamBeam: %s %s %s %s %s %s %s\n", stationID, unixTime, reader->calibration->calibrationSubbands, duration, integration, pointing, reader->calibration->calibrationFifo);
	char *argv[] = { "dreamBeamJonesGenerator.py", "--stn", stationID,  "--time", unixTime, "--sub", reader->calibration->calibrationSubbands, "--dur", duration, "--int", integration, "--pnt",  pointing, "--pipe", reader->calibration->calibrationFifo, NULL };
	pid_t pid = fork();

	printf("Fork\n");
	if (pid == 0) {
		execvp(argv[0], argv);
		_exit(1);
	} else  if (pid < 0) {
		fprintf(stderr, "ERROR: Unable to create child process to call dreamBeam. Exiting.\n");
		return 1;
	}

	printf("OpeningFifo\n");
	fifo = fopen(reader->calibration->calibrationFifo, "rb");

	// Get the number of time steps and frequency channles 
	returnVal = fscanf(fifo, "%d,%d\n", &numTimesamples, &numBeamlets);
	if (returnVal < 0) {
		fprintf(stderr, "ERROR: Failed to parse number of beamlets and time samples from dreamBeam. Exiting.\n");
		return 1;
	}


	printf("beamlets\n");
	// Ensure the calibration strategy matches the number of subbands we are processing
	if (numBeamlets != reader->meta->totalProcBeamlets) {
		fprintf(stderr, "ERROR: Calibration strategy returned %d beamlets, but we are setup to handle %d. Exiting. \n", numBeamlets, reader->meta->totalProcBeamlets);
		return 1;
	}

	printf("%d, %d\n", numTimesamples, numBeamlets);
	// Check if we already allocated storage for the calibration data, re-use already alloc'd data where possible
	if (reader->meta->jonesMatrices == NULL) {
		// Allocate numTimesamples * numBeamlets * (4 pmatrix elements) * (2 complex values per element)
		reader->meta->jonesMatrices = malloc(numTimesamples * sizeof(float*));
		for (int timeIdx = 0; timeIdx < numTimesamples; timeIdx += 1) {
			reader->meta->jonesMatrices[timeIdx] = aligned_alloc(numBeamlets * 8 * sizeof(float), 8);
		}
	// If we returned more time samples than last time, reallocate the array
	} else if (numTimesamples > reader->calibration->calibrationStepsGenerated) {
		// Free the old data
		for (int timeIdx = 0; timeIdx < reader->calibration->calibrationStepsGenerated; timeIdx += 1) {
			free(reader->meta->jonesMatrices[timeIdx]);
		}
		free(reader->meta->jonesMatrices);

		// Reallocate the data
		reader->meta->jonesMatrices = malloc(numTimesamples * sizeof(float*));
		for (int timeIdx = 0; timeIdx < numTimesamples; timeIdx += 1) {
			reader->meta->jonesMatrices[timeIdx] = aligned_alloc(numBeamlets * 8 * sizeof(float), 8);
		}
	// If less time samples, free the remaining steps and re-use the array
	} else if (numTimesamples < reader->calibration->calibrationStepsGenerated) {
		// Free the extra time steps
		for (int timeIdx = reader->calibration->calibrationStepsGenerated; timeIdx < numTimesamples; timeIdx += 1) {
			free(reader->meta->jonesMatrices[timeIdx]);
		}
	}

	printf("FIFO Parse\n");
	// Index into the jonesMatrices array
	long baseOffset = 0;
	for (int timeIdx = 0; timeIdx < numTimesamples; timeIdx += 1) {
		baseOffset = 0;
		for (int freqIdx = 0; freqIdx < numBeamlets - 1; freqIdx += 1) {
			// parse the FIFO to get the data
			returnVal = fscanf(fifo, "%f,%f,%f,%f,%f,%f,%f,%f,", 	&reader->meta->jonesMatrices[timeIdx][baseOffset], \
														&reader->meta->jonesMatrices[timeIdx][baseOffset + 1], \
														&reader->meta->jonesMatrices[timeIdx][baseOffset + 2], \
														&reader->meta->jonesMatrices[timeIdx][baseOffset + 3], \
														&reader->meta->jonesMatrices[timeIdx][baseOffset + 4], \
														&reader->meta->jonesMatrices[timeIdx][baseOffset + 5], \
														&reader->meta->jonesMatrices[timeIdx][baseOffset + 6], \
														&reader->meta->jonesMatrices[timeIdx][baseOffset + 7]);

			if (returnVal < 0) {
				fprintf(stderr, "ERROR: unable to parse main pipe from dreamBeam (%d, %d). Exiting.\n", timeIdx, freqIdx);
			}

			baseOffset += 8;
		}

		// Parse the final beamlet sample, confirm it ends with a | to signify the end of the time sample
		returnVal = fscanf(fifo, "%f,%f,%f,%f,%f,%f,%f,%f|", 	&reader->meta->jonesMatrices[timeIdx][baseOffset], \
													&reader->meta->jonesMatrices[timeIdx][baseOffset + 1], \
													&reader->meta->jonesMatrices[timeIdx][baseOffset + 2], \
													&reader->meta->jonesMatrices[timeIdx][baseOffset + 3], \
													&reader->meta->jonesMatrices[timeIdx][baseOffset + 4], \
													&reader->meta->jonesMatrices[timeIdx][baseOffset + 5], \
													&reader->meta->jonesMatrices[timeIdx][baseOffset + 6], \
													&reader->meta->jonesMatrices[timeIdx][baseOffset + 7]);
		if (returnVal < 0) {
			fprintf(stderr, "ERROR: unable to parse final pipe from dreamBeam (%d). Exiting.\n", timeIdx);
		}

	}

	printf("%f, %f, %f, %f... %f, %f, %f, %f\n", reader->meta->jonesMatrices[0][0], reader->meta->jonesMatrices[0][1], reader->meta->jonesMatrices[0][2], reader->meta->jonesMatrices[0][3], reader->meta->jonesMatrices[0][baseOffset], reader->meta->jonesMatrices[0][baseOffset + 1], reader->meta->jonesMatrices[0][baseOffset + 2], reader->meta->jonesMatrices[0][baseOffset + 3]);


	printf("Cleanup\n");
	// Close and remove the pipe
	fclose(fifo);
	if (remove(reader->calibration->calibrationFifo) != 0) {
		fprintf(stderr, "ERROR: Unable to remove claibration FIFO (%d). Exiting.\n", errno);
		return 1;
	}

	// Update the calibration state
	reader->meta->calibrationStep = 0;
	reader->calibration->calibrationStepsGenerated = numTimesamples;

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
 * @param[in]  knownOffset  The compressed reader's known offset
 *
 * @return     long: bytes read
 */
long lofar_udp_reader_nchars(lofar_udp_reader *reader, const int port, char *targetArray, const long nchars, const long knownOffset) {
	// Return if we have nothing to do
	if (nchars < 0) return -1;

	if (reader->compressedReader) {
		// Compressed file: Perform streaming decompression on a zstandard compressed file
		VERBOSE(if (reader->meta->VERBOSE) printf("reader_nchars: Entering read request (compressed): %d, %ld, %ld\n", port, nchars, knownOffset));

		long dataRead = 0;
		size_t previousDecompressionPos = 0;
		int compDataRead = 0, byteDelta = 0, returnVal = 0;

		// Ensure the decompression buffer has been updated
		reader->decompressionTracker[port].pos = knownOffset;


		// Loop until we hit an exit criteria (EOF / zstd error / nchars)
		// Suspicion after buffer changes: this might enter an infinite loop if the buffer isn't large enough to hold the data
		while(1) {
			VERBOSE(if (reader->meta->VERBOSE) printf("reader_nchars: start of read loop, %ld, %ld, %ld, %ld\n", reader->readingTracker[port].pos, reader->readingTracker[port].size, reader->decompressionTracker[port].pos, dataRead););

			// Check the decompression stream for compressed data
			if (reader->readingTracker[port].pos != reader->readingTracker[port].size) {
				// Loop across while decompressing the data (zstd decompressed in frame iterations, so it may take a few iterations)
				while (reader->readingTracker[port].pos < reader->readingTracker[port].size) {
					previousDecompressionPos = reader->decompressionTracker[port].pos;
					// zstd streaming decompression + check for errors
					returnVal = ZSTD_decompressStream(reader->dstream[port], &(reader->decompressionTracker[port]), &(reader->readingTracker[port]));
					if (ZSTD_isError(returnVal)) {
						fprintf(stderr, "ZSTD encountered an error decompressing a frame (code %d, %s), exiting data read early.\n", returnVal, ZSTD_getErrorName(returnVal));
						return dataRead;
					}

					// Determine how much data we just added to the buffer
					byteDelta = ((long) reader->decompressionTracker[port].pos - (long) previousDecompressionPos);

					// Update the total data read + check if we have reached our goal
					dataRead += byteDelta;
					VERBOSE(if (dataRead >= nchars) {
						if (reader->meta->VERBOSE) printf("Reader terminating: %ld read, %ld requested, %ld\n", dataRead, nchars, nchars - dataRead);
					});
					if (dataRead >= nchars) return dataRead;

					if (reader->decompressionTracker[port].pos == reader->decompressionTracker[port].size) {
						fprintf(stderr, "Failed to read %ld/%ld chars on port %d before filling the buffer. Attempting to continue...\n", dataRead, nchars, port);
						return dataRead;
					}
				}
			}

			VERBOSE(if (reader->meta->VERBOSE) printf("reader>nchars: mdddle of read loop, %ld, %ld, %ld, %ld\n", reader->readingTracker[port].pos, reader->readingTracker[port].size, reader->decompressionTracker[port].pos, dataRead););

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

	// If packets were dropped, shift the remaining packets back to the start of the array
	if ((checkReturnValue = lofar_udp_shift_remainder_packets(reader, reader->meta->portLastDroppedPackets, 1)) > 0) return 1;

	// Ensure we aren't passed the read length cap
	if (reader->meta->packetsRead >= (reader->meta->packetsReadMax - reader->meta->packetsPerIteration)) {
		reader->meta->packetsPerIteration = reader->meta->packetsReadMax - reader->meta->packetsRead;
		VERBOSE(if(reader->meta->VERBOSE) printf("Processing final read (%ld packets) before reaching maximum packet cap.\n", reader->meta->packetsPerIteration));
		returnVal = -2;
	}

	//else if (checkReturnValue < 0) if(lofar_udp_realign_data(reader) > 0) return 1;
	
	// Read in the required new data
	omp_set_num_threads(OMP_THREADS);
	#pragma omp parallel for shared(returnVal)
	for (int port = 0; port < reader->meta->numPorts; port++) {
		long charsToRead, charsRead, packetPerIter;
		
		// Determine how much data is needed and read-in to the offset after any leftover packets
		charsToRead = (reader->meta->packetsPerIteration - reader->meta->portLastDroppedPackets[port]) * reader->meta->portPacketLength[port];
		charsRead = lofar_udp_reader_nchars(reader, port, &(reader->meta->inputData[port][reader->meta->inputDataOffset[port]]), charsToRead, reader->meta->inputDataOffset[port]);

		// Raise a warning if we received less data than requested (EOF/file error)
		if (charsRead < charsToRead) {

			packetPerIter = (charsRead / reader->meta->portPacketLength[port]) + reader->meta->portLastDroppedPackets[port];
			#pragma omp critical (packetCheck) 
			{
			if(packetPerIter < reader->meta->packetsPerIteration) {
				// Sanity check, out of order packets can cause a negiative value here
				if (packetPerIter > 0) {
					reader->meta->packetsPerIteration = packetPerIter;
				} else {
					reader->meta->packetsPerIteration = 0;
				}
				fprintf(stderr, "Received less data from file on port %d than expected, may be nearing end of file.\nReducing packetsPerIteration to %ld, to account for the limited amount of input data.\n", port, reader->meta->packetsPerIteration);
			}
			#ifdef __SLOWDOWN
			sleep(5);
			#endif

			#pragma omp atomic write
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

	printf("Check cal %d, %d\n", reader->meta->calibrationStep, reader->calibration->calibrationStepsGenerated);
	if (reader->meta->calibrateData && reader->meta->calibrationStep >= reader->calibration->calibrationStepsGenerated) {
		printf("Start cal\n");
		VERBOSE(if (reader->meta->VERBOSE) printf("Calibration buffer has run out, generating new Jones matrices.\n"));
		if ((readReturnVal > lofar_udp_reader_calibration(reader))) {
			return readReturnVal;
		}
	}

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


	VERBOSE(if (reader->meta->VERBOSE) printf("reader_step ready2: %d, %d, %d\n", reader->meta->inputDataReady, reader->meta->outputDataReady, reader->meta->packetsPerIteration > 0));
	// Make sure there is a new input data set before running
	// On the setup iteration, the output data is marked as ready to prevent this occurring until the first read step is called
	if (reader->meta->outputDataReady != 1 && reader->meta->packetsPerIteration > 0) {
		if ((stepReturnVal = lofar_udp_cpp_loop_interface(reader->meta)) > 0) {
			return stepReturnVal;
		}
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
 * @return     int: 0: Success, < 0: Some data issues, but tolerable: > 1: Fatal
 *             errors
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
		returnVal = lofar_udp_shift_remainder_packets(reader, shiftPackets, 0);
		VERBOSE(if (reader->meta->VERBOSE) printf("Packets shifted, returnVal %d\n", returnVal));
		if (returnVal > 0) return returnVal;

		// Read in new data to the ports that required it, could probably `pragma omp parallel for` this
		for (int port = 0; port < reader->meta->numPorts; port++) {
			VERBOSE(if (reader->meta->VERBOSE) printf("Entering post-fixup loop port %d, dropped %d\n", port, reader->meta->portLastDroppedPackets[port]));
			
			// Check if a port needs to be topped up
			if (reader->meta->portLastDroppedPackets[port] > 0) {
				// Determine how much data is needed and read it in
				nchars = reader->meta->portLastDroppedPackets[port] * reader->meta->portPacketLength[port];
				returnLen = lofar_udp_reader_nchars(reader, port, &(reader->meta->inputData[port][reader->meta->inputDataOffset[port]]), nchars, reader->meta->inputDataOffset[port]);
				
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
	meta->lastPacket = maxPacketNumber - 1;
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
 * @param      reader         The udp reader struct
 * @param      shiftPackets   [PORTS] int array of number of packets to shift
 *                            from the tail of each port by
 * @param[in]  handlePadding  Allow the function to handle copying the last
 *                            packet to the padding offset
 * @param      meta  The input lofar_udp_meta struct to process
 *
 * @return     int: 0: Success, -1: Negative shift requested, out of order data
 *             on last gulp,
 */
int lofar_udp_shift_remainder_packets(lofar_udp_reader *reader, const int shiftPackets[], const int handlePadding) {

	int returnVal = 0, fixBuffer = 0;
	int packetShift, destOffset, portPacketLength;
	long byteShift, sourceOffset;

	char *inputData;
	int totalShift = 0;

	lofar_udp_meta *meta = reader->meta;
	

	// Check if we have any work to do, reset offsets, exit if no work requested
	for (int port = 0; port < meta->numPorts; port++) {
		meta->inputDataOffset[port] = 0;
		totalShift += shiftPackets[port];

		if (reader->compressedReader) {
			if ((long) reader->decompressionTracker[port].pos > meta->portPacketLength[port] * meta->packetsPerIteration) {
				fixBuffer = 1;
			}
		}
	}

	if (totalShift < 1 && fixBuffer == 0) return 0;


	// Shift the data on each port
	for (int port = 0; port < meta->numPorts; port++) {
		inputData = meta->inputData[port];
		packetShift = shiftPackets[port];


		VERBOSE(if (meta->VERBOSE) printf("shift_remainder: Port %d packet shift %d padding %d\n", port, packetShift, handlePadding));

		if (packetShift > reader->packetsPerIteration) {
			fprintf(stderr, "\nWARNING: Requested packet shift is larger than the size of our input buffer. Adjusting port %d from %d to %ld.\n", port, packetShift, reader->packetsPerIteration);
			packetShift = reader->packetsPerIteration;
		}

 		// If we have packet shift or want to copy the final packet for future refernece
		if (packetShift > 0 || handlePadding == 1 || (handlePadding == 1 && fixBuffer == 1)) {

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

			VERBOSE(if (meta->VERBOSE) printf("P: %d, SO: %ld, DO: %d, BS: %ld IDO: %ld\n", port, sourceOffset, destOffset, byteShift, destOffset + byteShift));

			if (reader->compressedReader) {
				if ((long) reader->decompressionTracker[port].pos > meta->portPacketLength[port] * meta->packetsPerIteration) {
					byteShift += reader->decompressionTracker[port].pos - meta->portPacketLength[port] * meta->packetsPerIteration;
				}
				reader->decompressionTracker[port].pos = destOffset + byteShift;
				VERBOSE(if (meta->VERBOSE) printf("Compressed offset: P: %d, SO: %ld, DO: %d, BS: %ld IDO: %ld\n", port, sourceOffset, destOffset, byteShift, destOffset + byteShift));

			}

			VERBOSE(if (meta->VERBOSE) printf("P: %d, SO: %ld, DO: %d, BS: %ld IDO: %ld\n", port, sourceOffset, destOffset, byteShift, destOffset + byteShift));

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
