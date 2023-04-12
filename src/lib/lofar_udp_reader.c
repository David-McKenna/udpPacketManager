#include "lofar_udp_reader.h"

/**
 * @brief 			Check input header data for malformed variables
 * @param port 		Reference port number in case an error occurs
 * @param header 	Header data
 * @return 0: success, other: failure
 */
int _lofar_udp_reader_malformed_header_checks(const int8_t header[16]) {
	lofar_source_bytes *source = (lofar_source_bytes *) &(header[CEP_HDR_SRC_OFFSET]);
	VERBOSE(printf("Port %d\n", port);
		        printf("RSP: %d\n", source->rsp);
		        printf("Padding0: %d\n", source->padding0);
		        printf("ERRORBIT: %d\n", source->errorBit);
		        printf("CLOCK: %d\n", source->clockBit);
		        printf("bitMode: %d\n", source->bitMode);
		        printf("padding1: %d\n\n", source->padding1));

	// Data integrity checks
	if ((uint8_t) header[CEP_HDR_RSP_VER_OFFSET] < UDPCURVER) {
		fprintf(stderr, "Input header appears malformed (RSP Version less than 3, %d), exiting.\n", (int) header[CEP_HDR_RSP_VER_OFFSET]);
		return -1;
	}

	if (*((int32_t *) &(header[CEP_HDR_TIME_OFFSET])) < LFREPOCH) {
		fprintf(stderr, "Input header appears malformed (data timestamp before 2008, %u), exiting.\n", *((uint32_t *) &(header[CEP_HDR_TIME_OFFSET])));
		return -1;
	}

	if (*((int32_t *) &(header[CEP_HDR_SEQ_OFFSET])) > RSPMAXSEQ) {
		fprintf(stderr,
		        "Input header appears malformed (sequence higher than 200MHz clock maximum, %d), exiting.\n", *((uint32_t *) &(header[CEP_HDR_SEQ_OFFSET])));
		return -1;
	}

	if ((uint8_t) header[CEP_HDR_NBEAM_OFFSET] > UDPMAXBEAM) {
		fprintf(stderr,
		        "Input header appears malformed (more than UDPMAXBEAM beamlets on a port, %d), exiting.\n",
		        header[CEP_HDR_NBEAM_OFFSET]);
		return -1;
	}

	if ((uint8_t) header[CEP_HDR_NTIMESLICE_OFFSET] != UDPNTIMESLICE) {
		fprintf(stderr,
		        "Input header appears malformed (time slices are %d, not UDPNTIMESLICE), exiting.\n",
		        header[CEP_HDR_NTIMESLICE_OFFSET]);
		return -1;
	}

	if (source->padding0 != (uint32_t) 0) {
		fprintf(stderr, "Input header appears malformed (padding bit (0) is set), exiting.\n");
		return -1;
	} else if (source->errorBit != (uint32_t) 0) {
		fprintf(stderr, "Input header appears malformed (error bit is set), exiting.\n");
		return -1;
	} else if (source->bitMode == (uint32_t) 3) {
		fprintf(stderr, "Input header appears malformed (BM of 3 doesn't exist), exiting.\n");
		return -1;
	} else if (source->padding1 > (uint32_t) 1) {
		fprintf(stderr, "Input header appears malformed (padding bits (1) are set), exiting.\n");
		return -1;
	} else if (source->padding1 == (uint32_t) 1) {
		fprintf(stderr,
		        "Input header appears malformed (our replay packet warning bit is set), continuing with caution...\n");
	}

	return 0;

}

/**
 * @brief			Extract LOFAR header metadata to a lofar_udp_obs_meta struct
 * @param port		Current port (for storing in meta)
 * @param meta		The output metadata struct
 * @param header	The raw header bytes
 * @param beamletLimits		The upper and lower limit beamlets used for data extraction
 */
