#include "lofar_cli_meta.h"

// Constant to define the length of the timing variable array
#define TIMEARRLEN 4

void helpMessages() {
	printf("LOFAR UDP Data extractor (CLI v%s, lib v%s)\n\n", UPM_CLI_VERSION, UPM_VERSION);
	printf("Usage: lofar_cli_extractor <flags>");

	printf("\n\n");

	sharedFlags();

	//printf();
	printf("-p: <mode>		Processing mode, options listed below (default: 0)\n");


	processingModes();

}

void CLICleanup(lofar_udp_config *config, lofar_udp_io_write_config *outConfig, int8_t *headerBuffer) {

	FREE_NOT_NULL(config);
	FREE_NOT_NULL(outConfig);
	FREE_NOT_NULL(headerBuffer);
}


int main(int argc, char *argv[]) {

	// Set up input local variables
	int inputOpt, input = 0;
	float seconds = 0.0f;
	char inputTime[256] = "", stringBuff[128] = "", inputFormat[DEF_STR_LEN] = "";
	int silent = 0, inputProvided = 0, outputProvided = 0;
	long maxPackets = -1, startingPacket = -1, splitEvery = LONG_MAX;
	int8_t clock200MHz = 1;

	lofar_udp_config *config = lofar_udp_config_alloc();
	lofar_udp_io_write_config *outConfig = lofar_udp_io_write_alloc();
	int8_t *headerBuffer = NULL;

	if (config == NULL || outConfig == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate memory for configuration structs (something has gone very wrong...), exiting.\n");
		FREE_NOT_NULL(config); FREE_NOT_NULL(outConfig);
		return 1;
	}

	// Set up reader loop variables
	int64_t loops = 0, localLoops = 0, returnValMeta = 0;
	int64_t returnVal;
	int64_t packetsProcessed = 0, packetsWritten = 0, packetsToWrite;

	// Timing variables
	double timing[TIMEARRLEN] = { 0. }, totalReadTime = 0., totalOpsTime = 0., totalWriteTime = 0., totalMetadataTime = 0.;
	struct timespec tick, tick0, tick1, tock, tock0, tock1;

	// strtol / option checks
	char *endPtr, flagged = 0;

	// Standard ugly input flags parser
	while ((inputOpt = getopt(argc, argv, "hzrqfvVi:o:m:M:I:u:t:s:S:e:p:a:n:b:ck:T:")) != -1) {
		input = 1;
		switch (inputOpt) {

			case 'i':
				strncpy(inputFormat, optarg, DEF_STR_LEN - 1);
				inputProvided = 1;
				break;


			case 'o':
				if (lofar_udp_io_write_parse_optarg(outConfig, optarg) < 0) {
					helpMessages();
					CLICleanup(config, outConfig, headerBuffer);
					return 1;
				}
				// If the metadata is not yet set, see if we can parse a requested type from the output filename
				if (config->metadata_config.metadataType == NO_META) config->metadata_config.metadataType = lofar_udp_metadata_parse_type_output(optarg);
				outputProvided = 1;
				break;

			case 'm':
				config->packetsPerIteration = strtol(optarg, &endPtr, 10);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'M':
				config->metadata_config.metadataType = lofar_udp_metadata_string_to_meta(optarg);
				break;

			case 'I':
				if (strncpy(config->metadata_config.metadataLocation, optarg, DEF_STR_LEN) != config->metadata_config.metadataLocation) {
					fprintf(stderr, "ERROR: Failed to copy metadata file location to config, exiting.\n");
					CLICleanup(config, outConfig, headerBuffer);
					return 1;
				}
				break;

			case 'u':
				config->numPorts = internal_strtoc(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 't':
				strncpy(inputTime, optarg, 255);
				break;

			case 's':
				seconds = strtof(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'S':
				splitEvery = internal_strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'p':
				config->processingMode = internal_strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'b':
				sscanf(optarg, "%hd,%hd", &(config->beamletLimits[0]), &(config->beamletLimits[1]));
				break;

			case 'r':
				config->replayDroppedPackets = 1;
				break;

			case 'c':
				config->calibrateData = APPLY_CALIBRATION;
				break;

			case 'z':
				clock200MHz = 0;
				break;

			case 'q':
				silent = 1;
				break;

			case 'f':
				outConfig->progressWithExisting = 1;
				break;

			case 'v':
			VERBOSE(config->verbose = 1;);
				break;

			case 'V':
			VERBOSE(config->verbose = 2;);
				break;

			case 'T':
				config->ompThreads = internal_strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;



				// Silence GCC warnings, fall-through is the desired behaviour
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic push
				// Handle edge/error cases
			case '?':
				if ((optopt == 'i') || (optopt == 'o') || (optopt == 'm') || (optopt == 'u') || (optopt == 't') ||
					(optopt == 's') || (optopt == 'e') || (optopt == 'p') || (optopt == 'a') || (optopt == 'c') ||
					(optopt == 'd')) {
					fprintf(stderr, "Option '%c' requires an argument.\n", optopt);
				} else {
					fprintf(stderr, "Option '%c' is unknown or encountered an error.\n", optopt);
				}

			case 'h':
			default:
#pragma GCC diagnostic pop
				helpMessages();
				CLICleanup(config, outConfig, headerBuffer);
				return 1;

		}
	}

	if (flagged) {
		CLICleanup(config, outConfig, headerBuffer);
		return 1;
	}

	if (!input) {
		fprintf(stderr, "ERROR: No inputs provided, exiting.\n");
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer);
		return 1;
	}

	if (!inputProvided) {
		fprintf(stderr, "ERROR: An input was not provided, exiting.\n");
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer);
		return 1;
	}

	if (_lofar_udp_io_read_internal_lib_parse_optarg(config, inputFormat) < 0) {
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer);
		return 1;
	}

	if (!outputProvided) {
		fprintf(stderr, "ERROR: An output was not provided, exiting.\n");
		CLICleanup(config, outConfig, headerBuffer);
		return 1;
	}

	if (config->metadata_config.metadataType == NO_META && strnlen(config->metadata_config.metadataLocation, DEF_STR_LEN)) {
		config->metadata_config.metadataType = lofar_udp_metadata_parse_type_output(outConfig->outputFormat);
	}

	if (config->calibrateData != NO_CALIBRATION && !strnlen(config->metadata_config.metadataLocation, DEF_STR_LEN)) {
		fprintf(stderr, "ERROR: Data calibration was enabled, but metadata was not provided. Exiting.\n");
		CLICleanup(config, outConfig, headerBuffer);
		return 1;
	}

	// Sanity check a few inputs
	if ((config->numPorts <= 0 || config->numPorts > MAX_NUM_PORTS) || // We are processing a sane number of ports
		(config->packetsPerIteration < 2) || // We are processing a sane number of packets
		(config->replayDroppedPackets > 1 || config->replayDroppedPackets < 0) || // Replay key was not malformed
		(config->processingMode > 1000 || config->processingMode < 0) ||
		// Processing mode is sane (may still fail later)
		(seconds < 0) || // Time is sane
		(config->ompThreads < 1)) // Number of threads will allow execution to continue
	{

		fprintf(stderr, "One or more inputs invalid or not fully initialised, exiting.\n");
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer);
		return 1;
	}


	if (silent == 0) {
		printf("LOFAR UDP Data extractor (v%s, lib v%s)\n\n", UPM_CLI_VERSION, UPM_VERSION);
		printf("=========== Given configuration ===========\n");

		printf("Input:\t%s\n", inputFormat);
		for (int i = 0; i < config->numPorts; i++) printf("\t\t%s\n", config->inputLocations[i]);
		printf("Output File: %s\n\n", outConfig->outputFormat);

		printf("Packets/Gulp:\t%ld\t\t\tPorts:\t%d\n\n", config->packetsPerIteration, config->numPorts);
		VERBOSE(printf("Verbose:\t%d\n", config->verbose););
		printf("Proc Mode:\t%03d\t\t\tReader:\t%d\n\n", config->processingMode, config->readerType);
		printf("Beamlet limits:\t%d, %d\n\n", config->beamletLimits[0], config->beamletLimits[1]);
	}

	headerBuffer = calloc(DEF_HDR_LEN, sizeof(char));

	if (strnlen(inputTime, 256)) {
		startingPacket = lofar_udp_time_get_packet_from_isot(inputTime, clock200MHz);
		if (startingPacket == 1) {
			helpMessages();
			CLICleanup(config, outConfig, headerBuffer);
			return 1;
		}
	}

	if (seconds != 0.0) {
		maxPackets = lofar_udp_time_get_packets_from_seconds(seconds, clock200MHz);
	}

	if (silent == 0) { printf("Start Time:\t%s\t200MHz Clock:\t%d\n", inputTime, clock200MHz); }
	if (silent == 0) {
		printf("Initial Packet:\t%ld\t\tFinal Packet:\t%ld\n", startingPacket, startingPacket + maxPackets);
	}

	if (silent == 0) { printf("============ End configuration ============\n\n"); }


	// If the largest requested data block is less than the packetsPerIteration input, lower the figure so we aren't doing unnecessary reads/writes
	if (config->packetsPerIteration > maxPackets) {
		if (silent == 0) {
			printf("Packet/Gulp is greater than the maximum packets requested, reducing from %ld to %ld.\n",
				   config->packetsPerIteration, maxPackets);
		}
		config->packetsPerIteration = maxPackets;
	}

	if (silent == 0) { printf("Starting data read/reform operations...\n"); }

	// Start our timers
	CLICK(tick);
	CLICK(tick0);

	// Generate the lofar_udp_reader, this also performs I/O to seeks to the required packet and gulps the first input
	config->startingPacket = startingPacket;
	config->packetsReadMax = maxPackets;
	lofar_udp_reader *reader = lofar_udp_reader_setup(config);

	// Returns null on error, check
	if (reader == NULL) {
		fprintf(stderr, "Failed to generate reader. Exiting.\n");
		CLICleanup(config, outConfig, headerBuffer);
		return 1;
	}

	// Sanity check that we were passed the correct clock bit
	if (((lofar_source_bytes *) &(reader->meta->inputData[0][1]))->clockBit != (uint32_t) clock200MHz) {
		fprintf(stderr,
				"ERROR: The clock bit of the first packet does not match the clock state given when starting the CLI. Add or remove -c from your command. Exiting.\n");
		CLICleanup(config, outConfig, headerBuffer);
		return 1;
	}



	if (silent == 0) {
		lofar_udp_time_get_current_isot(reader, stringBuff, sizeof(stringBuff) / sizeof(stringBuff[0]));
		printf("\n\n=========== Reader  Information ===========\n");
		printf("Total Beamlets:\t%d/%d\t\t\t\t\tFirst Packet:\t%ld\n", reader->meta->totalProcBeamlets,
			   reader->meta->totalRawBeamlets, reader->meta->lastPacket);
		printf("Start time:\t%s\t\tMJD Time:\t%lf\n", stringBuff,
			   lofar_udp_time_get_packet_time_mjd(reader->meta->inputData[0]));
		for (int port = 0; port < reader->meta->numPorts; port++) {
			printf("------------------ Port %d -----------------\n", port);
			printf("Port Beamlets:\t%d/%d\t\tPort Bitmode:\t%d\t\tInput Pkt Len:\t%d\n",
				   reader->meta->upperBeamlets[port] - reader->meta->baseBeamlets[port],
				   reader->meta->portRawBeamlets[port], reader->meta->inputBitMode,
				   reader->meta->portPacketLength[port]);
		}
		for (int out = 0; out < reader->meta->numOutputs; out++)
			printf("Output Pkt Len (%d):\t%d\t\t", out, reader->meta->packetOutputLength[out]);
		printf("\n");
		printf("============= End Information =============\n\n");
	}


	// Get the starting packet for output file names, fix the packets per iteration if we dropped packets on the last iter
	startingPacket = reader->meta->leadingPacket;
	if ((returnVal = _lofar_udp_io_write_internal_lib_setup_helper(outConfig, reader, 0)) < 0) {
		fprintf(stderr, "ERROR: Failed to open an output file (%ld, errno %d: %s), exiting.\n", returnVal, errno, strerror(errno));
		CLICleanup(config, outConfig, headerBuffer);
		return 1;
	}

	VERBOSE(if (config->verbose) { printf("Beginning data extraction loop for event %d\n", eventLoop); });
	// While we receive new data for the current event,
	localLoops = 0;
	while ((returnVal = lofar_udp_reader_step_timed(reader, timing)) < 1) {

		if (returnVal < -1) {
			returnValMeta = returnVal;
		}

		CLICK(tock0);
		if (localLoops == 0) {
			timing[0] = TICKTOCK(tick0, tock0) -
						timing[1];
		} // _file_reader_step or _reader_reuse does first I/O operation; approximate the time here
		if (silent == 0) {
			printf("Read complete for operation %ld after %f seconds (I/O: %lf, MemOps: %lf), return value: %ld\n",
				   loops, TICKTOCK(tick0, tock0), timing[0], timing[1], returnVal);
		}

		totalReadTime += timing[0];
		totalOpsTime += timing[1];

		// Write out the desired amount of packets; cap if needed.
		packetsToWrite = reader->meta->packetsPerIteration;
		if (splitEvery == LONG_MAX && maxPackets < packetsToWrite) {
			packetsToWrite = maxPackets;
		}

		for (int8_t out = 0; out < reader->meta->numOutputs; out++) {
			CLICK(tick1);
			if ((returnVal = lofar_udp_metadata_write_file(reader, outConfig, out, reader->metadata, headerBuffer, 4096 * 8, localLoops == 0)) < 0) {
				fprintf(stderr, "ERROR: Failed to write header to output (%ld, errno %d: %s), breaking.\n", returnVal, errno, strerror(errno));
				returnValMeta = (returnValMeta < 0 && returnValMeta > -4) ? returnValMeta : -4;
				break;
			}
			CLICK(tock1);
			timing[2] += TICKTOCK(tick1, tock1);

			CLICK(tick0);
			VERBOSE(printf("Writing %ld bytes (%ld packets) to disk for output %d...\n",
			               packetsToWrite * reader->meta->packetOutputLength[out], packetsToWrite, out));
			size_t outputLength = packetsToWrite * reader->meta->packetOutputLength[out];
			size_t outputWritten;
			if ((outputWritten = lofar_udp_io_write(outConfig, out, reader->meta->outputData[out],
								   outputLength)) != outputLength) {
				fprintf(stderr, "ERROR: Failed to write data to output (%ld bytes/%ld bytes writen, errno %d: %s)), breaking.\n", outputWritten, outputLength,  errno, strerror(errno));
				returnValMeta = (returnValMeta < 0 && returnValMeta > -5) ? returnValMeta : -5;
				break;
			}
			CLICK(tock0);
			timing[3] += TICKTOCK(tick0, tock0);

		}

		if (splitEvery != LONG_MAX && returnValMeta > -2) {
			if (!((localLoops + 1) % splitEvery)) {

				// Close existing files
				lofar_udp_io_write_cleanup(outConfig, 0);

				// Open new files
				if ((returnVal = _lofar_udp_io_write_internal_lib_setup_helper(outConfig, reader, (int32_t) (localLoops / splitEvery))) < 0) {
					fprintf(stderr, "ERROR: Failed to open new file are breakpoint reached (%ld, errno %d: %s), breaking.\n", returnVal, errno, strerror(errno));
					returnValMeta = (returnValMeta < 0 && returnValMeta > -6) ? returnValMeta : -6;
					break;
				}
			}
		}

		totalMetadataTime += timing[2];
		totalWriteTime += timing[3];

		packetsWritten += packetsToWrite;
		packetsProcessed += reader->meta->packetsPerIteration;

		if (silent == 0) {
			printf("Metadata processing for operation %ld after %f seconds.\n", loops, timing[2]);
			printf("Disk writes completed for operation %ld after %f seconds.\n", loops, timing[3]);

			for (int idx = 0; idx < TIMEARRLEN; idx++) {
				timing[idx] = 0.;
			}

			if (returnVal == -1) {
				for (int port = 0; port < reader->meta->numPorts; port++)
					if (reader->meta->portLastDroppedPackets[port] != 0) {
						printf("During this iteration there were %ld dropped packets on port %d.\n",
							   reader->meta->portLastDroppedPackets[port], port);
					}
			}
			printf("\n");
		}

		loops++;
		localLoops++;

#ifdef __SLOWDOWN
			sleep(1);
#endif
		CLICK(tick0);

		// returnVal below -1 indicates we will not be given data on the next iteration, so gracefully exit with the known reason
		if (returnValMeta < -1) {
			break;
		}

		lofar_udp_io_write_cleanup(outConfig, 1);

		// returnVal below -1 indicates we will not be given data on the next iteration, so gracefully exit with the known reason
		if (returnValMeta < -1) {
			printf("We've hit a termination return value (%ld, %s), exiting.\n", returnValMeta,
			       exitReasons[abs((int32_t) returnValMeta)]);
			break;
		}

	}

	CLICK(tock);

	int droppedPackets = 0;
	long totalPacketLength = 0, totalOutLength = 0;

	// Print out a summary of the operations performed, this does not contain data read for seek operations
	if (silent == 0) {
		for (int8_t port = 0; port < reader->meta->numPorts; port++)
			totalPacketLength += reader->meta->portPacketLength[port];
		for (int8_t out = 0; out < reader->meta->numOutputs; out++)
			totalOutLength += reader->meta->packetOutputLength[out];
		for (int8_t port = 0; port < reader->meta->numPorts; port++)
			droppedPackets += reader->meta->portTotalDroppedPackets[port];

		printf("Reader loop exited (%ld); overall process took %f seconds.\n", returnVal, TICKTOCK(tick, tock));
		printf("We processed %ld packets, representing %.03lf seconds of data", packetsProcessed,
			   (float) (reader->meta->numPorts * packetsProcessed * UDPNTIMESLICE) * 5.12e-6f);
		if (reader->meta->numPorts > 1) {
			printf(" (%.03lf per port)\n", (float) (packetsProcessed * UDPNTIMESLICE) * 5.12e-6f);
		} else { printf(".\n"); }
		printf("Total Read Time:\t%3.02lf s\t\t\tTotal CPU Ops Time:\t%3.02lf s\nTotal Write Time:\t%3.02lf s\t\t\tTotal MetaD Time:\t%3.02lf s\n", totalReadTime,
			   totalOpsTime, totalWriteTime, totalMetadataTime);
		printf("Total Data Read:\t%3.03lf GB\t\tTotal Data Written:\t%3.03lf GB\n",
			   (double) (packetsProcessed * totalPacketLength) / 1e+9,
			   (double) (packetsWritten * totalOutLength) / 1e+9);
		printf("Effective Read Speed:\t%3.01lf MB/s\t\tEffective Write Speed:\t%3.01lf MB/s\n", (double) (packetsProcessed * totalPacketLength) / 1e+6 / totalReadTime,
		       (double) (packetsWritten * totalOutLength) / 1e+6 / totalWriteTime);
		printf("Approximate Throughput:\t%3.01lf GB/s\n", (double) (reader->meta->numPorts * packetsProcessed * (totalPacketLength + totalOutLength)) / 1e+9 / totalOpsTime);
		printf("A total of %d packets were missed during the observation.\n", droppedPackets);
		printf("\n\nData processing finished. Cleaning up file and memory objects...\n");
	}



	// Clean-up the reader object, also closes the input files for us
	lofar_udp_reader_cleanup(reader);
	if (silent == 0) { printf("Reader cleanup performed successfully.\n"); }

	// Free our malloc'd objects
	CLICleanup(config, outConfig, headerBuffer);

	if (silent == 0) { printf("CLI memory cleaned up successfully. Exiting.\n"); }
	return 0;
}


/**
 * Copyright (C) 2023 David McKenna
 * This file is part of udpPacketManager <https://github.com/David-McKenna/udpPacketManager>.
 *
 * udpPacketManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * udpPacketManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with udpPacketManager.  If not, see <http://www.gnu.org/licenses/>.
 **/
