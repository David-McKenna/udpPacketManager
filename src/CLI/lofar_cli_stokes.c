#include "lofar_cli_meta.h"

// Import FFTW3 for channelisation
#include "fftw3.h"

// Import omp for parallelisaing the channelisation/downsamling operations
#include <omp.h>

// Constant to define the length of the timing variable array
#define TIMEARRLEN 7

#define quickdump(filename, arr, numele) {FILE *ptr = fopen(filename, "wb"); fwrite(arr, numele, 1, ptr); fclose(ptr);}

// Output Stokes Modes
typedef enum stokes_t {
	STOKESI = 1,
	STOKESQ = 2,
	STOKESU = 4,
	STOKESV = 8
} stokes_t;

void helpMessages() {
	printf("LOFAR Stokes Data extractor (CLI v%s, lib v%s)\n\n", UPM_CLI_VERSION, UPM_VERSION);
	printf("Usage: ./lofar_stokes_extractor <flags>");

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
	printf("-S: <iters>     Break into a new file every N given iterations (default: infinite, never break)\n");
	printf("-e: <fileName>	Specify a file of events to extract; newline separated start time and durations in seconds. Events must not overlap.\n");
	printf("-p: <mode>		Processing mode, options listed below (default: 0)\n");
	printf("-r:		Replay the previous packet when a dropped packet is detected (default: pad with 0 values)\n");
	printf("-z:		Change to the alternative clock used for modes 4/6 (160MHz clock) (default: False)\n");
	printf("-q:		Enable silent mode for the CLI, don't print any information outside of library error messes (default: False)\n");
	printf("-f:		Append files if they already exist (default: False, exit if exists)\n");
	printf("-T: <threads>	OpenMP Threads to use during processing (8+ highly recommended, default: %d)\n", OMP_THREADS);
	printf("-c: <factor>    Channelisation factor to apply when processing data (default: disabled == 1)\n");
	printf("-d <factor>     Temporal downsampling to apply when processing data (default: disabled == 1)\n");
	printf("-D              Apply temporal downsampling to spectral data (slower, but higher quality (default: disabled)\n");

	VERBOSE(printf("-v:		Enable verbose output (default: False)\n");
		        printf("-V:		Enable highly verbose output (default: False)\n"));

	processingModes();

}

void CLICleanup(int eventCount, char **dateStr, long *startingPackets, long *multiMaxPackets, float *eventSeconds,
                lofar_udp_config *config, lofar_udp_io_write_config *outConfig, fftwf_complex *X, fftwf_complex *Y) {

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

	if (X != NULL) {
		fftwf_free(X);
	}
	if (Y != NULL) {
		fftwf_free(Y);
	}
}

void reorderData(fftwf_complex (*x), fftwf_complex (*y), size_t bins, size_t channels, float scale) {
	fftwf_complex *data[] = { x, y };

	const int copySize = (bins / 2) * sizeof(fftwf_complex);

	#pragma omp parallel for default(shared)
	for (int i = 0; i < 2; i++) {
		fftwf_complex *workingPtr = data[i];
		fftwf_complex *tmpBuffer = fftwf_alloc_complex(bins / 2);

		for (size_t channel = 0; channel < channels; channel++) {
			size_t inputIdx = channel * bins;
			size_t outputIdx = inputIdx + (bins / 2);

			if ((bins / 2) > 1) {
				memcpy(tmpBuffer, workingPtr[inputIdx], copySize);
				memmove(workingPtr[inputIdx], workingPtr[outputIdx], copySize);
				memcpy(workingPtr[outputIdx], tmpBuffer, copySize);
			}

			if (scale != 1.0) {
				#pragma omp simd
				for (size_t sample = 0; sample < bins; sample++) {
					workingPtr[inputIdx + sample][0] *= scale;
					workingPtr[inputIdx + sample][1] *= scale;
				}
			}


		}
		fftwf_free(tmpBuffer);
	}
}


