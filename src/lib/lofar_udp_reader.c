#include "lofar_udp_reader.h"
#include "lofar_udp_time.h"



lofar_udp_reader* lofar_udp_reader_alloc() {
	lofar_udp_reader *reader = calloc(1, sizeof(lofar_udp_reader));
	(*reader) = lofar_udp_reader_default;
	lofar_udp_io_read_config *input = lofar_udp_io_alloc_read();
	reader->input = input;

	return reader;
}

/**
 * @brief      Parse LOFAR UDP headers to determine some metadata about the
 *             ports
 *
 * @param      meta           The lofar_udp_input_meta to initialise
 * @param[in]  header         The header data to process
 * @param[in]  beamletLimits  The upper/lower beamlets limits
 *
 * @return     0: Success, -1: Fatal error
 */
int lofar_udp_parse_headers(lofar_udp_input_meta *meta, char header[MAX_NUM_PORTS][UDPHDRLEN], const int beamletLimits[2]) {

	lofar_source_bytes *source;

	float bitMul;
	int cacheBitMode = 0;
	meta->totalRawBeamlets = 0;
	meta->totalProcBeamlets = 0;


	// Process each input port
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (meta->VERBOSE) { printf("Port %d/%d\n", port, meta->numPorts - 1); });
		// Data integrity checks
		if ((unsigned char) header[port][CEP_HDR_RSP_VER_OFFSET] < UDPCURVER) {
			fprintf(stderr, "Input header on port %d appears malformed (RSP Version less than 3), exiting.\n", port);
			return -1;
		}

		if (*((unsigned int *) &(header[port][CEP_HDR_TIME_OFFSET])) < LFREPOCH) {
			fprintf(stderr, "Input header on port %d appears malformed (data timestamp before 2008), exiting.\n", port);
			return -1;
		}

		if (*((unsigned int *) &(header[port][CEP_HDR_SEQ_OFFSET])) > RSPMAXSEQ) {
			fprintf(stderr,
					"Input header on port %d appears malformed (sequence higher than 200MHz clock maximum, %d), exiting.\n",
					port, *((unsigned int *) &(header[port][CEP_HDR_SEQ_OFFSET])));
			return -1;
		}

		if ((unsigned char) header[port][CEP_HDR_NBEAM_OFFSET] > UDPMAXBEAM) {
			fprintf(stderr,
					"Input header on port %d appears malformed (more than %d beamlets on a port, %d), exiting.\n", port,
					UDPMAXBEAM, header[port][CEP_HDR_NBEAM_OFFSET]);
			return -1;
		}

		if ((unsigned char) header[port][CEP_HDR_NTIMESLICE_OFFSET] != UDPNTIMESLICE) {
			fprintf(stderr,
					"Input header on port %d appears malformed (time slices are %d, not UDPNTIMESLICE), exiting.\n",
					port, header[port][CEP_HDR_NTIMESLICE_OFFSET]);
			return -1;
		}

		source = (lofar_source_bytes *) &(header[port][CEP_HDR_SRC_OFFSET]);
		if (source->padding0 != (unsigned int) 0) {
			fprintf(stderr, "Input header on port %d appears malformed (padding bit (0) is set), exiting.\n", port);
			return -1;
		} else if (source->errorBit != (unsigned int) 0) {
			fprintf(stderr, "Input header on port %d appears malformed (error bit is set), exiting.\n", port);
			return -1;
		} else if (source->bitMode == (unsigned int) 3) {
			fprintf(stderr, "Input header on port %d appears malformed (BM of 3 doesn't exist), exiting.\n", port);
			return -1;
		} else if (source->padding1 > (unsigned int) 1) {
			fprintf(stderr, "Input header on port %d appears malformed (padding bits (1) are set), exiting.\n", port);
			return -1;
		} else if (source->padding1 == (unsigned int) 1) {
			fprintf(stderr,
					"Input header on port %d appears malformed (our replay packet warning bit is set), continuing with caution...\n",
					port);
		}



		VERBOSE(printf("Port %d\n", port);
		printf("RSP: %d\n", source->rsp);
		printf("Padding0: %d\n", source->padding0);
		printf("ERRORBIT: %d\n", source->errorBit);
		printf("CLOCK: %d\n", source->clockBit);
		printf("bitMode: %d\n", source->bitMode);
		printf("padding1: %d\n\n", source->padding1));

		if (port != 0 && (unsigned int) meta->clockBit != source->clockBit) {
			fprintf(stderr,
					"ERROR: Input files contain a mixture of 200MHz clock and 160MHz clock (port %d differs), please process these observation separately. Exiting.\n",
					port);
			return -1;
		} else {
			meta->clockBit = (int) source->clockBit;
		}

		// Extract the station ID
		// Divide by 32 to convert from (my current guess based on SE607 / IE613 codes) RSP IDs to station codes
		meta->stationID = *((short *) &(header[port][CEP_HDR_STN_ID_OFFSET])) / 32;

		// Determine the number of beamlets on the port
		VERBOSE(printf("port %d, bitMode %d, beamlets %d (%u)\n", port, source->bitMode,
					   (int) ((unsigned char) header[port][CEP_HDR_NBEAM_OFFSET]),
					   (unsigned char) header[port][CEP_HDR_NBEAM_OFFSET]););
		meta->portRawBeamlets[port] = (int) ((unsigned char) header[port][CEP_HDR_NBEAM_OFFSET]);

		// Assume we are processing all beamlets by default
		meta->upperBeamlets[port] = meta->portRawBeamlets[port];

		// Update the cumulative beamlets, total raw beamlets
		// Ordering is intentional so that we get the number of beamlets before this port.
		meta->portRawCumulativeBeamlets[port] = meta->totalRawBeamlets;
		meta->portCumulativeBeamlets[port] = meta->totalProcBeamlets;

		// Set the  upper, lower limit of beamlets as needed

		// Check the upper limit first, so that we can modify upperBeamlets if needed.
		if (beamletLimits[1] != 0 && (beamletLimits[1] < ((port + 1) * meta->portRawBeamlets[port])) &&
			(beamletLimits[1] >= port * meta->portRawBeamlets[port])) {
			meta->upperBeamlets[port] = beamletLimits[1] - meta->totalRawBeamlets;
		}


		// Check the lower beamlet, and then update the total beamlets counters as needed
		if (beamletLimits[0] != 0 && (beamletLimits[0] < ((port + 1) * meta->portRawBeamlets[port])) &&
			(beamletLimits[0] >= port * meta->portRawBeamlets[port])) {
			meta->baseBeamlets[port] = beamletLimits[0] - meta->totalRawBeamlets;

			// Lower the total count of beamlets, while not modifying
			// 	upperBeamlets
			meta->totalProcBeamlets += meta->upperBeamlets[port] - beamletLimits[0];
		} else {
			meta->baseBeamlets[port] = 0;
			meta->totalProcBeamlets += meta->upperBeamlets[port];
		}

		// Update the number of raw beamlets now that we have used the previous values to determine offsets
		meta->totalRawBeamlets += meta->portRawBeamlets[port];


		// Check the bitmode on the port
		switch ((int) source->bitMode) {
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
				return -1;
		}

		// Ensure that the bitmode does not change between ports; we assume it is constant elsewhere.
		// Fairly certain the RSPs fall over if you try to do multiple bit modes anyway.
		if (port == 0) {
			cacheBitMode = meta->inputBitMode;
		} else {
			if (cacheBitMode != meta->inputBitMode) {
				fprintf(stderr,
						"Multiple input bit sizes detected; please parse these ports separately (port 0: %d, port %d: %d). Exiting.\n",
						cacheBitMode, port, meta->inputBitMode);
				return -1;
			}
		}



		// Determine the size of the input array
		// 4-bit: half the size per sample
		// 16-bit: 2x the size per sample
		bitMul = 1.0f - (0.5f * (float) (meta->inputBitMode == 4)) + (float) (meta->inputBitMode == 16);
		meta->portPacketLength[port] = ((UDPHDRLEN) + (meta->portRawBeamlets[port] * ((int) bitMul * UDPNTIMESLICE * UDPNPOL)));

		if (port > 1) {
			if (meta->portPacketLength[port] != meta->portPacketLength[port - 1]) {
				fprintf(stderr,
						"WARNING: Packet lengths different between port offsets %d and %d, proceeding with caution.\n",
						port, port - 1);
			}
		}

	}

	return 0;
}


/**
 * @brief      If a target packet is set, search for it and align each port with
 *             the target packet as the first packet in the inputData arrays
 *
 * @param      reader  The lofar_udp_reader to process
 *
 * @return     0: Success, -1: Fatal error
 */
