#include "lofar_cli_meta.h"

// Import FFTW3 for channelisation
#include "fftw3.h"

// Import omp for parallelisaing the channelisation/downsamling operations
#include <omp.h>

// Constant to define the length of the timing variable array
#define TIMEARRLEN 7

// Output Stokes Modes
typedef enum stokes_t {
	STOKESI = 1,
	STOKESQ = 2,
	STOKESU = 4,
	STOKESV = 8
} stokes_t;

void helpMessages() {
	printf("LOFAR Stokes extractor (CLI v%s, lib v%s)\n\n", UPM_CLI_VERSION, UPM_VERSION);
	printf("Usage: lofar_cli_stokes <flags>");

	printf("\n\n");


	printf("-C: <factor>    Channelisation factor to apply when processing data (default: disabled == 1)\n");
	printf("-d <factor>     Temporal downsampling to apply when processing data (default: disabled == 1)\n");
	printf("-D              Apply temporal downsampling to spectral data (slower, but higher quality (default: disabled)\n");

}

void CLICleanup(lofar_udp_config *config, lofar_udp_io_write_config *outConfig, int8_t *header, fftwf_complex *X, fftwf_complex *Y) {

	FREE_NOT_NULL(outConfig);
	FREE_NOT_NULL(config);
	FREE_NOT_NULL(header);


	if (X != NULL) {
		fftwf_free(X);
		X = NULL;
	}
	if (Y != NULL) {
		fftwf_free(Y);
		Y = NULL;
	}
}

void reorderData(fftwf_complex *x, fftwf_complex *y, size_t bins, size_t channels) {
	fftwf_complex *data[] = { x, y };
	fftwf_complex tmp;

	size_t inputIdx, outputIdx;

	for (int i = 0; i < 2; i++) {
		fftwf_complex *workingPtr = data[i];

		//#pragma omp parallel for default(shared) shared(workingPtr)
		for (size_t channel = 0; channel < channels; channel++) {
			for (size_t sample = 0; sample < (bins / 2); sample++) {
				inputIdx = sample + channel * bins;
				outputIdx = inputIdx + (bins / 2);

				tmp[0] = workingPtr[inputIdx][0];
				tmp[1] = workingPtr[inputIdx][1];
				workingPtr[inputIdx][0] = workingPtr[outputIdx][0];
				workingPtr[inputIdx][1] = workingPtr[outputIdx][1];
				workingPtr[outputIdx][0] = tmp[0];
				workingPtr[outputIdx][1] = tmp[1];

			}
		}
	}
}


