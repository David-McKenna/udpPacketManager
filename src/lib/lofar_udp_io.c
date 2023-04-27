#include "lofar_udp_io.h"


// Setup functions

/**
 * @brief  Setup a writer struct for a given input port
 *
 * @param input Input reader struct
 * @param port Port to configure
 *
 * @return 0: Success, <0: Failure
 */
int32_t lofar_udp_io_read_setup(lofar_udp_io_read_config *input, int8_t port) {
	if (input == NULL) {
		fprintf(stderr, "ERROR %s: passed null input configuration, exiting.\n", __func__);
		return -1;
	}

	if (port < 0 || port >= MAX_NUM_PORTS) {
		fprintf(stderr, "ERROR %s: Invalid port %d (>=%d), exiting.\n", __func__, port, MAX_NUM_PORTS);
		return -1;
	}

	switch (input->readerType) {
		// Normal files and FIFOs use the same read interface
		case NORMAL:
		case FIFO:
			input->numInputs++;
			return _lofar_udp_io_read_setup_FILE(input, input->inputLocations[port], port);

		case ZSTDCOMPRESSED:
		case ZSTDCOMPRESSED_INDIRECT:
			input->numInputs++;
			return _lofar_udp_io_read_setup_ZSTD(input, input->inputLocations[port], port);

		case DADA_ACTIVE:
			input->numInputs++;
			return _lofar_udp_io_read_setup_DADA(input, input->inputDadaKeys[port], port);

		case HDF5:
			fprintf(stderr, "ERROR: Reading from HDF5 files is currently not supported, exiting.\n");
			return -1;

		default:
			fprintf(stderr, "ERROR: Unknown reader (%d) provided, exiting.\n", input->readerType);
			return -1;
	}

}



/**
 * @brief      Setup a string to control writes to a writer
 *
 * @param      config  The configuration
 * @param[in]  iter    The iteration
 *
 * @return     0: Success, <0: Failure
 */
int32_t lofar_udp_io_write_setup(lofar_udp_io_write_config *config, int32_t iter) {
	if (config == NULL) {
		fprintf(stderr, "ERROR %s: passed null input configuration, exiting.\n", __func__);
		return -1;
	}

	if (iter < 0) {
		fprintf(stderr, "ERROR %s: Invalid iteration %d (<0), exiting.\n", __func__, iter);
		return -1;
	}

	if (config->numOutputs < 0 || config->numOutputs > MAX_OUTPUT_DIMS) {
		fprintf(stderr, "ERROR %s: Invalid number of output writers (%d > %d), exiting.\n", __func__, config->numOutputs, MAX_OUTPUT_DIMS);
		return -1;
	}

	if (config->readerType == DADA_ACTIVE && iter > 0) {
		fprintf(stderr, "ERROR %s: DADA writer does not support multiple iterations, exiting.\n", __func__);
		return -1;
	}

	int returnVal = 0;
	for (int8_t outp = 0; outp < config->numOutputs; outp++) {
		switch (config->readerType) {
			case NORMAL:
			case FIFO:
				returnVal = _lofar_udp_io_write_setup_FILE(config, outp, iter);
				break;

			case ZSTDCOMPRESSED:
			case ZSTDCOMPRESSED_INDIRECT:
				returnVal = _lofar_udp_io_write_setup_ZSTD(config, outp, iter);
				break;

			case DADA_ACTIVE:
				// DADA does not support iterations
				returnVal = _lofar_udp_io_write_setup_DADA(config, outp);
				break;

			case HDF5:
				returnVal = _lofar_udp_io_write_setup_HDF5(config, outp, iter);
				break;

			default:
				fprintf(stderr, "ERROR: Unknown reader (%d) provided, exiting.\n", config->readerType);
				return -1;
		}
	}

	return returnVal < 1 ? returnVal : -1;
}


/**
 * @brief Setup a reader struct based on a set of output buffers (specialised approach)
 *
 * @param input Input reader struct
 * @param config Reader configuration
 * @param meta Processing configuration
 * @param port Input port
 *
 * @return 0: Success, -1: Failure
 */