int lofar_udp_skip_to_packet(lofar_udp_reader *reader) {
	// This is going to be fun to document...
	long currentPacket, lastPacketOffset = 0, guessPacket = LONG_MAX, scanning = 0, packetDelta, startOff, nextOff, endOff, packetShift[MAX_NUM_PORTS], nchars, returnLen;
	int returnVal = 0;

	VERBOSE(printf("lofar_udp_skip_to_packet: starting scan to %ld...\n", reader->meta->lastPacket););

	// Find the first packet number on each port as a reference
	for (int port = 0; port < reader->meta->numPorts; port++) {
		currentPacket = lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][0]));
		guessPacket = (guessPacket < currentPacket) ? guessPacket : currentPacket;

		// Check if we are beyond the target start time
		if (currentPacket > reader->meta->lastPacket) {
			fprintf(stderr,
					"Requested packet prior to current observing block for port %d (req: %ld, 1st: %ld), exiting.\n",
					port, reader->meta->lastPacket, currentPacket);
			return -1;
		}
	}

	// Determine the offset between the first packet on each port and the minimum packet
	for (int port = 0; port < reader->meta->numPorts; port++) {
		lastPacketOffset = (reader->meta->packetsPerIteration - 1) * reader->meta->portPacketLength[port];
		currentPacket = lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][0]));
		if (lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][lastPacketOffset])) >= reader->meta->lastPacket) {
			reader->meta->portLastDroppedPackets[port] = reader->meta->packetsPerIteration;
		} else {
			reader->meta->portLastDroppedPackets[port] =
				lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][lastPacketOffset])) -
				(currentPacket + reader->meta->packetsPerIteration);
		}

		if (reader->meta->portLastDroppedPackets[port] < 0) {
			reader->meta->portLastDroppedPackets[port] = 0;
		}
	}

	// Scanning across each port,
	for (int port = 0; port < reader->meta->numPorts; port++) {
		// Get the offset to the last packet in the inputData array on a given port
		lastPacketOffset = (reader->meta->packetsPerIteration - 1) * reader->meta->portPacketLength[port];

		VERBOSE(printf("lofar_udp_skip_to_packet: first packet %ld...\n",
		               lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][0]))););

		// Find the packet number of the last packet in the inputData array
		currentPacket = lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][lastPacketOffset]));

		VERBOSE(printf("lofar_udp_skip_to_packet: last packet %ld, delta %ld...\n", currentPacket,
					   reader->meta->lastPacket - currentPacket););

		// Get the difference between the target packet and the current last packet
		packetDelta = reader->meta->lastPacket - currentPacket;
		// If the current last packet is too low, we'll need to load in more data. Perform this in a loop
		// 	until we are beyond the target packet (so the target packet is in the current array)
		//
		// Possible edge case: the packet is in the last slot on port < numPort for the current port, but is
		// 	in the next gulp on another port due to packet loss. As a result, reading the next packet will cause
		// 	us to lose the target packet on the previous port, artificially pushing the target packet higher later on.
		// 	Far too much work to try fix it, but worth remembering.
		while (currentPacket < reader->meta->lastPacket) {
			VERBOSE(printf("lofar_udp_skip_to_packet: scan at %ld...\n", currentPacket););

			// Set an int so we can update the progress line later
			scanning = 1;

			// Read in a new block of data on all ports, error check
			returnVal = lofar_udp_reader_read_step(reader);
			if (returnVal < 0) { return returnVal; }

			currentPacket = lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][lastPacketOffset]));

			// Account for packet de-sync between ports
			for (int portInner = 0; portInner < reader->meta->numPorts; portInner++) {
				if (lofar_udp_time_get_packet_number(&(reader->meta->inputData[portInner][lastPacketOffset])) >=
				    reader->meta->lastPacket) {
					// Skip a read for this port
					reader->meta->portLastDroppedPackets[portInner] = reader->meta->packetsPerIteration;
				} else {
					// Read at least a fraction of the data on this port
					reader->meta->portLastDroppedPackets[portInner] =
						lofar_udp_time_get_packet_number(&(reader->meta->inputData[portInner][lastPacketOffset])) -
						(currentPacket + reader->meta->packetsPerIteration);
				}
				VERBOSE(if (reader->meta->portLastDroppedPackets[portInner]) {
					printf("Port %d scan: %ld packets lost.\n", portInner,
						   reader->meta->portLastDroppedPackets[portInner]);
				});

				if (reader->meta->portLastDroppedPackets[portInner] < 0) {
					reader->meta->portLastDroppedPackets[portInner] = 0;
				}

				if (reader->meta->portLastDroppedPackets[portInner] > reader->meta->packetsPerIteration) {
					fprintf(stderr,
							"\nWARNING: Large amount of packets dropped on port %d during scan iteration (%ld lost), continuing...\n",
							portInner, reader->meta->portLastDroppedPackets[portInner]);
					returnVal = -2;
				}
			}

			// Print a status update to the CLI
			printf("\rScanning to packet %ld (~%.02f%% complete, currently at packet %ld on port %d, %ld to go)",
				   reader->meta->lastPacket,
				   (float) 100.0 - (float) (reader->meta->lastPacket - currentPacket) / (float) ((packetDelta) * 100),
				   currentPacket, port, reader->meta->lastPacket - currentPacket);
			fflush(stdout);
		}

		if (lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][0])) > reader->meta->lastPacket) {
			fprintf(stderr, "Port %d has scanned beyond target packet %ld (to start at %ld), exiting.\n", port,
			        reader->meta->lastPacket, lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][0])));
			return -1;
		}

		if (scanning) { printf("\33[2K\rPassed target packet %ld on port %d.\n", reader->meta->lastPacket, port); }
	}
	// Data is now all loaded such that every inputData array has or is past the target packet, now we need to internally align the arrays

	// For each port,
	for (int port = 0; port < reader->meta->numPorts; port++) {

		// Re-initialise the offset shift needed on each port
		for (int portS = 0; portS < reader->meta->numPorts; portS++) packetShift[portS] = 0;

		// Get the current packet, and guess the target packet by assuming no packet loss
		currentPacket = lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][0]));

		if ((reader->meta->lastPacket - currentPacket) > reader->meta->packetsPerIteration ||
			(reader->meta->lastPacket - currentPacket) < 0) {
			fprintf(stderr,
					"WARNING: lofar_udp_skip_to_packet just attempted to do an illegal memory access, resetting target packet to prevent it (%ld, %ld -> %ld).\n",
					reader->meta->lastPacket, currentPacket, reader->packetsPerIteration / 2);
			currentPacket = reader->packetsPerIteration / 2;
		}
		guessPacket = lofar_udp_time_get_packet_number(
			&(reader->meta->inputData[port][(reader->meta->lastPacket - currentPacket) *
			                                reader->meta->portPacketLength[port]]));

		VERBOSE(printf("lofar_udp_skip_to_packet: searching within current array starting index %ld (max %ld)...\n",
					   (reader->meta->lastPacket - currentPacket) * reader->meta->portPacketLength[port],
					   reader->meta->packetsPerIteration * reader->meta->portPacketLength[port]););
		VERBOSE(printf("lofar_udp_skip_to_packet: meta search: currentGuess %ld, 0th packet %ld, target %ld...\n",
					   guessPacket, currentPacket, reader->meta->lastPacket););

		// If noo packets are dropped, just shift across to the data
		if (guessPacket == reader->meta->lastPacket) {
			packetShift[port] = reader->meta->packetsPerIteration - (reader->meta->lastPacket - currentPacket);

			// The packet at the index is too high/low, we've had packet loss. Perform a binary-style search until we find the packet
		} else {

			// If the guess packet was too high, reset it to currentPacket so that starting offset is 0.
			// This was moved up 5 lines, please don't be a breaking change...
			if (guessPacket > reader->meta->lastPacket) { guessPacket = currentPacket; }

			// Starting offset: the different between the 0th packet and the guess packet
			startOff = guessPacket - currentPacket;

			// End offset: the end of the array
			endOff = reader->meta->packetsPerIteration;

			// By default, we will be shifting by at least the starting offset
			packetShift[port] = nextOff = startOff;

			// Make a new guess at our starting offset
			guessPacket = lofar_udp_time_get_packet_number(
				&(reader->meta->inputData[port][packetShift[port] * reader->meta->portPacketLength[port]]));

			// Iterate until we reach a target packet
			while (guessPacket != reader->meta->lastPacket) {
				VERBOSE(printf(
					"lofar_udp_skip_to_packet: meta search: currentGuess %ld, lastGuess %ld, target %ld...\n",
					guessPacket, lastPacketOffset, reader->meta->lastPacket););
				if (endOff > reader->packetsPerIteration || endOff < 0) {
					fprintf(stderr,
							"WARNING: lofar_udp_skip_to_packet just attempted to do an illegal memory access, resetting search end offset to %ld (%ld).\n",
							reader->packetsPerIteration, endOff);
					endOff = reader->packetsPerIteration;
				}

				if (startOff > reader->packetsPerIteration || startOff < 0) {
					fprintf(stderr,
							"WARNING: lofar_udp_skip_to_packet just attempted to do an illegal memory access, resetting search end offset to %d (%ld).\n",
							0, startOff);
					startOff = 0;
				}

				// Update the offset, binary search iteration style
				nextOff = (startOff + endOff) / 2;

				// Catch a weird edge case (I have no idea how this was happening, or how to reproduce it anymore)
				if (nextOff > reader->meta->packetsPerIteration) {
					fprintf(stderr, "Error: Unable to converge on solution for first packet on port %d, exiting.\n",
							port);
					return -1;
				}

				// Find the packet number of the next guess
				guessPacket = lofar_udp_time_get_packet_number(
					&(reader->meta->inputData[port][nextOff * reader->meta->portPacketLength[port]]));

				// Binary search updates
				if (guessPacket > reader->meta->lastPacket) {
					endOff = nextOff - 1;
				} else if (guessPacket < reader->meta->lastPacket) {
					startOff = nextOff + 1;
				} else if (guessPacket == reader->meta->lastPacket) {
					continue;
				}

				// If we can't find the packet, shift the indices away and try find the next packet
				if (startOff > endOff) {
					fprintf(stderr, "WARNING: Unable to find packet %ld in output array, attempting to find %ld\n",
							reader->meta->lastPacket, reader->meta->lastPacket + 1);
					reader->meta->lastPacket += 1;
					startOff -= 10;
					endOff += 10;
				}

			}
			// Set the offset to the final nextOff iteration (or first if we guessed right the first time)
			packetShift[port] = reader->meta->packetsPerIteration - nextOff;
		}

		VERBOSE(printf("lofar_udp_skip_to_packet: exited loop, shifting data...\n"););

		// Shift the data on the port as needed
		returnVal = lofar_udp_shift_remainder_packets(reader, packetShift, 0);
		if (returnVal < 0) { return -1; }

		// Find the amount of data needed, and read in new data to fill the gap left at the end of the array after the shift
		nchars = (reader->meta->packetsPerIteration * reader->meta->portPacketLength[port]) - reader->meta->inputDataOffset[port];
		if (nchars > 0) {
			returnLen = lofar_udp_io_read(reader->input, port,
										  &(reader->meta->inputData[port][reader->meta->inputDataOffset[port]]),
										  nchars);
			if (nchars > returnLen) {
				fprintf(stderr, "Unable to read enough data to fill first buffer, exiting.\n");
				return -1;
			}
		}


		// Reset the packetShit for the port to 0
		packetShift[port] = 0;
	}

	// Success!
	return returnVal;
}


