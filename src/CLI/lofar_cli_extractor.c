#include "lofar_cli_meta.h"


void helpMessages() {
	printf("LOFAR UDP Data extractor (CLI v%s, lib v%s)\n\n", UPM_CLI_VERSION, UPM_VERSION);
	printf("Usage: ./lofar_cli_extractor <flags>");

	printf("\n\n");

	printf("-i: <format>	[OUTDATED] Input file name format (default: './%%d')\n");
	printf("-o: <format>	[OUTDATED] Output file name format (provide %%d, %%s and %%ld to fill in output ID, date/time string and the starting packet number) (default: './output%%d_%%s_%%ld')\n");
	printf("-m: <numPack>	Number of packets to process in each read request (default: 65536)\n");
	printf("-M: <str>		Metadata format (SIGPROC, DADA, GUPPI, default: NONE)n");
	printf("-I: <str>		Input metadata file (default: '')\n");
	printf("-u: <numPort>	Number of ports to combine (default: 4)\n");
	printf("-n: <baseNum>	Base value to iterate when choosing ports (default: 0)\n");
	printf("-b: <lo>,<hi>	Beamlets to extract from the input dataset. Lo is inclusive, hi is exclusive ( eg. 0,300 will return 300 beamlets, 0:299). (default: 0,0 === all)\n");
	printf("-t: <timeStr>	String of the time of the first requested packet, format YYYY-MM-DDTHH:mm:ss (default: '')\n");
	printf("-s: <numSec>	Maximum number of seconds of raw data to extract/process (default: all)\n");
	printf("-e: <fileName>	Specify a file of events to extract; newline separated start time and durations in seconds. Events must not overlap.\n");
	printf("-p: <mode>		Processing mode, options listed below (default: 0)\n");
	printf("-r:		Replay the previous packet when a dropped packet is detected (default: pad with 0 values)\n");
	printf("-c:		Calibrate the data with the given strategy (default: disabled, eg 'HBA,12:499'). Will not run without -d\n");
	printf("-d:		Calibrate the data with the given pointing (default: disabled, eg '0.1,0.2,J2000'). Will not run without -c\n");
	printf("-z:		Change to the alternative clock used for modes 4/6 (160MHz clock) (default: False)\n");
	printf("-q:		Enable silent mode for the CLI, don't print any information outside of library error messes (default: False)\n");
	printf("-a: <args>		Call mockHeader with the specific flags to prefix output files with a header (default: False)\n");
	printf("-f:		Append files if they already exist (default: False, exit if exists)\n");
	printf("-T: <threads>	OpenMP Threads to use during processing (8+ highly recommended, default: %d)\n", OMP_THREADS);

	VERBOSE(printf("-v:		Enable verbose output (default: False)\n");
			printf("-V:		Enable highly verbose output (default: False)\n"));

	processingModes();

}

void CLICleanup(int eventCount, char **dateStr, long *startingPackets, long *multiMaxPackets, float *eventSeconds,
				lofar_udp_config *config, lofar_udp_io_write_config *outConfig, char *headerBuffer) {

	FREE_NOT_NULL(startingPackets);
	FREE_NOT_NULL(multiMaxPackets);
	FREE_NOT_NULL(eventSeconds);
	FREE_NOT_NULL(outConfig);

	if (dateStr != NULL) {
		for (int i = 0; i < eventCount; i++) {
			FREE_NOT_NULL(dateStr[i]);
		}
		FREE_NOT_NULL(dateStr);
	}


	if (config != NULL) {
		FREE_NOT_NULL(config->calibrationConfiguration);
		FREE_NOT_NULL(config);
	}

	FREE_NOT_NULL(headerBuffer);
}


