#include "lofar_cli_meta.h"
#include <fftw3.h>
#include <math.h>

// Local prototypes
void reorderSpectrum(fftwf_complex **workingArr, int nbin, int nruns);
void detect(int nbin, int processingFactor, int nfft, int nsubbands, fftwf_complex **output, float *stokesOutput);
void fftwAndDetect(lofar_udp_reader *reader, fftwf_plan *forward, fftwf_plan *backward, fftwf_complex **intermediate, fftwf_complex **output, float *stokesOutput, int nbin, int nfft, int nsub, int processingFactor);
void chirpKernel(float *taperArray, int totalBeamlets, int chanFactor, float fch1, float foff, int nbin);


void helpMessages() {
	printf("LOFAR UDP Data extractor (v%.1f)\n\n", VERSIONCLI);
	printf("Usage: ./lofar_cli_extractor <flags>");

	printf("\n\n");

	printf("-i: <format>	Input file name format (default: './%%d')\n");
	printf("-o: <format>	Output file name format (provide %%d, %%s and %%ld to fill in output ID, date/time string and the starting packet number) (default: './output%%d_%%s_%%ld')\n");
	printf("-m: <numPack>	Number of packets to process in each read request (default: 65536)\n");
	printf("-u: <numPort>	Number of ports to combine (default: 4)\n");
	printf("-t: <timeStr>	String of the time of the first requested packet, format YYYY-MM-DDTHH:mm:ss (default: '')\n");
	printf("-s: <numSec>	Maximum number of seconds to process (default: all)\n");
	printf("-e: <fileName>	Specify a file of events to extract; newline separated start time and durations in seconds. Events must not overlap.\n");
	printf("-p: <chans>		Channelisation factor, must be a multiple of the ingested number of samples (default : 16)\n");
	printf("-r:		Replay the previous packet when a dropped packet is detected (default: 0 pad)\n");
	printf("-c:		Change to the alternative clock used for modes 4/6 (160MHz clock) (default: False)\n");
	printf("-q:		Enable silent mode for the CLI, don't print any information outside of library error messes (default: False)\n");
	printf("-a: <args>		Call mockHeader with the specific flags to prefix output files with a header (default: False)\n");
	printf("-f:		Append files if they already exist (default: False, exit if exists)\n");
	
	VERBOSE(printf("-v:		Enable verbose output (default: False)\n");
			printf("-V:		Enable highly verbose output (default: False)\n"));

}