/**
 * @brief      Re-use a reader on the same input files but targeting a later
 *             timestamp
 *
 * @param      reader          The lofar_udp_reader to re-initialize
 * @param[in]  startingPacket  The starting packet number for the next iteration
 *                             (LOFAR packet number, not an offset)
 * @param[in]  packetsReadMax  The maximum number of packets to read in during
 *                             this lifetime of the reader
 *
 * @return     int: 0: Success, > 0: Some data issues, but tolerable: < 0: Fatal
 *             errors
 */
int lofar_udp_file_reader_reuse(lofar_udp_reader *reader, const long startingPacket, const long packetsReadMax) {

	int returnVal;
	// Reset the maximum packets to LONG_MAX if set to an unreasonable value
	long localMaxPackets = packetsReadMax;
	if (packetsReadMax < 0) { localMaxPackets = LONG_MAX; }

	if (reader->input == NULL) {
		fprintf(stderr, "ERROR: Input information has been set to NULL at some point, cannot continue, exiting.\n");
		return -1;
	}

	// Reset old variables
	reader->meta->packetsPerIteration = reader->packetsPerIteration;
	reader->meta->packetsRead = 0;
	reader->meta->packetsReadMax = startingPacket - reader->meta->lastPacket + 2 * reader->packetsPerIteration;
	reader->meta->lastPacket = startingPacket;
	// Ensure we are always recalculating the Jones matrix on reader re-use
	reader->meta->calibrationStep = reader->calibration->calibrationStepsGenerated + 1;

	for (int port = 0; port < reader->meta->numPorts; port++) {
		reader->meta->inputDataOffset[port] = 0;
		reader->meta->portLastDroppedPackets[port] = 0;
		// reader->meta->portTotalDroppedPackets[port] = 0; // -> maybe keep this for an overall view?
	}

	VERBOSE(if (reader->meta->VERBOSE) { printf("reader_setup: First packet %ld\n", reader->meta->lastPacket); });

	// Setup to search for the next starting packet
	reader->meta->inputDataReady = 0;
	if (reader->meta->lastPacket > LFREPOCH) {
		returnVal = lofar_udp_skip_to_packet(reader);
		if (returnVal < 0) {
			return -1 * returnVal;
		}
	}

	// Align the first packet of each port, previous search may still have a 1/2 packet delta if there was packet loss
	returnVal = lofar_udp_get_first_packet_alignment(reader);
	if (returnVal < 0) {
		return returnVal;
	}

	// Set the reader status as ready to process, but not read in, on next step
	reader->meta->packetsReadMax = localMaxPackets;
	reader->meta->inputDataReady = 1;
	reader->meta->outputDataReady = 0;
	return returnVal;
}


/**
 * @brief      Set the processing function. output length per packet based on
 *             the processing mode given by the metadata,
 *
 * @param      meta  The lofar_udp_input_meta to read from and update
 *
 * @return     0: Success, -1: Unknown processing mode supplied
 */
int lofar_udp_setup_processing(lofar_udp_input_meta *meta) {
	int hdrOffset = -1 * UDPHDRLEN; // Default: no header, offset by -16
	int equalIO = 0; // If packet length in + out should match
	float mulFactor = 1.0f; // Scale packet length linearly
	int workingData;


	// Sanity check the processing mode
	switch (meta->processingMode) {
		case 0 ... 1:
			if (meta->calibrateData > NO_CALIBRATION) {
				fprintf(stderr, "WARNING: Modes 0 and 1 cannot be calibrated as they are copying full packets, disabling calibration and continuing.\n");
				meta->calibrateData = NO_CALIBRATION;
			}
		case 2:
		case 10 ... 11:
		case 20 ... 21:
		case 30 ... 32:
		case 35:
		case 100 ... 104:
		case 110 ... 114:
		case 120 ... 124:
		case 130 ... 134:
		case 150 ... 154:
		case 160 ... 164:
		case 200 ... 204:
		case 210 ... 214:
		case 220 ... 224:
		case 230 ... 234:
		case 250 ... 254:
		case 260 ... 264:
			break;

		default:
			fprintf(stderr, "Unknown processing mode %d, exiting...\n", meta->processingMode);
			return -1;
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic push
		// Mode 35: FFTW format, forced to floats so that we can directly use the output
		case 35:
			meta->outputBitMode = 32;
#pragma GCC diagnostic pop
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
			mulFactor = 1.0f / 4.0f;
			meta->outputBitMode = 32;
			break;

		case 150:
			meta->numOutputs = 4;
			// 4 input words -> 1 larger word x 4
			// => Equivalent
			meta->outputBitMode = 32;
			break;

		case 160:
			meta->numOutputs = 2;
			// 4 input words -> 2 larger word x 4
			mulFactor = 2.0f / 4.0f;
			meta->outputBitMode = 32;
			break;

		case 101 ... 104:
		case 111 ... 114:
		case 121 ... 124:
		case 131 ... 134:
			meta->numOutputs = 1;
			// Bit shift based on processing mode, 2^(mode % 10) * 4, 4 as in modes 100..130
			mulFactor = 1.0f / (float) (1 << ((meta->processingMode % 10) + 2));
			meta->outputBitMode = 32;
			break;

		case 151 ... 154:
			meta->numOutputs = 4;
			// Bit shift based on processing mode, 2^(mode % 10)
			mulFactor = 1.0f / (float) (1 << (meta->processingMode % 10));
			meta->outputBitMode = 32;
			break;

		case 161 ... 164:
			meta->numOutputs = 2;
			// Bit shift based on processing mode, 2^(mode % 10) * 2, 2 as in mode 160
			mulFactor = 1.0f / (float) (1 << ((meta->processingMode % 10) + 1));
			meta->outputBitMode = 32;
			break;

		default:
			fprintf(stderr, "Unknown processing mode %d, exiting...\n", meta->processingMode);
			return -1;
	}

	// If we are calibrating the data, the output bit mode is always 32.
	if (meta->calibrateData == APPLY_CALIBRATION) {
		meta->outputBitMode = 32;
	}

	if (equalIO) {
		for (int port = 0; port < meta->numPorts; port++) {
			meta->packetOutputLength[port] = hdrOffset + meta->portPacketLength[port];
		}
	} else {
		// Calculate the numbers of output samples pr packets
		workingData = (meta->numPorts * (hdrOffset + UDPHDRLEN)) + (meta->totalProcBeamlets * UDPNPOL * (((float) meta->inputBitMode / 8.0f) * UDPNTIMESLICE));

		// Adjust for scaling factor, output bitmode and number of outputs
		workingData = workingData * (((float) meta->outputBitMode / (float) meta->inputBitMode) * mulFactor);
		workingData /= meta->numOutputs;

		// Populate the outputs with the amount of data needed per packet processed
		for (int out = 0; out < meta->numOutputs; out++) {
			meta->packetOutputLength[out] = workingData;
		}
	}

	return 0;
}