void transposeDetect(fftwf_complex *X, fftwf_complex *Y, float **outputs, size_t mbin, size_t channelisation, size_t nsub, size_t channelDownsample, int stokesFlags) {
	float accumulator[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (channelDownsample < 1) {
		channelDownsample = 1;
	}

	size_t inputIdx, outputIdx, outputArr;
	size_t accumulations = 0;
	size_t outputNchan = nsub * channelisation / channelDownsample;

	printf("Detecting for: %zu, %zu, %zu, %zu\n", mbin, channelisation, nsub, channelDownsample);

	//#pragma omp parallel for default(shared) private(accumulator, accumulations, inputIdx, outputIdx, outputArr)
	for (size_t sub = 0; sub < nsub; sub++) {
		for (size_t sample = 0; sample < mbin; sample++) {
			ARR_INIT(accumulator, 4, 0.0f);

			accumulations = 0;

			//#pragma omp simd //nontemporal(outputs)
			for (size_t chan = 0; chan < channelisation; chan++) {

				// Input is time major
				//         curr     total samples      size per channel
				inputIdx = sample + (chan * mbin) + (sub * mbin * channelisation);

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
					outputIdx = (chan / channelDownsample) + (sub * channelisation / channelDownsample) + (outputNchan * sample);

					outputArr = 0;
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

void temporalDownsample(float **data, size_t numOutputs, size_t nbin, size_t nchans, size_t downsampleFactor) {

	if (downsampleFactor < 2) {
		// Nothing to do
		return;
	}

	size_t inputIdx, outputIdx;
	size_t accumulations = 0;
	float accumulator = 0.0f;

	VERBOSE(printf("Downsampling for: %zu, %zu, %zu\n", nchans, nbin, downsampleFactor));

	//#pragma omp parallel for default(shared) private(accumulator, accumulations, inputIdx, outputIdx)
	for (size_t output = 0; output < numOutputs; output++) {
		for (size_t sub = 0; sub < nchans; sub++) {
			accumulations = 0;
			accumulator = 0.0f;

			//#pragma omp simd //nontemporal(data)
			for (size_t sample = 0; sample < nbin; sample++) {
				// Input is time major
				//         curr   size per sample
				inputIdx = sub + (sample * nchans);

				accumulator += data[output][inputIdx];

				if (++accumulations == downsampleFactor) {
					// Output is channel major
					//          curr          size per time sample
					outputIdx = sub + (sample / downsampleFactor) * nchans;
					data[output][outputIdx] = accumulator;
					accumulations = 0;
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {

	// Set up input local variables
	int32_t inputOpt, input = 0;
	float seconds = 0.0f;
	char inputTime[256] = "", stringBuff[128] = "", inputFormat[DEF_STR_LEN] = "";
	int32_t silent = 0, inputProvided = 0, outputProvided = 0;
	int64_t maxPackets = -1, startingPacket = -1, splitEvery = LONG_MAX;
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
	int32_t loops = 0, localLoops = 0;
	int64_t returnVal, returnValMeta = 0;
	int64_t packetsProcessed = 0, packetsWritten = 0, packetsToWrite;

	// Timing variables
	double timing[TIMEARRLEN] = { 0.0 }, totalReadTime = 0., totalOpsTime = 0., totalWriteTime = 0., totalMetadataTime = 0., totalChanTime = 0., totalDetectTime = 0., totalDownsampleTime = 0.;
	ARR_INIT(timing, TIMEARRLEN, 0.0);
	struct timespec tick, tick0, tick1, tock, tock0, tock1, tickChan, tockChan, tickDown, tockDown, tickDetect, tockDetect;

	// strtol / option checks
	char *endPtr, flagged = 0;

	// FFTW strategy
	int32_t channelisation = 1, downsampling = 1, spectralDownsample = 0;
	fftwf_complex *intermediateX = NULL;
	fftwf_complex *intermediateY = NULL;
	fftwf_plan fftForwardX;
	fftwf_plan fftForwardY;
	fftwf_plan fftBackwardX;
	fftwf_plan fftBackwardY;
	int8_t stokesParameters = 0, numStokes = 0;

	// Standard ugly input flags parser
	while ((inputOpt = getopt(argc, argv, "hzrqfvVi:o:m:M:I:u:t:s:S:e:p:a:n:b:c:d:k:T:DP:")) != -1) {
		input = 1;
		switch (inputOpt) {

			case 'i':
				if (strncpy(inputFormat, optarg, DEF_STR_LEN - 1) != inputFormat) {
					fprintf(stderr, "ERROR: Failed to store input data file format, exiting.\n");
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
					return 1;
				}
				inputProvided = 1;
				break;


			case 'o':
				if (lofar_udp_io_write_parse_optarg(outConfig, optarg) < 0) {
					helpMessages();
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
					return 1;
				}
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
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
					return 1;
				}
				break;

			case 'u':
				config->numPorts = internal_strtoc(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 't':
				if (strncpy(inputTime, optarg, 255) != inputTime) {
					fprintf(stderr, "ERROR: Failed to copy start time from input, exiting.\n");
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
					return 1;
				}
				break;

			case 's':
				seconds = strtof(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'S':
				splitEvery = internal_strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'b':
				if (sscanf(optarg, "%hd,%hd", &(config->beamletLimits[0]), &(config->beamletLimits[1])) < 0) {
					fprintf(stderr, "ERROR: Failed to scan input beamlets, exiting.\n");
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
					return 1;
				}
				break;

			case 'r':
				config->replayDroppedPackets = 1;
				break;

			case 'C':
				channelisation = internal_strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'd':
				downsampling = internal_strtoi(optarg, &endPtr);
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
				config->ompThreads = internal_strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'D':
				spectralDownsample = 1;
				break;

			case 'P':
				if (numStokes > 0) {
					fprintf(stderr, "ERROR: -P flag has been parsed more than once. Exiting.\n");
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
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
				CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
				return 1;

		}
	}

	// Pre-set processing mode
	// TODO: Floating processing mode if chanellisation is disabled?
	config->processingMode = TIME_MAJOR_ANT_POL_FLOAT;

	if (flagged) {
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}

	if (!input) {
		fprintf(stderr, "ERROR: No inputs provided, exiting.\n");
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}

	if (!inputProvided) {
		fprintf(stderr, "ERROR: An input was not provided, exiting.\n");
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}

	if (spectralDownsample) {
		spectralDownsample = downsampling;
	}


	if ((int64_t) (channelisation * downsampling) != (config->packetsPerIteration * UDPNTIMESLICE)) {
		fprintf(stderr, "ERROR: Number of samples needed per iterations for channelisation factor %d and downsampling factor %d (%d) is larger than set number of packets per iteration (%ld), exiting.\n", channelisation, downsampling, channelisation * downsampling, config->packetsPerIteration);
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}

	/*
	if (((long long) (channelisation * downsampling)) % config->packetsPerIteration != 0) {
		fprintf(stderr, "ERROR: Number of packets per iteration is not evenly divisible by the number of samples needed to process at the given channelisation factor %ld and downsampling factor %ld (%ld, %ld remainder), exiting.\n", channelisation, downsampling, channelisation * downsampling, (channelisation * downsampling) % config->packetsPerIteration);
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}
	*/

	if (channelisation < 1 || ( channelisation > 1 && channelisation % 2) != 0) {
		fprintf(stderr, "ERROR: Invalid channelisation factor (less than 1, non-factor of 2)\n");
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}

	if (downsampling < 1) {
		fprintf(stderr, "ERROR: Invalid downsampling factor (less than 1)\n");
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}

	if (_lofar_udp_io_read_internal_lib_parse_optarg(config, inputFormat) < 0) {
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}

	if (!outputProvided) {
		fprintf(stderr, "ERROR: An output was not provided, exiting.\n");
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}

	if (config->calibrateData != NO_CALIBRATION && strcmp(config->metadata_config.metadataLocation, "") == 0) {
		fprintf(stderr, "ERROR: Data calibration was enabled, but metadata was not provided. Exiting.\n");
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
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
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}


	if (silent == 0) {
		printf("LOFAR Stokes Data extractor (v%s, lib v%s)\n\n", UPM_CLI_VERSION, UPM_VERSION);
		printf("=========== Given configuration ===========\n");

		printf("Input:\t%s\n", inputFormat);
		for (int8_t i = 0; i < config->numPorts; i++) printf("\t\t%s\n", config->inputLocations[i]);
		printf("Output File: %s\n\n", outConfig->outputFormat);

		printf("Packets/Gulp:\t%ld\t\t\tPorts:\t%d\n\n", config->packetsPerIteration, config->numPorts);
		VERBOSE(printf("Verbose:\t%d\n", config->verbose););
		printf("Proc Mode:\t%03d\t\t\tReader:\t%d\n\n", config->processingMode, config->readerType);
		printf("Beamlet limits:\t%d, %d\n\n", config->beamletLimits[0], config->beamletLimits[1]);
	}

	if (strcmp(inputTime, "") != 0) {
		startingPacket = lofar_udp_time_get_packet_from_isot(inputTime, clock200MHz);
		if (startingPacket == 1) {
			helpMessages();
			CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
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

	// Pass forward output channelisation and downsampling factors
	config->metadata_config.externalChannelisation = channelisation;
	config->metadata_config.externalDownsampling = downsampling;

	if (channelisation > 1) {
		if (silent == 0) { printf("Computing channelisation chirp\n"); }

		if (spectralDownsample) {
			channelisation *= downsampling;
		}
	}

	//if (fftwf_init_threads() == 0) {
	//	fprintf(stderr, "ERROR: Failed to initialise multi-threaded FFTWF.\n");
	//}
	//fftwf_plan_with_nthreads(config->ompThreads);
	//omp_set_num_threads(config->ompThreads);



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
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}

	// Sanity check that we were passed the correct clock bit
	if (((lofar_source_bytes *) &(reader->meta->inputData[0][1]))->clockBit != (unsigned int) clock200MHz) {
		fprintf(stderr,
		        "ERROR: The clock bit of the first packet does not match the clock state given when starting the CLI. Add or remove -c from your command. Exiting.\n");
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}

	if (reader->packetsPerIteration * UDPNTIMESLICE > INT32_MAX) {
		fprintf(stderr, "ERROR: Input FFT bins are too long (%ld vs %d), exiting.\n", reader->packetsPerIteration * UDPNTIMESLICE, INT32_MAX);
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}

	// Channelisation setup
	int32_t nbin = (int32_t) reader->packetsPerIteration * UDPNTIMESLICE;
	int32_t mbin = nbin / channelisation;
	int32_t nsub = reader->meta->totalProcBeamlets;
	int32_t nchan = reader->meta->totalProcBeamlets * channelisation;

	fftwf_complex * in1 = NULL;
	fftwf_complex * in2 = NULL;
	//printf("%d, %d, %d, %d\n", nbin, mbin, nsub, nchan);

	float *outputStokes[MAX_OUTPUT_DIMS];

	if (channelisation > 0) {
		in1 = fftwf_alloc_complex(nbin * nsub);
		in2 = fftwf_alloc_complex(nbin * nsub);

		intermediateX = fftwf_alloc_complex(nbin * nsub);
		intermediateY = fftwf_alloc_complex(nbin * nsub);
		if (intermediateX == NULL || intermediateY == NULL) {
			fprintf(stderr, "ERROR: Failed to allocate output FFTW buffers, exiting.\n");
			CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
			return -1;
		}

		fftForwardX = fftwf_plan_many_dft(1, &nbin, nsub, in1, &nbin, 1, nbin, intermediateX, &nbin, 1, nbin, FFTW_FORWARD, FFTW_ESTIMATE_PATIENT);
		fftForwardY = fftwf_plan_many_dft(1, &nbin, nsub, in2, &nbin, 1, nbin, intermediateY, &nbin, 1, nbin, FFTW_FORWARD, FFTW_ESTIMATE_PATIENT);

		fftBackwardX = fftwf_plan_many_dft(1, &mbin, nchan, intermediateX, &mbin, 1, mbin, in1, &mbin, 1, mbin, FFTW_BACKWARD, FFTW_ESTIMATE_PATIENT);
		fftBackwardY = fftwf_plan_many_dft(1, &mbin, nchan, intermediateY, &mbin, 1, mbin, in2, &mbin, 1, mbin, FFTW_BACKWARD, FFTW_ESTIMATE_PATIENT);
	}

	size_t outputFloats = reader->packetsPerIteration * UDPNTIMESLICE * reader->meta->totalProcBeamlets / (spectralDownsample ?: 1);
	for (int8_t i = 0; i < numStokes; i++) {
		outputStokes[i] = calloc(outputFloats, sizeof(float));
	}

	if (silent == 0) {
		lofar_udp_time_get_current_isot(reader, stringBuff, sizeof(stringBuff) / sizeof(stringBuff[0]));
		printf("\n\n=========== Reader  Information ===========\n");
		printf("Total Beamlets:\t%d/%d\t\t\t\t\tFirst Packet:\t%ld\n", reader->meta->totalProcBeamlets,
		       reader->meta->totalRawBeamlets, reader->meta->lastPacket);
		printf("Start time:\t%s\t\tMJD Time:\t%lf\n", stringBuff,
		       lofar_udp_time_get_packet_time_mjd(reader->meta->inputData[0]));
		for (int8_t port = 0; port < reader->meta->numPorts; port++) {
			printf("------------------ Port %d -----------------\n", port);
			printf("Port Beamlets:\t%d/%d\t\tPort Bitmode:\t%d\t\tInput Pkt Len:\t%d\n",
			       reader->meta->upperBeamlets[port] - reader->meta->baseBeamlets[port],
			       reader->meta->portRawBeamlets[port], reader->meta->inputBitMode,
			       reader->meta->portPacketLength[port]);
		}
		for (int8_t out = 0; out < reader->meta->numOutputs; out++)
			printf("Output Pkt Len (%d):\t%d\t\t", out, reader->meta->packetOutputLength[out]);
		printf("\n");
		printf("============= End Information =============\n\n");
	}


	localLoops = 0;

	// Get the starting packet for output file names, fix the packets per iteration if we dropped packets on the last iter
	startingPacket = reader->meta->leadingPacket;
	if ((returnVal = _lofar_udp_io_write_internal_lib_setup_helper(outConfig, reader, 0)) < 0) {
		fprintf(stderr, "ERROR: Failed to open an output file (%ld, errno %d: %s), breaking.\n", returnVal, errno, strerror(errno));
		returnValMeta = (returnValMeta < 0 && returnValMeta > -7) ? returnValMeta : -7;
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);
		return 1;
	}


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
			printf("Read complete for operation %d after %f seconds (I/O: %lf, MemOps: %lf), return value: %ld\n",
			       loops, TICKTOCK(tick0, tock0), timing[0], timing[1], returnVal);
		}

		totalReadTime += timing[0];
		totalOpsTime += timing[1];

		// Write out the desired amount of packets; cap if needed.
		packetsToWrite = reader->meta->packetsPerIteration;
		if (splitEvery == LONG_MAX && maxPackets < packetsToWrite) {
			packetsToWrite = maxPackets;
		}

		printf("Begin channelisation %d %d %d\n", channelisation, spectralDownsample, downsampling);
		// Perform channelisation, temporal downsampling as needed
		memcpy(in1, reader->meta->outputData[0], nbin * nsub * 2 * sizeof(float));
		memcpy(in2, reader->meta->outputData[1], nbin * nsub * 2 * sizeof(float));
		if (channelisation > 1) {
			CLICK(tickChan);
			printf("Execute\n");
			fftwf_execute(fftForwardX);
			fftwf_execute(fftForwardY);
			printf("Reorder\n");
			reorderData(intermediateX, intermediateY, nbin, nsub);
			//windowData();
			printf("Reorder\n");

			reorderData(intermediateX, intermediateY, mbin, nsub * channelisation);
			ARR_INIT(((float *) in1),nbin * nsub * 2,0.0f);
			ARR_INIT(((float *) in2),nbin * nsub * 2,0.0f);
			printf("Execute\n");
			fftwf_execute(fftBackwardX);
			fftwf_execute(fftBackwardY);
			CLICK(tockChan);
			timing[4] = TICKTOCK(tickChan, tockChan);
			totalChanTime += timing[4];
			CLICK(tickDetect);
			printf("Detect\n");
			transposeDetect(in1, in2, outputStokes, mbin, channelisation, nsub, spectralDownsample, stokesParameters);
		} else {
			fftwf_execute(fftForwardX);
			fftwf_execute(fftForwardY);
			ARR_INIT(((float *) in1),nbin * nsub * 2,0.0f);
			ARR_INIT(((float *) in2),nbin * nsub * 2,0.0f);
			fftwf_execute(fftBackwardX);
			fftwf_execute(fftBackwardY);
			CLICK(tickDetect);
			printf("Detectsolo\n");
			transposeDetect(in1, in2, outputStokes, mbin, channelisation, nsub, spectralDownsample, stokesParameters);
		}
		ARR_INIT(((float *) intermediateX), (int64_t) nbin * nsub * 2,0.0f);
		ARR_INIT(((float *) intermediateY), (int64_t) nbin * nsub * 2,0.0f);
		CLICK(tockDetect);
		timing[5] = TICKTOCK(tickDetect, tockDetect);
		totalDetectTime += timing[5];

		printf("Begin downsampling\n");
		if (downsampling > 1 && !spectralDownsample) {
			CLICK(tickDown);
			temporalDownsample(outputStokes, numStokes, mbin, nchan, downsampling);
			CLICK(tockDown);
			timing[6] = TICKTOCK(tickDown, tockDown);
			totalDownsampleTime += timing[6];
		}
		printf("Finish downsampling %d\n", numStokes);

		for (int8_t out = 0; out < numStokes; out++) {
			printf("Enter loop\n");
			if (reader->metadata != NULL) {
				if (reader->metadata->type != NO_META) {
					CLICK(tick1);
					if ((returnVal = lofar_udp_metadata_write_file(reader, outConfig, out, reader->metadata, headerBuffer, 4096 * 8, localLoops == 0)) <
					    0) {
						fprintf(stderr, "ERROR: Failed to write header to output (%ld, errno %d: %s), breaking.\n", returnVal, errno, strerror(errno));
						returnValMeta = (returnValMeta < 0 && returnValMeta > -4) ? returnValMeta : -4;
						break;
					}
					CLICK(tock1);
					timing[2] += TICKTOCK(tick1, tock1);
				}
			}
			printf("Sizing\n");

			CLICK(tick0);
			size_t outputLength = packetsToWrite * UDPNTIMESLICE * reader->meta->totalProcBeamlets / downsampling * sizeof(float);
			VERBOSE(printf("Writing %ld bytes (%ld packets) to disk for output %d...\n",
			               outputLength, packetsToWrite, out));
			size_t outputWritten;
			printf("Writing...\n");
			if ((outputWritten = lofar_udp_io_write(outConfig, out, (int8_t *) outputStokes[out],
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
				for (int8_t outp = 0; outp < reader->meta->numOutputs; outp++) {
					lofar_udp_io_write_cleanup(outConfig, outp, 0);
				}

				// Open new files
				if ((returnVal = _lofar_udp_io_write_internal_lib_setup_helper(outConfig, reader, localLoops / splitEvery)) < 0) {
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
			if (reader->metadata != NULL) if (reader->metadata->type != NO_META) printf("Metadata processing for operation %d after %f seconds.\n", loops, timing[2]);
			printf("Disk writes completed for operation %d (%d) after %f seconds.\n", loops, localLoops, timing[3]);
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

		for (int8_t outp = 0; outp < numStokes; outp++) {
			lofar_udp_io_write_cleanup(outConfig, outp, 1);
		}

		// returnVal below -1 indicates we will not be given data on the next iteration, so gracefully exit with the known reason
		if (returnValMeta < -1) {
			printf("We've hit a termination return value (%ld, %s), exiting.\n", returnValMeta,
			       exitReasons[abs((int32_t) returnValMeta)]);
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

	int64_t droppedPackets = 0, totalPacketLength = 0, totalOutLength = 0;

	// Print out a summary of the operations performed, this does not contain data read for seek operations
	if (silent == 0) {
		for (int port = 0; port < reader->meta->numPorts; port++)
			totalPacketLength += reader->meta->portPacketLength[port];
		for (int out = 0; out < numStokes; out++)
			totalOutLength += reader->meta->packetOutputLength[out];
		for (int port = 0; port < reader->meta->numPorts; port++)
			droppedPackets += reader->meta->portTotalDroppedPackets[port];

		printf("Reader loop exited (%ld); overall process took %f seconds.\n", returnVal, TICKTOCK(tick, tock));
		printf("We processed %ld packets, representing %.03lf seconds of data", packetsProcessed,
		       (float) (reader->meta->numPorts * packetsProcessed * UDPNTIMESLICE) * 5.12e-6f);
		if (reader->meta->numPorts > 1) {
			printf(" (%.03lf per port)\n", (float) (packetsProcessed * UDPNTIMESLICE) * 5.12e-6f);
		} else { printf(".\n"); }
		printf("Total Read Time:\t%3.02lf s\t\t\tTotal CPU Ops Time:\t%3.02lf s\nTotal Write Time:\t%3.02lf s\t\t\tTotal MetaD Time:\t%3.02lf s\n", totalReadTime,
		       totalOpsTime, totalWriteTime, totalMetadataTime);
		printf("Total Channelisation Time:\t%3.02lf s\t\t\tTotal Detection Time:\t%3.02lf s\nTotal Downsampling Time:\t%3.02lf s\n", totalChanTime, totalDetectTime, totalDownsampleTime);
		printf("Total Data Read:\t%3.03lf GB\t\tTotal Data Written:\t%3.03lf GB\n",
		       (double) (packetsProcessed * totalPacketLength) / 1e+9,
		       (double) (packetsWritten * totalOutLength) / 1e+9);
		printf("Effective Read Speed:\t%3.01lf MB/s\t\tEffective Write Speed:\t%3.01lf MB/s\n", (double) (packetsProcessed * totalPacketLength) / 1e+6 / totalReadTime,
		       (double) (packetsWritten * totalOutLength) / 1e+6 / totalWriteTime);
		printf("Approximate Throughput:\t%3.01lf GB/s\n", (double) (reader->meta->numPorts * packetsProcessed * (totalPacketLength + totalOutLength)) / 1e+9 / totalOpsTime);
		printf("A total of %ld packets were missed during the observation.\n", droppedPackets);
		printf("\n\nData processing finished. Cleaning up file and memory objects...\n");
	}



	// Clean-up the reader object, also closes the input files for us
	lofar_udp_reader_cleanup(reader);
	if (silent == 0) { printf("Reader cleanup performed successfully.\n"); }

	// Free our malloc'd objects
	CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY);

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