int32_t _lofar_udp_io_read_setup_internal_lib_helper(lofar_udp_io_read_config *const input, const lofar_udp_config *config, lofar_udp_obs_meta *meta,
                                                 int8_t port) {

	if (input == NULL || config == NULL || meta == NULL) {
		fprintf(stderr, "ERROR %s: passed null input configuration (input: %p, config: %p, meta: %p), exiting.\n", __func__, input, config, meta);
		return -1;
	}

	if (port < 0 || port >= MAX_NUM_PORTS) {
		fprintf(stderr, "ERROR %s: Invalid port %d (>=%d), exiting.\n", __func__, port, MAX_NUM_PORTS);
		return -1;
	}

	// If this is the first initialisation call, copy over the specified reader type
	if (input->readerType == NO_ACTION) {
		if (config->readerType == NO_ACTION) {
			fprintf(stderr, "ERROR: Input reader configuration does not specify a reader type, exiting.\n");
			return -1;
		}
		input->readerType = config->readerType;
	}

	// Check the readerType matches the expected value between calls
	if (input->readerType != config->readerType) {
		fprintf(stderr, "ERROR: Mismatch between reader types on port %d (%d vs %d), exiting.\n", port, input->readerType, config->readerType);
		return -1;
	}

	// PSRDADA needs port packet length to do corrections if we reconnect to a partially processed ringbuffer
	input->portPacketLength[port] = meta->portPacketLength[port];

	// Copy over the port parameters
	input->basePort = config->basePort;
	input->stepSizePort = config->stepSizePort;
	input->offsetPortCount = config->offsetPortCount;

	if (input->readerType == DADA_ACTIVE) {
		input->inputDadaKeys[port] = config->inputDadaKeys[port];
	}

	// Copy over the raw input locations
	if (strcpy(input->inputLocations[port], config->inputLocations[port]) != input->inputLocations[port]) {
		fprintf(stderr, "ERROR: Failed to copy input location to reader on port %d, exiting.\n", port);
		return -1;
	}

	// Allocate the memory needed to store the raw data, initialise some variables along the way

	// The input buffer is extended by 2 packets to allow for both a reference packet from a previous iteration
	// and a fall-back packet encase replaying packets is disabled (all 0s to pad the output)

	// If we have a compressed reader, align the length with the ZSTD buffer sizes
	size_t additionalBufferSize = _lofar_udp_io_read_ZSTD_fix_buffer_size(meta->portPacketLength[port] * meta->packetsPerIteration, 1);
	size_t inputBufferSize = meta->portPacketLength[port] * (meta->packetsPerIteration) +
							 PREBUFLEN + // << 2 buffer packets mentioned above, use fixed size encase of unexpected packet sizes
	                         additionalBufferSize * (config->readerType == ZSTDCOMPRESSED);
	meta->inputData[port] = calloc(inputBufferSize, sizeof(int8_t));
	CHECK_ALLOC(meta->inputData[port], -1,
	            for (int8_t i = 0; i < port; i++) { free(meta->inputData[i]); }
	);
	input->readBufSize[port] = inputBufferSize - PREBUFLEN - additionalBufferSize * (config->readerType == ZSTDCOMPRESSED);
	input->decompressionTracker[port].size = inputBufferSize - PREBUFLEN;
	// Offset the pointer to the end of the two initial buffer packets
	VERBOSE(
		if (meta->VERBOSE) {
			printf("calloc at %p for %ld +(%ld ZSTD, %d packet) bytes\n",
			       meta->inputData[port],
			       inputBufferSize, additionalBufferSize,
			       PREBUFLEN);
		}
	);
	// Shift for buffer packets
	meta->inputData[port] += PREBUFLEN;
	input->preBufferSpace[port] = PREBUFLEN;

	// Initialise these arrays while we're looping
	meta->inputDataOffset[port] = 0;
	meta->portLastDroppedPackets[port] = 0;
	meta->portTotalDroppedPackets[port] = 0;

	return lofar_udp_io_read_setup_helper(input, (int8_t**) meta->inputData, input->readBufSize[port], port);
}

/**
 * @brief Setup a reader struct based on a set of output buffers (generic approach)
 *
 * @param input	Input reader struct
 * @param outputArr	Output buffers
 * @param maxReadSize	Size of buffers
 * @param port	Port to assign information to
 *
 * @return 0: Success, -1: Failure
 */
int32_t lofar_udp_io_read_setup_helper(lofar_udp_io_read_config *input, int8_t **outputArr, int64_t maxReadSize, int8_t port) {
	if (input == NULL || outputArr == NULL || outputArr[port] == NULL) {
		fprintf(stderr, "ERROR %s: Input pointer is null (%p, %p, <remaining>), exiting.\n", __func__, input, outputArr);
		return -1;
	}
	// If this is the first initialisation call, copy over the specified reader type
	if (input->readerType == NO_ACTION) {
		fprintf(stderr, "ERROR: Input reader configuration does not specify a reader type, exiting.\n");
		return -1;
	}

	if (port < 0 || port >= MAX_NUM_PORTS) {
		fprintf(stderr, "ERROR %s: Invalid port %d (>=%d), exiting.\n", __func__, port, MAX_NUM_PORTS);
		return -1;
	}

	// PSRDADA needs port packet length to do corrections if we reconnect to a partially processed ringbuffer
	input->portPacketLength[port] = 1;
	// Set the maximum length read buffer
	input->readBufSize[port] = maxReadSize;

	// ZSTD needs to write directly to a given output buffer
	if (input->readerType == ZSTDCOMPRESSED) {
		input->decompressionTracker[port].dst = outputArr[port];


		if (maxReadSize % ZSTD_DStreamOutSize()) {
			if (input->decompressionTracker[port].size < 1 || input->decompressionTracker[port].size % ZSTD_DStreamOutSize()) {
				int64_t trueBufferLength = input->readBufSize[port] + input->preBufferSpace[port];
				int64_t additionalBufferLength = _lofar_udp_io_read_ZSTD_fix_buffer_size(input->readBufSize[port], 1);
				fprintf(stderr, "WARNING: Resizing ZSTD buffer from %ld bytes to %ld(+%ld) bytes.\n", trueBufferLength,
				        trueBufferLength + additionalBufferLength, additionalBufferLength);
				void *tmp = realloc(outputArr[port] - input->preBufferSpace[port], trueBufferLength + additionalBufferLength);
				CHECK_ALLOC_NOCLEAN(tmp, -1);
				outputArr[port] = tmp + input->preBufferSpace[port];
				// Update the true buffer size
				input->decompressionTracker[port].size = input->readBufSize[port] + additionalBufferLength;
			}
		}
	}

	// Call the main setup function
	return lofar_udp_io_read_setup(input, port);
}