/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_reader_config_check(lofar_udp_config *config) {

	if (config->numPorts > MAX_NUM_PORTS) {
		fprintf(stderr, "ERROR: You requested %d ports, but LOFAR can only produce %d, exiting.\n", config->numPorts,
				MAX_NUM_PORTS);
		return -1;
	}

	if (config->packetsPerIteration < 1) {
		fprintf(stderr,
				"ERROR: Packets per iteration indicates no work will be performed (%ld per iteration), exiting.\n",
				config->packetsPerIteration);
		return -1;
	}

	if (config->beamletLimits[0] > 0 && config->beamletLimits[1] > 0) {
		if (config->beamletLimits[0] > config->beamletLimits[1]) {
			fprintf(stderr,
					"ERROR: Upper beamlet limit is lower than the lower beamlet limit. Please fix your ordering (%d, %d), exiting.\n",
					config->beamletLimits[0], config->beamletLimits[1]);
			return -1;
		}

		if (config->processingMode < 2) {
			fprintf(stderr, "ERROR: Processing modes 0 and 1 do not support setting beamlet limits, exiting.\n");
			return -1;
		}
	}

	if (config->calibrateData < NO_CALIBRATION && config->calibrateData > APPLY_CALIBRATION) {
		fprintf(stderr, "ERROR: Invalid value for calibrateData (%d, should be 0 or 1), exiting.\n",
				config->calibrateData);
		return -1;
	}

	if (config->calibrateData > NO_CALIBRATION && config->calibrationConfiguration == NULL) {
		fprintf(stderr,
				"ERROR: Calibration was enabled, but the config->calibrationConfiguration struct was not initialised, exiting.\n");
		return -1;
	} else if (config->calibrateData > NO_CALIBRATION && config->calibrationConfiguration != NULL) {
		if (strcmp(config->calibrationConfiguration->calibrationFifo, "") == 0) {
			fprintf(stderr, "ERROR: Failed to provide valid path to calibration FIFO, exiting.\n");
			return -1;
		}
	}

	/*
	if (config->calibrationConfiguration == 0 && config->calibrationConfiguration != NULL)  {
		fprintf(stderr, "WARNING: Calibration was disabled, but you allocated a calibration configuration.\n");
	}
	*/

	if (config->processingMode < 0) {
		fprintf(stderr, "ERROR: Invalid processing mode %d, exiting.\n", config->processingMode);
		return -1;
	}

	if (config->startingPacket > 0 && config->startingPacket < LFREPOCH) {
		fprintf(stderr, "ERROR: Start packet seems invalid (%ld, before 2008), exiting.\n", config->startingPacket);
		return -1;
	}

	if (config->packetsReadMax < 1) {
		fprintf(stderr, "ERROR@ Invalid cap on packets to read (%ld), exiting.\n", config->packetsReadMax);
		return -1;
	}

	if (config->ompThreads < config->numPorts) {
		fprintf(stderr, "WARNING: You have requested less threads than import ports, this might be a slow run (%d threads, %d ports).\n", config->ompThreads, config->numPorts);

		if (config->ompThreads < 1) {
			fprintf(stderr, "WARNING: Resetting number of threads to compile-time default, %d.\n", OMP_THREADS);
			config->ompThreads = OMP_THREADS;
		}
	}

	if (config->replayDroppedPackets != 0 && config->replayDroppedPackets != 1) {
		fprintf(stderr, "ERROR: Invalid value for replayDroppedPackets (%d, should be 0 or 1), exiting.\n",
				config->replayDroppedPackets);
		return -1;
	}


	return 0;
}

/**
 * @brief      Set up a lofar_udp_reader and associated lofar_udp_input_meta using a
 *             set of input files and pre-set control/metadata parameters
 *
 * @param      config  The configuration struct, detailed options above
 *
 * @return     lofar_udp_reader ptr, or NULL on error
 */