void
transposeDetect(fftwf_complex *X, fftwf_complex *Y, float **outputs, size_t mbin, size_t nfft, size_t channelisation, size_t nsub, size_t channelDownsample,
                int stokesFlags) {
	float accumulator[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (channelDownsample < 1) {
		channelDownsample = 1;
	}

	const size_t outputNchan = nsub * channelisation / channelDownsample;

	printf("Detecting for: %zu, %zu, %zu, %zu\n", mbin, channelisation, nsub, channelDownsample);

	#pragma omp parallel for default(shared) private(accumulator)
	for (size_t sub = 0; sub < nsub; sub++) {
		for (size_t fft = 0; fft < nfft; fft++) {
			for (size_t sample = 0; sample < mbin; sample++) {
				ARR_INIT(accumulator, 4, 0.0f);

				size_t accumulations = 0;

				#pragma omp simd //nontemporal(outputs)
				for (size_t chan = 0; chan < channelisation; chan++) {

					// Input is time major
					//         curr     fft offset     channel offset         size per channel
					size_t inputIdx = sample + (chan * mbin) + (fft * mbin * channelisation) + (sub * channelisation) * (mbin * nfft);

					if (stokesFlags & STOKESI) {
						accumulator[0] += stokesI(X[inputIdx][0], X[inputIdx][1], Y[inputIdx][0], Y[inputIdx][1]);
					}
					if (stokesFlags & STOKESQ) {
						accumulator[1] += stokesQ(X[inputIdx][0], X[inputIdx][1], Y[inputIdx][0], Y[inputIdx][1]);
					}
					if (stokesFlags & STOKESU) {
						accumulator[2] += stokesU(X[inputIdx][0], X[inputIdx][1], Y[inputIdx][0], Y[inputIdx][1]);
					}
					if (stokesFlags & STOKESV) {
						accumulator[3] += stokesV(X[inputIdx][0], X[inputIdx][1], Y[inputIdx][0], Y[inputIdx][1]);
					}

					if (++accumulations == channelDownsample) {
						// Output is channel major
						//          curr                         total channels         size per time sample
						size_t outputIdx = (chan / channelDownsample) + (sub * channelisation / channelDownsample) + (outputNchan * (sample + mbin * fft));
						size_t outputArr = 0;
						if (stokesFlags & STOKESI) {
							outputs[outputArr][outputIdx] = accumulator[0];
							accumulator[0] = 0.0f;
							outputArr++;
						}
						if (stokesFlags & STOKESQ) {
							outputs[outputArr][outputIdx] = accumulator[1];
							accumulator[1] = 0.0f;
							outputArr++;
						}
						if (stokesFlags & STOKESU) {
							outputs[outputArr][outputIdx] = accumulator[2];
							accumulator[2] = 0.0f;
							outputArr++;
						}
						if (stokesFlags & STOKESV) {
							outputs[outputArr][outputIdx] = accumulator[3];
							accumulator[3] = 0.0f;
							outputArr++;
						}
						accumulations = 0;
					}
				}
			}
		}
	}
}

void temporalDownsample(float **data, size_t numOutputs, size_t nbin, size_t nchans, size_t downsampleFactor) {

	if (downsampleFactor < 2) {
		// Nothing to do
		return;
	}

	printf("Dwonsampling for: %zu, %zu, %zu\n", nchans, nbin, downsampleFactor);

	#pragma omp parallel for default(shared)
	for (size_t output = 0; output < numOutputs; output++) {
		for (size_t sub = 0; sub < nchans; sub++) {
			size_t accumulations = 0;
			float accumulator = 0.0f;

			#pragma omp simd //nontemporal(data)
			for (size_t sample = 0; sample < nbin; sample++) {
				// Input is time major
				//         curr   size per sample
				size_t inputIdx = sub + (sample * nchans);

				accumulator += data[output][inputIdx];

				if (++accumulations == downsampleFactor) {
					// Output is channel major
					//          curr          size per time sample
					size_t outputIdx = sub + (sample / downsampleFactor) * nchans;
					data[output][outputIdx] = accumulator;
					accumulations = 0;
					accumulator = 0.0f;
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {

	// Set up input local variables
	int inputOpt, input = 0;
	float seconds = 0.0f;
	char inputTime[256] = "", eventsFile[DEF_STR_LEN] = "", stringBuff[128] = "", inputFormat[DEF_STR_LEN] = "";
	int silent = 0, returnCounter = 0, eventCount = 0, inputProvided = 0, outputProvided = 0;
	long maxPackets = -1, startingPacket = -1, splitEvery = LONG_MAX;
	int clock200MHz = 1;
	FILE *eventsFilePtr;

	lofar_udp_config *config = calloc(1, sizeof(lofar_udp_config));
	lofar_udp_calibration *cal = calloc(1, sizeof(struct lofar_udp_calibration));
	lofar_udp_io_write_config *outConfig = lofar_udp_io_alloc_write();

	char *headerBuffer = NULL;

	if (config == NULL || outConfig == NULL || cal == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate memory for configuration structs (something has gone very wrong...), exiting.\n");
		FREE_NOT_NULL(config); FREE_NOT_NULL(outConfig); FREE_NOT_NULL(cal);
		return 1;
	}

	(*config) = lofar_udp_config_default;
	(*cal) = lofar_udp_calibration_default;
	config->calibrationConfiguration = cal;

	(*outConfig) = lofar_udp_io_write_config_default;

	// Set up reader loop variables
	int loops = 0, localLoops = 0, returnValMeta = 0, returnVal;
	long packetsProcessed = 0, packetsWritten = 0, eventPacketsLost[MAX_NUM_PORTS], packetsToWrite;

	// Timing variables
	double timing[TIMEARRLEN] = { 0.0 }, totalReadTime = 0., totalOpsTime = 0., totalWriteTime = 0., totalMetadataTime = 0., totalChanTime = 0., totalDetectTime = 0., totalDownsampleTime = 0.;
	ARR_INIT(timing, TIMEARRLEN, 0.0);
	struct timespec tick, tick0, tick1, tock, tock0, tock1, tickChan, tockChan, tickDown, tockDown, tickDetect, tockDetect;

	// Malloc'd variables: need to be free'd later.
	long *startingPackets = NULL, *multiMaxPackets = NULL;
	float *eventSeconds = NULL;
	char **dateStr = NULL; // Sub elements need to be free'd too.

	char *endPtr, flagged = 0;

	size_t channelisation = 1, downsampling = 1, spectralDownsample = 0;
	fftwf_complex *intermediateX = NULL;
	fftwf_complex *intermediateY = NULL;
	fftwf_plan fftForwardX;
	fftwf_plan fftForwardY;
	fftwf_plan fftBackwardX;
	fftwf_plan fftBackwardY;
	int stokesParameters = 0, numStokes = 0;

	// Standard ugly input flags parser
	while ((inputOpt = getopt(argc, argv, "zrqfvVi:o:m:M:I:u:t:s:S:e:p:a:n:b:c:d:k:T:DP:")) != -1) {
		input = 1;
		switch (inputOpt) {

			case 'i':
				if (strncpy(inputFormat, optarg, DEF_STR_LEN - 1) != inputFormat) {
					fprintf(stderr, "ERROR: Failed to store input data file format, exiting.\n");
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
					return 1;
				}
				inputProvided = 1;
				break;


			case 'o':
				if (lofar_udp_io_write_parse_optarg(outConfig, optarg) < 0) {
					helpMessages();
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
					return 1;
				}
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
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
					return 1;
				}
				break;

			case 'u':
				config->numPorts = strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 't':
				if (strncpy(inputTime, optarg, 255) != inputTime) {
					fprintf(stderr, "ERROR: Failed to copy start time from input, exiting.\n");
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
					return 1;
				}
				break;

			case 's':
				seconds = strtof(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'S':
				splitEvery = strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'e':
				if (strncpy(eventsFile, optarg, DEF_STR_LEN) != eventsFile) {
					fprintf(stderr, "ERROR: Failed to copy events file from input, exiting.\n");
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
					return 1;
				}
				break;

			case 'b':
				if (sscanf(optarg, "%d,%d", &(config->beamletLimits[0]), &(config->beamletLimits[1])) < 0) {
					fprintf(stderr, "ERROR: Failed to scan input beamlets, exiting.\n");
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
					return 1;
				}
				break;

			case 'r':
				config->replayDroppedPackets = 1;
				break;

			case 'c':
				channelisation = strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'd':
				downsampling = strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
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
				config->ompThreads = strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'D':
				spectralDownsample = 1;
				break;

			case 'P':
				if (numStokes > 0) {
					fprintf(stderr, "ERROR: -P flag has been parsed more than once. Exiting.\n");
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
					return -1;
				}
				if (strchr(optarg, 'I') != NULL) {
					numStokes += 1;
					stokesParameters += STOKESI;
				}
				if (strchr(optarg, 'Q') != NULL) {
					numStokes += 1;
					stokesParameters += STOKESQ;
				}
				if (strchr(optarg, 'U') != NULL) {
					numStokes = 1;
					stokesParameters += STOKESU;
				}
				if (strchr(optarg, 'V') != NULL) {
					numStokes += 1;
					stokesParameters += STOKESV;
				}
				break;



				// Silence GCC warnings, fall-through is the desired behaviour
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic push

				// Handle edge/error cases
			case '?':
				if ((optopt == 'i') || (optopt == 'o') || (optopt == 'm') || (optopt == 'u') || (optopt == 't') ||
				    (optopt == 's') || (optopt == 'e') || (optopt == 'p') || (optopt == 'a') || (optopt == 'c') ||
				    (optopt == 'd') || (optopt == 'P')) {
					fprintf(stderr, "Option '%c' requires an argument.\n", optopt);
				} else {
					fprintf(stderr, "Option '%c' is unknown or encountered an error.\n", optopt);
				}

			case 'h':
			default:

#pragma GCC diagnostic pop

				helpMessages();
				CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
				return 1;

		}
	}

	config->processingMode = 35;

	if (flagged) {
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}

	if (!input) {
		fprintf(stderr, "ERROR: No inputs provided, exiting.\n");
		helpMessages();
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}

	if (!inputProvided) {
		fprintf(stderr, "ERROR: An input was not provided, exiting.\n");
		helpMessages();
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}

	if (channelisation < 1 || ( channelisation > 1 && channelisation % 2) != 0) {
		fprintf(stderr, "ERROR: Invalid channelisation factor (less than 1, non-factor of 2)\n");
		helpMessages();
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}

	if (downsampling < 1) {
		fprintf(stderr, "ERROR: Invalid downsampling factor (less than 1)\n");
		helpMessages();
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}

	if (spectralDownsample) {
		spectralDownsample = downsampling;
		downsampling = 1;
	}

	int FFTBIN = 64;
	if (((config->packetsPerIteration * UDPNTIMESLICE / FFTBIN) % (channelisation * downsampling)) != 0) {
		fprintf(stderr, "ERROR: Number of samples needed per iterations must be a multiple of the product of the channelisation factor %ld and downsampling factor %ld (%ld) exiting.\n", config->packetsPerIteration * UDPNTIMESLICE, channelisation, downsampling, channelisation * downsampling);
		helpMessages();
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}
	int numFft = (config->packetsPerIteration * UDPNTIMESLICE) / FFTBIN / (channelisation * (spectralDownsample ?: 1));

	/*
	if (((long long) (channelisation * downsampling)) % config->packetsPerIteration != 0) {
		fprintf(stderr, "ERROR: Number of packets per iteration is not evenly divisible by the number of samples needed to process at the given channelisation factor %ld and downsampling factor %ld (%ld, %ld remainder), exiting.\n", channelisation, downsampling, channelisation * downsampling, (channelisation * downsampling) % config->packetsPerIteration);
		helpMessages();
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}
	*/

	if (lofar_udp_io_read_parse_optarg(config, inputFormat) < 0) {
		helpMessages();
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}

	// Split-every and events are considered incompatible to simplify the implementation
	if (splitEvery != LONG_MAX && strcmp(eventsFile, "") != 0) {
		fprintf(stderr, "ERROR: Events file and split-every functionality cannot be combined, exiting.\n");
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}

	// DADA outputs should not be fragments, or require writing to disk (mockHeader deleted files and rewrites...)
	if (config->readerType == DADA_ACTIVE && strcmp(eventsFile, "") != 0) {
		fprintf(stderr, "ERROR: DADA output does not support events parsing, exiting.\n");
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}

	if (!outputProvided) {
		fprintf(stderr, "ERROR: An output was not provided, exiting.\n");
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}

	if (config->calibrateData != NO_CALIBRATION && strcmp(config->metadata_config.metadataLocation, "") == 0) {
		fprintf(stderr, "ERROR: Data calibration was enabled, but metadata was not provided. Exiting.\n");
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
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
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}


	if (silent == 0) {
		printf("LOFAR Stokes Data extractor (v%s, lib v%s)\n\n", UPM_CLI_VERSION, UPM_VERSION);
		printf("=========== Given configuration ===========\n");

		printf("Input:\t%s\n", inputFormat);
		for (int i = 0; i < config->numPorts; i++) printf("\t\t%s\n", config->inputLocations[i]);
		printf("Output File: %s\n\n", outConfig->outputFormat);

		printf("Packets/Gulp:\t%ld\t\t\tPorts:\t%d\n\n", config->packetsPerIteration, config->numPorts);
		VERBOSE(printf("Verbose:\t%d\n", config->verbose););
		printf("Proc Mode:\t%03d\t\t\tReader:\t%d\n\n", config->processingMode, config->readerType);
		printf("Beamlet limits:\t%d, %d\n\n", config->beamletLimits[0], config->beamletLimits[1]);
	}

	// If given an events file,
	if (strcmp(eventsFile, "") != 0) {

		// Try to read it
		eventsFilePtr = fopen(eventsFile, "r");
		if (eventsFilePtr == NULL) {
			fprintf(stderr, "Unable to open events file at %s, exiting.\n", eventsFile);
			CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
			return 1;
		}

		// The first line should be an int of the amount of events we need to process
		returnCounter = fscanf(eventsFilePtr, "%d", &eventCount);
		if (returnCounter != 1 || eventCount < 1) {
			fprintf(stderr, "Unable to parse events file (got %d as number of events), exiting.\n", eventCount);
			CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
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
				CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
				return 1;
			}

			// Determine the packet corresponding to the initial time and the amount of packets needed to observe for  the length of the event
			startingPackets[idx] = lofar_udp_time_get_packet_from_isot(stringBuff, clock200MHz);
			if (startingPackets[idx] == 1) {
				fprintf(stderr, "ERROR: Failed to get starting packet for event %d, exiting.\n", idx);
				helpMessages();
				CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
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
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
					return 1;
				}

				if (startingPackets[idx] < startingPackets[idx - 1] + multiMaxPackets[idx - 1]) {
					fprintf(stderr,
					        "Events %d and %d overlap, please combine them or ensure there is some buffer time between them, exiting.",
					        idx, idx - 1);
					CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
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
				CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
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

	// Pass forward output channelisation and downsampling factors
	config->metadata_config.externalChannelisation = channelisation;
	config->metadata_config.externalDownsampling = downsampling;

	if (channelisation > 1) {
		if (silent == 0) { printf("Computing channelisation chirp\n"); }

		if (spectralDownsample) {
			channelisation *= spectralDownsample;
		}
	}

	if (fftwf_init_threads() == 0) {
		fprintf(stderr, "ERROR: Failed to initialise multi-threaded FFTWF.\n");
	}
	fftwf_plan_with_nthreads(config->ompThreads);
	omp_set_num_threads(config->ompThreads);



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
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}

	// Sanity check that we were passed the correct clock bit
	if (((lofar_source_bytes *) &(reader->meta->inputData[0][1]))->clockBit != (unsigned int) clock200MHz) {
		fprintf(stderr,
		        "ERROR: The clock bit of the first packet does not match the clock state given when starting the CLI. Add or remove -c from your command. Exiting.\n");
		CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
		return 1;
	}

	// Channelisation setup
	int nbinIn = FFTBIN * channelisation;
	int mbinOut = nbinIn / channelisation;
	int nSubIn = reader->meta->totalProcBeamlets;
	int nChanOut = reader->meta->totalProcBeamlets * channelisation;
	size_t fftBufferElements = 0, fftBufferSize = 0;

	fftwf_complex * in1 = NULL;
	fftwf_complex * in2 = NULL;
	printf("%d, %d, %d, %d, %d\n", numFft, nbinIn, mbinOut, nSubIn, nChanOut);

	float *outputStokes[MAX_OUTPUT_DIMS];

	if (channelisation > 0) {
		fftBufferElements = numFft * nbinIn * nSubIn;
		fftBufferSize = fftBufferElements * sizeof(fftwf_complex);
		in1 = fftwf_alloc_complex(fftBufferElements);
		in2 = fftwf_alloc_complex(fftBufferElements);

		intermediateX = fftwf_alloc_complex(fftBufferElements);
		intermediateY = fftwf_alloc_complex(fftBufferElements);
		if (intermediateX == NULL || intermediateY == NULL) {
			fprintf(stderr, "ERROR: Failed to allocate output FFTW buffers, exiting.\n");
			CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);
			return -1;
		}

		fftForwardX = fftwf_plan_many_dft(1, &nbinIn, numFft * nSubIn, in1, &nbinIn, 1, nbinIn, intermediateX, &nbinIn, 1, nbinIn, FFTW_FORWARD, FFTW_ESTIMATE_PATIENT);
		fftForwardY = fftwf_plan_many_dft(1, &nbinIn, numFft * nSubIn, in2, &nbinIn, 1, nbinIn, intermediateY, &nbinIn, 1, nbinIn, FFTW_FORWARD, FFTW_ESTIMATE_PATIENT);

		fftBackwardX = fftwf_plan_many_dft(1, &mbinOut, numFft * nChanOut, intermediateX, &mbinOut, 1, mbinOut, in1, &mbinOut, 1, mbinOut, FFTW_BACKWARD, FFTW_ESTIMATE_PATIENT);
		fftBackwardY = fftwf_plan_many_dft(1, &mbinOut, numFft * nChanOut, intermediateY, &mbinOut, 1, mbinOut, in2, &mbinOut, 1, mbinOut, FFTW_BACKWARD, FFTW_ESTIMATE_PATIENT);
	}

	size_t outputFloats = reader->packetsPerIteration * UDPNTIMESLICE * reader->meta->totalProcBeamlets / (spectralDownsample ?: 1);
	printf("floats %ld, %ld, %ld, %ld\n", outputFloats, reader->packetsPerIteration * UDPNTIMESLICE * reader->meta->totalProcBeamlets, spectralDownsample, (spectralDownsample ?: 1));
	for (int i = 0; i < numStokes; i++) {
		outputStokes[i] = calloc(outputFloats, sizeof(float));
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
				returnValMeta = (returnValMeta < 0 && returnValMeta > -6) ? returnValMeta : -6;
				break;
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


		// Get the starting packet for output file names, fix the packets per iteration if we dropped packets on the last iter
		startingPacket = reader->meta->leadingPacket;
		reader->meta->packetsPerIteration = reader->packetsPerIteration;
		if ((returnVal = lofar_udp_io_write_setup_helper(outConfig, reader->meta, eventLoop)) < 0) {
			fprintf(stderr, "ERROR: Failed to open a new output file (%d, errno %d: %s), breaking.\n", returnVal, errno, strerror(errno));
			returnValMeta = (returnValMeta < 0 && returnValMeta > -7) ? returnValMeta : -7;
			break;
		}

		outConfig->metadata = reader->metadata;


		VERBOSE(if (config->verbose) { printf("Beginning data extraction loop for event %d\n", eventLoop); });
		// While we receive new data for the current event,
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
				printf("Read complete for operation %d after %f seconds (I/O: %lf, MemOps: %lf), return value: %d\n",
				       loops, TICKTOCK(tick0, tock0), timing[0], timing[1], returnVal);
			}

			totalReadTime += timing[0];
			totalOpsTime += timing[1];

			// Write out the desired amount of packets; cap if needed.
			packetsToWrite = reader->meta->packetsPerIteration;
			if (splitEvery == LONG_MAX && multiMaxPackets[eventLoop] < packetsToWrite) {
				packetsToWrite = multiMaxPackets[eventLoop];
			}

			printf("Begin channelisation %zu %zu %zu %d\n", channelisation, spectralDownsample, downsampling, numFft);
			// Perform channelisation, temporal downsampling as needed
			if (channelisation > 1) {
				CLICK(tickChan);
				memcpy(in1, reader->meta->outputData[0], fftBufferSize);
				memcpy(in2, reader->meta->outputData[1], fftBufferSize);
				printf("Execute\n");
				fftwf_execute(fftForwardX);
				fftwf_execute(fftForwardY);
				//quickdump("test_0", in1, fftBufferSize);
				//quickdump("test_1", intermediateX, fftBufferSize);

				printf("Reorder 0\n");
				reorderData(intermediateX, intermediateY, nbinIn, numFft * nSubIn, 1.0 / (float) nbinIn);
				printf("Reorder 1\n");
				//windowData();
				printf("Reorder 2\n");
				//quickdump("test_2", intermediateX, fftBufferSize);
				printf("Reorder 3\n");
				reorderData(intermediateX, intermediateY, mbinOut, numFft * nSubIn * channelisation, 1.0);
				printf("Reorder 4\n");
				//quickdump("test_3", intermediateX, fftBufferSize);
				printf("Reorder 5\n");
				ARR_INIT(((float *) in1), fftBufferElements * sizeof(fftwf_complex) / sizeof(float), 0.0f);
				printf("Reorder 6\n");
				ARR_INIT(((float *) in2), fftBufferElements * sizeof(fftwf_complex) / sizeof(float), 0.0f);
				printf("Execute\n");
				fftwf_execute(fftBackwardX);
				fftwf_execute(fftBackwardY);
				//quickdump("test_4", in1, nbinIn * nSubIn * 2 * sizeof(float));
				CLICK(tockChan);
				timing[4] = TICKTOCK(tickChan, tockChan);
				totalChanTime += timing[4];
				CLICK(tickDetect);
				printf("Detect\n");
				transposeDetect(in1, in2, outputStokes, mbinOut, numFft, channelisation, nSubIn, spectralDownsample, stokesParameters);
				//quickdump("test_5", outputStokes[0], 4 * mbinOut * channelisation * nSubIn / (spectralDownsample ?:1));
				ARR_INIT(((float *) intermediateX), fftBufferElements * sizeof(fftwf_complex) / sizeof(float), 0.0f);
				ARR_INIT(((float *) intermediateY), fftBufferElements * sizeof(fftwf_complex) / sizeof(float), 0.0f);
			} else {
				CLICK(tickDetect);
				printf("Detectsolo\n");
				transposeDetect((fftwf_complex *) reader->meta->outputData[0], (fftwf_complex *) reader->meta->outputData[1], outputStokes,
				                packetsToWrite * UDPNTIMESLICE, 1, 1, nSubIn, 1, stokesParameters);
			}

			//return 0;

			CLICK(tockDetect);
			timing[5] = TICKTOCK(tickDetect, tockDetect);
			totalDetectTime += timing[5];

			printf("Begin downsampling\n");
			if (downsampling > 1) {
				CLICK(tickDown);
				temporalDownsample(outputStokes, numStokes, mbinOut * numFft, nChanOut, downsampling);
				CLICK(tockDown);
				timing[6] = TICKTOCK(tickDown, tockDown);
				totalDownsampleTime += timing[6];
			}

			for (int out = 0; out < numStokes; out++) {
				printf("Enter loop\n");
				if (outConfig->metadata != NULL) {
					if (outConfig->metadata->type != NO_META) {
						CLICK(tick1);
						if ((returnVal = lofar_udp_metadata_write_file(reader, outConfig, out, reader->metadata, headerBuffer, 4096 * 8, localLoops == 0)) <
						    0) {
							fprintf(stderr, "ERROR: Failed to write header to output (%d, errno %d: %s), breaking.\n", returnVal, errno, strerror(errno));
							returnValMeta = (returnValMeta < 0 && returnValMeta > -4) ? returnValMeta : -4;
							break;
						}
						CLICK(tock1);
						timing[2] += TICKTOCK(tick1, tock1);
					}
				}
				printf("Sizing\n");

				CLICK(tick0);
				size_t outputLength = outputFloats * sizeof(float) / downsampling;
				VERBOSE(printf("Writing %ld bytes (%ld packets) to disk for output %d...\n",
				               outputLength, packetsToWrite, out));
				size_t outputWritten;
				printf("Writing %ld...\n", outputLength);
				if ((outputWritten = lofar_udp_io_write(outConfig, out, (char *) outputStokes[out],
				                                        outputLength)) != outputLength) {
					fprintf(stderr, "ERROR: Failed to write data to output (%ld bytes/%ld bytes writen, errno %d: %s)), breaking.\n", outputWritten, outputLength,  errno, strerror(errno));
					returnValMeta = (returnValMeta < 0 && returnValMeta > -5) ? returnValMeta : -5;
					break;
				}
				CLICK(tock0);
				timing[3] += TICKTOCK(tick0, tock0);

			}
			printf("Splitting?\n");
			if (splitEvery != LONG_MAX && returnValMeta > -2) {
				if ((localLoops + 1) == splitEvery) {
					eventLoop += 1;


					// Close existing files
					for (int outp = 0; outp < numStokes; outp++) {
						lofar_udp_io_write_cleanup(outConfig, outp, 0);
					}

					// Open new files
					reader->meta->packetsPerIteration = reader->packetsPerIteration;
					if ((returnVal = lofar_udp_io_write_setup_helper(outConfig, reader->meta, eventLoop)) < 0) {
						fprintf(stderr, "ERROR: Failed to open new file are breakpoint reached (%d, errno %d: %s), breaking.\n", returnVal, errno, strerror(errno));
						returnValMeta = (returnValMeta < 0 && returnValMeta > -6) ? returnValMeta : -6;
						break;
					}

					localLoops = -1;
				}

			}

			totalMetadataTime += timing[2];
			totalWriteTime += timing[3];

			packetsWritten += packetsToWrite;
			packetsProcessed += reader->meta->packetsPerIteration;

			if (silent == 0) {
				if (outConfig->metadata != NULL) if (outConfig->metadata->type != NO_META) printf("Metadata processing for operation %d after %f seconds.\n", loops, timing[2]);
				printf("Disk writes completed for operation %d (%ld) after %f seconds.\n", loops, outputFloats * sizeof(float) / downsampling, timing[3]);
				printf("Detection completed for operation %d after %f seconds.\n", loops, timing[5]);
				if (channelisation) printf("Channelisation completed for operation %d after %f seconds.\n", loops, timing[4]);
				if (downsampling) printf("Temporal downsampling completed for operation %d after %f seconds.\n", loops, timing[6]);

				ARR_INIT(timing, TIMEARRLEN, 0.0);

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
		}

		for (int outp = 0; outp < numStokes; outp++) {
			lofar_udp_io_write_cleanup(outConfig, outp, 1);
		}

		// returnVal below -1 indicates we will not be given data on the next iteration, so gracefully exit with the known reason
		if (returnValMeta < -1) {
			printf("We've hit a termination return value (%d, %s), exiting.\n", returnValMeta,
			       exitReasons[abs(returnValMeta)]);
			break;
		}

	}

	fftwf_destroy_plan(fftForwardX);
	fftwf_destroy_plan(fftForwardY);
	fftwf_destroy_plan(fftBackwardX);
	fftwf_destroy_plan(fftBackwardY);
	fftwf_cleanup_threads();
	fftwf_cleanup();

	CLICK(tock);

	int droppedPackets = 0;
	long totalPacketLength = 0, totalOutLength = 0;

	// Print out a summary of the operations performed, this does not contain data read for seek operations
	if (silent == 0) {
		for (int port = 0; port < reader->meta->numPorts; port++)
			totalPacketLength += reader->meta->portPacketLength[port];
		for (int out = 0; out < numStokes; out++)
			totalOutLength += reader->meta->packetOutputLength[out];
		for (int port = 0; port < reader->meta->numPorts; port++)
			droppedPackets += reader->meta->portTotalDroppedPackets[port];

		printf("Reader loop exited (%d); overall process took %f seconds.\n", returnVal, TICKTOCK(tick, tock));
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
	CLICleanup(eventCount, dateStr, startingPackets, multiMaxPackets, eventSeconds, config, outConfig, intermediateX, intermediateY);

	if (silent == 0) { printf("CLI memory cleaned up successfully. Exiting.\n"); }
	return 0;
}