/**
 * @brief Setup a writer based on the processing information for the current reader
 * @param config Writer config
 * @param meta Reader processing config
 * @param iter Current output iteration
 *
 * @return 0: Success, -1: Failure
 */
int32_t _lofar_udp_io_write_setup_helper(lofar_udp_io_write_config *config, const lofar_udp_obs_meta *meta, int32_t iter) {
	if (config == NULL || meta == NULL) {
		fprintf(stderr, "ERROR %s: passed null input configuration (onfig: %p, meta: %p), exiting.\n", __func__, config, meta);
		return -1;
	}
	config->numOutputs = meta->numOutputs;
	for (int8_t outp = 0; outp < config->numOutputs; outp++) {
		config->writeBufSize[outp] = meta->packetsPerIteration * meta->packetOutputLength[outp];
	}

	config->firstPacket = meta->lastPacket;

	return lofar_udp_io_write_setup(config, iter);
}


// Cleanup functions

/**
 * @brief      Perform cleanup of allocated memory and open references for a reader struct
 *
 * @param      input      The input
 * @param[in]  port       The port
 */
void lofar_udp_io_read_cleanup(lofar_udp_io_read_config *input, int8_t port) {
	if (input == NULL) {
		return;
	}

	if (port < 0 || port >= MAX_NUM_PORTS) {
		return;
	}

	switch (input->readerType) {
		// Normal files and FIFOs use the same interface
		case NORMAL:
		case FIFO:
			return _lofar_udp_io_read_cleanup_FILE(input, port);

		case ZSTDCOMPRESSED:
		case ZSTDCOMPRESSED_INDIRECT:
			return _lofar_udp_io_read_cleanup_ZSTD(input, port);

		case DADA_ACTIVE:
			return _lofar_udp_io_read_cleanup_DADA(input, port);

		case HDF5:
			return _lofar_udp_io_read_cleanup_HDF5(input, port);

		default:
			fprintf(stderr, "ERROR: Unknown reader (%d) provided, exiting.\n", input->readerType);
			return;
	}
}

/**
 * @brief      Perform cleanup of allocated memory and open references for a writer struct
 *
 * @param      config     The configuration
 * @param[in]  outp       The outp
 * @param[in]  fullClean  bool: Fully cleanup / close all references in the cases of shared memory writers
 */
void lofar_udp_io_write_cleanup(lofar_udp_io_write_config *config, int8_t outp, int8_t fullClean) {
	if (config == NULL) {
		return;
	}

	if (outp < 0 || outp >= MAX_OUTPUT_DIMS) {
		return;
	}

	switch (config->readerType) {
		// Normal files and FIFOs use the same interface
		case NORMAL:
		case FIFO:
			return _lofar_udp_io_write_cleanup_FILE(config, outp);

		case ZSTDCOMPRESSED:
		case ZSTDCOMPRESSED_INDIRECT:
			return _lofar_udp_io_write_cleanup_ZSTD(config, outp, fullClean);

		case DADA_ACTIVE:
			return _lofar_udp_io_write_cleanup_DADA(config, outp, fullClean);

		case HDF5:
			return _lofar_udp_io_write_cleanup_HDF5(config, outp, fullClean);

		default:
			fprintf(stderr, "ERROR: Unknown type (%d) provided, exiting.\n", config->readerType);
			return;
	}
}


/**
 * @brief      Parse an input file format and determine the intended reader
 *
 * @param[in]  optargc     The optarg
 * @param      fileFormat  The file format
 * @param      baseVal     The base value
 * @param      offsetVal   The offset value
 *
 * @return    reader_t > NO_ACTION: success, NO_ACTION (-1): Failure
 */
reader_t lofar_udp_io_parse_type_optarg(const char optargc[], char *fileFormat, int32_t *baseVal, int16_t *stepSize, int16_t *offsetVal) {
	if (optargc == NULL || fileFormat == NULL || baseVal == NULL || stepSize == NULL || offsetVal == NULL) {
		fprintf(stderr, "ERROR %s: Passed null ptr (optargc: %p, fileFormat: %p, baseVal: %p, stepSize: %p, offsetVal: %p), exiting.\n", __func__, optargc, fileFormat, baseVal, stepSize, offsetVal);
		return NO_ACTION;
	}
	reader_t reader;

	if (strstr(optargc, "%") != NULL) {
		fprintf(stderr, "\n\nWARNING: A %% was detected in your input, UPM v0.7+ no longer uses these for indexing.\n");
		fprintf(stderr, "WARNING: Use [[port]], [[outp]], [[pack]] and [[iter]] instead, check the docs for more details.\n\n");
		sleep(1);
	}

	VERBOSE(printf("a: %s: %d, %d, %d\n", __func__, *baseVal, *stepSize, *offsetVal));
	// Check if we have a prefix name
	if (optargc[4] == ':') {
		sscanf(optargc, "%*[^:]:%[^,],%d,%hd,%hd", fileFormat, baseVal, stepSize, offsetVal);
		VERBOSE(printf("b: %s: %d, %d, %d\n", __func__, *baseVal, *stepSize, *offsetVal));


		if (strstr(optargc, "FILE:") != NULL) {
			reader = NORMAL;
		} else if (strstr(optargc, "ZSTD:") != NULL) {
			reader = ZSTDCOMPRESSED;
		} else if (strstr(optargc, "FIFO:") != NULL) {
			reader = FIFO;
		} else if (strstr(optargc, "DADA:") != NULL) {
			reader = DADA_ACTIVE;
		} else if (strstr(optargc, "HDF5:") != NULL) {
			reader = HDF5;
		} else {
			// Unknown prefix
			reader = NO_ACTION;
		}
	// Otherwise attempt to find a substring hint
	} else {
		if (strstr(optargc, ".zst") != NULL) {
			VERBOSE(printf("%s, COMPRESSED\n", optargc));
			reader = ZSTDCOMPRESSED;
		} else if (strstr(optargc, ".hdf5") != NULL || strstr(optargc, ".h5") != NULL) {
			VERBOSE(printf("%s, HDF5\n", optargc));
			reader = HDF5;
		} else {
			reader = NORMAL;
		}
		sscanf(optargc, "%[^,],%d,%hd", fileFormat, baseVal, offsetVal);
		VERBOSE(printf("c: %s: %d, %d, %d\n", __func__, *baseVal, *stepSize, *offsetVal));

	}

	if (reader == NO_ACTION) {
		fprintf(stderr, "ERROR: Failed to determine reader type for int '%s', exiting.\n", optargc);
		return -1;
	}

	return reader;
}