lofar_udp_reader *lofar_udp_reader_setup(lofar_udp_config *config) {

	// Sanity check our inputs
	if (lofar_udp_reader_config_check(config) < 0) {
		return NULL;
	}

	if (lofar_udp_prepare_signal_handler() < 0) {
		return NULL;
	}

	// Setup the metadata struct and a few variables we'll need
	lofar_udp_input_meta *meta = calloc(1, sizeof(lofar_udp_input_meta));
	(*meta) = lofar_udp_input_meta_default;

	// Reset the maximum packets to LONG_MAX if set to an unreasonable value
	long localMaxPackets = config->packetsReadMax;
	if (config->packetsReadMax < 0) { localMaxPackets = LONG_MAX; }

	// Set the simple metadata defaults
	meta->numPorts = config->numPorts;
	meta->replayDroppedPackets = config->replayDroppedPackets;
	meta->processingMode = config->processingMode;
	meta->packetsPerIteration = config->packetsPerIteration;
	meta->packetsReadMax = localMaxPackets;
	meta->lastPacket = config->startingPacket;
	meta->calibrateData = config->calibrateData;

	VERBOSE(meta->VERBOSE = config->verbose);

	// Prepare to read in the first header on each input.
	char inputHeaders[MAX_NUM_PORTS][UDPHDRLEN];
	int readlen;

	#ifndef ALLOW_VERBOSE
	if (config->verbose) fprintf(stderr, "Warning: verbosity was disabled at compile time, but you requested it. Continuing...\n");
	#endif





	// Scan in the first header on each port
	for (int port = 0; port < meta->numPorts; port++) {
		readlen = lofar_udp_io_read_temp(config, port, &(inputHeaders[port]), sizeof(char), UDPHDRLEN, 1);
		if (readlen != UDPHDRLEN) {
			fprintf(stderr, "Unable to read header on port %d, exiting.\n", port);
			FREE_NOT_NULL(meta);
			return NULL;
		}

	}


	// Parse the input file headers to get packet metadata on each port
	// We may repeat this step if the selected beamlets cause us to drop a port of dataa
	int updateBeamlets = (config->beamletLimits[0] > 0 || config->beamletLimits[1] > 0);
	int beamletLimits[2] = { 0, 0 };
	while (updateBeamlets != -1) {
		VERBOSE(if (meta->VERBOSE) { printf("Handle headers: %d\n", updateBeamlets); });
		// Standard setup
		if (lofar_udp_parse_headers(meta, inputHeaders, beamletLimits) < 0) {
			fprintf(stderr, "Unable to setup metadata using given headers; exiting.\n");
			FREE_NOT_NULL(meta);
			return NULL;

			// If we are only parsing a subset of beamlets
		} else if (updateBeamlets) {
			VERBOSE(if (meta->VERBOSE) { printf("Handle headers chain: %d\n", updateBeamlets); });
			int lowerPort = 0;
			int upperPort = meta->numPorts - 1;

			// Iterate over the given ports
			for (int port = 0; port < meta->numPorts; port++) {
				// Check if the lower limit is on the given port
				if (config->beamletLimits[0] > 0) {
					if ((meta->portRawCumulativeBeamlets[port] <= config->beamletLimits[0]) &&
						((meta->portRawCumulativeBeamlets[port] + meta->portRawBeamlets[port]) >
						 config->beamletLimits[0])) {
						VERBOSE(if (meta->VERBOSE) {
							printf("Lower beamlet %d found on port %d\n", config->beamletLimits[0], port);
						});
						lowerPort = port;
					}
				}

				// Check if the upper limit is on the given port
				if (config->beamletLimits[1] > 0) {
					if ((meta->portRawCumulativeBeamlets[port] < config->beamletLimits[1]) &&
						((meta->portRawCumulativeBeamlets[port] + meta->portRawBeamlets[port]) >=
						 config->beamletLimits[1])) {
						VERBOSE(if (meta->VERBOSE) {
							printf("Upper beamlet %d found on port %d\n", config->beamletLimits[1], port);
						});
						upperPort = port;
					}
				}
			}

			// Sanity check before progressing
			if (lowerPort > upperPort) {
				fprintf(stderr,
						"ERROR: Upon updating beamletLimits, we found the upper beamlet is in a port higher than the lower port (%d, %d), exiting.\n",
						upperPort, lowerPort);
				FREE_NOT_NULL(meta);
				return NULL;
			}

			if (lowerPort > 0) {
				// Shift input files down
				for (int port = lowerPort; port <= upperPort; port++) {
					if (strcpy(config->inputLocations[port - lowerPort], config->inputLocations[port]) !=
						config->inputLocations[port - lowerPort]) {
						fprintf(stderr, "ERROR: Failed to copy file location during port shift on port %d, exiting.\n",
								port);
					}

					// GCC 10 has a warning about this line. Why?
					memcpy(&(inputHeaders[port - lowerPort][0]), &(inputHeaders[port][0]), UDPHDRLEN);
				}

				// Update beamlet limits to be relative to the new ports
				config->beamletLimits[0] -= meta->portRawCumulativeBeamlets[lowerPort];
				config->beamletLimits[1] -= meta->portRawCumulativeBeamlets[lowerPort];

			}

			// If we are dropping any ports, update numPorts
			if ((lowerPort != 0) || ((upperPort + 1) != config->numPorts)) {
				meta->numPorts = (upperPort + 1) - lowerPort;
			}

			VERBOSE(if (meta->VERBOSE) { printf("New numPorts: %d\n", meta->numPorts); });

			// Update updateBeamlets so that we can start the loop again, but not enter this code block.
			updateBeamlets = 0;
			// Update the limits to our current limits.
			beamletLimits[0] = config->beamletLimits[0];
			beamletLimits[1] = config->beamletLimits[1];
		} else {
			VERBOSE(if (meta->VERBOSE) { printf("Handle headers: %d\n", updateBeamlets); });
			updateBeamlets = -1;
		}
	}


	if (lofar_udp_setup_processing(meta)) {
		fprintf(stderr, "Unable to setup processing mode %d, exiting.\n", config->processingMode);
		return NULL;
	}

	// Allocate the memory needed to store the raw / reprocessed data, initialise the variables that are stored on a per-port basis.
	for (int port = 0; port < meta->numPorts; port++) {
		// The input buffer is extended by 2 packets to allow for both a reference packet from a previous iteration
		// and a fall-back packet encase replaying packets is disabled (all 0s to pad the output)


		// If we have a compressed reader, align the length with the ZSTD buffer sizes
		size_t additionalBufferSize = ZSTD_DStreamOutSize() -
					 ((meta->portPacketLength[port] * (meta->packetsPerIteration)) % ZSTD_DStreamOutSize());

		meta->inputData[port] = calloc(meta->portPacketLength[port] * (meta->packetsPerIteration + 2) +
									   additionalBufferSize * (config->readerType == ZSTDCOMPRESSED), sizeof(char));
		// Offset the pointer to the end of the two iniital buffer packets
		meta->inputData[port] += (meta->portPacketLength[port] * 2);
		VERBOSE(if (meta->VERBOSE) {
			printf("calloc at %p for %ld +(%d) bytes\n",
				   meta->inputData[port] - (meta->portPacketLength[port] * 2),
				   meta->portPacketLength[port] * (meta->packetsPerIteration + 2) +
				   additionalBufferSize * (config->readerType == ZSTDCOMPRESSED) - meta->portPacketLength[port] * 2,
				   meta->portPacketLength[port] * 2);
		});

		// Initialise these arrays while we're looping
		meta->inputDataOffset[port] = 0;
		meta->portLastDroppedPackets[port] = 0;
		meta->portTotalDroppedPackets[port] = 0;
	}

	for (int out = 0; out < meta->numOutputs; out++) {
		VERBOSE(printf("%d, %ld\n", meta->packetOutputLength[out], meta->packetsPerIteration));
		meta->outputData[out] = calloc((long) meta->packetOutputLength[out] * meta->packetsPerIteration, sizeof(char));
		VERBOSE(if (meta->VERBOSE) {
			printf("calloc at %p for %ld bytes\n", meta->outputData[out],
				   meta->packetOutputLength[out] * meta->packetsPerIteration);
		});
	}


	VERBOSE(if (meta->VERBOSE) {
		printf("Meta debug:\ntotalBeamlets %d, numPorts %d, replayDroppedPackets %d, processingMode %d, outputBitMode %d, packetsPerIteration %ld, packetsRead %ld, packetsReadMax %ld, lastPacket %ld, \n",
			   meta->totalRawBeamlets, meta->numPorts, meta->replayDroppedPackets, meta->processingMode,
			   meta->outputBitMode, meta->packetsPerIteration, meta->packetsRead, meta->packetsReadMax,
			   meta->lastPacket);

		for (int iVerb = 0; iVerb < meta->numPorts; iVerb++) {
			printf("Port %d: inputDataOffset %ld, portBeamlets %d, cumulativeBeamlets %d, inputBitMode %d, portPacketLength %d, packetOutputLength %d, portLastDroppedPackets %ld, portTotalDroppedPackets %ld\n",
				   iVerb,
				   meta->inputDataOffset[iVerb], meta->portRawBeamlets[iVerb], meta->portCumulativeBeamlets[iVerb],
				   meta->inputBitMode, meta->portPacketLength[iVerb], meta->packetOutputLength[iVerb],
				   meta->portLastDroppedPackets[iVerb], meta->portTotalDroppedPackets[iVerb]);
		}
		for (int iVerb = 0; iVerb < meta->numOutputs; iVerb++) {
			printf("Output %d, packetLength %d, numOut %d\n", iVerb, meta->packetOutputLength[iVerb],
				   meta->numOutputs);
		}
	});


	// Allocate the structs and initialise them
	lofar_udp_reader *reader = lofar_udp_reader_alloc();

	// Initialise the reader struct from config
	reader->input->readerType = (reader_t) config->readerType;
	reader->packetsPerIteration = meta->packetsPerIteration;
	reader->meta = meta;

	reader->ompThreads = config->ompThreads;
	omp_set_num_threads(reader->ompThreads);

	if ((meta->numPorts + config->offsetPortCount) > 4) {
		fprintf(stderr, "ERROR: Requested data beyond the 4th port. Either your offset (%d) or number of ports requested (%d) was too high, exiting.\n", config->offsetPortCount, meta->numPorts);
		lofar_udp_reader_cleanup(reader);
		return NULL;
	}

	for (int port = 0; port < (meta->numPorts); port++) {
		// Check for errors, cleanup and return if needed
		if (lofar_udp_io_read_setup_helper(reader->input, config, meta, port) < 0) {
			lofar_udp_reader_cleanup(reader);
			return NULL;
		}

	}

	// TODO: Copy values to calibration struct as needed.
	if (config->metadata_config.metadataType != NO_META) {
		VERBOSE(printf("Priming metadata type %d from %s\n", config->metadata_config.metadataType, config->metadata_config.metadataLocation));
		reader->metadata = calloc(1, sizeof(lofar_udp_metadata));
		*(reader->metadata) = lofar_udp_metadata_default;

		for (int i = 0; i < MAX_NUM_PORTS * UDPMAXBEAM; i++) {
			reader->metadata->subbands[i] = -1;
		}

		reader->metadata->type = config->metadata_config.metadataType;
		if (lofar_udp_metadata_setup(reader->metadata, reader, &(config->metadata_config)) < 0) {
			lofar_udp_reader_cleanup(reader);
			return NULL;
		}
	}

	if (config->calibrateData > NO_CALIBRATION) {
		reader->calibration = calloc(1, sizeof(lofar_udp_calibration));
		if (memcpy(reader->calibration, config->calibrationConfiguration, sizeof(lofar_udp_calibration)) != reader->calibration) {
			fprintf(stderr, "ERROR: Failed to copy calibration to reader struct, exiting.\n");
			lofar_udp_reader_cleanup(reader);
			return NULL;
		}
	}

	// Gulp the first set of raw data
	if (lofar_udp_reader_read_step(reader) < 0) {
		lofar_udp_reader_cleanup(reader);
		return NULL;
	}
	reader->meta->inputDataReady = 0;

	VERBOSE(
	for (int port = 0; port < meta->numPorts; port++) {
		printf("Packs: %ld, %ld, %ld\n", lofar_udp_time_get_packet_number(meta->inputData[port]),
		       lofar_udp_time_get_packet_number(&(meta->inputData[port][(meta->packetsPerIteration - 1) * meta->portPacketLength[port]])),
		       lofar_udp_time_get_packet_number(meta->inputData[port]) -
		                                                                                                                                                                                                    lofar_udp_time_get_packet_number(&(meta->inputData[port][(meta->packetsPerIteration - 1) * meta->portPacketLength[port]])));
	}
	if (meta->VERBOSE) { printf("reader_setup: First packet %ld\n", meta->lastPacket); });

	// If we have been given a starting packet, search for it and align the data to it
	if (reader->meta->lastPacket > LFREPOCH) {
		if (lofar_udp_skip_to_packet(reader) < 0) {
			for (int port = 0; port < meta->numPorts; port++) {
				if (lofar_udp_time_get_packet_number(meta->inputData[port]) > meta->lastPacket) {
					meta->lastPacket = lofar_udp_time_get_packet_number(meta->inputData[port]);
				}

				printf("Packs: %ld, %ld, %ld\n", lofar_udp_time_get_packet_number(meta->inputData[port]),
				       lofar_udp_time_get_packet_number(&(meta->inputData[port][(meta->packetsPerIteration - 1) * meta->portPacketLength[port]])),
				       lofar_udp_time_get_packet_number(meta->inputData[port]) -
				                                                                                                                                                                                                    lofar_udp_time_get_packet_number(&(meta->inputData[port][(meta->packetsPerIteration - 1) * meta->portPacketLength[port]])));
			}

			fprintf(stderr, "WARNING: Falling back to first packet, %ld.\n", meta->lastPacket);
			if (lofar_udp_skip_to_packet(reader) < 0) {
				fprintf(stderr, "ERROR: Failed to find secondary starting point, exiting.\n");
				lofar_udp_reader_cleanup(reader);
				return NULL;
			}
		}
	}

	VERBOSE(if (meta->VERBOSE) { printf("reader_setup: Skipped, aligning to %ld\n", meta->lastPacket); });

	// Align the first packet on each port, previous search may still have a 1/2 packet delta if there was packet loss
	if (lofar_udp_get_first_packet_alignment(reader) < 0) {
		lofar_udp_reader_cleanup(reader);
		return NULL;
	}

	reader->meta->inputDataReady = 1;
	return reader;
}