int main(int argc, char  *argv[]) {

	// Set up input local variables
	int inputOpt, outputFilesCount, input = 0;
	float seconds = 0.0;
	double sampleTime = 0.0;
	char inputFormat[256] = "./%d", outputFormat[256] = "./output%d_%s_%ld", inputTime[256] = "", eventsFile[256] = "", stringBuff[128], mockHdrArg[2048] = "", mockHdrCmd[4096] = "";
	int ports = 4, processingFactor = 16, replayDroppedPackets = 0, verbose = 0, silent = 0, appendMode = 0, compressedReader = 0, eventCount = 0, returnCounter = 0, callMockHdr = 0;
	long packetsPerIteration = 65536, maxPackets = -1, startingPacket = -1;
	unsigned int clock200MHz = 1;
	FILE *eventsFilePtr;

	// Set up reader loop variables
	int loops = 0, localLoops = 0, returnVal, dummy;
	long packetsProcessed = 0, packetsWritten = 0, eventPacketsLost[MAX_NUM_PORTS], packetsToWrite;
	double timing[2] = {0., 0.}, totalReadTime = 0, totalOpsTime = 0, totalWriteTime = 0;
	struct timespec tick, tick0, tock, tock0;

	// I/O variables
	FILE *inputFiles[MAX_NUM_PORTS];
	FILE *outputFiles[MAX_OUTPUT_DIMS];

	// FFTW variables
	fftwf_plan forward[2], backward[2];
	fftwf_complex *intermediate[2], *output[2];
	
	// Malloc'd variables: need to be free'd later.
	long *startingPackets, *multiMaxPackets;
	float *eventSeconds;
	char **dateStr; // Sub elements need to be free'd too.

	// Standard ugly input flags parser
	while((inputOpt = getopt(argc, argv, "rcqfvVi:o:m:u:t:s:e:p:a:")) != -1) {
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
				strcpy(eventsFile, optarg);
				break;

			case 'p':
				processingFactor = atoi(optarg);
				break;

			case 'a':
				strcpy(mockHdrArg, optarg);
				callMockHdr = 1;
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
				if ((optopt == 'i') || (optopt == 'o') || (optopt == 'm') || (optopt == 'u') || (optopt == 't') || (optopt == 's') || (optopt == 'e') || (optopt == 'p') || (optopt == 'a')) {
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
	

	if ((packetsPerIteration * UDPNTIMESLICE % processingFactor) != 0) {
		fprintf(stderr, "Channelisation factor (%d) is not a multiple of the number of input samples (%ld), exiting.\n", processingFactor, packetsPerIteration * UDPNTIMESLICE);
		return 1;
	}

	// Sanity check a few inputs
	if ( (strcmp(inputFormat, "") == 0) || (ports == 0) || (packetsPerIteration < 2)  || (replayDroppedPackets > 1 || replayDroppedPackets < 0) || (seconds < 0)) {
		fprintf(stderr, "One or more inputs invalid or not fully initialised, exiting.\n");
		helpMessages();
		return 1;
	}

	// Check if we have a compressed input file
	if (strstr(inputFormat, "zst") != NULL) {
		compressedReader = 1;
	}

	// Make sure mockHeader is on the path if we want to use it.
	if (callMockHdr) {
		printf("Checking for mockHeader on system path... ");
		callMockHdr += system("which mockHeader > /tmp/udp_reader_mockheader.log 2>&1"); // Add the return code (multiplied by 256 from bash return) to the execution variable, ensure it doesn't change
		printf("\n");
		if (callMockHdr != 1) {
			fprintf(stderr, "Error occured while attempting to find mockHeader, exiting.\n");
			return 1;
		}

		sampleTime = clock160MHzSample * (1 - clock200MHz) + clock200MHzSample * clock200MHz;
	}

	if (silent == 0) {
		printf("LOFAR UDP Data extractor (CLI v%.1f, Backend V%.1f)\n\n", VERSIONCLI, VERSION);
		printf("=========== Given configuration ===========\n");
		printf("Input File:\t%s\nOutput File: %s\n\n", inputFormat, outputFormat);
		printf("Packets/Gulp:\t%ld\t\t\tPorts:\t%d\n\n", packetsPerIteration, ports);
		VERBOSE(printf("Verbose:\t%d\n", verbose););
		printf("Proc Fac:\t%03d\t\t\tCompressed:\t%d\n\n", processingFactor, compressedReader);
	}



	// If given an events file,
	if (strcmp(eventsFile, "") != 0) {

		// Try to read it
		eventsFilePtr = fopen(eventsFile, "r");
		if (eventsFilePtr == NULL) {
			fprintf(stderr, "Unable to open events file at %s, exiting.\n", eventsFile);
			return 1;
		}

		// The first line should be an int of the amount of events we need to process
		returnCounter = fscanf(eventsFilePtr, "%d", &eventCount);
		if (returnCounter != 1 || eventCount < 1) {
			fprintf(stderr, "Unable to parse events file (got %d as number of events), exiting.\n", eventCount);
			return 1;
		}

		// Malloc the arrays of the right length
		startingPackets = calloc(eventCount, sizeof(long));
		multiMaxPackets = calloc(eventCount, sizeof(long));
		dateStr = calloc(eventCount, sizeof(char*));
		eventSeconds = calloc(eventCount, sizeof(float));
		for (int i =0; i < eventCount; i++) dateStr[i] = calloc(128, sizeof(char));

		if (silent == 0) printf("Events File:\t%s\t\tEvent Count:\t%d\t\t\t200MHz Clock:\t%d\n", eventsFile, eventCount, clock200MHz);

		// For each event,
		for (int idx = 0; idx < eventCount; idx++) {
			// Get the time string and length of the event
			returnCounter = fscanf(eventsFilePtr, "%s %f", &stringBuff[0], &seconds);
			strcpy(dateStr[idx], stringBuff);

			if (returnCounter != 2) {
				fprintf(stderr, "Unable to parse line %d of events file, exiting ('%s', %lf).\n", idx + 1, stringBuff, seconds);
				return 1;
			}

			// Determine the packet corresponding to the initial time and the amount of packets needed to observe for  the length of the event
			startingPackets[idx] = getStartingPacket(stringBuff, clock200MHz);
			if (startingPackets[idx] == 1) return 1;

			eventSeconds[idx] = seconds;
			multiMaxPackets[idx] = getSecondsToPacket(seconds, clock200MHz);
			// If packetsPerIteration is too high, we can reduce it later by tracking the largest requested input size
			if (multiMaxPackets[idx] > maxPackets) maxPackets = multiMaxPackets[idx];

			if(silent == 0) printf("Event:\t%d\tSeconds:\t%.02lf\tInitial Packet:\t%ld\t\tFinal Packet:\t%ld\n", idx, seconds, startingPackets[idx], startingPackets[idx] + multiMaxPackets[idx]);

			// Safety check: all events are correctly ordered in increasing time, and do not overlap
			// Compressed observations cannot be fseek'd, so this is a required design choice
			if (idx > 0) {
				if (startingPackets[idx] < startingPackets[idx-1]) {
					fprintf(stderr, "Events %d and %d are out of order, please only use increasing event times, exiting.\n", idx, idx - 1);
					return 1;
				}

				if (startingPackets[idx] < startingPackets[idx-1] + multiMaxPackets[idx -1]) {
					fprintf(stderr, "Events %d and %d overlap, please combine them or ensure there is some buffer time between them, exiting.", idx, idx -1);
					return 1;
				}
			}

		}

	} else {
		// Repeat the step above for a single event, but read the defaults / -t and -s flags as the inputs
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
	}

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
	lofar_udp_reader *reader =  lofar_udp_meta_file_reader_setup(&(inputFiles[0]), ports, 1, 32, verbose, packetsPerIteration, startingPackets[0], multiMaxPackets[0], compressedReader);

	// Returns null on error, check
	if (reader == NULL) {
		fprintf(stderr, "Failed to generate reader. Exiting.\n");
		return 1;

	}


	// Inialise the FFTW variables
	for (int i = 0; i < 2; i++) {
		free(reader->meta->outputData[i]);
		reader->meta->outputData[i] = fftwf_malloc(sizeof(fftwf_complex) * packetsPerIteration * UDPNTIMESLICE * reader->meta->totalBeamlets);
		intermediate[i] = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * packetsPerIteration * UDPNTIMESLICE * reader->meta->totalBeamlets);
		output[i] = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * packetsPerIteration * UDPNTIMESLICE * reader->meta->totalBeamlets);
	}

	float *stokesIOutput = calloc(reader->meta->totalBeamlets * packetsPerIteration * UDPNTIMESLICE, sizeof(float));

	const int nbin = packetsPerIteration;
	int nbin_int = (int) packetsPerIteration;
	forward[0] = fftwf_plan_many_dft(1, &nbin, UDPNTIMESLICE * reader->meta->totalBeamlets, (fftwf_complex*) reader->meta->outputData[0], &nbin, 1, nbin_int, intermediate[0], &nbin, 1, nbin_int, -1, FFTW_MEASURE);
	forward[1] = fftwf_plan_many_dft(1, &nbin, UDPNTIMESLICE * reader->meta->totalBeamlets, (fftwf_complex*) reader->meta->outputData[1], &nbin, 1, nbin_int, intermediate[1], &nbin, 1, nbin_int, -1, FFTW_MEASURE);
	backward[0] = fftwf_plan_many_dft(1, &nbin, UDPNTIMESLICE * reader->meta->totalBeamlets, intermediate[0], &nbin, 1, nbin_int, output[0], &nbin, 1, nbin_int, 1, FFTW_MEASURE);
	backward[1] = fftwf_plan_many_dft(1, &nbin, UDPNTIMESLICE * reader->meta->totalBeamlets, intermediate[1], &nbin, 1, nbin_int, output[1], &nbin, 1, nbin_int, 1, FFTW_MEASURE);


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
	omp_set_num_threads(OMP_THREADS);
	for (int eventLoop = 0; eventLoop < eventCount; eventLoop++) {
		localLoops = 0;
		returnVal = 0;

		// Initialise / empty the packets lost array
		for (int port = 0; port < reader->meta->numPorts; port++) eventPacketsLost[port] = 0;

		// If we are not on the first event, set-up the reader for the current event
		if (loops != 0) {
			if ((returnVal = lofar_udp_file_reader_reuse(reader, startingPackets[eventLoop], multiMaxPackets[eventLoop])) > 0) {
				fprintf(stderr, "Error re-initialising reader for event %d (error %d), exiting.\n", eventLoop, returnVal);
				return 1;
			}
		} 

		// Output information about the current/last event if we're performing more than one event
		if (eventCount > 1) 
			if (silent == 0) {
				if (eventLoop > 0)  {
					printf("Completed work for event %d, packet loss for each port during this event was", eventCount -1);
					for (int port = 0; port < reader->meta->numPorts; port++) printf(" %ld", eventPacketsLost[port]);
					printf(".\n\n\n");
				}
				printf("Beginning work on event %d at %s: packets %ld to %ld...\n", eventLoop, dateStr[eventLoop], startingPackets[eventLoop], startingPackets[eventLoop] + multiMaxPackets[eventLoop]);
				getStartTimeString(reader, stringBuff);
				printf("============ Event %d Information ===========\n", eventLoop);
				printf("Target Time:\t%s\t\tActual Time:\t%s\n", dateStr[eventLoop], stringBuff);
				printf("Target Packet:\t%ld\tActual Packet:\t%ld\n", startingPackets[eventLoop], reader->meta->lastPacket + 1);
				printf("Event Length:\t%fs\t\tPacket Count:\t%ld\n", eventSeconds[eventLoop], multiMaxPackets[eventLoop]);
				printf("MJD Time:\t%lf\n", lofar_get_packet_time_mjd(reader->meta->inputData[0]));
				printf("============= End Information ==============\n");
			}


		// Get the starting packet for output file names
		startingPacket = reader->meta->leadingPacket;

		// Open the output files for this event
		for (int out = 0; out < outputFilesCount; out++) {
			sprintf(workingString, outputFormat, out, dateStr[eventLoop], startingPacket);
			VERBOSE(if (verbose) printf("Testing output file for output %d @ %s\n", out, workingString));
			
			if (appendMode != 1 && access(workingString, F_OK) != -1) {
				fprintf(stderr, "Output file at %s already exists; exiting.\n", workingString);
				return 1;
			}
			
			if (callMockHdr) {
				sprintf(mockHdrCmd, "mockHeader -tstart %.9lf -nchans %d -nbits %d -tsamp %.9lf %s %s > /tmp/udp_reader_mockheader.log 2>&1", lofar_get_packet_time_mjd(reader->meta->inputData[0]), reader->meta->totalBeamlets, reader->meta->outputBitMode, sampleTime, mockHdrArg, workingString);
				dummy = system(mockHdrCmd);

				if (dummy != 0) fprintf(stderr, "Encountered error while calling mockHeader (%s), continuing with caution.\n", mockHdrCmd);
			}

			VERBOSE(if (verbose) printf("Opening file at %s\n", workingString));

			outputFiles[out] = fopen(workingString, "a");
			if (outputFiles[out] == NULL) {
				fprintf(stderr, "Output file at %s could not be created, exiting.\n", workingString);
				return 1;
			}
		}

		VERBOSE(if (verbose) printf("Begining data extraction loop for event %d\n", eventLoop));
		// While we receive new data for the current event,
		while ((returnVal = lofar_udp_reader_step_timed(reader, timing)) < 1) {

			CLICK(tock0);
			if (localLoops == 0) timing[0] = TICKTOCK(tick0, tock0) - timing[1]; // _file_reader_step or _reader_reuse does first I/O operation; approximate the time here
			if (silent == 0) printf("Read complete for operation %d after %f seconds (I/O: %lf, MemOps: %lf), return value: %d\n", loops, TICKTOCK(tick0, tock0), timing[0], timing[1], returnVal);
			
			totalReadTime += timing[0];
			totalOpsTime += timing[1];

			// Write out the desired amount of packets; cap if needed.
			packetsToWrite = reader->meta->packetsPerIteration;
			if (multiMaxPackets[eventLoop] < packetsToWrite) packetsToWrite = multiMaxPackets[eventLoop];

			CLICK(tick0);
			
			fftwAndDetect(reader, forward, backward, intermediate, output, stokesIOutput, packetsPerIteration, UDPNTIMESLICE, reader->meta->totalBeamlets, processingFactor);
			#ifndef BENCHMARKING
			VERBOSE(printf("Writing %ld bytes (%ld packets) to disk for output %d...\n", reader->meta->totalBeamlets * packetsToWrite * UDPNTIMESLICE, packetsToWrite, 0));
			fwrite(stokesIOutput, sizeof(float), reader->meta->totalBeamlets * packetsToWrite * UDPNTIMESLICE, outputFiles[0]);
			#endif

			packetsWritten += packetsToWrite;
			packetsProcessed += reader->meta->packetsPerIteration;

			CLICK(tock0);
			totalWriteTime += TICKTOCK(tick0, tock0);
			if (silent == 0) {
				timing[0] = 9.;
				timing[1] = 0.;
				printf("FFT Ops and Disk writes completed for operation %d after %f seconds.\n", loops, TICKTOCK(tick0, tock0));
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
				break;
			}

			#ifdef __SLOWDOWN
			sleep(1);
			#endif
			CLICK(tick0);
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
	for (int i = 0; i < 2; i++) {
		fftwf_free(reader->meta->outputData[i]);
		reader->meta->outputData[i] = calloc(1, sizeof(char));
	}

	lofar_udp_reader_cleanup(reader);
	if (silent == 0) printf("Reader cleanup performed successfully.\n");

	// Free our malloc'd objects
	for (int i =0; i < eventCount; i++) free(dateStr[i]);
	free(dateStr);
	free(multiMaxPackets);
	free(startingPackets);
	fftwf_destroy_plan(forward[0]);
	fftwf_destroy_plan(forward[1]);
	fftwf_destroy_plan(backward[0]);
	fftwf_destroy_plan(backward[1]);

	if (silent == 0) printf("CLI memory cleaned up successfully. Exiting.\n");
	return 0;
}


void chirpKernel(float *taperArray, int totalBeamlets, int chanFactor, float fch1, float foff, int nbin) {
	// doi:10.1088/1009-9271/6/S2/11
	float chanbw = foff / chanFactor;
	float freq;
	for (int beamlet = 0; beamlet < totalBeamlets * chanFactor; beamlet++) {
		freq = fch1 + beamlet * chanbw;
		taperArray[beamlet] = 1.0 / sqrt( 1.0 + pow(freq / (0.47 * chanbw), 80)) / nbin;
	}
}

// Heavily based on transpose_unpadd_and_detect from CDMT, Bassa et al.
void fftwAndDetect(lofar_udp_reader *reader, fftwf_plan *forward, fftwf_plan *backward, fftwf_complex **intermediate, fftwf_complex **output, float *stokesOutput, int nbin, int nfft, int nsub, int processingFactor) {

	// Forward FFT
	printf("Forward FFT\n");
	for (int i = 0; i < 2; i++)
		fftwf_execute(forward[i]);

	// Reorder
	printf("Reorder 1\n");
	reorderSpectrum(intermediate, nbin, nfft * nsub);


	// Chirp


	// Reorder
	printf("Reorder 2\n");
	reorderSpectrum(intermediate, nbin / processingFactor, processingFactor * nfft * nsub);

	// Backward FFT
	printf("Backward\n");
	for (int i = 0; i < 2; i++)
		fftwf_execute(backward[i]);

	// Detect
	printf("Detect\n");
	detect(nbin / processingFactor, processingFactor, nfft, reader->meta->totalBeamlets, output, stokesOutput);
}


void detect(int nbin, int processingFactor, int nfft, int nsubbands, fftwf_complex **output, float *stokesOutput) {
	#pragma omp parallel for
	for (int ibin = 0; ibin < nbin / processingFactor; ibin++) {
		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 122
		#endif
		for (int ichan = 0; ichan < processingFactor; ichan++) {
			#ifdef __INTEL_COMPILER
			#pragma omp simd
			#else
			#pragma GCC unroll 16
			#endif
			for (int ifft = 0; ifft < nfft; ifft++) {
				for (int isub = 0; isub < nsubbands; isub++) {
					int voltIdx = ibin+ichan*nbin+(nsubbands-isub-1)*nbin*processingFactor+ifft*nbin*processingFactor*nsubbands;
					int stokesIdx = (processingFactor-ichan-1)+isub*processingFactor+nsubbands*processingFactor*(ibin+nbin*ifft);

					stokesOutput[stokesIdx] = output[0][voltIdx][0] * output[0][voltIdx][0] + output[0][voltIdx][1] * output[0][voltIdx][1]  + \
									output[1][voltIdx][0] * output[1][voltIdx][0] + output[1][voltIdx][1] * output[1][voltIdx][1];
				}
			}
		}
	}
}


void reorderSpectrum(fftwf_complex **workingArr, int nbin, int nruns) {
	fftwf_complex tempVal[2];
	int halfBin = nbin / 2, k;

	#pragma omp parallel for
	for (int i = 0; i < nbin; i++) {
		
		if (i < 0.5 * nbin) {
			k = i + halfBin;
		} else{
			k = i - halfBin;
		}
		
		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int j = 0; j < nruns; j++) {
			int l = i + nbin * j;
			int m = k + nbin * j;

			for (int pol = 0; pol < 2; pol++) {
				for (int ax = 0; ax < 2; ax++) {
					tempVal[pol][ax] = workingArr[pol][l][ax];
				}
			}

			for (int pol = 0; pol < 2; pol++) {
				for (int ax = 0; ax < 2; ax++) {
					workingArr[pol][l][ax] = workingArr[pol][m][ax];
					workingArr[pol][m][ax] = tempVal[pol][ax];
				}
			}
		}
	}
}