/**
 * @brief      Parse a format string and output the configured string to a buffer
 *
 * @param      dest    The destination buffer
 * @param[in]  format  The format (assumed to be DEF_STR_LEN bytes long)
 * @param[in]  port    The port
 * @param[in]  iter    The iterator
 * @param[in]  outp    The outp
 * @param[in]  pack    The pack
 *
 * @return     0: Success, <0: Failure
 */
int lofar_udp_io_parse_format(char *dest, const char format[], int32_t port, int iter, int idx, long pack) {
	if (dest == NULL || format == NULL) {
		fprintf(stderr, "ERROR: Passed null input, (dest: %p, format: %p), exiting.\n", dest, format);
		return -1;
	}

	// We might not always want to parse port/etc, flag that for invalid values
	int8_t parsePort = 1, parseIter = 1, parseIdx = 1;
	if (port < 0 || port >= MAX_NUM_PORTS) {
		parsePort = 0;
	}
	if (iter < 0) {
		parseIter = 0;
	}
	if (parseIdx < 0 || parseIdx > MAX_OUTPUT_DIMS) {
		parseIdx = 0;
	}

	const int32_t formatStrLen = strnlen(format, DEF_STR_LEN);
	if (!formatStrLen) {
		fprintf(stderr, "ERROR %s: Passed empty string, exiting.\n", __func__);
		return -1;
	}

	if (formatStrLen > (int32_t) (0.9 * DEF_STR_LEN)) {
		fprintf(stderr, "WARNING %s: Input path is close to the internal string size (%d vs %d), we may need to about early here.\n",
				__func__, formatStrLen, DEF_STR_LEN);
	}

	char *formatCopyOne = calloc(DEF_STR_LEN, sizeof(char));
	CHECK_ALLOC_NOCLEAN(formatCopyOne, -1);
	if (strncpy(formatCopyOne, format, DEF_STR_LEN) != formatCopyOne) {
		fprintf(stderr, "ERROR %s: Failed to make a copy of the input format (1, %s), exiting.\n", __func__, format);
		FREE_NOT_NULL(formatCopyOne);
		return -1;
	}
	char *formatCopyTwo = calloc(DEF_STR_LEN, sizeof(char));
	CHECK_ALLOC(formatCopyTwo, -1, free(formatCopyOne));
	if (formatCopyTwo == NULL || (strncpy(formatCopyTwo, format, DEF_STR_LEN) != formatCopyTwo)) {
		fprintf(stderr, "ERROR: Failed to make a copy of the input format (2, %s), exiting.\n", format);
		FREE_NOT_NULL(formatCopyOne); FREE_NOT_NULL(formatCopyTwo);
		return -1;
	}

	char *formatCopyDst = formatCopyOne, *formatCopySrc = formatCopyTwo;


	char *prefix = calloc(DEF_STR_LEN, sizeof(char));
	CHECK_ALLOC(formatCopyTwo, -1, free(formatCopyOne); free(formatCopyTwo););
	char *suffix = calloc(DEF_STR_LEN, sizeof(char));
	CHECK_ALLOC(formatCopyTwo, -1, free(formatCopyOne); free(formatCopyTwo); free(suffix););



	char *startSubStr;
	// Please don't ever bring up how disgusting this loop is.
	// TODO: Macro-ify?
	int notrigger = 1;
	while (strstr(formatCopySrc, "[[")) {
		notrigger = notrigger ?: 1;
		if ((startSubStr = strstr(formatCopySrc, "[[port]]"))) {
			if (!parsePort) {
				fprintf(stderr, "ERROR %s: Input string (%s) contains [[port]], but port index is invalid (%d), exiting.\n", __func__, format, port);
				FREE_NOT_NULL(formatCopyOne); FREE_NOT_NULL(formatCopyTwo); FREE_NOT_NULL(prefix); FREE_NOT_NULL(suffix);
				return -1;
			}
			(*startSubStr) = '\0';
			if (snprintf(formatCopyDst, DEF_STR_LEN, "%s%d%s", formatCopySrc, port, startSubStr + sizeof("[port]]")) < 0) {
				fprintf(stderr, "ERROR: Failed to configure port (errno %d: %s), exiting.\n", errno, strerror(errno));
				FREE_NOT_NULL(formatCopyOne); FREE_NOT_NULL(formatCopyTwo); FREE_NOT_NULL(prefix); FREE_NOT_NULL(suffix);
				return -1;
			}
			_swapCharPtr(&formatCopyDst, &formatCopySrc);
			notrigger = 0;
		}

		if ((startSubStr = strstr(formatCopySrc, "[[iter]]"))) {
			if (!parseIter) {
				fprintf(stderr, "ERROR %s: Input string (%s) contains [[port]], but iter index is invalid (%d), exiting.\n", __func__, format, iter);
				FREE_NOT_NULL(formatCopyOne); FREE_NOT_NULL(formatCopyTwo); FREE_NOT_NULL(prefix); FREE_NOT_NULL(suffix);
				return -1;
			}
			(*startSubStr) = '\0';
			if (snprintf(formatCopyDst, DEF_STR_LEN, "%s%04d%s", formatCopySrc, iter, startSubStr + sizeof("[iter]]")) < 0) {
				fprintf(stderr, "ERROR: Failed to configure iter (errno %d: %s), exiting.\n", errno, strerror(errno));
				FREE_NOT_NULL(formatCopyOne); FREE_NOT_NULL(formatCopyTwo); FREE_NOT_NULL(prefix); FREE_NOT_NULL(suffix);
				return -1;
			}
			_swapCharPtr(&formatCopyDst, &formatCopySrc);
			notrigger = 0;
		}

		if ((startSubStr = strstr(formatCopySrc, "[[idx]]"))) {
			if (!parseIdx) {
				fprintf(stderr, "ERROR %s: Input string (%s) contains [[idx]], but index is invalid (%d), exiting.\n", __func__, format, idx);
				FREE_NOT_NULL(formatCopyOne); FREE_NOT_NULL(formatCopyTwo); FREE_NOT_NULL(prefix); FREE_NOT_NULL(suffix);
				return -1;
			}
			(*startSubStr) = '\0';
			if (snprintf(formatCopyDst, DEF_STR_LEN, "%s%d%s", formatCopySrc, idx, startSubStr + sizeof("[idx]]")) < 0) {
				fprintf(stderr, "ERROR: Failed to configure idx (errno %d: %s), exiting.\n", errno, strerror(errno));
				FREE_NOT_NULL(formatCopyOne); FREE_NOT_NULL(formatCopyTwo); FREE_NOT_NULL(prefix); FREE_NOT_NULL(suffix);
				return -1;
			}
			_swapCharPtr(&formatCopyDst, &formatCopySrc);
			notrigger = 0;
		}

		if ((startSubStr = strstr(formatCopySrc, "[[pack]]"))) {
			(*startSubStr) = '\0';
			if (snprintf(formatCopyDst, DEF_STR_LEN, "%s%ld%s", formatCopySrc, pack, startSubStr + sizeof("[pack]]")) < 0) {
				fprintf(stderr, "ERROR: Failed to configure pack (errno %d: %s), exiting.\n", errno, strerror(errno));
				FREE_NOT_NULL(formatCopyOne); FREE_NOT_NULL(formatCopyTwo); FREE_NOT_NULL(prefix); FREE_NOT_NULL(suffix);
				return -1;
			}
			_swapCharPtr(&formatCopyDst, &formatCopySrc);
			notrigger = 0;
		}

		// Break in the case of an infinite loop for a non-parsed [[keyword]]
		if (notrigger != 0) {
			if (notrigger != 1) {
				fprintf(stderr, "WARNING %s: Failed to detect keyword in input format %s, but key [[ is still present.\n", __func__, formatCopySrc);
				break;
			} else {
				notrigger += 1;
			}
		}
	}

	int32_t returnBool = (dest != strncpy(dest, formatCopySrc, DEF_STR_LEN)) || notrigger;
	FREE_NOT_NULL(formatCopyOne); FREE_NOT_NULL(formatCopyTwo); FREE_NOT_NULL(prefix); FREE_NOT_NULL(suffix);
	return returnBool > 0 ? -1 : 0;

}