/**
 * @brief      Optionally lose input files, free alloc'd memory, free zstd
 *             decompression streams once we are finished.
 *
 * @param[in]  reader  The lofar_udp_reader struct to cleanup
 * @param[in]  closeFiles  bool: close input files (1) or don't (0)
 *
 * @return     int: 0: Success, other: ???
 */
void lofar_udp_reader_cleanup(lofar_udp_reader *reader) {

	// Cleanup the malloc/calloc'd memory addresses, close the input files.
	for (int i = 0; i < reader->meta->numOutputs; i++) {
		FREE_NOT_NULL(reader->meta->outputData[i]);
	}

	for (int i = 0; i < reader->meta->numPorts; i++) {
		// Free input data pointer (from the correct offset)
		if (reader->meta->inputData[i] != NULL) {
			
			VERBOSE(if (reader->meta->VERBOSE) {
				printf("On port: %d freeing inputData at %p\n", i, reader->meta->inputData[i] -
																   2 * reader->meta->portPacketLength[i]);
			});

			char *tmpPtr = (reader->meta->inputData[i] - 2 * reader->meta->portPacketLength[i]);
			FREE_NOT_NULL(tmpPtr);
			reader->meta->inputData[i] = NULL;
		}
		if (reader->input != NULL) {
			lofar_udp_io_read_cleanup(reader->input, i);
		}
	}

	// Cleanup Jones matrices if they are allocated
	if (reader->meta->jonesMatrices != NULL) {
		for (int i = 0; i < reader->calibration->calibrationStepsGenerated; i++) {
			FREE_NOT_NULL(reader->meta->jonesMatrices[i]);
		}
		FREE_NOT_NULL(reader->meta->jonesMatrices);
	}

	if (reader->metadata != NULL) {
		lofar_udp_metadata_cleanup(reader->metadata);
	}

	// Free the reader
	FREE_NOT_NULL(reader->meta);
	FREE_NOT_NULL(reader->input);
	FREE_NOT_NULL(reader->calibration);
	FREE_NOT_NULL(reader);

	//return 0;
}


/**
 * @brief      Generate the inverted Jones matrix to calibrate the observed data
 *
 * @param      reader  The lofar_udp_reader
 *
 * @return     int: 0: Success, -1: Failure
 */
