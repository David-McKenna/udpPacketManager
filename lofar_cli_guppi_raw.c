#include "lofar_cli_meta.h"
#include "ascii_hdr_manager.h"


void getStartTimeStringDAQ(lofar_udp_reader *reader, char stringBuff[]);

void helpMessages() {
	printf("LOFAR UDP Data extractor (v%.1f)\n\n", VERSIONCLI);
	printf("Usage: ./lofar_cli_extractor <flags>");

	printf("\n\n");

	printf("-i: <format>	Input file name format (default: './%%d')\n");
	printf("-o: <format>	Output file name format (provide %%s and %%ld to fill in ate/time string and the starting packet number) (default: './output%%s_%%ld')\n");
	printf("-m: <numPack>	Number of packets to process in each read request (default: 65536)\n");
	printf("-u: <numPort>	Number of ports to combine (default: 4)\n");
	printf("-t: <timeStr>	String of the time of the first requested packet, format YYYY-MM-DDTHH:mm:ss (default: '')\n");
	printf("-s: <numSec>	Maximum number of seconds to process (default: all)\n");
	printf("-e: <iters>		Split the file every N iterations (default: inf)\n");
	printf("-r:		Replay the previous packet when a dropped packet is detected (default: 0 pad)\n");
	printf("-c:		Change to the alternative clock used for modes 4/6 (160MHz clock) (default: False)\n");
	printf("-q:		Enable silent mode for the CLI, don't print any information outside of library error messes (default: False)\n");
	printf("-a: <file>		File to open with parameters for the ASCII headers\n");
	printf("-f:		Append files if they already exist (default: False, exit if exists)\n");
	
	VERBOSE(printf("-v:		Enable verbose output (default: False)\n");
			printf("-V:		Enable highly verbose output (default: False)\n"));

}


