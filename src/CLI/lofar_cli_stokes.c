#include "lofar_cli_meta.h"

// Import FFTW3 for channelisation
#include <math.h>
#include "fftw3.h"

// Import omp for parallelisaing the channelisation/downsamling operations
#include <omp.h>

// Constant to define the length of the timing variable array
#define TIMEARRLEN 7

#define dmPhaseConst 2.41e-10 // Hidden somewhere in Hankins et al, can't find a copy

// Output Stokes Modes
typedef enum stokes_t {
	STOKESI = 1,
	STOKESQ = 2,
	STOKESU = 4,
	STOKESV = 8
} stokes_t;

typedef enum window_t {
	// Eq. 1 https://articles.adsabs.harvard.edu/pdf/2006ChJAS...6b..53L
	NO_WINDOW = 0,
	COHERENT_DEDISP = 1,
	PSR_STANDARD = 2,
	BOXCAR = 4,

} window_t;

void helpMessages() {
	printf("LOFAR Stokes extractor (CLI v%s, lib v%s)\n\n", UPM_CLI_VERSION, UPM_VERSION);
	printf("Usage: lofar_cli_stokes <flags>");

	helpMessages();
	printf("\n\n");


	printf("-C <factor>     Channelisation factor to apply when processing data (default: disabled == 1)\n");
	printf("-d <factor>     Temporal downsampling to apply when processing data (default: disabled == 1)\n");
	printf("-B <binFactor>  The count of FFT bins as a factor of the channelisation factor (default: 8, minimum: 3)\n");
	printf("-N <factor>     The number of FFTs per channel to perform per iteration (default: 512)\n");
	printf("-F              Apply temporal downsampling to spectral data (slower, but higher quality (default: disabled)\n");
	printf("-D <dm>         Apply coherent dedispersion to the data (default: false; 0 pc/cc)\n");
	printf("-W              Window function to apply to the data (defulat: PSR_STANDARD)\n");


}

static void
CLICleanup(lofar_udp_config *config, lofar_udp_io_write_config *outConfig, int8_t *header, fftwf_complex (*X), fftwf_complex (*Y), fftwf_complex *in1,
           fftwf_complex (*in2), fftwf_complex (*chirpData)) {

	FREE_NOT_NULL(outConfig);
	FREE_NOT_NULL(config);
	FREE_NOT_NULL(header);


	if (X != NULL) {
		fftwf_free(X);
	}
	if (Y != NULL) {
		fftwf_free(Y);
	}
	if (in1 != NULL) {
		fftwf_free(in1);
	}
	if (in2 != NULL) {
		fftwf_free(in2);
	}
	if (chirpData != NULL) {
		fftwf_free(chirpData);
	}
}

static void windowGenerator(fftwf_complex *const chirpFunc, float coherentDM, float fbottom, float subbandbw, int32_t mbin, int32_t nsub, int32_t chanFac) {
	const float dmFactConst = 2.0 * M_PI * coherentDM / dmPhaseConst;
	const float chanbw = subbandbw / chanFac;
	for (int32_t subband = 0; subband < nsub; subband++) {
		const float subbandFreq = fbottom + subband * subbandbw + 0.5 * subbandbw;
		for (int32_t chan = 0; chan < chanFac; chan++) {
			const float channelFreq = subbandFreq - 0.5 * subbandbw + subbandbw * ((float) chan / (float) chanFac) + 0.5 * chanbw;
			for (int32_t bin = 0; bin < mbin; bin++) {
				const int32_t fftSpaceIdx = subband * mbin * chanFac + chan * mbin + bin;

				// Frequency in FFT space
				const float binFreq = chanbw * ((float) bin / (float) mbin) + 0.5 * (chanbw / mbin - chanbw);

				// Attempting to maintain similarity to CDMT, which is based upon
				// Eq. 1 https://articles.adsabs.harvard.edu/pdf/2006ChJAS...6b..53L
				const float phase = -1.0f * binFreq * binFreq * dmPhaseConst / ((channelFreq + binFreq) + channelFreq * channelFreq);
				const float taperScale = 1.0f / sqrt(1.0f + pow(binFreq / (0.47 * chanbw), 80.0f));

				chirpFunc[fftSpaceIdx][0] = cos(phase) * taperScale;
				chirpFunc[fftSpaceIdx][1] = sin(phase) * taperScale;
			}
		}
	}
}