int lofar_udp_reader_calibration(lofar_udp_reader *reader) {
	FILE *fifo;
	int numBeamlets, numTimesamples, returnVal;
	// Ensure we are meant to be calibrating data
	if (reader->meta->calibrateData == NO_CALIBRATION) {
		fprintf(stderr, "ERROR: Requested calibration while calibration is disabled. Exiting.\n");
		return -1;
	}

	// For security, add a few  random ASCII characters to the end of the suggested name
	static int numRandomChars = 4;
	char *fifoName = calloc(strlen(reader->calibration->calibrationFifo) + numRandomChars, sizeof(char));
	char randomChars[numRandomChars];

	for (int i = 0; i < numRandomChars; i++) {
		// Offset to base of letters in ASCII (65) In the range of letters +(0  - 25), + 0.5 chance of +32 to switch between lower and upper case
		randomChars[i] = (char) (65 + (rand() % 26) + (rand() % 2 * 32)); // NOLINT
	}
	returnVal = sprintf(fifoName, "%s_%4s", reader->calibration->calibrationFifo, randomChars);

	if (returnVal < 0) {
		fprintf(stderr, "ERROR: Failed to modify FIFO name (%s, %s, %d). Exiting.\n",
				reader->calibration->calibrationFifo, randomChars, errno);
	}
	// Make a FIFO pipe to communicate with dreamBeam
	VERBOSE(printf("Making fifo\n"));
	if (access(fifoName, F_OK) != -1) {
		if (remove(fifoName) != 0) {
			fprintf(stderr, "ERROR: Unable to cleanup old file on calibration FIFO path (%d). Exiting.\n", errno);
			return -1;
		}
	}

	returnVal = mkfifo(fifoName, 0664);
	if (returnVal < 0) {
		fprintf(stderr, "ERROR: Unable to create FIFO pipe at %s (%d). Exiting.\n", fifoName, errno);
		return -1;
	}

	// Call dreamBeam to generate calibration
	// dreamBeamJonesGenerator.py --stn STNID --sub ANT,SBL:SBU --time TIME --dur DUR --int INT --pnt P0,P1,BASIS --pipe /tmp/pipe
	char stationID[16] = "", mjdTime[32] = "", duration[32] = "", integration[32] = "", pointing[512] = "";


	lofar_udp_metadata_get_station_name(reader->meta->stationID, &(stationID[0]));
	sprintf(mjdTime, "%lf", lofar_udp_time_get_packet_time_mjd(reader->meta->inputData[0]));
	sprintf(duration, "%31.10f", reader->calibration->calibrationDuration);
	sprintf(integration, "%15.10f", (float) (reader->packetsPerIteration * UDPNTIMESLICE) *
									(float) (clock200MHzSampleTime * reader->meta->clockBit +
									         clock160MHzSampleTime * (1 - reader->meta->clockBit)));
	sprintf(pointing, "%f,%f,%s", reader->calibration->calibrationPointing[0],
			reader->calibration->calibrationPointing[1], reader->calibration->calibrationPointingBasis);

	VERBOSE(printf("Calling dreamBeam: %s %s %s %s %s %s %s\n", stationID, mjdTime,
				   reader->calibration->calibrationSubbands, duration, integration, pointing, fifoName));

	char *argv[] = { "dreamBeamJonesGenerator.py", "--stn", stationID, "--time", mjdTime,
					 "--sub", reader->calibration->calibrationSubbands,
					 "--dur", duration, "--int", integration, "--pnt", pointing,
					 "--pipe", fifoName,
					 NULL };

	pid_t pid;
	printf("Generating Jones matrices via dreamBeamJonesGenerator.py...\n");
	returnVal = posix_spawnp(&pid, "dreamBeamJonesGenerator.py", NULL, NULL, &(argv[0]), environ);

	VERBOSE(printf("Fork\n"););
	if (returnVal == 0) {
		VERBOSE(printf("dreamBeam has been launched.\n"));
	} else if (returnVal < 0) {
		fprintf(stderr, "ERROR: Unable to create child process to call dreamBeam. Exiting.\n");
		return -1;
	}

	VERBOSE(printf("OpeningFifo\n"));
	fifo = fopen(fifoName, "rb");

	returnVal = (int) waitpid(pid, &returnVal, WNOHANG);
	// Check if dreamBeam exited early
	if (returnVal < 0) {
		return -1;
	}

	// Get the number of time steps and frequency channels
	returnVal = fscanf(fifo, "%d,%d\n", &numTimesamples, &numBeamlets);
	if (returnVal == -1) {
		fprintf(stderr, "ERROR: Failed to parse number of beamlets and time samples from dreamBeam. Exiting.\n");
		return -1;
	}


	VERBOSE(printf("beamlets\n"));
	// Ensure the calibration strategy matches the number of subbands we are processing
	if (numBeamlets != reader->meta->totalProcBeamlets) {
		fprintf(stderr, "ERROR: Calibration strategy returned %d beamlets, but we are setup to handle %d. Exiting. \n",
				numBeamlets, reader->meta->totalProcBeamlets);
		return -1;
	}

	VERBOSE(printf("%d, %d\n", numTimesamples, numBeamlets));
	// Check if we already allocated storage for the calibration data, re-use already alloc'd data where possible
	if (reader->meta->jonesMatrices == NULL) {
		// Allocate numTimesamples * numBeamlets * (4 matrix elements) * (2 complex values per element)
		reader->meta->jonesMatrices = calloc(numTimesamples, sizeof(float *));
		for (int timeIdx = 0; timeIdx < numTimesamples; timeIdx += 1) {
			reader->meta->jonesMatrices[timeIdx] = calloc(numBeamlets * 8, sizeof(float));
		}
		// If we returned more time samples than last time, reallocate the array
	} else if (numTimesamples > reader->calibration->calibrationStepsGenerated) {
		// Free the old data
		for (int timeIdx = 0; timeIdx < reader->calibration->calibrationStepsGenerated; timeIdx += 1) {
			FREE_NOT_NULL(reader->meta->jonesMatrices[timeIdx]);
		}
		FREE_NOT_NULL(reader->meta->jonesMatrices);

		// Reallocate the data
		reader->meta->jonesMatrices = calloc(numTimesamples, sizeof(float *));
		for (int timeIdx = 0; timeIdx < numTimesamples; timeIdx += 1) {
			reader->meta->jonesMatrices[timeIdx] = calloc(numBeamlets * 8, sizeof(float));
		}
		// If less time samples, free the remaining steps and re-use the array
	} else if (numTimesamples < reader->calibration->calibrationStepsGenerated) {
		// Free the extra time steps
		for (int timeIdx = reader->calibration->calibrationStepsGenerated; timeIdx < numTimesamples; timeIdx += 1) {
			FREE_NOT_NULL(reader->meta->jonesMatrices[timeIdx]);
		}
	}

	VERBOSE(printf("FIFO Parse\n"));
	// Index into the jonesMatrices array
	long baseOffset = 0;
	for (int timeIdx = 0; timeIdx < numTimesamples; timeIdx += 1) {
		baseOffset = 0;
		for (int freqIdx = 0; freqIdx < numBeamlets - 1; freqIdx += 1) {
			// parse the FIFO to get the data
			returnVal = fscanf(fifo, "%f,%f,%f,%f,%f,%f,%f,%f,", &reader->meta->jonesMatrices[timeIdx][baseOffset], \
                                                        &reader->meta->jonesMatrices[timeIdx][baseOffset + 1], \
                                                        &reader->meta->jonesMatrices[timeIdx][baseOffset + 2], \
                                                        &reader->meta->jonesMatrices[timeIdx][baseOffset + 3], \
                                                        &reader->meta->jonesMatrices[timeIdx][baseOffset + 4], \
                                                        &reader->meta->jonesMatrices[timeIdx][baseOffset + 5], \
                                                        &reader->meta->jonesMatrices[timeIdx][baseOffset + 6], \
                                                        &reader->meta->jonesMatrices[timeIdx][baseOffset + 7]);

			if (returnVal < 0) {
				fprintf(stderr, "ERROR: unable to parse main pipe from dreamBeam (%d, %d). Exiting.\n", timeIdx,
						freqIdx);
			}

			baseOffset += 8;
		}

		// Parse the final beamlet sample, confirm it ends with a | to signify the end of the time sample
		returnVal = fscanf(fifo, "%f,%f,%f,%f,%f,%f,%f,%f|", &reader->meta->jonesMatrices[timeIdx][baseOffset], \
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

	VERBOSE(printf("%f, %f, %f, %f... %f, %f, %f, %f\n", reader->meta->jonesMatrices[0][0],
				   reader->meta->jonesMatrices[0][1], reader->meta->jonesMatrices[0][2],
				   reader->meta->jonesMatrices[0][3], reader->meta->jonesMatrices[0][baseOffset],
				   reader->meta->jonesMatrices[0][baseOffset + 1], reader->meta->jonesMatrices[0][baseOffset + 2],
				   reader->meta->jonesMatrices[0][baseOffset + 3]););

	// Close and remove the pipe
	fclose(fifo);
	if (remove(fifoName) != 0) {
		fprintf(stderr, "ERROR: Unable to remove calibration FIFO (%d). Exiting.\n", errno);
		return -1;
	}
	FREE_NOT_NULL(fifoName);

	// Update the calibration state
	reader->meta->calibrationStep = 0;
	reader->calibration->calibrationStepsGenerated = numTimesamples;
	VERBOSE(printf("%s: Exit calibration.\n", __func__););
	return 0;
}


/**
 * @brief      Attempt to fill the reader->meta->inputData buffers with new
 *             data. Performs a shift on the last N packets of a given port if
 *             there was packet loss on the last iteration (so the packets were
 *             not used, allowing us to use them on the next iteration)
 *
 * @param      reader  The input lofar_udp_reader struct to process
 *
 * @return     int: 0: Success, -1, Failure, -2: Final iteration, reached packet cap, -3:
 *             Final iteration, received less data than requested (EOF)
 */
int lofar_udp_reader_read_step(lofar_udp_reader *reader) {
	int returnVal = 0;

	// Make sure we have work to perform
	if (reader->meta->packetsPerIteration == 0) {
		fprintf(stderr, "Last packets per iteration was 0, there is no work to perform, exiting...\n");
		return -1;
	}

	// Reset the packets per iteration to the intended length (can be lowered due to out of order packets)
	reader->meta->packetsPerIteration = reader->packetsPerIteration;

	// If packets were dropped, shift the remaining packets back to the start of the array
	if (lofar_udp_shift_remainder_packets(reader, reader->meta->portLastDroppedPackets, 1) < 0) { return -1; }

	// Ensure we aren't passed the read length cap
	if (reader->meta->packetsRead >= (reader->meta->packetsReadMax - reader->meta->packetsPerIteration)) {
		reader->meta->packetsPerIteration = reader->meta->packetsReadMax - reader->meta->packetsRead;
		VERBOSE(if (reader->meta->VERBOSE) {
			printf("Processing final read (%ld packets) before reaching maximum packet cap.\n",
				   reader->meta->packetsPerIteration);
		});
		returnVal = -2;
	}

	//else if (checkReturnValue < 0) if(lofar_udp_realign_data(reader) < 0) return -1;

	// Read in the required new data
#pragma omp parallel for shared(returnVal, reader, stderr)
	for (int port = 0; port < reader->meta->numPorts; port++) {
		long charsToRead, charsRead, packetPerIter;

		// Determine how much data is needed and read-in to the offset after any leftover packets
		if (reader->meta->portLastDroppedPackets[port] > reader->meta->packetsPerIteration) {
			fprintf(stderr, "\nWARNING: Port %d not performing read due to excessive packet loss.\n", port);
			continue;
		} else {
			charsToRead = (reader->meta->packetsPerIteration - reader->meta->portLastDroppedPackets[port]) *
						  reader->meta->portPacketLength[port];
		}
		VERBOSE(if (reader->meta->VERBOSE) {
			printf("Port %d: read %ld packets.\n", port, (reader->meta->packetsPerIteration -
														  reader->meta->portLastDroppedPackets[port]));
		});
		charsRead = lofar_udp_io_read(reader->input, port,
									  &(reader->meta->inputData[port][reader->meta->inputDataOffset[port]]),
									  charsToRead);

		// Raise a warning if we received less data than requested (EOF/file error)
		if (charsRead < charsToRead) {
			VERBOSE(printf("%ld, %f, %d\n", charsRead, (float) reader->meta->inputDataOffset[port] / reader->meta->portPacketLength[port], reader->meta->portPacketLength[port]));
			packetPerIter =
					(charsRead + reader->meta->inputDataOffset[port]) / reader->meta->portPacketLength[port];

			#pragma omp critical (packetCheck)
			{
				if (packetPerIter < reader->meta->packetsPerIteration) {
					// Sanity check, out of order packets can cause a negative value here
					if (packetPerIter > 0) {
						reader->meta->packetsPerIteration = packetPerIter;
					} else {
						reader->meta->packetsPerIteration = 0;
					}
					fprintf(stderr,
							"Received less data from file on port %d than expected, may be nearing end of file.\nReducing packetsPerIteration to %ld, to account for the limited amount of input data.\n",
							port, reader->meta->packetsPerIteration);
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
 * @param      timing  A length of 2 double array to store (I/O, Mem-ops) timing
 *                     data
 *
 * @return     int: 0: Success, < 0: Some data issues, but tolerable: > 1: Fatal
 *             errors
 */
int lofar_udp_reader_step_timed(lofar_udp_reader *reader, double timing[2]) {
	int readReturnVal = 0, stepReturnVal = 0;
	struct timespec tick0, tick1, tock0, tock1;
	const int time = (timing[0] != -1.0);


	if ((reader->meta->calibrateData > NO_CALIBRATION) &&
		reader->meta->calibrationStep >= reader->calibration->calibrationStepsGenerated) {
		VERBOSE(printf("Calibration buffer has run out, generating new Jones matrices.\n"));
		if ((readReturnVal = lofar_udp_reader_calibration(reader)) < 0) {
			return readReturnVal;
		}
	}

	// Start the reader clock
	if (time) { clock_gettime(CLOCK_MONOTONIC_RAW, &tick0); }

	VERBOSE(if (reader->meta->VERBOSE) {
		printf("reader_step ready: %d, %d\n", reader->meta->inputDataReady, reader->meta->outputDataReady);
	});
	// Check if the data states are ready for a new gulp
	if (reader->meta->inputDataReady != 1 && reader->meta->outputDataReady != 0) {
		if ((readReturnVal = lofar_udp_reader_read_step(reader)) == -1) { return readReturnVal; }
		reader->meta->leadingPacket = reader->meta->lastPacket + 1;
		reader->meta->outputDataReady = 0;
	}


	// End the I/O timer. start the processing timer
	if (time) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &tock0);
		clock_gettime(CLOCK_MONOTONIC_RAW, &tick1);
	}


	VERBOSE(if (reader->meta->VERBOSE) {
		printf("reader_step ready2: %d, %d, %d\n", reader->meta->inputDataReady, reader->meta->outputDataReady,
			   reader->meta->packetsPerIteration > 0);
	});

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

	// Return the "worst" value.
	if (readReturnVal < stepReturnVal) {
		return readReturnVal;
	}
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
__attribute__((unused)) int lofar_udp_reader_step(lofar_udp_reader *reader) {
	double fakeTiming[2] = { -1.0, 0 };

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
 * @return     0: Success, -1: Fatal error
 */
int lofar_udp_get_first_packet_alignment(lofar_udp_reader *reader) {

	long currentPacket;
	// Determine the maximum port packet number, update the variables as needed
	for (int port = 0; port < reader->meta->numPorts; port++) {
		// Initialise/Reset the dropped packet counters
		reader->meta->portLastDroppedPackets[port] = 0;
		reader->meta->portTotalDroppedPackets[port] = 0;
		currentPacket = lofar_udp_time_get_packet_number(reader->meta->inputData[port]);

		// Check if the highest packet number needs to be updated
		if (currentPacket > reader->meta->lastPacket) {
			reader->meta->lastPacket = currentPacket;
		}

	}


	int returnVal = lofar_udp_skip_to_packet(reader);
	reader->meta->lastPacket -= 1;
	return returnVal;

}


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
 * @param      meta  The input lofar_udp_input_meta struct to process
 *
 * @return     int: 0: Success, -1: Negative shift requested, out of order data
 *             on last gulp,
 */
int lofar_udp_shift_remainder_packets(lofar_udp_reader *reader, const long shiftPackets[], const int handlePadding) {

	int returnVal = 0, fixBuffer = 0;
	int destOffset, portPacketLength;
	long byteShift, sourceOffset;

	char *inputData;
	long totalShift = 0, packetShift;


	// Check if we have any work to do, reset offsets, exit if no work requested
	for (int port = 0; port < reader->meta->numPorts; port++) {
		reader->meta->inputDataOffset[port] = 0;
		totalShift += shiftPackets[port];
	}

	if (totalShift < 1 && fixBuffer == 0) { return 0; }


	// Shift the data on each port
	for (int port = 0; port < reader->meta->numPorts; port++) {
		inputData = reader->meta->inputData[port];

		// Work around a segfault, cap the size of a shift to the size of the input buffers.
		if (shiftPackets[port] <= reader->packetsPerIteration) {
			packetShift = shiftPackets[port];
		} else {
			fprintf(stderr,
					"\nWARNING: Requested packet shift is larger than the size of our input buffer. Adjusting port %d from %ld to %ld.\n",
					port, shiftPackets[port], reader->packetsPerIteration);
			packetShift = reader->packetsPerIteration;
		}


		VERBOSE(if (reader->meta->VERBOSE) {
			printf("shift_remainder: Port %d packet shift %ld padding %d\n", port, packetShift, handlePadding);
		});

		// If we have packet shift or want to copy the final packet for future reference
		if (packetShift > 0 || handlePadding == 1 || (handlePadding == 1 && fixBuffer == 1)) {

			// Negative shift -> negative packet loss -> out of order data -> do nothing, we'll  drop a few packets and resume work
			if (packetShift < 0) {
				fprintf(stderr, "Requested shift on port %d is negative (%ld);", port, packetShift);
				if (packetShift < -5) {
					fprintf(stderr,
							" this is an indication of data integrity issues. Be careful with outputs from this dataset.\n");
				} else { fprintf(stderr, " attempting to continue...\n"); }
				returnVal = -1;
				reader->meta->inputDataOffset[port] = 0;
				if (handlePadding == 0) { continue; }
				packetShift = 0;
			}

			portPacketLength = reader->meta->portPacketLength[port];

			// Calculate the origin + destination indices, and the amount of data to shift
			sourceOffset = reader->meta->portPacketLength[port] *
						   (reader->meta->packetsPerIteration - packetShift - handlePadding); // Verify -1...
			destOffset = -1 * portPacketLength * handlePadding;
			byteShift = ((long) sizeof(char)) * (packetShift + handlePadding) * portPacketLength;

			VERBOSE(if (reader->meta->VERBOSE) {
				printf("P: %d, SO: %ld, DO: %d, BS: %ld IDO: %ld\n", port, sourceOffset, destOffset, byteShift,
					   destOffset + byteShift);
			});


			// Memmove the data as needed (memcpy can't act on the same array)
			memmove(&(inputData[destOffset]), &(inputData[sourceOffset]), byteShift);

			// Reset the 0-valued buffer to wipe any time/sequence data remaining
			if (!reader->meta->replayDroppedPackets) {
				sourceOffset = -2 * portPacketLength;
				for (int i = 0; i < portPacketLength; i++) inputData[sourceOffset + i] = 0;
			}
			// Update the inputDataOffset variable to point to the new  'head' of the array, including the new offset
			reader->meta->inputDataOffset[port] = destOffset + byteShift;
			VERBOSE(if (reader->meta->VERBOSE) {
				printf("shift_remainder: Final data offset %d: %ld\n", port,
					   reader->meta->inputDataOffset[port]);
			});
		}

		VERBOSE(if (reader->meta->VERBOSE) {
			printf("shift_remainder: Port %d end offset: %ld\n", port, reader->meta->inputDataOffset[port]);
		});
	}

	return returnVal;
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
			fprintf(stderr, "Data integrity compromised on all ports, exiting.\n");
			return -1;
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
					return -1;
				} else if (offset > workingBlockLength) {
					fprintf(stderr, "Reading in new block of data for search on port %d...\n", port);
					returnVal = lofar_udp_reader_nchars(reader, port, reader->meta->inputData[port], workingBlockLength);
					if (returnVal != reader->meta->packetsPerIteration * reader->meta->portPacketLength[port]) {
						fprintf(stderr, "Reached end of file before aligning data, exiting...\n");
						return -1;
					}
					idx = workingBlockLength;
				}

				if (status == 0) {
					if (memcmp(referenceData, &(reader->meta->inputData[port][idx]), 8)) {
						status = 1;
						offset = 0;
					} else continue;
				} else {
					workingPkt = lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][idx]));
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

// Abandoned option to just seek to data in FILE* inputs
// TODO:
/*
int lofar_udp_skip_to_packet_meta(lofar_udp_reader *reader, long currentPacket, long targetPacket) {
	if (reader->readerType == ZSTDCOMPRESSED) {
		// Standard search method
	} else {
		// fseek(); // ... then search
	}
}
*/