void lofar_udp_parse_extract_header_metadata(int port, lofar_udp_obs_meta *meta, const int8_t header[UDPHDRLEN], const int16_t beamletLimits[2]) {
	lofar_source_bytes *source = (lofar_source_bytes *) &(header[CEP_HDR_SRC_OFFSET]);

	// Extract the station ID
	// Divide by 32 to convert from (my current guess based on SE607 / IE613 codes) RSP IDs to station codes
	int16_t stationID = (int16_t) (*((int16_t *) &(header[CEP_HDR_STN_ID_OFFSET])) / 32);
	if (port == 0) {
		meta->stationID = stationID;
	} else if (meta->stationID != stationID) {
		fprintf(stderr, "WARNING: Telescope ID does not seem to be consistent between ports (port %d, %d vs %d), continue with caution.\n", port, meta->stationID, stationID);
	}

	// Determine the number of beamlets on the port
	VERBOSE(printf("port %d, bitMode %d, beamlets %d (%u)\n", port, source->bitMode,
	               (int32_t) ((uint8_t) header[CEP_HDR_NBEAM_OFFSET]),
	               (uint8_t) header[CEP_HDR_NBEAM_OFFSET]););
	meta->portRawBeamlets[port] = (int16_t) ((uint8_t) header[CEP_HDR_NBEAM_OFFSET]);

	// Assume we are processing all beamlets by default
	meta->upperBeamlets[port] = meta->portRawBeamlets[port];

	// Update the cumulative beamlets, total raw beamlets
	// Ordering is intentional so that we get the number of beamlets before this port.
	meta->portRawCumulativeBeamlets[port] = meta->totalRawBeamlets;
	meta->portCumulativeBeamlets[port] = meta->totalProcBeamlets;

	// Set the  upper, lower limit of beamlets as needed

	// Check the upper limit first, so that we can modify upperBeamlets if needed.
	if (beamletLimits[1] > 0 && (beamletLimits[1] < ((port + 1) * meta->portRawBeamlets[port])) &&
	    (beamletLimits[1] >= port * meta->portRawBeamlets[port])) {
		meta->upperBeamlets[port] = beamletLimits[1] - meta->totalRawBeamlets;
	}


	// Check the lower beamlet, and then update the total beamlets counters as needed
	if (beamletLimits[0] > 0 && (beamletLimits[0] < ((port + 1) * meta->portRawBeamlets[port])) &&
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
	switch ((int32_t) source->bitMode) {
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
			__builtin_unreachable();
			fprintf(stderr, "How did we get here? BM=3 should have been caught already, but we have case BM=%d...\n", source->bitMode);
			break;
	}

}

/**
 * @brief      Parse LOFAR UDP headers to determine some metadata about the
 *             input port data
 *
 * @param      meta           The lofar_udp_obs_meta to initialise
 * @param[in]  header         The header data to process
 * @param[in]  beamletLimits  The upper/lower beamlets limits
 *
 * @return     0: Success, -1: Fatal error
 */
int _lofar_udp_parse_headers(lofar_udp_obs_meta *meta, const int8_t header[4][16], const int16_t beamletLimits[2]) {

	float bitMul;
	int8_t cacheBitMode = 0;
	int16_t beamsDelta = beamletLimits[1] - beamletLimits[0];
	meta->totalRawBeamlets = 0;
	meta->totalProcBeamlets = 0;


	// Process each input port
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (meta->VERBOSE) { printf("Port %d/%d\n", port, meta->numPorts - 1); });
		if (_lofar_udp_reader_malformed_header_checks(header[port])) {
			fprintf(stderr, "ERROR: Failed header checks on port %d, exiting.\n", port);
			return -1;
		}

		// TODO: Require/warn to ensure increasing RSPs?
		// Extract the header components into our metadata struct
		lofar_udp_parse_extract_header_metadata(port, meta, header[port], beamletLimits);

		// Ensure that the bitmode does not change between ports; we assume it is constant elsewhere.
		// Fairly certain the RSPs fall over if you try to do multiple bit modes anyway.
		if (port == 0) {
			cacheBitMode = meta->inputBitMode;
		} else if (cacheBitMode != meta->inputBitMode) {
			fprintf(stderr,
			        "Multiple input bit sizes detected; please parse these ports separately (port 0: %d, port %d: %d). Exiting.\n",
			        cacheBitMode, port, meta->inputBitMode);
			return -1;
		}

		// Determine the size of the input array
		// 4-bit: half the size per sample
		// 16-bit: 2x the size per sample
		bitMul = 1.0f + (-0.5f * (float) (meta->inputBitMode == 4)) + (float) (meta->inputBitMode == 16);
		meta->portPacketLength[port] = ((UDPHDRLEN) + (meta->portRawBeamlets[port] * ((int16_t) (bitMul * UDPNTIMESLICE * UDPNPOL))));

		if (port > 0) {
			if (meta->portPacketLength[port] != meta->portPacketLength[port - 1]) {
				fprintf(stderr,
						"WARNING: Packet lengths different between port offsets %d and %d (%d vs %d bytes), proceeding with caution.\n",
						port, port - 1, meta->portPacketLength[port], meta->portPacketLength[port - 1]);
			}
		}

		if (beamletLimits[0] > 0 || beamletLimits[1] > 0) {
			if (meta->totalProcBeamlets == beamsDelta) {
				break;
			} else if (meta->totalProcBeamlets > beamsDelta) {
				fprintf(stderr, "ERROR: Beam limits were incorrectly handled, we expected %d beams but are setup to process %d.Please report your limits and a sample of data as a bug. Exiting.\n", meta->totalProcBeamlets, beamsDelta);
				return -1;
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
	// Exit early if the start time is before our observation begins
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

		// Adjust the dropped packets to cause the next reader operation to do nothing,
		if (lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][lastPacketOffset])) >= reader->meta->lastPacket) {
			reader->meta->portLastDroppedPackets[port] = reader->meta->packetsPerIteration;
		// Or perform a partial read
		} else {
			reader->meta->portLastDroppedPackets[port] =
				lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][lastPacketOffset])) -
				(currentPacket + reader->meta->packetsPerIteration);
		}

		// Reset variable if the offset was negative to allow for a full read
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
		// 	until the last packet is beyond the target packet (so the target packet is in, or passed in, the current array)
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
			returnVal = _lofar_udp_reader_internal_read_step(reader);
			if (returnVal < 0) { return returnVal; }

			currentPacket = lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][lastPacketOffset]));

			// Account for packet de-sync between ports
			for (int portInner = 0; portInner < reader->meta->numPorts; portInner++) {
				// Check if the target is passed for this array
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

				// Always ensure we have a 0/positive dropped packet count
				if (reader->meta->portLastDroppedPackets[portInner] < 0) {
					reader->meta->portLastDroppedPackets[portInner] = 0;
				}

				// Raise a warning if we see a large amount of data dropped during the scan, as this may cause the target to be missed
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

		// Check if we passed the final target packet at the start of our array (packet is no longer accessible, as a result of the
		// entire observation window being lost to packet loss, or a significant logic failure above)
		if (lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][0])) > reader->meta->lastPacket) {
			fprintf(stderr, "\nPort %d has scanned beyond target packet %ld (to start at %ld), exiting.\n", port,
			        reader->meta->lastPacket, lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][0])));
			return -1;
		}

		if (scanning) { printf("\n\33[2KPassed target packet %ld on port %d.\n", reader->meta->lastPacket, port); }
	}
	// Data is now all loaded such that every inputData array has or is past the target packet, now we need to internally align and pad the arrays

	// For each port,
	for (int8_t port = 0; port < reader->meta->numPorts; port++) {

		// Re-initialise the offset shift needed on each port
		for (int8_t portS = 0; portS < reader->meta->numPorts; portS++) packetShift[portS] = 0;

		// Get the current packet, and guess the target packet by assuming no packet loss
		currentPacket = lofar_udp_time_get_packet_number(&(reader->meta->inputData[port][0]));

		// Memory check: avoid segfaults during heavy packet loss
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

		// If no packets are dropped, just shift across to the data
		if (guessPacket == reader->meta->lastPacket) {
			packetShift[port] = reader->meta->packetsPerIteration - (reader->meta->lastPacket - currentPacket);

		// The packet at the index is too high/low, we've had packet loss. Perform a binary-style search until we find the packet, or the next best target
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
		returnVal = _lofar_udp_shift_remainder_packets(reader, packetShift, 0);
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


		// Reset the packetShift for the port to 0
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
	if (packetsReadMax < 1) { localMaxPackets = LONG_MAX; }

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
	returnVal = _lofar_udp_get_first_packet_alignment(reader);
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
 * @param      meta  The lofar_udp_obs_meta to read from and update
 *
 * @return     0: Success, -1: Unknown processing mode supplied
 */
int _lofar_udp_setup_processing(lofar_udp_obs_meta *meta) {
	int hdrOffset = -1 * UDPHDRLEN; // Default: no header, offset by -16
	int equalIO = 0; // If packet length in + out should match
	float divFactor = 1.0f; // Scale packet length linearly
	int32_t workingData;


	// Sanity check the processing mode
	switch (meta->processingMode) {
		case PACKET_FULL_COPY ... PACKET_NOHDR_COPY:
			if (meta->calibrateData > GENERATE_JONES) {
				fprintf(stderr, "WARNING: Modes 0 and 1 cannot be calibrated as they are copying full packets, we will generate calibration tables but not apply them, continuing.\n");
				meta->calibrateData = GENERATE_JONES;
			}
		case PACKET_SPLIT_POL:
		case BEAMLET_MAJOR_FULL ... BEAMLET_MAJOR_SPLIT_POL:
		case BEAMLET_MAJOR_FULL_REV ... BEAMLET_MAJOR_SPLIT_POL_REV:
		case TIME_MAJOR_FULL ... TIME_MAJOR_ANT_POL:
		case TIME_MAJOR_ANT_POL_FLOAT:
		case STOKES_I ... STOKES_I_DS16:
		case STOKES_Q ... STOKES_Q_DS16:
		case STOKES_U ... STOKES_U_DS16:
		case STOKES_V ... STOKES_V_DS16:
		case STOKES_IQUV ... STOKES_IQUV_DS16:
		case STOKES_IV ... STOKES_IV_DS16:
		case STOKES_I_REV ... STOKES_I_DS16_REV:
		case STOKES_Q_REV ... STOKES_Q_DS16_REV:
		case STOKES_U_REV ... STOKES_U_DS16_REV:
		case STOKES_V_REV ... STOKES_V_DS16_REV:
		case STOKES_IQUV_REV ... STOKES_IQUV_DS16_REV:
		case STOKES_IV_REV ... STOKES_IV_DS16_REV:
			break;

		case UNSET_MODE:
		case TEST_INVALID_MODE:
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
		case PACKET_FULL_COPY:
			hdrOffset = 0; // include header
#pragma GCC diagnostic pop
		case PACKET_NOHDR_COPY:
			meta->numOutputs = meta->numPorts;
			equalIO = 1;

			// These are the only processing modes that do not require 4-bit
			// data to be extracted. EqualIO will handle our sizes anyway
			if (meta->inputBitMode == 4) {
				meta->outputBitMode = 4;
			}
			break;

		case BEAMLET_MAJOR_FULL:
		case BEAMLET_MAJOR_FULL_REV:
		case TIME_MAJOR_FULL:
			meta->numOutputs = 1;
			break;

		case PACKET_SPLIT_POL:
		case BEAMLET_MAJOR_SPLIT_POL:
		case BEAMLET_MAJOR_SPLIT_POL_REV:
		case TIME_MAJOR_SPLIT_POL:
			meta->numOutputs = UDPNPOL;
			break;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic push
		// Mode 35: FFTW format, forced to floats so that we can directly use the output
		case TIME_MAJOR_ANT_POL_FLOAT:
			meta->outputBitMode = 32;
#pragma GCC diagnostic pop
		case TIME_MAJOR_ANT_POL:
			meta->numOutputs = 2;
			break;

			// Base Stokes Methods
		case STOKES_I ... STOKES_I_DS16:
		case STOKES_Q ... STOKES_Q_DS16:
		case STOKES_U ... STOKES_U_DS16:
		case STOKES_V ... STOKES_V_DS16:
		case STOKES_I_REV ... STOKES_I_DS16_REV:
		case STOKES_Q_REV ... STOKES_Q_DS16_REV:
		case STOKES_U_REV ... STOKES_U_DS16_REV:
		case STOKES_V_REV ... STOKES_V_DS16_REV:
			meta->numOutputs = 1;
			meta->outputBitMode = 32;
			// 4 input words -> 1 larger word
			// Bit shift based on processing mode, 2^(mode % 10)
			// Base of 4 from 4:1 i/o ratio
			divFactor = (float) (4 << ((meta->processingMode % 10)));
			break;

		case STOKES_IQUV ... STOKES_IQUV_DS16:
		case STOKES_IQUV_REV ... STOKES_IQUV_DS16_REV:
			meta->numOutputs = 4;
			meta->outputBitMode = 32;
			// 4 input words -> 4 larger words
			// Bit shift based on processing mode, 2^(mode % 10)
			// Base of 1 from 1:1 i/o ratio
			divFactor = (float) (1 << ((meta->processingMode % 10)));
			break;

		case STOKES_IV ... STOKES_IV_DS16:
		case STOKES_IV_REV ... STOKES_IV_DS16_REV:
			meta->numOutputs = 2;
			meta->outputBitMode = 32;
			// 4 input words -> 2 larger word
			// Bit shift based on processing mode, 2^(mode % 10)
			// Base of 2 from 2:1 i/o ratio
			divFactor = (float) (2 << ((meta->processingMode % 10)));
			break;

		case UNSET_MODE:
		case TEST_INVALID_MODE:
		default:
			// Any missing cases should have been caught by the previous switch statement
			__builtin_unreachable();
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
		// Calculate the number of input bytes per iteration
		workingData = (int32_t) (((float) (meta->totalProcBeamlets * UDPNPOL * UDPNTIMESLICE)) * ((float) meta->inputBitMode / 8.0f)
        // Convert to the number of output bytes per iteration
								* (((float) meta->outputBitMode / (float) meta->inputBitMode) / divFactor));

		// Add headers if needed
		workingData += meta->numPorts * (hdrOffset + UDPHDRLEN);

		// Split across the ports
		workingData /= meta->numOutputs;

		// Populate the outputs with the amount of data needed per packet processed
		for (int out = 0; out < meta->numOutputs; out++) {
			meta->packetOutputLength[out] = workingData;
		}
	}

	return 0;
}


/**
 * @brief      Sanity check an input configuration for major/minor issues
 *
 * @param      config  The configuration
 *
 * @return     0: Success, other: Major issue/failure
 */
int _lofar_udp_reader_config_check(lofar_udp_config *config) {

	if (config->numPorts > MAX_NUM_PORTS || config->numPorts < 1) {
		fprintf(stderr, "ERROR: You requested %d ports, but LOFAR can only produce between 1 and %d, exiting.\n", config->numPorts,
				MAX_NUM_PORTS);
		return -1;
	}

	for (int32_t port = 0; port < config->numPorts; port++) {
		if (!strlen(config->inputLocations[port])) {
			fprintf(stderr, "ERROR: You requested %d ports, but port %d is an empty string, exiting.\n", config->numPorts, port);
			return -1;
		} else if (access(config->inputLocations[port], F_OK) != 0) {
			fprintf(stderr, "ERROR: Failed to open file at %s (port %d), exiting.\n", config->inputLocations[port], port);
			return -1;
		}
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

		if (config->processingMode == PACKET_FULL_COPY || config->processingMode == PACKET_NOHDR_COPY) {
			fprintf(stderr, "ERROR: Processing modes PACKET_FULL_COPY and PACKET_NOHDR_COPY do not support setting beamlet limits, exiting.\n");
			return -1;
		}
	}

	if (config->calibrateData < NO_CALIBRATION || config->calibrateData > APPLY_CALIBRATION) {
		fprintf(stderr, "ERROR: Invalid value for calibrateData (%d), exiting.\n",
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
		fprintf(stderr, "ERROR: Invalid cap on packets to read (%ld), exiting.\n", config->packetsReadMax);
		return -1;
	}

	if (config->packetsReadMax < config->packetsPerIteration) {
		fprintf(stderr, "WARNING: Maximum time is less that initial iteration size, reducing packets per iteration to %ld.\n", config->packetsReadMax);
		config->packetsReadMax = config->packetsPerIteration;
	}

	if ((2 * config->ompThreads) < config->numPorts) {
		if (config->ompThreads < 1) {
			fprintf(stderr, "WARNING: Thread count unset. Resetting number of threads to compile-time default, %d.\n", OMP_THREADS);
			config->ompThreads = OMP_THREADS;
		} else {
			fprintf(stderr, "WARNING: You have requested less threads than advised (2* number of ports), this might be a slow run (%d threads, %d ports).\n", config->ompThreads, config->numPorts);
		}
	}

	if (config->replayDroppedPackets != 0 && config->replayDroppedPackets != 1) {
		fprintf(stderr, "ERROR: Invalid value for replayDroppedPackets (%d, should be 0 or 1), exiting.\n",
				config->replayDroppedPackets);
		return -1;
	}


	return 0;
}


void _lofar_udp_configure_obs_meta(const lofar_udp_config *config, lofar_udp_obs_meta *meta) {
	// Set the simple metadata defaults
	meta->numPorts = config->numPorts;
	meta->replayDroppedPackets = config->replayDroppedPackets;
	meta->processingMode = config->processingMode;
	meta->packetsPerIteration = config->packetsPerIteration;
	meta->packetsReadMax = config->packetsReadMax < 1 ? LONG_MAX : config->packetsReadMax;
	meta->lastPacket = config->startingPacket;
	meta->calibrateData = config->calibrateData;

	VERBOSE(meta->VERBOSE = config->verbose);
}

/**
 * @brief      Set up a lofar_udp_reader and associated lofar_udp_obs_meta using a
 *             set of input files and pre-set control/metadata parameters
 *
 * @param      config  The configuration struct, detailed options above
 *
 * @return     lofar_udp_reader ptr, or NULL on error
 */
lofar_udp_reader *lofar_udp_reader_setup(lofar_udp_config *config) {

	// Sanity check our inputs
	if (_lofar_udp_reader_config_check(config) < 0) {
		return NULL;
	}

	if (lofar_udp_prepare_signal_handler() < 0) {
		return NULL;
	}

	// Setup the metadata struct
	lofar_udp_obs_meta *meta = lofar_udp_obs_meta_alloc();
	CHECK_ALLOC_NOCLEAN(meta, NULL);
	_lofar_udp_configure_obs_meta(config, meta);

	// Prepare to read in the first header on each input.
	int8_t inputHeaders[MAX_NUM_PORTS][UDPHDRLEN];
	int64_t readlen;

	#ifndef ALLOW_VERBOSE
	if (config->verbose) fprintf(stderr, "Warning: verbosity was disabled at compile time, but you requested it. Continuing...\n");
	#endif





	// Scan in the first header on each port
	for (int8_t port = 0; port < meta->numPorts; port++) {
		readlen = lofar_udp_io_read_temp(config, port, &(inputHeaders[port][0]), sizeof(int8_t), UDPHDRLEN, 1);
		if (readlen != UDPHDRLEN) {
			fprintf(stderr, "Unable to read header on port %d, exiting.\n", port);
			FREE_NOT_NULL(meta);
			return NULL;
		}

	}


	// Parse the input file headers to get packet metadata on each port
	// We may repeat this step if the selected beamlets cause us to drop a port of dataa
	int updateBeamlets = (config->beamletLimits[0] > 0 || config->beamletLimits[1] > 0);
	int16_t beamletLimits[2] = { 0, 0 };
	while (updateBeamlets != -1) {
		VERBOSE(if (meta->VERBOSE) { printf("Handle headers: %d\n", updateBeamlets); });
		// Standard setup
		if (_lofar_udp_parse_headers(meta, inputHeaders, beamletLimits) < 0) {
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


	if (_lofar_udp_setup_processing(meta)) {
		fprintf(stderr, "Unable to setup processing mode %d, exiting.\n", config->processingMode);
		FREE_NOT_NULL(meta);
		return NULL;
	}

	// Allocate the memory needed to store the raw / reprocessed data, initialise the variables that are stored on a per-port basis.
	for (int port = 0; port < meta->numPorts; port++) {
		// The input buffer is extended by 2 packets to allow for both a reference packet from a previous iteration
		// and a fall-back packet encase replaying packets is disabled (all 0s to pad the output)


		// If we have a compressed reader, align the length with the ZSTD buffer sizes
		size_t additionalBufferSize = _lofar_udp_io_read_ZSTD_fix_buffer_size(meta->portPacketLength[port] * meta->packetsPerIteration, 1);

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
	lofar_udp_reader *reader = lofar_udp_reader_alloc(meta);

	// Initialise the reader struct from config
	reader->input->readerType = config->readerType;
	reader->packetsPerIteration = meta->packetsPerIteration;
	reader->meta = meta;

	reader->ompThreads = config->ompThreads;
	omp_set_num_threads(reader->ompThreads);

	if ((meta->numPorts + config->offsetPortCount) > 4) {
		fprintf(stderr, "ERROR: Requested data beyond the 4th port. Either your offset (%d) or number of ports requested (%d) was too high, exiting.\n", config->offsetPortCount, meta->numPorts);
		lofar_udp_reader_cleanup(reader);
		return NULL;
	}

	for (int8_t port = 0; port < (meta->numPorts); port++) {
		// Check for errors, cleanup and return if needed
		if (_lofar_udp_io_read_setup_internal_lib_helper(reader->input, config, meta, port) < 0) {
			lofar_udp_reader_cleanup(reader);
			return NULL;
		}

	}

	// TODO: Copy values to calibration struct as needed.
	if (config->metadata_config.metadataType != NO_META) {
		VERBOSE(printf("Priming metadata type %d from %s\n", config->metadata_config.metadataType, config->metadata_config.metadataLocation));

		reader->metadata = lofar_udp_metadata_alloc();
		CHECK_ALLOC(reader->metadata, NULL, lofar_udp_reader_cleanup(reader););

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
	if (_lofar_udp_reader_internal_read_step(reader) < 0) {
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
	if (_lofar_udp_get_first_packet_alignment(reader) < 0) {
		lofar_udp_reader_cleanup(reader);
		return NULL;
	}

	reader->meta->inputDataReady = 1;
	return reader;
}



/**
 * @brief      Generate the inverted Jones matrix to calibrate the observed data
 *
 * @param      reader  The lofar_udp_reader
 *
 * @return     int: 0: Success, -1: Failure
 */
int _lofar_udp_reader_calibration_generate_data(lofar_udp_reader *reader) {
	FILE *fifo;
	int numBeamlets, numTimesamples, returnVal;
	// Ensure we are meant to be calibrating data
	if (reader->meta->calibrateData == NO_CALIBRATION) {
		fprintf(stderr, "ERROR: Requested calibration while calibration is disabled. Exiting.\n");
		return -1;
	}

	char* const jonesGenerator = "dreamBeamJonesGenerator.py";

	if (access(jonesGenerator, X_OK) != 0) {
		fprintf(stderr, "ERROR %s: Unable to find executable %s on path, exiting.\n", __func__, jonesGenerator);
		return -1;
	}

	if (!reader->meta->clockBit) {
		fprintf(stderr, "ERROR: Mode 6 is currently not supported for calibration, exiting.\n");
		return -1;
	}

	// For security, add a few random ASCII characters to the end of the suggested name
	const int numRandomChars = 4;
	size_t fifoNameSize = strnlen(reader->calibration->calibrationFifo, DEF_STR_LEN) + numRandomChars;
	char *fifoName = calloc(fifoNameSize + 1, sizeof(char));
	char randomChars[numRandomChars];

	for (int i = 0; i < numRandomChars; i++) {
		// Offset to base of letters in ASCII (65) In the range of letters +(0  - 25), + 0.5 chance of +32 to switch between lower and upper case
		randomChars[i] = (char) (65 + (rand() % 26) + (rand() % 2 * 32)); // NOLINT
	}
	returnVal = snprintf(fifoName, fifoNameSize, "%s_%4s", reader->calibration->calibrationFifo, randomChars);

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
	const int shortStr = 31, longStr = 511, longestStr = 2 * DEF_STR_LEN - 1;
	char stationID[shortStr + 1], mjdTime[shortStr + 1], duration[shortStr + 1], integration[shortStr + 1], pointing[longStr + 1];


	_lofar_udp_metadata_get_station_name(reader->meta->stationID, &(stationID[0]));
	snprintf(mjdTime, shortStr,  "%lf", lofar_udp_time_get_packet_time_mjd(reader->meta->inputData[0]));
	snprintf(duration, shortStr, "%30.10f", reader->calibration->calibrationDuration);
	snprintf(integration, shortStr, "%15.10f", (float) (reader->packetsPerIteration * UDPNTIMESLICE) *
									(float) (clock200MHzSampleTime * reader->meta->clockBit +
									         clock160MHzSampleTime * (1 - reader->meta->clockBit)));
	snprintf(pointing, longStr, "%f,%f,%s", reader->metadata->ra_rad,
			reader->metadata->dec_rad, reader->metadata->coord_basis);

	char calibrationSubbands[longestStr + 1];
	snprintf(calibrationSubbands, longestStr, "%d", reader->metadata->subbands[reader->metadata->lowerBeamlet]);

	int strOffset = 0;
	for (int sbb = reader->metadata->lowerBeamlet + 1; sbb < reader->metadata->upperBeamlet; sbb++) {
		int sub = reader->metadata->subbands[sbb];
		// TOOD: Better memory safety
		strOffset += snprintf(&(calibrationSubbands[strOffset]), longestStr - strOffset, ",%d", sub);
	}


	VERBOSE(printf("Calling dreamBeam: %s %s %s %s %s %s %s\n", stationID, mjdTime,
				   calibrationSubbands, duration, integration, pointing, fifoName));

	char * const argv[] = { jonesGenerator, "--stn", stationID, "--time", mjdTime,
					 "--sub", calibrationSubbands,
					 "--dur", duration, "--int", integration, "--pnt", pointing,
					 "--pipe", fifoName,
					 NULL };

	pid_t pid;
	printf("Generating Jones matrices via %s...\n", jonesGenerator);
	returnVal = posix_spawnp(&pid, jonesGenerator, NULL, NULL, argv, environ);

	VERBOSE(printf("Fork\n"););
	if (returnVal == 0) {
		VERBOSE(printf("%s has been launched.\n", jonesGenerator));
	} else if (returnVal < 0) {
		fprintf(stderr, "ERROR: Unable to create child process to call %s. Exiting.\n", jonesGenerator);
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
		fprintf(stderr, "ERROR: Failed to parse number of beamlets and time samples from %s. Exiting.\n", jonesGenerator);
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
				fprintf(stderr, "ERROR: Unable to parse main pipe from %s (%d, %d). Exiting.\n", jonesGenerator, timeIdx,
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
			fprintf(stderr, "ERROR: unable to parse final pipe from %s (%d). Exiting.\n", jonesGenerator, timeIdx);
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
	reader->meta->calibrationStep = -1; // We will bump this on the next usage so that it starts at 0
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
int _lofar_udp_reader_internal_read_step(lofar_udp_reader *reader) {
	int returnVal = 0;

	// Make sure we have work to perform
	if (reader->meta->packetsPerIteration == 0) {
		fprintf(stderr, "Last packets per iteration was 0, there is no work to perform, exiting...\n");
		return -1;
	}

	// Reset the packets per iteration to the intended length (can be lowered due to out of order packets)
	reader->meta->packetsPerIteration = reader->packetsPerIteration;

	// If packets were dropped, shift the remaining packets back to the start of the array
	if (_lofar_udp_shift_remainder_packets(reader, reader->meta->portLastDroppedPackets, 1) < 0) { return -1; }

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
	long charsToRead = reader->meta->packetsPerIteration;
#pragma omp parallel for shared(returnVal, reader, stderr) firstprivate(charsToRead)
	for (int8_t port = 0; port < reader->meta->numPorts; port++) {
		long charsRead, packetPerIter;

		// Determine how much data is needed and read-in to the offset after any leftover packets
		if (reader->meta->portLastDroppedPackets[port] > charsToRead) {
			fprintf(stderr, "\nWARNING: Port %d not performing read due to excessive packet loss.\n", port);
			continue;
		} else {
			charsToRead = (charsToRead - reader->meta->portLastDroppedPackets[port]) *
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
			returnVal = -3;
			VERBOSE(printf("%ld, %f, %d\n", charsRead, (float) reader->meta->inputDataOffset[port] / reader->meta->portPacketLength[port], reader->meta->portPacketLength[port]));
			packetPerIter =
					(charsRead + reader->meta->inputDataOffset[port]) / reader->meta->portPacketLength[port];

			#pragma omp critical (packetCheck)
			{
				if (packetPerIter < reader->meta->packetsPerIteration) {
					// Sanity check, out of order packets can cause a negative value here
					if (packetPerIter > 0) {
#pragma omp atomic write
						reader->meta->packetsPerIteration = packetPerIter;
					} else {
#pragma omp atomic write
						reader->meta->packetsPerIteration = 0;
#pragma omp atomic write
						returnVal = 1;

					}
					fprintf(stderr,
							"Received less data from file on port %d than expected, may be nearing end of file.\nReducing packetsPerIteration to %ld, to account for the limited amount of input data.\n",
							port, reader->meta->packetsPerIteration);
				}
#ifdef __SLOWDOWN
				sleep(5);
#endif

			}
		}
	}

	// Mark the input data are ready to be processed
	if (reader->meta->packetsPerIteration > 0) {
		reader->meta->inputDataReady = 1;
		return returnVal;
	}
	return 1;
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
		reader->meta->calibrationStep >= (reader->calibration->calibrationStepsGenerated - 1)) {
		VERBOSE(printf("Calibration buffer has run out, generating new Jones matrices.\n"));
		if ((readReturnVal = _lofar_udp_reader_calibration_generate_data(reader)) < 0) {
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
		readReturnVal = _lofar_udp_reader_internal_read_step(reader);
		if (readReturnVal == -1 || readReturnVal == 1) { return readReturnVal; }
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
int lofar_udp_reader_step(lofar_udp_reader *reader) {
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
int _lofar_udp_get_first_packet_alignment(lofar_udp_reader *reader) {

	// Determine the maximum port packet number, update the variables as needed
	for (int port = 0; port < reader->meta->numPorts; port++) {
		// Initialise/Reset the dropped packet counters
		reader->meta->portLastDroppedPackets[port] = 0;
		reader->meta->portTotalDroppedPackets[port] = 0;
		int64_t currentPacket = lofar_udp_time_get_packet_number(reader->meta->inputData[port]);

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
 * @param      meta  The input lofar_udp_obs_meta struct to process
 *
 * @return     int: 0: Success, -1: Negative shift requested, out of order data
 *             on last gulp,
 */
int _lofar_udp_shift_remainder_packets(lofar_udp_reader *reader, const long shiftPackets[], int handlePadding) {

	int returnVal = 0, fixBuffer = 0;
	int destOffset, portPacketLength;
	long byteShift, sourceOffset;

	int8_t *inputData;
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
			if (destOffset != sourceOffset) {
				memmove(&(inputData[destOffset]), &(inputData[sourceOffset]), byteShift);
			}

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

/**
 * @brief      Optionally close input files, free alloc'd memory, free zstd
 *             decompression streams once we are finished.
 *
 * @param[in]  reader  The lofar_udp_reader struct to cleanup
 * @param[in]  closeFiles  bool: close input files (1) or don't (0)
 */
void lofar_udp_reader_cleanup(lofar_udp_reader *reader) {

	if (reader->meta != NULL) {
		// Cleanup the malloc/calloc'd memory addresses, close the input files.
		for (int8_t i = 0; i < reader->meta->numOutputs; i++) {
			FREE_NOT_NULL(reader->meta->outputData[i]);
		}

		for (int8_t i = 0; i < reader->meta->numPorts; i++) {
			// Free input data pointer (from the correct offset)
			if (reader->meta->inputData[i] != NULL) {

				VERBOSE(if (reader->meta->VERBOSE) {
					printf("On port: %d freeing inputData at %p\n", i, reader->meta->inputData[i] -
					                                                   2 * reader->meta->portPacketLength[i]);
				});

				int8_t *tmpPtr = (reader->meta->inputData[i] - 2 * reader->meta->portPacketLength[i]);
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



/*
// TODO: In the case that the input data appears corrupted, do a byte-by-byte search looking for data that matches the expected header format
int lofar_udp_realign_data(lofar_udp_reader *reader) {

}
*/