#include "lofar_cli_meta.h"
#include "ascii_hdr_manager.h"

// Unique prototype to this CLI
void getStartTimeStringDAQ(lofar_udp_reader *reader, char stringBuff[]);

void helpMessages() {
	printf("LOFAR UDP Data extractor (GUPPI v%s, lib v%s)\n\n", UPM_CLI_VERSION, UPM_VERSION);
	printf("Usage: ./lofar_cli_extractor <flags>");

	printf("\n\n");

	printf("-i: <format>	Input file name format (default: './%%d')\n");
#ifndef NODADA
	printf("-k: <key>		Input PSRDADA ringbuffer keys, a base value and an offset (>2 to allow for headers) (default: '', example ('16130,10'))\n");
#endif
	printf("-o: <format>	Output file name format (provide %%d to fill in the output file number) (default: './output%%d')\n");
	printf("-m: <numPack>	Number of packets to process in each read request (default: 65536)\n");
	printf("-u: <numPort>	Number of ports to combine (default: 4)\n");
	printf("-n: <baseNum>	Base value to iterate when choosing ports (default: 0)\n");
	printf("-b: <lo>,<hi>	Beamlets to extract from the input dataset. Lo is inclusive, hi is exclusive ( eg. 0,300 will return 300 beamlets, 0:299). (default: 0,0 === all)\n");
	printf("-t: <timeStr>	String of the time of the first requested packet, format YYYY-MM-DDTHH:mm:ss (default: '')\n");
	printf("-s: <numSec>	Maximum number of seconds to process (default: all)\n");
	printf("-e: <iters>		Split the file every N iterations (default: inf)\n");
	printf("-r:		Replay the previous packet when a dropped packet is detected (default: 0 pad)\n");
	printf("-c:		Change to the alternative clock used for modes 4/6 (160MHz clock) (default: False)\n");
	printf("-q:		Enable silent mode for the CLI, don't print any information outside of library error messes (default: False)\n");
	printf("-a: <file>		File to open with parameters for the ASCII headers\n");
	printf("-f:		Append files if they already exist (default: False, exit if exists)\n");
	printf("-T: <threads>	OpenMP Threads to use during processing (8+ highly recommended, default: %d)\n", OMP_THREADS);
	
	VERBOSE(printf("-v:		Enable verbose output (default: False)\n");
			printf("-V:		Enable highly verbose output (default: False)\n"));

}