/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  optargc  The optarg
 *
 * @return     0: Success, <0: Failure
 */
int32_t lofar_udp_io_read_parse_optarg(lofar_udp_config *config, const char optargc[]) {
	if (config == NULL || optargc == NULL) {
		fprintf(stderr, "ERROR %s: Input pointer is null (config: %p, optargc: %p), exiting.\n", __func__, config, optargc);
		return -1;
	}

	char fileFormat[DEF_STR_LEN];
	config->readerType = lofar_udp_io_parse_type_optarg(optargc, fileFormat, &(config->basePort), &(config->stepSizePort), &(config->offsetPortCount));


	if (config->readerType <= NO_ACTION) {
		fprintf(stderr, "ERROR: Failed to parse input pattern (%s), exiting.\n", optargc);
		return -1;
	}

	switch (config->readerType) {
		case NORMAL:
		case FIFO:
		case ZSTDCOMPRESSED:
		case ZSTDCOMPRESSED_INDIRECT:
		case HDF5:
			for (int i = 0; i < (MAX_NUM_PORTS - config->offsetPortCount); i++) {
				int32_t port = (config->basePort + config->offsetPortCount * config->stepSizePort) + i * config->stepSizePort;

				if (lofar_udp_io_parse_format(config->inputLocations[i], fileFormat, port, -1, i, -1) < 0) {
					return -1;
				}

				VERBOSE(printf("%s\n", config->inputLocations[i]));
			}
			break;

		case DADA_ACTIVE:
			// Swap values, default value is in the fileFormat for ringbuffers
			if (config->basePort < IPC_PRIVATE || config->basePort > INT32_MAX) {
				fprintf(stderr, "ERROR: Failed to parse PSRDADA keys correctly, exiting.\n");
				return -1;
			}
			config->stepSizePort = (int16_t) config->basePort;

			// Parse the base value from the input
			char *endPtr;
			config->basePort = internal_strtoi(fileFormat, &endPtr);
			if (!(fileFormat != endPtr && *(endPtr) == '\0')) {
				fprintf(stderr,"ERROR: Failed to parse base port number (%s), exiting.\n", fileFormat);
				return -1;
			}

			if (config->basePort < 1) {
				fprintf(stderr,
						"ERROR: Failed to parse PSRDADA default value (given %s, parsed %d, must be > 0), exiting.\n",
						fileFormat, config->basePort);
			}

			// Minimum offset between ringbuffers of 2 to account for header ringbuffers.
			if (config->stepSizePort < 2 && config->stepSizePort > -2) {
				if (config->stepSizePort == 0) config->stepSizePort = 1;
				config->stepSizePort *= 2;
				fprintf(stderr, "WARNING: Doubling ringbuffer offset to %d prevent overlaps with headers.\n",
						config->stepSizePort);
			}

			// Populate the dada keys
			for (int i = 0; i < MAX_NUM_PORTS; i++) {
				config->inputDadaKeys[i] = (config->basePort + config->offsetPortCount * config->stepSizePort) + i * config->stepSizePort;
			}
			break;

		default:
			fprintf(stderr, "ERROR: Unhandled reader %d, exiting.\n", config->readerType);
			return -1;
	}

	return 0;
}