#define npol 2
static void windowData(fftwf_complex *const x, fftwf_complex *const y, const fftwf_complex *chirpData, size_t nx, size_t nfft) {
	fftwf_complex *data[npol] = { x, y };

	for (int i = 0; i < npol; i++) {
		fftwf_complex *const workingPtr = data[i];

#pragma omp parallel for default(shared) shared(workingPtr)
		for (size_t fft = 0; fft < nfft; fft++) {
			for (size_t sample = 0; sample < nx; sample++) {
				const size_t inputIdx = sample + fft * nx;

				workingPtr[inputIdx][0] *= chirpData[sample][0];
				workingPtr[inputIdx][1] *= chirpData[sample][1];

			}
		}
	}
}

static void overlapAndPad(fftwf_complex *output, int8_t *input, int32_t nbin, int32_t noverlap, int32_t nsub, int32_t nfft) {
	#pragma omp parallel for default(shared)
	for (int32_t sub = 0; sub < nsub; sub++) {
		const size_t baseIndexInput = sub * (nbin - 2 * noverlap) * nfft;
		const size_t baseIndexOutput = sub * nbin * nfft;
		for (int32_t fft = 0; fft < nfft; fft++) {
			// On the first FFT: don't overwrite previous iteration's padding
			const int32_t binStart = fft ? 0 : noverlap * 2;
			#pragma omp simd
			for (int32_t bin = binStart; bin < nbin; bin++) {
				const size_t outputIndex = baseIndexOutput + fft * nbin + bin;
				const size_t inputIndex = 2 * (baseIndexInput + fft * (nbin - 2 * noverlap) + bin - binStart);
				//if (sub < 3) printf("%zu %zu\n", inputIndex / 2, outputIndex);
				output[outputIndex][0] = (float) input[inputIndex];
				output[outputIndex][1] = (float) input[inputIndex + 1];
			}
		}
	}
}

static void padNextIteration(fftwf_complex *output, int8_t *input, int32_t nbin, int32_t noverlap, int32_t nsub, int32_t nfft) {
	// First iteration: reflection padding
	if (nfft < 0) {
		nfft *= -1;
		#pragma omp parallel for default(shared)
		for (int32_t sub = 0; sub < nsub; sub++) {
			const size_t baseIndexInput = sub * (nbin - 2 * noverlap) * nfft + (2 * noverlap) - 1;
			const size_t baseIndexOutput = sub * nbin * nfft;

			#pragma omp simd
			for (int32_t bin = 0; bin < 2 * noverlap; bin++) {
				const size_t inputIndex = 2 * (baseIndexInput - bin);
				const size_t outputIndex = baseIndexOutput + bin;
				output[outputIndex][0] = (float) input[inputIndex];
				output[outputIndex][1] = (float) input[inputIndex + 1];
			}
		}
	// Otherwise: nomal padding
	} else {
		#pragma omp parallel for default(shared)
		for (int32_t sub = 0; sub < nsub; sub++) {
			const size_t baseIndexInput = (sub + 1) * (nbin - 2 * noverlap) * nfft - 2 * noverlap;
			const size_t baseIndexOutput = sub * nbin * nfft;

			#pragma omp simd
			for (int32_t bin = 0; bin < 2 * noverlap; bin++) {
				const size_t inputIndex = 2 * (baseIndexInput + bin);
				const size_t outputIndex = baseIndexOutput + bin;
				output[outputIndex][0] = (float) input[inputIndex];
				output[outputIndex][1] = (float) input[inputIndex + 1];
			}
		}
	}
}