int main(int argc, char  *argv[]) {

	// Set up input local variables
	int inputOpt, outputFilesCount, input = 0;
	float seconds = 0.0;
	double sampleTime = 0.0;
	char inputFormat[256] = "./%d", outputFormat[256] = "./output_%d", inputTime[256] = "", stringBuff[128], hdrFile[2048] = "", timeStr[28] = "";
	int silent = 0, appendMode = 0, itersPerFile = INT_MAX, basePort = 0, dadaInput = 0, dadaOffset = 0;
	unsigned int clock200MHz = 1;
	
	lofar_udp_config config = lofar_udp_config_default;
	ascii_hdr header = ascii_hdr_default;

	// Set up reader loop variables
	int loops = 0, localLoops = 0, returnVal;
	long packetsProcessed = 0, packetsWritten = 0, packetsToWrite;
	double timing[2] = {0., 0.}, totalReadTime = 0, totalOpsTime = 0, totalWriteTime = 0;
	struct timespec tick, tick0, tock, tock0;

	// I/O variables
	FILE *inputFiles[MAX_NUM_PORTS] = { NULL };
	FILE *outputFiles[MAX_OUTPUT_DIMS] = { NULL };
	
	// Malloc'd variables: need to be free'd later.
	char **dateStr; // Sub elements need to be free'd too.

	// Standard ugly input flags parser
	while((inputOpt = getopt(argc, argv, "rcqfvVi:o:m:u:t:s:e:a:n:b:k:T:")) != -1) {
		input = 1;
		switch(inputOpt) {
			
			case 'i':
				strcpy(inputFormat, optarg);
				break;

			case 'k':
#ifndef NODADA
				if (dadaInput == -1) {
					fprintf(stderr, "ERROR: Specified input ringbuffer after defining an input file, exiting.\n");
					return 1;
				}
				dadaInput = sscanf(optarg, "%d,%d", &(config.dadaKeys[0]), &dadaOffset);
				if (dadaInput < 1) {
					fprintf(stderr, "ERROR: Failed to parse PSRDADA keys input (%d values parsed), exiting.\n", dadaInput);
					return 1;
				} 
				
				dadaInput = 1;
#else
				fprintf(stderr, "ERROR: PSRDADA key specified when PSRDADA was disabled at compile time, exiting.\n");
				return 1;
#endif
				break;

			case 'o':
				strcpy(outputFormat, optarg);
				break;

			case 'm':
				config.packetsPerIteration = atol(optarg);
				break;

			case 'u':
				config.numPorts = atoi(optarg);
				break;

			case 'n':
				basePort = atoi(optarg);
				break;

			case 't':
				strcpy(inputTime, optarg);
				break;

			case 's':
				seconds = atof(optarg);
				break;

			case 'e':
				itersPerFile = atoi(optarg);
				break;

			case 'a':
				strcpy(hdrFile, optarg);
				break;

			case 'b':
				sscanf(optarg, "%d,%d", &(config.beamletLimits[0]), &(config.beamletLimits[1]));
				break;

			case 'r':
				config.replayDroppedPackets = 1;
				break;

			case 'c':
				clock200MHz = 0;
				break;

			case 'q':
				silent = 1;
				break;

			case 'f':
				appendMode = 1;
				break;

			case 'v': 
				if (!config.verbose)
					VERBOSE(config.verbose = 1;);
				break;
			case 'V': 
				VERBOSE(config.verbose = 2;);
				break;

			case 'T':
				config.ompThreads = atoi(optarg);
				break;



			// Silence GCC warnings, fall-through is the desired behaviour
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
			#pragma GCC diagnostic push

			// Handle edge/error cases
			case '?':
				if ((optopt == 'i') || (optopt == 'o') || (optopt == 'm') || (optopt == 'u') || (optopt == 't') || (optopt == 's') || (optopt == 'e') || (optopt == 'a')) {
					fprintf(stderr, "Option '%c' requires an argument.\n", optopt);
				} else {
					fprintf(stderr, "Option '%c' is unknown or encountered an error.\n", optopt);
				}

			case 'h':
			default:

			#pragma GCC diagnostic pop

				helpMessages();
				return 1;

		}
	}

	if (input == 0) {
		helpMessages();
		return 1;
	}

	// workingString will hold updated file names
	char workingString[1024];
	// GUPPI_RAW will only output to a single file
	outputFilesCount = 1;

	// Sanity check a few inputs
	if ( (strcmp(inputFormat, "") == 0 && dadaInput < 1) || (config.dadaKeys[0] < 1 && dadaOffset > 1) || (dadaInput == 0) ||  (config.numPorts <= 0) || (config.packetsPerIteration < 2)  || (config.replayDroppedPackets > 1 || config.replayDroppedPackets < 0) || (seconds < 0) || (config.ompThreads < 1)) {
		fprintf(stderr, "One or more inputs invalid or not fully initialised, exiting.\n");
		helpMessages();
		return 1;
	}

	// Warn if we haven't been provided metadata for the header
	if (strcmp(hdrFile, "") == 0) {
		fprintf(stderr, "WARNING: A header file was not provided; we are using the default values for the output file.\n");
	} else if (access(hdrFile, F_OK) == -1) {
		fprintf(stderr, "Header file does not exist at given location %s; exiting.\n", hdrFile);
		return 1;
	}


	if (dadaInput < 1) {
		// Check if we have a compressed input file
		if (strstr(inputFormat, "zst") != NULL) {
			config.readerType = ZSTDCOMPRESSED;
		} else {
			config.readerType = NORMAL;
		}

		// Set-up the input files, with checks to ensure they're opened
		for (int port = basePort; port < config.numPorts + basePort; port++) {
			sprintf(workingString, inputFormat, port);

			if (strcmp(inputFormat, workingString) == 0 && config.numPorts > 1) {
				fprintf(stderr, "ERROR: Input file was not iterated while trying to load raw data, please ensure it contains a '%%d' value. Exiting.\n");
				return 1;
			}

			VERBOSE(if (config.verbose) printf("Opening file at %s\n", workingString));

			inputFiles[port - basePort] = fopen(workingString, "r");
			if (inputFiles[port - basePort] == NULL) {
				fprintf(stderr, "Input file at %s does not exist, exiting.\n", workingString);
				return 1;
			}
			PAUSE;
		}
	
	} else {
		for (int i = 1; i < config.numPorts; i++) {
			config.dadaKeys[i] = config.dadaKeys[0] + i * dadaOffset;
		}
	}


	// Determine the clock time
	sampleTime = clock160MHzSample * (1 - clock200MHz) + clock200MHzSample * clock200MHz;


	if (silent == 0) {
		printf("LOFAR UDP Data extractor (GUPPI v%s, lib v%s)\n\n", UPM_CLI_VERSION, UPM_VERSION);
		printf("=========== Given configuration ===========\n");
		if (dadaInput < 0) {
			printf("Input File:\t%s\n", inputFormat);
		} else {
			printf("Input Ringbuffer/Offset:\t%d, %d\n", config.dadaKeys[0], dadaOffset);
		}
		printf("Output File: %s\n\n", outputFormat);
		printf("Packets/Gulp:\t%ld\t\t\tPorts:\t%d\n\n", config.packetsPerIteration, config.numPorts);
		VERBOSE(printf("Verbose:\t%d\n", config.verbose););
		printf("Proc Mode:\t%03d\t\t\tCompressed:\t%d\n\n", 30, config.readerType);
		printf("Beamlet limits:\t%d, %d\n\n", config.beamletLimits[0], config.beamletLimits[1]);
	}



	// Determine the expected initial values based on the input flags
	dateStr = calloc(1, sizeof(char*));
	dateStr[0] = calloc(1, sizeof("2020-20-20T-20:20:20"));
	if (strcmp(inputTime, "") != 0) {
		config.startingPacket = getStartingPacket(inputTime, clock200MHz);
		if (config.startingPacket == 1) {
			helpMessages();
			free(dateStr[0]); free(dateStr);
			return 1;
		}
	}

	strcpy(dateStr[0], inputTime);

	if(seconds != 0.0) config.packetsReadMax = getSecondsToPacket(seconds, clock200MHz);
	else config.packetsReadMax = LONG_MAX;

	if (silent == 0) printf("Start Time:\t%s\t200MHz Clock:\t%d\n", inputTime, clock200MHz);
	if (silent == 0) printf("Initial Packet:\t%ld\t\tFinal Packet:\t%ld\n", config.startingPacket, config.startingPacket + config.packetsReadMax);
	if (silent == 0) printf("============ End configuration ============\n\n");


	// If the largest requested data block is less than the packetsPerIteration input, lower the figure so we aren't doing unnecessary reads/writes
	if (config.packetsPerIteration > config.packetsReadMax)  {
		if (silent == 0) printf("Packet/Gulp is greater than the maximum packets requested, reducing from %ld to %ld.\n", config.packetsPerIteration, config.packetsReadMax);
		config.packetsPerIteration = config.packetsReadMax;

	}

	// Check that the output files don't already exist (no append mode), or that they can be written to (append mode)		
	if (strstr(outputFormat, "%ld") != NULL && silent == 0)  {
		printf("WARNING: we cannot predict whether or not files following the prefix '%s' will exist due to the packet number being variable due to packet loss.\nContinuing with caution.\n\n", outputFormat);
	} else {
		for (int out = 0; out < outputFilesCount; out++) {
			sprintf(workingString, outputFormat, out, dateStr[0]);

			VERBOSE(if (config.verbose) printf("Checking if file at %s exists / can be written to\n", workingString));
			if (!appendMode) {
				if (access(workingString, F_OK) != -1) {
					fprintf(stderr, "Output file at %s already exists; exiting.\n", workingString);
					free(dateStr[0]); free(dateStr);
					return 1;
				}
			} else {
				outputFiles[0] = fopen(workingString, "a");
				if (outputFiles[0] == NULL) {
					fprintf(stderr, "Output file at %s could not be opened for writing, exiting.\n", workingString);
					free(dateStr[0]); free(dateStr);
					return 1;
				}

				fclose(outputFiles[0]);
			}
		}
	}


	
	if (silent == 0) printf("Starting data read/reform operations...\n");

	// Start our timers early, _setup performs the first read operation in order to seek if needed
	CLICK(tick);
	CLICK(tick0);

	// Generate the lofar_udp_reader, this also does I/O for the first input or seeks to the required packet
	config.inputFiles = &(inputFiles[0]);
	lofar_udp_reader *reader =  lofar_udp_meta_file_reader_setup_struct(&(config));

	// Returns null on error, check
	if (reader == NULL) {
		fprintf(stderr, "Failed to generate reader. Exiting.\n");
		free(dateStr[0]); free(dateStr);
		return 1;
	}

	// Sanity check that we were passed the correct clock bit
	if (((lofar_source_bytes*) &(reader->meta->inputData[0][1]))->clockBit != clock200MHz) {
		fprintf(stderr, "ERROR: The clock bit of the first packet does not match the clock state given when starting the CLI. Add or remove -c from your command. Exiting.\n");
		free(dateStr[0]); free(dateStr);
		return 1;
	}

	// Initialise the ASCII header struct if a metadata file was provided
	if (strcmp(hdrFile, "") != 0) {
		if (parseHdrFile(hdrFile, &header) > 0) {
			fprintf(stderr, "ERROR: Error initialising ASCII header struct, exiting.");
			free(dateStr[0]); free(dateStr);
			return 1;
		}
	}

	// Pull the reader parameters into the ASCII header
	// Frequency indo
	header.obsnchan = reader->meta->totalProcBeamlets;
	header.chan_bw = (CLOCK160MHZ / 1e6) * (1 - clock200MHz) + (CLOCK200MHZ / 1e6) * clock200MHz;
	header.obsbw = header.obsnchan * header.chan_bw;

	// Data format into
	header.nbits = reader->meta->inputBitMode;
	header.npol = 2;
	header.overlap = 0;
	strcpy(header.pktfmt, "1SFA");
	header.pktsize = reader->meta->packetOutputLength[0];

	// Timing info
	double mjdTime = lofar_get_packet_time_mjd(reader->meta->inputData[0]);
	header.stt_imjd = (int) mjdTime;
	header.stt_smjd = (int) ((mjdTime - (int) mjdTime) * 86400);
	header.stt_offs = ((mjdTime - (int) mjdTime) * 86400) - header.stt_smjd;
	header.tbin = sampleTime;


	if (silent == 0) {
		getStartTimeString(reader, stringBuff);
		printf("\n\n=========== Reader  Information ===========\n");
		printf("Total Beamlets:\t%d%d\t\t\t\t\tFirst Packet:\t%ld\n", reader->meta->totalProcBeamlets, reader->meta->totalRawBeamlets, reader->meta->lastPacket);
		printf("Start time:\t%s\t\tMJD Time:\t%lf\n", stringBuff, lofar_get_packet_time_mjd(reader->meta->inputData[0]));
		for (int port = 0; port < reader->meta->numPorts; port++) {
			printf("------------------ Port %d -----------------\n", port);
			printf("Port Beamlets:\t%d/%d\t\tPort Bitmode:\t%d\t\tInput Pkt Len:\t%d\n", reader->meta->upperBeamlets[port] - reader->meta->baseBeamlets[port], reader->meta->portRawBeamlets[port], reader->meta->inputBitMode, reader->meta->portPacketLength[port]);
		}
		for (int out = 0; out < reader->meta->numOutputs; out++) printf("Output Pkt Len (%d):\t%d\t\t", out, reader->meta->packetOutputLength[out]);
		printf("\n"); 
		printf("============= End Information =============\n\n");
	}

	// Start iterating
	int endCondition = 0;
	int totalDropped = 0, blockDropped = 0;
	while (!endCondition) {
		// Reset the local values for each file
		localLoops = 0;
		returnVal = 0;

		// Output information about the current/last event if we're performing more than one event
		getStartTimeString(reader, timeStr);

		// Open the output files for this event
		for (int out = 0; out < outputFilesCount; out++) {
			sprintf(workingString, outputFormat, loops / itersPerFile);
			VERBOSE(if (config.verbose) printf("Testing output file for output %d @ %s\n", out, workingString));
			
			if (appendMode != 1 && access(workingString, F_OK) != -1) {
				fprintf(stderr, "Output file at %s already exists; exiting.\n", workingString);
				free(dateStr[0]); free(dateStr);
				return 1;
			}
			
			VERBOSE(if (config.verbose) printf("Opening file at %s\n", workingString));

			outputFiles[out] = fopen(workingString, "a");
			if (outputFiles[out] == NULL) {
				fprintf(stderr, "Output file at %s could not be created, exiting.\n", workingString);
				free(dateStr[0]); free(dateStr);
				return 1;
			}

			if (loops > 0) {
				if (silent == 0) printf("\n\nCompleted work for %d iterations, creating new file at %s\n\n\n", loops, workingString);
			}
		}

		VERBOSE(if (config.verbose) printf("Begining data extraction loop for event %d\n", loops));
		// While we receive new data for the current event,
		while ((returnVal = lofar_udp_reader_step_timed(reader, timing)) < 1) {

			CLICK(tock0);
			if (localLoops == 0) timing[0] = TICKTOCK(tick0, tock0) - timing[1]; // _file_reader_step or _reader_reuse does first I/O operation; approximate the time here
			if (silent == 0) printf("Read complete for operation %d after %f seconds (I/O: %lf, MemOps: %lf), return value: %d\n", loops, TICKTOCK(tick0, tock0), timing[0], timing[1], returnVal);
			
			totalReadTime += timing[0];
			totalOpsTime += timing[1];

			// Write out the desired amount of packets; cap if needed.
			packetsToWrite = reader->packetsPerIteration;

			// Get the number of dropped packets for the ASCII header
			for(int port = 0; port < reader->meta->numPorts; port++)
				if (reader->meta->portLastDroppedPackets[port] != 0) {
					blockDropped += reader->meta->portLastDroppedPackets[port];
					totalDropped += blockDropped;
				}

			CLICK(tick0);
			
			#ifndef BENCHMARKING
			for (int out = 0; out < reader->meta->numOutputs; out++) {
				// Update header parameters, then write it to disk before we write the next block of data
				header.blocsize = packetsToWrite * reader->meta->packetOutputLength[out];
				
				// Should this be processing time, rather than the recording time?
				getStartTimeStringDAQ(reader, timeStr);
				strcpy(header.daqpulse, timeStr);

				header.dropblk = blockDropped / packetsToWrite;
				header.droptot = totalDropped / (packetsWritten + packetsToWrite);

				if (loops > 0) {
					header.pktidx = packetsWritten;
				}

				writeHdr(outputFiles[out], &header);
				
				VERBOSE(printf("Writing %ld bytes (%ld packets) to disk for output %d...\n", packetsToWrite * reader->meta->packetOutputLength[out], packetsToWrite, out));
				fwrite(reader->meta->outputData[out], sizeof(char), packetsToWrite * reader->meta->packetOutputLength[out], outputFiles[out]);
			}
			#endif

			// Count the number of packets written + processed (should always be the same for the GUPPI CLI)
			packetsWritten += packetsToWrite;
			packetsProcessed += reader->meta->packetsPerIteration;

			CLICK(tock0);
			totalWriteTime += TICKTOCK(tick0, tock0);
			if (silent == 0) {
				timing[0] = 9.;
				timing[1] = 0.;
				printf("Disk writes completed for operation %d after %f seconds.\n", loops, TICKTOCK(tick0, tock0));
				if (returnVal < 0) 
					for(int port = 0; port < reader->meta->numPorts; port++)
						if (reader->meta->portLastDroppedPackets[port] != 0)
							printf("During this iteration there were %d dropped packets on port %d.\n", reader->meta->portLastDroppedPackets[port], port);
				printf("\n");
			}

			loops++; localLoops++;
			blockDropped = 0;
			// returnVal below 0 indicates we will not be given data on the next iteration, so gracefully exit with the known reason
			if (returnVal < -1) {
				printf("We've hit a termination return value (%d, %s), exiting.\n", returnVal, exitReasons[abs(returnVal)]);
				endCondition = 1;
				break;
			}

			#ifdef __SLOWDOWN
			sleep(1);
			#endif
			CLICK(tick0);

			if (localLoops == itersPerFile) {
				break;
			}
		}

		// Close the output files before we open new ones or exit
		for (int out = 0; out < outputFilesCount; out++) fclose(outputFiles[out]);

	}

	CLICK(tock);

	int droppedPackets = 0;
	long totalPacketLength = 0, totalOutLength = 0;

	// Print out a summary of the operations performed, this does not contain data read for seek operations
	if (silent == 0) {
		for (int port = 0; port < reader->meta->numPorts; port++) totalPacketLength += reader->meta->portPacketLength[port];
		for (int out = 0; out < outputFilesCount; out++) totalOutLength += reader->meta->packetOutputLength[out];
		for (int port = 0; port < reader->meta->numPorts; port++) droppedPackets += reader->meta->portTotalDroppedPackets[port];

		printf("Reader loop exited (%d); overall process took %f seconds.\n", returnVal, (double) TICKTOCK(tick, tock));
		printf("We processed %ld packets, representing %.03lf seconds of data", packetsProcessed, reader->meta->numPorts * packetsProcessed * UDPNTIMESLICE * 5.12e-6);
		if (reader->meta->numPorts > 1) printf(" (%.03lf per port)\n", packetsProcessed * UDPNTIMESLICE * 5.12e-6);
		else printf(".\n");
		printf("Total Read Time:\t%3.02lf\t\tTotal CPU Ops Time:\t%3.02lf\tTotal Write Time:\t%3.02lf\n", totalReadTime, totalOpsTime, totalWriteTime);
		printf("Total Data Read:\t%3.03lfGB\t\t\t\tTotal Data Written:\t%3.03lfGB\n", (double) packetsProcessed * totalPacketLength / 1e+9, (double) packetsWritten* totalOutLength / 1e+9);
		printf("A total of %d packets were missed during the observation.\n", droppedPackets);
		printf("\n\nData processing finished. Cleaning up file and memory objects...\n");
	}



	// Clean-up the reader object, also closes the input files for us
	lofar_udp_reader_cleanup(reader);
	if (silent == 0) printf("Reader cleanup performed successfully.\n");

	// Free our malloc'd objects
	free(dateStr[0]);
	free(dateStr);

	if (silent == 0) printf("CLI memory cleaned up successfully. Exiting.\n");
	return 0;
}


/**
 * @brief      Emulate the DAQ time string from the current data timestamp
 *
 * @param      reader      The lofar_udp_reader
 * @param      stringBuff  The output time string buffer
 */
void getStartTimeStringDAQ(lofar_udp_reader *reader, char stringBuff[24]) {
	double startTime;
	time_t startTimeUnix;
	struct tm *startTimeStruct;

	startTime = lofar_get_packet_time(reader->meta->inputData[0]);
	startTimeUnix = (unsigned int) startTime;
	startTimeStruct = gmtime(&startTimeUnix);

	strftime(stringBuff, 24, "%a %b %e %H:%M:%S %Y", startTimeStruct);
}