/**
 * @brief      Parse an output format (optarg) and configure a writer struct as needed
 *
 * @param      config  The configuration
 * @param[in]  optargc  The optarg
 *
 * @return     0: Success, <0: Failure
 */
int32_t lofar_udp_io_write_parse_optarg(lofar_udp_io_write_config *config, const char optargc[]) {
	if (config == NULL || optargc == NULL) {
		fprintf(stderr, "ERROR %s: Input pointer is null (config: %p, optargc: %p), exiting.\n", __func__, config, optargc);
		return -1;
	}

	int16_t dummyInt = 0;
	config->readerType = lofar_udp_io_parse_type_optarg(optargc, config->outputFormat, &(config->baseVal),
	                                                    &(config->stepSize), &(dummyInt));

	if (config->readerType <= NO_ACTION) {
		fprintf(stderr, "ERROR: Failed to parse input pattern (%s), exiting.\n", optargc);
		return -1;
	}

	switch (config->readerType) {
		case NORMAL:
		case FIFO:
		case ZSTDCOMPRESSED:
		case ZSTDCOMPRESSED_INDIRECT:
		case HDF5:
			// Nothing needs to be done for normal files here
			break;

		case DADA_ACTIVE:
			// Swap values, default value is in the output format for ringbuffers
			if (config->baseVal < IPC_PRIVATE || config->baseVal > INT32_MAX) {
				fprintf(stderr, "ERROR: Failed to parse PSRDADA keys correctly, exiting.\n");
				return -1;
			}
			config->stepSize = (int16_t) config->baseVal;
			char *endPtr;
			config->baseVal = internal_strtoi(config->outputFormat, &endPtr);
			if (!(config->outputFormat != endPtr && *(endPtr) == '\0')) {
				fprintf(stderr, "ERROR: Failed to parse base ringbuffer number (%s), exiting.\n", config->outputFormat);
				return -1;
			}

			if (config->baseVal < 1) {
				fprintf(stderr,
						"ERROR: Failed to parse PSRDADA default value (given %s, parsed %d, must be > 0), exiting.\n",
						config->outputFormat, config->baseVal);
			}

			// Minimum offset between ringbuffers of 2 to account for header ringbuffers.
			if (config->stepSize < 2 && config->stepSize > -2) {
				config->stepSize *= 2;
				fprintf(stderr, "WARNING: Doubling ringbuffer offset to %d prevent overlaps with headers.\n",
						config->stepSize);
			}
			break;

		default:
			fprintf(stderr, "ERROR: Unhandled reader %d, exiting.\n", config->readerType);
			return -1;
	}

	return 0;
}

/**
 * @brief Perform a read operation, following the reader configuration to a specified buffer
 *
 * @param input Input configuration
 * @param port Input port of data
 * @param targetArray Output buffer
 * @param nchars Number of characters to read
 *
 * @return >0: Success, bytes read, <=0: Failure/no operation
 */