int main(int argc, char *argv[]) {

	// Set up input local variables
	int inputOpt, input = 0;
	float seconds = 0.0f;
	double sampleTime = 0.0;
	char inputTime[256] = "", eventsFile[DEF_STR_LEN] = "", stringBuff[128], mockHdrArg[2048] = "", mockHdrCmd[8192] = "", inputFormat[DEF_STR_LEN] = "", metadataFile[DEF_STR_LEN] = "";
	int silent = 0, returnCounter = 0, eventCount = 0, callMockHdr = 0, calPoint = 0, calStrat = 0, inputProvided = 0, outputProvided = 0, metadata = 0;
	long maxPackets = -1, startingPacket = -1;
	int clock200MHz = 1;
	FILE *eventsFilePtr;

	lofar_udp_config *config = calloc(1, sizeof(lofar_udp_config));
	(*config) = lofar_udp_config_default;
	lofar_udp_calibration *cal = calloc(1, sizeof(struct lofar_udp_calibration));
	(*cal) = lofar_udp_calibration_default;
	config->calibrationConfiguration = cal;

	lofar_udp_io_write_config *outConfig = calloc(1, sizeof(lofar_udp_io_write_config));
	(*outConfig) = lofar_udp_io_write_config_default;

	char *headerBuffer = NULL;

	if (config == NULL || outConfig == NULL || cal == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate memory for configuration structs (something has gone very wrong...), exiting.\n");
		FREE_NOT_NULL(config); FREE_NOT_NULL(outConfig); FREE_NOT_NULL(cal);
		return 1;
	}

	// Set up reader loop variables
	int loops = 0, localLoops = 0, returnVal;
	long packetsProcessed = 0, packetsWritten = 0, eventPacketsLost[MAX_NUM_PORTS], packetsToWrite;
	double timing[3] = { 0. }, totalReadTime = 0, totalOpsTime = 0, totalWriteTime = 0;
	struct timespec tick, tick0, tick1, tock, tock0, tock1;

	// Malloc'd variables: need to be free'd later.
	long *startingPackets = NULL, *multiMaxPackets = NULL;
	float *eventSeconds = NULL;
	char **dateStr = NULL; // Sub elements need to be free'd too.

	// Standard ugly input flags parser
	while ((inputOpt = getopt(argc, argv, "zrqfvVi:o:m:M:I:u:t:s:e:p:a:n:b:c:d:k:T:")) != -1) {
		input = 1;
		switch (inputOpt) {

			case 'i':
				strcpy(inputFormat, optarg);
				inputProvided = 1;
				break;


			case 'o':
				if (lofar_udp_io_write_parse_optarg(outConfig, optarg) < 0) {
					helpMessages();
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
					return 1;
				}
				outputProvided = 1;
				break;

			case 'm':
				config->packetsPerIteration = atol(optarg);
				break;

			case 'M':
				metadata = 1;
				config->metadataType = lofar_udp_metadata_string_to_meta(optarg);
				break;

			case 'I':
				if (strncpy(config->metadataLocation, optarg, DEF_STR_LEN) != config->metadataLocation) {
					fprintf(stderr, "ERROR: Failed to copy metadata file location to config, exiting.\n");
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
					return 1;
				}
				break;

			case 'u':
				config->numPorts = atoi(optarg);
				break;

			case 't':
				strcpy(inputTime, optarg);
				break;

			case 's':
				seconds = (float) atof(optarg);
				break;

			case 'e':
				strcpy(eventsFile, optarg);
				break;

			case 'p':
				config->processingMode = atoi(optarg);
				break;

			case 'a':
				strcpy(mockHdrArg, optarg);
				callMockHdr = 1;
				break;

			case 'b':
				sscanf(optarg, "%d,%d", &(config->beamletLimits[0]), &(config->beamletLimits[1]));
				break;

			case 'r':
				config->replayDroppedPackets = 1;
				break;

			case 'c':
				calPoint = 1;
				strcpy(&(config->calibrationConfiguration->calibrationSubbands[0]), optarg);
				break;

			case 'd':
				calStrat = 1;
				sscanf(optarg, "%f,%f,%128s", &(config->calibrationConfiguration->calibrationPointing[0]),
					   &(config->calibrationConfiguration->calibrationPointing[1]),
					   &(config->calibrationConfiguration->calibrationPointingBasis[0]));
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
				config->ompThreads = atoi(optarg);
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
				CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
				return 1;

		}
	}

	if (!input) {
		fprintf(stderr, "ERROR: No inputs provided, exiting.\n");
		helpMessages();
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
		return 1;
	}

	if (!inputProvided) {
		fprintf(stderr, "ERROR: An input was not provided, exiting.\n");
		helpMessages();
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
		return 1;
	}

	if (lofar_udp_io_read_parse_optarg(config, inputFormat) < 0) {
		helpMessages();
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
		return 1;
	}

	// DADA outputs should not be fragments, or require writing to disk (mockHeader deleted files and rewrites...)
	if (config->readerType == DADA_ACTIVE && strcmp(eventsFile, "") != 0) {
		fprintf(stderr, "ERROR: DADA output does not support events parsing, exiting.\n");
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
		return 1;
	}
	if (outConfig->readerType == DADA_ACTIVE && callMockHdr) {
		fprintf(stderr, "ERROR: DADA output does not support attaching a sigproc header, exiting.\n");
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
		return 1;
	}

	if (!outputProvided) {
		fprintf(stderr, "ERROR: An output was not provided, exiting.\n");
		helpMessages();
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
		return 1;
	}

	if (calPoint || calStrat) {
		if (calPoint && calStrat) {
			config->calibrateData = 1;
		} else {
			fprintf(stderr, "ERROR: Calibration not fully initialised. You only provided the ");
			if (calPoint) {
				fprintf(stderr, "pointing. ");
			} else {
				fprintf(stderr, "strategy. ");
			}
			fprintf(stderr, "Exiting.\n");
			CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
			return 1;
		}
	}

	// Sanity check a few inputs
	if ((config->numPorts <= 0 || config->numPorts > MAX_NUM_PORTS) || // We are processing a sane number of ports
		(config->packetsPerIteration < 2) || // We are processing a sane number of packets
		(config->replayDroppedPackets > 1 || config->replayDroppedPackets < 0) || // Replay key was not malformed
		(config->processingMode > 1000 || config->processingMode < 0) ||
		// Processing mode is sane (may still fail later)
		(seconds < 0) || // Time is sane
		(config->ompThreads < 1)) // Number of threads will allow excution to continue
	{

		fprintf(stderr, "One or more inputs invalid or not fully initialised, exiting.\n");
		helpMessages();
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
		return 1;
	}

	// Make sure mockHeader is on the path if we want to use it.
	if (callMockHdr) {
		if (config->processingMode < 99 || config->processingMode > 199) {
			fprintf(stderr,
					"WARNING: Processing mode %d may not confirm to the Sigproc spec, but you requested a header. Continuing with caution...\n",
					config->processingMode);
		}

		printf("Checking for mockHeader on system path... ");
		// Add the return code (multiplied by 256 from bash return) to the execution variable, ensure it doesn't change
		callMockHdr += system("which mockHeader > /tmp/udp_reader_mockheader.log 2>&1"); 
		if (callMockHdr != 1) {
			fprintf(stderr, "Error occurred while attempting to find mockHeader, exiting.\n");
			CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
			return 1;
		}

		sampleTime = clock160MHzSampleTime * (1 - clock200MHz) + clock200MHzSampleTime * clock200MHz;
		if (config->processingMode > 100) {
			sampleTime *= 1 << ((config->processingMode % 10));
		}
	}

	if (silent == 0) {
		printf("LOFAR UDP Data extractor (v%s, lib v%s)\n\n", UPM_CLI_VERSION, UPM_VERSION);
		printf("=========== Given configuration ===========\n");

		printf("Input:\t%s\n", inputFormat);
		printf("Output File: %s\n\n", outConfig->outputFormat);

		printf("Packets/Gulp:\t%ld\t\t\tPorts:\t%d\n\n", config->packetsPerIteration, config->numPorts);
		VERBOSE(printf("Verbose:\t%d\n", config->verbose););
		printf("Proc Mode:\t%03d\t\t\tReader:\t%d\n\n", config->processingMode, config->readerType);
		printf("Beamlet limits:\t%d, %d\n\n", config->beamletLimits[0], config->beamletLimits[1]);
	}

	headerBuffer = calloc(4096 * 8, sizeof(char));


	// If given an events file,
	if (strcmp(eventsFile, "") != 0) {

		// Try to read it
		eventsFilePtr = fopen(eventsFile, "r");
		if (eventsFilePtr == NULL) {
			fprintf(stderr, "Unable to open events file at %s, exiting.\n", eventsFile);
			CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
			return 1;
		}

		// The first line should be an int of the amount of events we need to process
		returnCounter = fscanf(eventsFilePtr, "%d", &eventCount);
		if (returnCounter != 1 || eventCount < 1) {
			fprintf(stderr, "Unable to parse events file (got %d as number of events), exiting.\n", eventCount);
			CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
			return 1;
		}

		// Malloc the arrays of the right length
		startingPackets = calloc(eventCount, sizeof(long));
		multiMaxPackets = calloc(eventCount, sizeof(long));
		dateStr = calloc(eventCount, sizeof(char *));
		eventSeconds = calloc(eventCount, sizeof(float));
		for (int i = 0; i < eventCount; i++) dateStr[i] = calloc(128, sizeof(char));

		if (silent == 0) {
			printf("Events File:\t%s\t\tEvent Count:\t%d\t\t\t200MHz Clock:\t%d\n", eventsFile, eventCount,
				   clock200MHz);
		}

		// For each event,
		for (int idx = 0; idx < eventCount; idx++) {
			// Get the time string and length of the event
			returnCounter = fscanf(eventsFilePtr, "%s %f", &stringBuff[0], &seconds);
			strcpy(dateStr[idx], stringBuff);

			if (returnCounter != 2) {
				fprintf(stderr, "Unable to parse line %d of events file, exiting ('%s', %lf).\n", idx + 1, stringBuff,
						seconds);
				CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
				return 1;
			}

			// Determine the packet corresponding to the initial time and the amount of packets needed to observe for  the length of the event
			startingPackets[idx] = lofar_udp_time_get_packet_from_isot(stringBuff, clock200MHz);
			if (startingPackets[idx] == 1) {
				fprintf(stderr, "ERROR: Failed to get starting packet for event %d, exiting.\n", idx);
				helpMessages();
				CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
				return 1;
			}

			eventSeconds[idx] = seconds;
			multiMaxPackets[idx] = lofar_udp_time_get_packets_from_seconds(seconds, clock200MHz);
			// If packetsPerIteration is too high, we can reduce it later by tracking the largest requested input size
			if (multiMaxPackets[idx] > maxPackets) { maxPackets = multiMaxPackets[idx]; }

			if (silent == 0) {
				printf("Event:\t%d\tSeconds:\t%.02lf\tInitial Packet:\t%ld\t\tFinal Packet:\t%ld\n", idx, seconds,
					   startingPackets[idx], startingPackets[idx] + multiMaxPackets[idx]);
			}

			// Safety check: all events are correctly ordered in increasing time, and do not overlap
			// Compressed observations cannot be fseek'd, so this is a required design choice
			if (idx > 0) {
				if (startingPackets[idx] < startingPackets[idx - 1]) {
					fprintf(stderr,
							"Events %d and %d are out of order, please only use increasing event times, exiting.\n",
							idx, idx - 1);
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
					return 1;
				}

				if (startingPackets[idx] < startingPackets[idx - 1] + multiMaxPackets[idx - 1]) {
					fprintf(stderr,
							"Events %d and %d overlap, please combine them or ensure there is some buffer time between them, exiting.",
							idx, idx - 1);
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
					return 1;
				}
			}

		}

	} else {
		// Repeat the step above for a single event, but read the defaults / -t and -s flags as the inputs
		eventCount = 1;

		startingPackets = calloc(1, sizeof(long));
		dateStr = calloc(1, sizeof(char *));
		dateStr[0] = calloc(1, sizeof("2020-20-20T-20:20:20"));
		if (strcmp(inputTime, "") != 0) {
			startingPacket = lofar_udp_time_get_packet_from_isot(inputTime, clock200MHz);
			if (startingPacket == 1) {
				helpMessages();
				CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
				return 1;
			}
		}
		startingPackets[0] = startingPacket;
		strcpy(dateStr[0], inputTime);

		eventSeconds = calloc(1, sizeof(float));
		eventSeconds[0] = seconds;
		multiMaxPackets = calloc(1, sizeof(long));
		if (seconds != 0.0) { multiMaxPackets[0] = lofar_udp_time_get_packets_from_seconds(seconds, clock200MHz); }
		else { multiMaxPackets[0] = LONG_MAX; }

		maxPackets = multiMaxPackets[0];
		if (silent == 0) { printf("Start Time:\t%s\t200MHz Clock:\t%d\n", inputTime, clock200MHz); }
		if (silent == 0) {
			printf("Initial Packet:\t%ld\t\tFinal Packet:\t%ld\n", startingPackets[0], startingPackets[0] + maxPackets);
		}
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

	// Generate the lofar_udp_reader, this also does I/O to seeks to the required packet and gulps the first input
	config->startingPacket = startingPackets[0];
	config->packetsReadMax = multiMaxPackets[0];
	lofar_udp_reader *reader = lofar_udp_reader_setup(config);

	// Returns null on error, check
	if (reader == NULL) {
		fprintf(stderr, "Failed to generate reader. Exiting.\n");
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
		return 1;
	}

	// Sanity check that we were passed the correct clock bit
	if (((lofar_source_bytes *) &(reader->meta->inputData[0][1]))->clockBit != (unsigned int) clock200MHz) {
		fprintf(stderr,
				"ERROR: The clock bit of the first packet does not match the clock state given when starting the CLI. Add or remove -c from your command. Exiting.\n");
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
		return 1;
	}


	if (silent == 0) {
		lofar_udp_time_get_current_isot(reader, stringBuff);
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

	// Scan over the registered events
	for (int eventLoop = 0; eventLoop < eventCount; eventLoop++) {
		localLoops = 0;

		// Initialise / empty the packets lost array
		for (int port = 0; port < reader->meta->numPorts; port++) eventPacketsLost[port] = 0;

		// If we are not on the first event, set-up the reader for the current event
		if (loops != 0) {
			if ((returnVal = lofar_udp_file_reader_reuse(reader, startingPackets[eventLoop],
														 multiMaxPackets[eventLoop])) > 0) {
				fprintf(stderr, "Error re-initialising reader for event %d (error %d), exiting.\n", eventLoop,
						returnVal);
				CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
				return 1;
			}
		}

		// Output information about the current/last event if we're performing more than one event
		if (eventCount > 1) {
			if (silent == 0) {
				if (eventLoop > 0) {
					printf("Completed work for event %d, packets lost for each port during this event was",
						   eventLoop - 1);
					for (int port = 0; port < reader->meta->numPorts; port++) printf(" %ld", eventPacketsLost[port]);
					printf(".\n\n\n");
				}
				printf("Beginning work on event %d at %s: packets %ld to %ld...\n", eventLoop, dateStr[eventLoop],
					   startingPackets[eventLoop], startingPackets[eventLoop] + multiMaxPackets[eventLoop]);
				lofar_udp_time_get_current_isot(reader, stringBuff);
				printf("============ Event %d Information ===========\n", eventLoop);
				printf("Target Time:\t%s\t\tActual Time:\t%s\n", dateStr[eventLoop], stringBuff);
				printf("Target Packet:\t%ld\tActual Packet:\t%ld\n", startingPackets[eventLoop],
					   reader->meta->lastPacket + 1);
				printf("Event Length:\t%fs\t\tPacket Count:\t%ld\n", eventSeconds[eventLoop],
					   multiMaxPackets[eventLoop]);
				printf("MJD Time:\t%lf\n", lofar_udp_time_get_packet_time_mjd(reader->meta->inputData[0]));
				printf("============= End Information ==============\n");
			}
		}


		// Get the starting packet for output file names
		startingPacket = reader->meta->leadingPacket;

		if (lofar_udp_io_write_setup_helper(outConfig, reader->meta, eventLoop) < 0) {
			CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);
			return 1;
		}


		VERBOSE(if (config->verbose) { printf("Begining data extraction loop for event %d\n", eventLoop); });
		// While we receive new data for the current event,
		while ((returnVal = lofar_udp_reader_step_timed(reader, timing)) < 1) {

			CLICK(tock0);
			if (localLoops == 0) {
				timing[0] = TICKTOCK(tick0, tock0) -
							timing[1];
			} // _file_reader_step or _reader_reuse does first I/O operation; approximate the time here
			if (silent == 0) {
				printf("Read complete for operation %d after %f seconds (I/O: %lf, MemOps: %lf), return value: %d\n",
					   loops, TICKTOCK(tick0, tock0), timing[0], timing[1], returnVal);
			}

			totalReadTime += timing[0];
			totalOpsTime += timing[1];

			// Write out the desired amount of packets; cap if needed.
			packetsToWrite = reader->meta->packetsPerIteration;
			if (multiMaxPackets[eventLoop] < packetsToWrite) { packetsToWrite = multiMaxPackets[eventLoop]; }





			for (int out = 0; out < reader->meta->numOutputs; out++) {
				CLICK(tick1);
				long hdrLen;
				if (lofar_udp_metadata_write_file(reader, outConfig, out, reader->metadata, headerBuffer, 4096 * 8, localLoops == 0) < 0) {
					returnVal = -4;
					break;
				}
				CLICK(tock1);
				timing[2] += TICKTOCK(tick1, tock1);
				printf("Metadata for output %d took %lf (%lf) seconds to parse.\n", timing[2] - TICKTOCK(tick1, tock1), timing[2]);

				CLICK(tick0);
				VERBOSE(printf("Writing %ld bytes (%ld packets) to disk for output %d...\n",
				               packetsToWrite * reader->meta->packetOutputLength[out], packetsToWrite, out));
				if (lofar_udp_io_write(outConfig, out, reader->meta->outputData[out],
									   packetsToWrite * reader->meta->packetOutputLength[out]) < 0) {
					returnVal = -5;
					break;
				}
				CLICK(tock1);
				totalWriteTime += TICKTOCK(tick0, tock0);

			}

			packetsWritten += packetsToWrite;
			packetsProcessed += reader->meta->packetsPerIteration;

			if (silent == 0) {
				timing[0] = 0.;
				timing[1] = 0.;
				printf("Disk writes completed for operation %d after %f seconds.\n", loops, TICKTOCK(tick0, tock0));
				if (returnVal < 0) {
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

			// returnVal below 0 indicates we will not be given data on the next iteration, so gracefully exit with the known reason
			if (returnVal < -1) {
				printf("We've hit a termination return value (%d, %s), exiting.\n", returnVal,
					   exitReasons[abs(returnVal)]);
				break;
			}

#ifdef __SLOWDOWN
				sleep(1);
#endif
			CLICK(tick0);
		}

	}

	CLICK(tock);

	for (int outp = 0; outp < reader->meta->numOutputs; outp++) {
		lofar_udp_io_write_cleanup(outConfig, outp, 1);
	}

	int droppedPackets = 0;
	long totalPacketLength = 0, totalOutLength = 0;

	// Print out a summary of the operations performed, this does not contain data read for seek operations
	if (silent == 0) {
		for (int port = 0; port < reader->meta->numPorts; port++)
			totalPacketLength += reader->meta->portPacketLength[port];
		for (int out = 0; out < reader->meta->numOutputs; out++)
			totalOutLength += reader->meta->packetOutputLength[out];
		for (int port = 0; port < reader->meta->numPorts; port++)
			droppedPackets += reader->meta->portTotalDroppedPackets[port];

		printf("Reader loop exited (%d); overall process took %f seconds.\n", returnVal, TICKTOCK(tick, tock));
		printf("We processed %ld packets, representing %.03lf seconds of data", packetsProcessed,
			   (float) (reader->meta->numPorts * packetsProcessed * UDPNTIMESLICE) * 5.12e-6f);
		if (reader->meta->numPorts > 1) {
			printf(" (%.03lf per port)\n", (float) (packetsProcessed * UDPNTIMESLICE) * 5.12e-6f);
		} else { printf(".\n"); }
		printf("Total Read Time:\t%3.02lf\t\tTotal CPU Ops Time:\t%3.02lf\tTotal Write Time:\t%3.02lf\n", totalReadTime,
			   totalOpsTime, totalWriteTime);
		printf("Total Data Read:\t%3.03lfGB\t\t\t\tTotal Data Written:\t%3.03lfGB\n",
			   (double) (packetsProcessed * totalPacketLength) / 1e+9,
			   (double) (packetsWritten * totalOutLength) / 1e+9);
		printf("A total of %d packets were missed during the observation.\n", droppedPackets);
		printf("\n\nData processing finished. Cleaning up file and memory objects...\n");
	}



	// Clean-up the reader object, also closes the input files for us
	lofar_udp_reader_cleanup(reader);
	if (silent == 0) { printf("Reader cleanup performed successfully.\n"); }

	// Free our malloc'd objects
	CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, headerBuffer);

	if (silent == 0) { printf("CLI memory cleaned up successfully. Exiting.\n"); }
	return 0;
}