static void reorderData(fftwf_complex *const x, fftwf_complex *const y, int32_t bins, int32_t channels) {
	fftwf_complex *data[npol] = { x, y };
	const size_t spectrumOffset = bins / 2;
	for (int i = 0; i < npol; i++) {
		fftwf_complex *const workingPtr = data[i];

#pragma omp parallel for default(shared) shared(workingPtr)
		for (size_t channel = 0; channel < channels; channel++) {
			fftwf_complex tmp;

			#pragma omp simd
			for (size_t sample = 0; sample < spectrumOffset; sample++) {
				const size_t inputIdx = sample + channel * bins;
				const size_t outputIdx = inputIdx + spectrumOffset;
				//if (channel < 3) printf("%zu %zu\n", inputIdx, outputIdx);
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
#undef npol


static void
transposeDetect(fftwf_complex (*X), fftwf_complex (*Y), float **outputs, size_t mbin, size_t noverlap, size_t nfft, size_t channelisation, size_t nsub,
                size_t channelDownsample, int stokesFlags) {

	if (channelDownsample < 1) {
		channelDownsample = 1;
	}


	noverlap /= channelisation;
	const size_t outputNchan = nsub * channelisation / channelDownsample;
	const size_t outputMbin = mbin - 2 * noverlap;

#pragma omp parallel for default(shared)
	for (size_t sub = 0; sub < nsub; sub++) {
		float accumulator[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		size_t inputIdx, outputIdx, outputArr;
		size_t accumulations = 0;

		for (size_t fft = 0; fft < nfft; fft++) {
			for (size_t sample = noverlap; sample < (mbin - noverlap); sample++) {
				ARR_INIT(accumulator, 4, 0.0f);

				accumulations = 0;

				#pragma omp simd
				for (size_t chan = 0; chan < channelisation; chan++) {

					// Input is time major
					//         curr     fftoffset       total samples          size per channel
					inputIdx = sample + fft * (mbin) +  (chan * mbin * nfft) + (sub * mbin * nfft * channelisation);

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
						int32_t effectiveSample = (sample - noverlap) + fft * outputMbin;
						//          curr                         total channels                               size per time sample
						outputIdx = (chan / channelDownsample) + (sub * channelisation / channelDownsample) + (outputNchan * effectiveSample);

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
}

static void temporalDownsample(float **data, size_t numOutputs, size_t nbin, size_t nchans, size_t downsampleFactor) {

	if (downsampleFactor < 2) {
		// Nothing to do
		return;
	}

	VERBOSE(printf("Downsampling for: %zu, %zu, %zu\n", nchans, nbin, downsampleFactor));

	for (size_t output = 0; output < numOutputs; output++) {
#pragma omp parallel for default(shared)
		for (size_t sub = 0; sub < nchans; sub++) {
			size_t inputIdx, outputIdx;
			size_t accumulations = 0;
			float accumulator = 0.0f;

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
	int64_t maxPackets = LONG_MAX, startingPacket = -1, splitEvery = LONG_MAX;
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
	char *endPtr;
	int8_t flagged = 0;

	// FFTW strategy
	int32_t channelisation = 1, downsampling = 1, spectralDownsample = 0, nfactor = 8, nforward = 512;
	fftwf_complex *intermediateX = NULL;
	fftwf_complex *intermediateY = NULL;
	fftwf_complex * in1 = NULL;
	fftwf_complex * in2 = NULL;
	fftwf_complex * chirpData = NULL;
	fftwf_plan fftForwardX;
	fftwf_plan fftForwardY;
	fftwf_plan fftBackwardX;
	fftwf_plan fftBackwardY;
	int8_t stokesParameters = 0, numStokes = 0;

	// Standard ugly input flags parser
	while ((inputOpt = getopt(argc, argv, "crzqfvVFhD:i:o:m:M:I:u:t:s:S:b:C:d:P:T:B:N:")) != -1) {
		input = 1;
		switch (inputOpt) {

			case 'i':
				if (strncpy(inputFormat, optarg, DEF_STR_LEN - 1) != inputFormat) {
					fprintf(stderr, "ERROR: Failed to store input data file format, exiting.\n");
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
					return 1;
				}
				inputProvided = 1;
				break;


			case 'o':
				if (lofar_udp_io_write_parse_optarg(outConfig, optarg) < 0) {
					helpMessages();
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
					return 1;
				}
				if (config->metadata_config.metadataType == NO_META) config->metadata_config.metadataType = lofar_udp_metadata_parse_type_output(optarg);
				outputProvided = 1;
				break;

			case 'M':
				config->metadata_config.metadataType = lofar_udp_metadata_string_to_meta(optarg);
				break;

			case 'I':
				if (strncpy(config->metadata_config.metadataLocation, optarg, DEF_STR_LEN) != config->metadata_config.metadataLocation) {
					fprintf(stderr, "ERROR: Failed to copy metadata file location to config, exiting.\n");
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
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
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
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
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
					return 1;
				}
				break;

			case 'r':
				config->replayDroppedPackets = 1;
				break;

			case 'c':
				config->calibrateData = GENERATE_JONES;
				break;

			case 'C':
				channelisation = internal_strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'd':
				downsampling = internal_strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
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

			case 'B':
				nfactor = internal_strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				if (!flagged && nfactor < 3) {
					fprintf(stderr, "ERROR: nfactor must be at least 3 due to FFT overlaps. Exiting.\n");
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
					return 1;
				}
				break;

			case 'N':
				nforward = internal_strtoi(optarg, &endPtr);
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'F':
				spectralDownsample = 1;
				break;

			case 'D':
				coherentDM = strtof(optarg, &endPtr);
				window |= 1;
				if (checkOpt(inputOpt, optarg, endPtr)) { flagged = 1; }
				break;

			case 'P':
				if (numStokes > 0) {
					fprintf(stderr, "ERROR: -P flag has been parsed more than once. Exiting.\n");
					CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
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
				CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
				return 1;

		}
	}

	// Pre-set processing mode
	// TODO: Floating processing mode if chanellisation is disabled?
	config->processingMode = TIME_MAJOR_ANT_POL;

	if (flagged) {
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
		return 1;
	}

	if (!input) {
		fprintf(stderr, "ERROR: No inputs provided, exiting.\n");
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
		return 1;
	}

	if (!inputProvided) {
		fprintf(stderr, "ERROR: An input was not provided, exiting.\n");
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
		return 1;
	}

	// Pass forward output channelisation and downsampling factors
	config->metadata_config.externalChannelisation = channelisation;
	config->metadata_config.externalDownsampling = downsampling;

	if (spectralDownsample) {
		spectralDownsample = downsampling;
		channelisation *= downsampling;
	}

	// TODO: Support non non-even channelisation
	// TODO: Should I just automate these parameters based on the input channelisation/downsampling requirements?
	const int32_t noverlap = 2 * channelisation;
	const int32_t nbin = nfactor * channelisation;
	const int32_t nbin_valid = nbin - 2 * noverlap;

	if (nbin_valid * nforward % UDPNTIMESLICE) {
		fprintf(stderr, "WARNING: Increasing nforward from %d to ", nforward);
		nforward += (UDPNTIMESLICE - (nforward % UDPNTIMESLICE));
		fprintf(stderr, "%d to ensure data can be loaded correctly.\n", nforward);
	}
	config->packetsPerIteration = nbin_valid * nforward / UDPNTIMESLICE;

	if (channelisation > 1) {
		printf("%d, %d, %d, %ld, %ld\n", nbin, noverlap, nbin_valid, config->packetsPerIteration * UDPNTIMESLICE,
		       (config->packetsPerIteration * UDPNTIMESLICE) % nbin_valid);
		// Should no longer be needed; keeping for debug purposes; remove before release
		int32_t invalidation = (config->packetsPerIteration * UDPNTIMESLICE) % nbin_valid;
		if (invalidation) {
			fprintf(stderr, "WARNING: Reducing packets per iteration by %d to attempt to align with FFT boundaries.\n", invalidation / UDPNTIMESLICE);
			config->packetsPerIteration -= invalidation / UDPNTIMESLICE;
		}
		if ((config->packetsPerIteration * UDPNTIMESLICE) % nbin_valid) {
			fprintf(stderr,
			        "ERROR: Number of samples needed per iterations for channelisation factor %d and downsampling factor %d (%d) is not a multiple of number of the set number of timesamples/packets per iteration (%ld), exiting.\n",
			        channelisation, downsampling, nbin_valid, UDPNTIMESLICE * config->packetsPerIteration);
			helpMessages();
			CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
			return 1;
		}
	}

	if (channelisation < 1 || ( channelisation > 1 && channelisation % 2) != 0) {
		fprintf(stderr, "ERROR: Invalid channelisation factor (less than 1, non-factor of 2)\n");
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
		return 1;
	}

	if (downsampling < 1) {
		fprintf(stderr, "ERROR: Invalid downsampling factor (less than 1)\n");
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
		return 1;
	}

	if (lofar_udp_io_read_parse_optarg(config, inputFormat) < 0) {
		helpMessages();
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
		return 1;
	}

	if (!outputProvided) {
		fprintf(stderr, "ERROR: An output was not provided, exiting.\n");
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
		return 1;
	}

	if (config->calibrateData != NO_CALIBRATION && strcmp(config->metadata_config.metadataLocation, "") == 0) {
		fprintf(stderr, "ERROR: Data calibration was enabled, but metadata was not provided. Exiting.\n");
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
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
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
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
			CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
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
	if (maxPackets > 0 && config->packetsPerIteration > maxPackets) {
		if (silent == 0) {
			printf("Packet/Gulp is greater than the maximum packets requested, reducing from %ld to %ld.\n",
			       config->packetsPerIteration, maxPackets);
		}
		config->packetsPerIteration = maxPackets;
	}

	if (fftwf_init_threads() == 0) {
		fprintf(stderr, "ERROR: Failed to initialise multi-threaded FFTWF.\n");
	}
	omp_set_num_threads(config->ompThreads);
	fftwf_plan_with_nthreads(omp_get_num_threads());



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
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
		return 1;
	}

	// Sanity check that we were passed the correct clock bit
	if (((lofar_source_bytes *) &(reader->meta->inputData[0][1]))->clockBit != (unsigned int) clock200MHz) {
		fprintf(stderr,
		        "ERROR: The clock bit of the first packet does not match the clock state given when starting the CLI. Add or remove -c from your command. Exiting.\n");
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
		return 1;
	}

	if (reader->packetsPerIteration * UDPNTIMESLICE > INT32_MAX) {
		fprintf(stderr, "ERROR: Input FFT bins are too long (%ld vs %d), exiting.\n", reader->packetsPerIteration * UDPNTIMESLICE, INT32_MAX);
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
		return 1;
	}

	// Channelisation setup
	const int32_t mbin = nbin / channelisation;
	const int32_t nsub = reader->meta->totalProcBeamlets;
	const int32_t nchan = reader->meta->totalProcBeamlets * channelisation;

	const int32_t nfft = (int32_t) ((reader->packetsPerIteration * UDPNTIMESLICE) / nbin_valid);
	//int32_t nfft = 1;
	//printf("%d, %d, %d, %d\n", nbin, mbin, nsub, nchan);

	in1 = fftwf_alloc_complex(nbin * nsub * nfft);
	in2 = fftwf_alloc_complex(nbin * nsub * nfft);

	if (in1 == NULL || in2 == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate input FFTW buffers, exiting.\n");
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
		return -1;
	}


	if (channelisation > 1) {
		intermediateX = fftwf_alloc_complex(nbin * nsub * nfft);
		intermediateY = fftwf_alloc_complex(nbin * nsub * nfft);

		chirpData = fftwf_alloc_complex(nbin * nsub * nfft);

		if (intermediateX == NULL || intermediateY == NULL || chirpData == NULL) {
			fprintf(stderr, "ERROR: Failed to allocate output FFTW buffers, exiting.\n");
			CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
			return -1;
		}

		fftForwardX = fftwf_plan_many_dft(1, &nbin, nsub * nfft, in1, &nbin, 1, nbin, intermediateX, &nbin, 1, nbin, FFTW_FORWARD, FFTW_ESTIMATE_PATIENT | FFTW_ALLOW_LARGE_GENERIC | FFTW_DESTROY_INPUT);
		fftForwardY = fftwf_plan_many_dft(1, &nbin, nsub * nfft, in2, &nbin, 1, nbin, intermediateY, &nbin, 1, nbin, FFTW_FORWARD, FFTW_ESTIMATE_PATIENT | FFTW_ALLOW_LARGE_GENERIC | FFTW_DESTROY_INPUT);

		fftBackwardX = fftwf_plan_many_dft(1, &mbin, nchan * nfft, intermediateX, &mbin, 1, mbin, in1, &mbin, 1, mbin, FFTW_BACKWARD, FFTW_ESTIMATE_PATIENT | FFTW_ALLOW_LARGE_GENERIC | FFTW_DESTROY_INPUT);
		fftBackwardY = fftwf_plan_many_dft(1, &mbin, nchan * nfft, intermediateY, &mbin, 1, mbin, in2, &mbin, 1, mbin, FFTW_BACKWARD, FFTW_ESTIMATE_PATIENT | FFTW_ALLOW_LARGE_GENERIC | FFTW_DESTROY_INPUT);

		//static void windowGenerator(fftwf_complex *const chirpFunc, float coherentDM, float fbottom, float subbandbw, int32_t mbin, int32_t nsub, int32_t chanFac)
		windowGenerator(chirpData, coherentDM, reader->metadata->ftop_raw, reader->metadata->subband_bw, mbin, nsub, channelisation);
	}

	float *outputStokes[MAX_OUTPUT_DIMS];
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
	int64_t expectedWriteSize[MAX_OUTPUT_DIMS];
	// lofar_udp_io_write_setup_helper(lofar_udp_io_write_config *config, int64_t outputLength[], int8_t numOutputs, int32_t iter, int64_t firstPacket)
	for (int8_t i = 0; i < numStokes; i++) {
		expectedWriteSize[i] = reader->packetsPerIteration * UDPNTIMESLICE * reader->meta->totalProcBeamlets / downsampling * sizeof(float);
	}

	if ((returnVal = lofar_udp_io_write_setup_helper(outConfig, expectedWriteSize, numStokes, 0, startingPacket)) < 0) {
		fprintf(stderr, "ERROR: Failed to open an output file (%ld, errno %d: %s), breaking.\n", returnVal, errno, strerror(errno));
		returnValMeta = (returnValMeta < 0 && returnValMeta > -7) ? returnValMeta : -7;
		CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);
		return 1;
	}


	VERBOSE(if (config->verbose) { printf("Beginning data extraction loop.\n"); });
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

		printf("Begin channelisation %d %d %d %d %d %d %d (%d)\n", channelisation, spectralDownsample, downsampling, nfft, nbin, noverlap, mbin, nfft * nbin);
		// Perform channelisation, temporal downsampling as needed
		if (channelisation > 1) {
			CLICK(tickChan);
			if (localLoops == 0) {
				padNextIteration(in1, reader->meta->outputData[0], nbin, noverlap, nsub, -1 * nfft);
				padNextIteration(in2, reader->meta->outputData[1], nbin, noverlap, nsub, -1 * nfft);
			}
			overlapAndPad(in1, reader->meta->outputData[0], nbin, noverlap, nsub, nfft);
			overlapAndPad(in2, reader->meta->outputData[1], nbin, noverlap, nsub, nfft);
			fftwf_execute(fftForwardX);
			fftwf_execute(fftForwardY);
			reorderData(intermediateX, intermediateY, nbin, nsub * nfft);
			windowData(intermediateX, intermediateY, chirpData, nbin * nsub, nfft);
			reorderData(intermediateX, intermediateY, mbin, nchan * nfft);
			fftwf_execute(fftBackwardX);
			fftwf_execute(fftBackwardY);
			CLICK(tockChan);
			timing[4] = TICKTOCK(tickChan, tockChan);
			totalChanTime += timing[4];
			CLICK(tickDetect);
			transposeDetect(in1, in2, outputStokes, mbin, noverlap, nfft, channelisation, nsub, spectralDownsample, stokesParameters);
			padNextIteration(in1, reader->meta->outputData[0], nbin, noverlap, nsub, nfft);
			padNextIteration(in2, reader->meta->outputData[1], nbin, noverlap, nsub, nfft);
		} else {
			CLICK(tickDetect);
			overlapAndPad(in1, reader->meta->outputData[0], reader->meta->packetsPerIteration * UDPNTIMESLICE, 0, nsub, 1);
			overlapAndPad(in2, reader->meta->outputData[1], reader->meta->packetsPerIteration * UDPNTIMESLICE, 0, nsub, 1);
			transposeDetect(in1, in2, outputStokes, reader->meta->packetsPerIteration * UDPNTIMESLICE, 0, 1, 1, nsub, 1, stokesParameters);
		}
		//ARR_INIT(((float *) intermediateX), (int64_t) nbin * nsub * nfft * 2,0.0f);
		//ARR_INIT(((float *) intermediateY), (int64_t) nbin * nsub * nfft * 2,0.0f);
		CLICK(tockDetect);
		timing[5] = TICKTOCK(tickDetect, tockDetect);
		totalDetectTime += timing[5];

		printf("Begin downsampling\n");
		if (downsampling > 1 && !spectralDownsample) {
			CLICK(tickDown);
			size_t samples = reader->meta->packetsPerIteration * UDPNTIMESLICE / channelisation;
			temporalDownsample(outputStokes, numStokes, samples, nchan, downsampling);
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
				if (!silent) printf("Hit splitting condition; closing writers and re-opening for iteration %ld.\n", localLoops / splitEvery);
				// Close existing files
				lofar_udp_io_write_cleanup(outConfig, 0);

				// Open new files
				if ((returnVal = lofar_udp_io_write_setup_helper(outConfig, expectedWriteSize, numStokes, (int32_t) (localLoops / splitEvery), reader->meta->lastPacket + 1)) < 0) {
					fprintf(stderr, "ERROR: Failed to open new file are breakpoint reached (%ld, errno %d: %s), breaking.\n", returnVal, errno,
					        strerror(errno));
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

	// Cleanup the writer object, close any outputs
	lofar_udp_io_write_cleanup(outConfig, 1);
	outConfig = NULL;

	// Free our malloc'd objects
	CLICleanup(config, outConfig, headerBuffer, intermediateX, intermediateY, in1, in2, chirpData);

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