int64_t lofar_udp_io_read(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars) {

	// Sanity check input
	if (nchars < 0) {
		fprintf(stderr, "ERROR: Requested negative read size %ld on port %d, exiting.\n", nchars, port);
		return -1;
	}
	if (nchars == 0) {
		return 0;
	}

	if (nchars > input->readBufSize[port]) {
		fprintf(stderr, "ERROR: Request read of %ld is larger than buffer size %ld, exiting.\n", nchars, input->readBufSize[port]);
		return -1;
	}

	if (input == NULL || targetArray == NULL) {
		fprintf(stderr, "ERROR: Inputs were nulled at some point, cannot read new data, exiting.\n");
		return -1;
	}

	if (port < 0 || port >= MAX_NUM_PORTS) {
		fprintf(stderr, "ERROR: Invalid port index (%d)\n, exiting.", port);
		return -1;
	}

	switch (input->readerType) {
		case NORMAL:
		case FIFO:
			return _lofar_udp_io_read_FILE(input, port, targetArray, nchars);


		case ZSTDCOMPRESSED:
		case ZSTDCOMPRESSED_INDIRECT:
			return _lofar_udp_io_read_ZSTD(input, port, targetArray, nchars);


		case DADA_ACTIVE:
			return _lofar_udp_io_read_DADA(input, port, targetArray, nchars);

		case HDF5:
		default:
			fprintf(stderr, "ERROR: Unknown reader %d, exiting.\n", input->readerType);
			return -1;

	}
}



// Write Functions

/**
 * @brief      Write to a specified output from a memory buffer
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 * @param      src     The source (non-const due to ipcio_write being non-const)
 * @param[in]  nchars  The nchars
 *
 * @return     >0: Success, bytes written, <=0: Failure/No operations performed
 */
int64_t lofar_udp_io_write(lofar_udp_io_write_config *const config, int8_t outp, const int8_t *src, int64_t nchars) {
	// Sanity check input
	if (nchars < 0) {
		fprintf(stderr, "ERROR: Requested negative write size %ld on output %d, exiting.\n", nchars, outp);
		return -1;
	}
	if (nchars == 0) {
		return 0;
	}

	if (config == NULL || src == NULL) {
		fprintf(stderr, "ERROR: Target was nulled at some point, cannot write new data, exiting.\n");
		return -1;
	}

	if (outp < 0 || outp >= MAX_OUTPUT_DIMS) {
		fprintf(stderr, "ERROR: Invalid port index (%d)\n, exiting.", outp);
		return -1;
	}

	switch (config->readerType) {
		case NORMAL:
			return _lofar_udp_io_write_FILE(config, outp, src, nchars);

		case FIFO:
			return _lofar_udp_io_write_FIFO(config, outp, src, nchars);

		case ZSTDCOMPRESSED:
		case ZSTDCOMPRESSED_INDIRECT:
			return _lofar_udp_io_write_ZSTD(config, outp, src, nchars);

		case DADA_ACTIVE:
			return _lofar_udp_io_write_DADA(config->dadaWriter[outp].hdu->data_block, src, nchars, 0);

		case HDF5:
			return _lofar_udp_io_write_HDF5(config, outp, src, nchars);

		default:
			fprintf(stderr, "ERROR %s: Unknown writer %d, exiting.\n", __func__, config->readerType);
			return -1;

	}
}

/**
 * @brief Write a metadata buffer to a specified output file
 *
 * @param outConfig Writer configuration
 * @param outp Output file
 * @param metadata Metadata struct
 * @param headerBuffer  Metadata buffer
 * @param headerLength  Metadata buffer length
 *
 * @return >0: Success, bytes written, <=0: Failure
 */
int64_t lofar_udp_io_write_metadata(lofar_udp_io_write_config *const outConfig, int8_t outp, const lofar_udp_metadata *metadata, const int8_t *headerBuffer, int64_t headerLength) {
	if (outConfig == NULL || metadata == NULL) {
		fprintf(stderr, "ERROR %s: Invalid struct (outConfig: %p, metadata: %p), exiting.\n", __func__, outConfig, metadata);
		return -1;
	}

	if (outp < 0 || outp >= MAX_OUTPUT_DIMS) {
		fprintf(stderr, "ERROR: Invalid port index (%d)\n, exiting.", outp);
		return -1;
	}

	if ((outConfig->readerType == HDF5 && metadata->type != HDF5_META)
		|| (outConfig->readerType != HDF5 && metadata->type == HDF5_META)) {
		fprintf(stderr, "ERROR %s: Only HDF5 metadata can only be written to a HDF5 output (outp: %d, meta: %d), exiting.\n", __func__, outConfig->readerType, metadata->type);
		return -1;
	}

	if (outConfig->readerType != HDF5 && headerBuffer == NULL) {
		fprintf(stderr, "ERROR %s: Header buffer is null, exiting.\n", __func__);
		return -1;
	}

	ssize_t outputHeaderSize = (ssize_t) strnlen((const char *) headerBuffer, headerLength);

	switch (outConfig->readerType) {
		// Normal file writes
		case NORMAL:
		case FIFO:
		case ZSTDCOMPRESSED:
		case ZSTDCOMPRESSED_INDIRECT:
			return lofar_udp_io_write(outConfig, outp, headerBuffer, outputHeaderSize);

		// Ringbuffer is offset by 1 from normal writes
		case DADA_ACTIVE:
			return _lofar_udp_io_write_DADA((ipcio_t*) outConfig->dadaWriter[outp].hdu->header_block, headerBuffer, outputHeaderSize, 1);

		// HDF5 does its own thing
		case HDF5:
			return _lofar_udp_io_write_metadata_HDF5(outConfig, metadata);

		default:
			fprintf(stderr, "ERROR: Unknown reader %d, exiting.\n", outConfig->readerType);
			return -1;
	}
}