int main(int argc, char  *argv[]) {

	// Set up input local variables
	int inputOpt, outputFilesCount, input = 0;
	float seconds = 0.0;
	double sampleTime = 0.0;
	char inputFormat[256] = "./%d", outputFormat[256] = "./output_%s_%ld", inputTime[256] = "", stringBuff[128], hdrFile[2048] = "", timeStr[24] = "";
	int ports = 4, replayDroppedPackets = 0, verbose = 0, silent = 0, appendMode = 0, compressedReader = 0, eventCount = 0, returnCounter = 0, itersPerFile = -1;
	long packetsPerIteration = 65536, maxPackets = -1, startingPacket = -1;
	unsigned int clock200MHz = 1;
	ascii_hdr header = ascii_hdr_default;
	FILE *eventsFilePtr;

	// Set up reader loop variables
	int loops = 0, localLoops = 0, returnVal;
	long packetsProcessed = 0, packetsWritten = 0, eventPacketsLost[MAX_NUM_PORTS], packetsToWrite;
	double timing[2] = {0., 0.}, totalReadTime = 0, totalOpsTime = 0, totalWriteTime = 0;
	struct timespec tick, tick0, tock, tock0;

	// I/O variables
	FILE *inputFiles[MAX_NUM_PORTS];
	FILE *outputFiles[MAX_OUTPUT_DIMS];
	
	// Malloc'd variables: need to be free'd later.
	long *startingPackets, *multiMaxPackets;
	float *eventSeconds;
	char **dateStr; // Sub elements need to be free'd too.

	// Standard ugly input flags parser
	while((inputOpt = getopt(argc, argv, "rcqfvVi:o:m:u:t:s:e:a:")) != -1) {
		input = 1;
		switch(inputOpt) {
			
			case 'i':
				strcpy(inputFormat, optarg);
				break;

			case 'o':
				strcpy(outputFormat, optarg);
				break;

			case 'm':
				packetsPerIteration = atol(optarg);
				break;

			case 'u':
				ports = atoi(optarg);
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


			case 'r':
				replayDroppedPackets = 1;
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
				if (!verbose)
					VERBOSE(verbose = 1;);
				break;
			case 'V': 
				VERBOSE(verbose = 2;);
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

	char workingString[1024];
	outputFilesCount = 1;

	// Sanity check a few inputs
	if ( (strcmp(inputFormat, "") == 0) || (ports == 0) || (packetsPerIteration < 2)  || (replayDroppedPackets > 1 || replayDroppedPackets < 0) || (seconds < 0)) {
		fprintf(stderr, "One or more inputs invalid or not fully initialised, exiting.\n");
		helpMessages();
		return 1;
	}

	if (strcmp(hdrFile, "") == 0) {
		fprintf(stderr, "WARNING: A header file was not provided; we are using the default values for the output file.\n");
	}

	// Check if we have a compressed input file
	if (strstr(inputFormat, "zst") != NULL) {
		compressedReader = 1;
	}

	// Determine the clock time
	sampleTime = clock160MHzSample * (1 - clock200MHz) + clock200MHzSample * clock200MHz;


	if (silent == 0) {
		printf("LOFAR UDP Data extractor (CLI v%.1f, Backend V%.1f)\n\n", VERSIONCLI, VERSION);
		printf("=========== Given configuration ===========\n");
		printf("Input File:\t%s\nOutput File: %s\n\n", inputFormat, outputFormat);
		printf("Packets/Gulp:\t%ld\t\t\tPorts:\t%d\n\n", packetsPerIteration, ports);
		VERBOSE(printf("Verbose:\t%d\n", verbose););
		printf("Proc Mode:\t%03d\t\t\tCompressed:\t%d\n\n", 30, compressedReader);
	}



	eventCount = 1;

	startingPackets = calloc(1, sizeof(long));
	dateStr = calloc(1, sizeof(char*));
	dateStr[0] = calloc(1, sizeof("2020-20-20T-20:20:20"));
	if (strcmp(inputTime, "") != 0) {
		startingPacket = getStartingPacket(inputTime, clock200MHz);
		if (startingPacket == 1) return 1;
	}
	startingPackets[0] = startingPacket;
	strcpy(dateStr[0], inputTime);

	eventSeconds = calloc(1, sizeof(float));
	eventSeconds[0] = seconds;
	multiMaxPackets = calloc(1, sizeof(long));
	if(seconds != 0.0) multiMaxPackets[0] = getSecondsToPacket(seconds, clock200MHz);
	else multiMaxPackets[0] = LONG_MAX;

	maxPackets = multiMaxPackets[0];
	if (silent == 0) printf("Start Time:\t%s\t200MHz Clock:\t%d\n", inputTime, clock200MHz);
	if (silent == 0) printf("Initial Packet:\t%ld\t\tFinal Packet:\t%ld\n", startingPackets[0], startingPackets[0] + maxPackets);
	if (silent == 0) printf("============ End configuration ============\n\n");


	// If the largest requested data block is less than the packetsPerIteration input, lower the figure so we aren't doing unnecessary reads/writes
	if (packetsPerIteration > maxPackets)  {
		if (silent == 0) printf("Packet/Gulp is greater than the maximum packets requested, reducing from %ld to %ld.\n", packetsPerIteration, maxPackets);
		packetsPerIteration = maxPackets;

	}

	// Set-up the input files, with checks to ensure they're opened
	for (int port = 0; port < ports; port++) {
		sprintf(workingString, inputFormat, port);

		VERBOSE(if (verbose) printf("Opening file at %s\n", workingString));

		inputFiles[port] = fopen(workingString, "r");
		if (inputFiles[port] == NULL) {
			fprintf(stderr, "Input file at %s does not exist, exiting.\n", workingString);
			return 1;
		}
		PAUSE;
	}

	// Check that the output files don't already exist (no append mode), or that they can be written to (append mode)
	for (int eventLoop = 0; eventLoop < eventCount; eventLoop++) {
		
		if (strstr(outputFormat, "%ld") != NULL && silent == 0)  {
			printf("WARNING: we cannot predict whether or not files following the prefix '%s' will exist due to the packet number being variable due to packet loss.\nContinuing with caution.\n\n", outputFormat);
			break;
		}

		for (int out = 0; out < outputFilesCount; out++) {
			sprintf(workingString, outputFormat, out, dateStr[eventLoop]);

			VERBOSE( if(verbose) printf("Checking if file at %s exists / can be written to\n", workingString));
			if (!appendMode) {
				if (access(workingString, F_OK) != -1) {
					fprintf(stderr, "Output file at %s already exists; exiting.\n", workingString);
					return 1;
				}
			} else {
				outputFiles[0] = fopen(workingString, "a");
				if (outputFiles[0] == NULL) {
					fprintf(stderr, "Output file at %s could not be opened for writing, exiting.\n", workingString);
					return 1;
				}

				fclose(outputFiles[0]);
			}
		}
	}


	
	if (silent == 0) printf("Starting data read/reform operations...\n");

	// Start our timers
	CLICK(tick);
	CLICK(tick0);

	// Generate the lofar_udp_reader, this also does I/O for the first input or seeks to the required packet
	lofar_udp_reader *reader =  lofar_udp_meta_file_reader_setup(&(inputFiles[0]), ports, replayDroppedPackets, 30, verbose, packetsPerIteration, startingPackets[0], multiMaxPackets[0], compressedReader);

	if (parseHdrFile(hdrFile, &header) > 0) {
		fprintf(stderr, "Error initialising ASCII header struct, exiting.");
	}


	// Pull the reader parameters into the ASCII header
	header.obsnchan = reader->meta->totalBeamlets;
	header.nbits = reader->meta->inputBitMode;
	header.tbin = sampleTime;

	double mjdTime = lofar_get_packet_time_mjd(reader->meta->inputData[0]);
	header.stt_imjd = (int) mjdTime;
	header.stt_smjd = (int) ((mjdTime - (int) mjdTime) * 86400);


	// Returns null on error, check
	if (reader == NULL) {
		fprintf(stderr, "Failed to generate reader. Exiting.\n");
		return 1;

	}

	if (silent == 0) {
		getStartTimeString(reader, stringBuff);
		printf("\n\n=========== Reader  Information ===========\n");
		printf("Total Beamlets:\t%d\t\t\t\t\tFirst Packet:\t%ld\n", reader->meta->totalBeamlets, reader->meta->lastPacket);
		printf("Start time:\t%s\t\tMJD Time:\t%lf\n", stringBuff, lofar_get_packet_time_mjd(reader->meta->inputData[0]));
		for (int port = 0; port < reader->meta->numPorts; port++) {
			printf("------------------ Port %d -----------------\n", port);
			printf("Port Beamlets:\t%d\t\tPort Bitmode:\t%d\t\tInput Pkt Len:\t%d\n", reader->meta->portBeamlets[port], reader->meta->inputBitMode, reader->meta->portPacketLength[port]);
		}
		for (int out = 0; out < reader->meta->numOutputs; out++) printf("Output Pkt Len (%d):\t%d\t\t", out, reader->meta->packetOutputLength[out]);
		printf("\n"); 
		printf("============= End Information =============\n\n");
	}

	// Scan over the registered events
	int endCondition = 0;
	while (!endCondition) {
		localLoops = 0;
		returnVal = 0;

		// Output information about the current/last event if we're performing more than one event
		getStartTimeString(reader, timeStr);
		if (eventCount > 1) 
			if (silent == 0) {
				if (loops > 0)  {
					printf("Completed work for event %d, packet loss for each port during this event was", eventCount -1);
					for (int port = 0; port < reader->meta->numPorts; port++) printf(" %ld", eventPacketsLost[port]);
					printf(".\n\n\n");
				}
				printf("Beginning work on event %d at %s...\n", loops, timeStr);
				getStartTimeString(reader, stringBuff);
				printf("============ Event %d Information ===========\n", loops);
				printf("Target Time:\t%s\t\tActual Time:\t%s\n", timeStr, stringBuff);
				printf("MJD Time:\t%lf\n", lofar_get_packet_time_mjd(reader->meta->inputData[0]));
				printf("============= End Information ==============\n");
			}


		// Get the starting packet for output file names
		startingPacket = reader->meta->leadingPacket;

		// Open the output files for this event
		for (int out = 0; out < outputFilesCount; out++) {
			sprintf(workingString, outputFormat, timeStr, startingPacket);
			VERBOSE(if (verbose) printf("Testing output file for output %d @ %s\n", out, workingString));
			
			if (appendMode != 1 && access(workingString, F_OK) != -1) {
				fprintf(stderr, "Output file at %s already exists; exiting.\n", workingString);
				return 1;
			}
			
			VERBOSE(if (verbose) printf("Opening file at %s\n", workingString));

			outputFiles[out] = fopen(workingString, "a");
			if (outputFiles[out] == NULL) {
				fprintf(stderr, "Output file at %s could not be created, exiting.\n", workingString);
				return 1;
			}

			if (loops > 0) {
				if (silent == 0) printf("\n\nCompleted work for %d iterations, creating new file at %s\n\n\n", loops, workingString);
			}
		}

		VERBOSE(if (verbose) printf("Begining data extraction loop for event %d\n", loops));
		// While we receive new data for the current event,
		while ((returnVal = lofar_udp_reader_step_timed(reader, timing)) < 1) {

			CLICK(tock0);
			if (localLoops == 0) timing[0] = TICKTOCK(tick0, tock0) - timing[1]; // _file_reader_step or _reader_reuse does first I/O operation; approximate the time here
			if (silent == 0) printf("Read complete for operation %d after %f seconds (I/O: %lf, MemOps: %lf), return value: %d\n", loops, TICKTOCK(tick0, tock0), timing[0], timing[1], returnVal);
			
			totalReadTime += timing[0];
			totalOpsTime += timing[1];

			// Write out the desired amount of packets; cap if needed.
			packetsToWrite = reader->meta->packetsPerIteration;
			if (multiMaxPackets[loops] < packetsToWrite) packetsToWrite = multiMaxPackets[loops];

			CLICK(tick0);
			
			#ifndef BENCHMARKING
			for (int out = 0; out < reader->meta->numOutputs; out++) {
				header.blocsize = packetsToWrite * reader->meta->packetOutputLength[out];
				
				getStartTimeStringDAQ(reader, timeStr);
				strcpy(header.daqpulse, timeStr);

				// May cause issues if there's packet loss at the start of a data block
				mjdTime = lofar_get_packet_time_mjd(reader->meta->inputData[0]);
				header.stt_imjd = (int) mjdTime;
				header.stt_smjd = (int) ((mjdTime - (int) mjdTime) * 86400);
				header.pktidx += packetsToWrite;

				writeHdr(outputFiles[out], &header);
				
				VERBOSE(printf("Writing %ld bytes (%ld packets) to disk for output %d...\n", packetsToWrite * reader->meta->packetOutputLength[out], packetsToWrite, out));
				fwrite(reader->meta->outputData[out], sizeof(char), packetsToWrite * reader->meta->packetOutputLength[out], outputFiles[out]);
			}
			#endif

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

			if (localLoops > itersPerFile) {
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
		for (int port = 0; port < ports; port++) totalPacketLength += reader->meta->portPacketLength[port];
		for (int out = 0; out < outputFilesCount; out++) totalOutLength += reader->meta->packetOutputLength[out];
		for (int port = 0; port < ports; port++) droppedPackets += reader->meta->portTotalDroppedPackets[port];

		printf("Reader loop exited (%d); overall process took %f seconds.\n", returnVal, (double) TICKTOCK(tick, tock));
		printf("We processed %ld packets, representing %.03lf seconds of data", packetsProcessed, ports * packetsProcessed * UDPNTIMESLICE * 5.12e-6);
		if (ports > 1) printf(" (%.03lf per port)\n", packetsProcessed * UDPNTIMESLICE * 5.12e-6);
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
	for (int i =0; i < eventCount; i++) free(dateStr[i]);
	free(dateStr);
	free(multiMaxPackets);
	free(startingPackets);

	if (silent == 0) printf("CLI memory cleaned up successfully. Exiting.\n");
	return 0;
}


void getStartTimeStringDAQ(lofar_udp_reader *reader, char stringBuff[24]) {
	double startTime;
	time_t startTimeUnix;
	struct tm *startTimeStruct;

	startTime = lofar_get_packet_time(reader->meta->inputData[0]);
	startTimeUnix = (unsigned int) startTime;
	startTimeStruct = gmtime(&startTimeUnix);

	strftime(stringBuff, 24, "%c", startTimeStruct);
}