/**
 * @brief  Temp read functions (shortcuts to read + reverse read head/requested struct allocation afterwards)
 *
 * @param config Input config, with locations specified
 * @param port Specific port to read
 * @param outbuf Output buffer
 * @param size Number of elements to read
 * @param num Size of each element
 * @param resetSeek bool: do/don't reset the seek of the readerafter reading
 *
 * @return >0: Success, bytes read, <=0: Failure
 */
int64_t
lofar_udp_io_read_temp(const lofar_udp_config *config, int8_t port, int8_t *outbuf, int64_t size, int64_t num,
                       int8_t resetSeek) {
	if (config == NULL || outbuf == NULL) {
		fprintf(stderr, "ERROR: Pass a null buffer (config: %p, outbuf: %p), exiting.\n", config, outbuf);
		return -1;
	}

	if (port < 0 || port >= MAX_NUM_PORTS) {
		fprintf(stderr, "ERROR: Invalid port index (%d)\n, exiting.", port);
		return -1;
	}

	if (num < 1 || size < 1) {
		fprintf(stderr, "ERROR: Invalid number of elements to read (%ld * %ld), exiting.\n", num, size);
		return -1;
	}

	switch (config->readerType) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic push
		case FIFO:
			if (resetSeek) {
				fprintf(stderr, "ERROR %s: Cannot perform a temporary read on a FIFO and reset the pointer location, exiting.\n", __func__);
				return -1;
			}
		case NORMAL:
#pragma GCC diagnostic pop
			return _lofar_udp_io_read_temp_FILE(outbuf, size, num, config->inputLocations[port], resetSeek);


		case ZSTDCOMPRESSED:
		case ZSTDCOMPRESSED_INDIRECT:
			return _lofar_udp_io_read_temp_ZSTD(outbuf, size, num, config->inputLocations[port], resetSeek);


		case DADA_ACTIVE:
			return _lofar_udp_io_read_temp_DADA(outbuf, size, num, config->inputDadaKeys[port], resetSeek);

		case HDF5:
			fprintf(stderr, "ERROR: Reading from HDF5 files is not currently supported, exiting.\n");
			return -1;

		case NO_ACTION:
			return 0;

		case UNSET_READER:
		default:
			fprintf(stderr, "ERROR: Unknown reader type (%d), exiting.\n", config->readerType);
			return -1;
	}

}

// Helper functions

/**
 * @brief	Patch the size of a zstandard buffer so that it will be a multiple of the zstandard output frame size
 *
 * @param bufferSize	Target buffer size
 * @param deltaOnly		bool: Only return the difference from the current buffer
 *
 * @return	>0: Requested buffer length, <0: Failure
 */
int64_t _lofar_udp_io_read_ZSTD_fix_buffer_size(int64_t bufferSize, int8_t deltaOnly) {
	const int64_t zstdAlignedSize = ZSTD_DStreamOutSize(); // ~132kB
	// This should be impossible; it should just be returning a header define, but check anyway.
	if (zstdAlignedSize < 0) {
		__builtin_unreachable();
		fprintf(stderr, "ERROR %s: Zstandard library appears to be corrupted, got %ld as the frame length, exiting.\n", __func__, zstdAlignedSize);
		return -1;
	}

	// Extreme edge case: add an extra frame of data encase we need a small partial read at the end of a frame.
	// Only possible for very small packetsPerIteration, but it's still possible.
	const int64_t bufferMul = bufferSize / zstdAlignedSize + 1 * ((bufferSize % zstdAlignedSize) || (bufferSize == 0) ? 1 : 0);
	const int64_t newBufferSize = bufferMul * zstdAlignedSize;

	if (deltaOnly) {
		return newBufferSize - bufferSize;
	}

	return newBufferSize;
}

/**
 * @brief Swap the values of two character pointers
 *
 * @param a A pointer
 * @param b A pointer
 */
void _swapCharPtr(char **a, char **b) {
	char *tmp = *a;
	*a = *b;
	*b = tmp;
}

/**
 * @brief      Get the size of a FILE* struct target on disk
 *
 * @param[in]  fileptr	FILE* struct
 *
 * @return     Size of file on disk in bytes
 */
int64_t _FILE_file_size(FILE *fileptr) {
	return _fd_file_size(fileno(fileptr));
}

/**
 * @brief      Get the size of a file descriptor on disk
 *
 * @param[in]  fd    File descriptor int
 *
 * @return     Size of file on disk in bytes
 */
int64_t _fd_file_size(int32_t fd) {
	struct stat stat_s;
	int32_t status = fstat(fd, &stat_s);

	if (status == -1) {
		fprintf(stderr, "ERROR: Unable to stat input file for fd %d. Errno: %d. Exiting.\n", fd, errno);
		return -1;
	}

	return stat_s.st_size;
}

#include "./io/lofar_udp_io_FILE.c" // NOLINT(bugprone-suspicious-include)
#include "./io/lofar_udp_io_ZSTD.c" // NOLINT(bugprone-suspicious-include)
#include "./io/lofar_udp_io_DADA.c" // NOLINT(bugprone-suspicious-include)
#include "./io/lofar_udp_io_HDF5.c" // NOLINT(bugprone-suspicious-include)