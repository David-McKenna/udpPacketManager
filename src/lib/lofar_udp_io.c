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
		return -2;
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
		return -2;
	}

	if (config->numOutputs < 1 || config->numOutputs > MAX_OUTPUT_DIMS) {
		fprintf(stderr, "ERROR %s: Invalid number of output writers (0 > %d > %d), exiting.\n", __func__, config->numOutputs, MAX_OUTPUT_DIMS);
		return -3;
	}

	if (config->readerType == DADA_ACTIVE && iter > 0) {
		fprintf(stderr, "ERROR %s: DADA writer does not support multiple iterations, exiting.\n", __func__);
		return -4;
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

	return returnVal < 1 ? returnVal : -5;
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
	if (strncpy(input->inputLocations[port], config->inputLocations[port], DEF_STR_LEN) != input->inputLocations[port]) {
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
	if (port < 0 || port >= MAX_NUM_PORTS) {
		fprintf(stderr, "ERROR %s: Invalid port %d (>=%d), exiting.\n", __func__, port, MAX_NUM_PORTS);
		return -1;
	}

	if (input == NULL || outputArr == NULL) {
		fprintf(stderr, "ERROR %s: Input pointer is null (%p, %p), exiting.\n", __func__, input, outputArr);
		return -1;
	}

	if (port != input->numInputs) {
		fprintf(stderr, "ERROR %s: Input port/array is invalid (%d != %d), exiting.\n", __func__, port, input->numInputs);
		return -1;
	}
	if (outputArr[port] == NULL) {
		fprintf(stderr, "ERROR %s: Output array is null (port %d, %p), exiting.\n", __func__, port, outputArr[port]);
		return -1;
	}

	// If this is the first initialisation call, copy over the specified reader type
	if (input->readerType == NO_ACTION) {
		fprintf(stderr, "ERROR: Input reader configuration does not specify a reader type, exiting.\n");
		return -1;
	}

	if (maxReadSize < 1) {
		fprintf(stderr, "ERROR %s: maxReadSize is invalid (%ld < 1), exiting.\n", __func__, maxReadSize);
		return -1;
	}

	// PSRDADA needs port packet length to do corrections if we reconnect to a partially processed ringbuffer
	input->portPacketLength[port] = 1;
	// Set the maximum length read buffer
	input->readBufSize[port] = maxReadSize;

	// ZSTD needs to write directly to a given output buffer
	if (input->readerType == ZSTDCOMPRESSED) {
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

		input->decompressionTracker[port].dst = outputArr[port];
	}

	// Call the main setup function
	return lofar_udp_io_read_setup(input, port);
}

/**
 * @brief Helper function to initialsie a write config
 * 
 * @param config The writer struct
 * @param outputLength Array of maximum write lengths, ignored if the first value is LONG_MIN
 * @param numOutputs Number of outputs to be written
 * @param iter The current iteration number
 * @param firstPacket The first packet number ofr [[pack]] format replacement
 * 
 * @return 0: Success, <0: Failure
 */
int32_t lofar_udp_io_write_setup_helper(lofar_udp_io_write_config *config, int64_t outputLength[], int8_t numOutputs, int32_t iter, int64_t firstPacket) {
	if (config == NULL || outputLength == NULL) {
		fprintf(stderr, "ERROR %s: passed null input configuration (config: %p, outputLength: %p), exiting.\n", __func__, config, outputLength);
		return -1;
	}

	if (numOutputs > MAX_OUTPUT_DIMS || numOutputs < 1) {
		fprintf(stderr, "ERROR %s: Requested more output dimensions that allowed (%d vs %d), exiting.\n", __func__, numOutputs, MAX_OUTPUT_DIMS);
		return -2;
	}

	config->numOutputs = numOutputs;
	if (outputLength[0] != LONG_MIN) {
		for (int8_t outp = 0; outp < config->numOutputs; outp++) {
			if (outputLength[outp] < 1) {
				fprintf(stderr, "ERROR %s: Output length for output %d is invalid (%ld), exiting.\n", __func__, outp, outputLength[outp]);
				return -3;
			}
			config->writeBufSize[outp] = outputLength[outp];
		}
	}

	config->firstPacket = firstPacket;

	return lofar_udp_io_write_setup(config, iter);
}

/**
 * @brief Setup a writer based on the processing information for the current reader
 * @param config Writer config
 * @param reader Reader processing config
 * @param iter Current output iteration
 *
 * @return 0: Success, -1: Failure
 */
int32_t _lofar_udp_io_write_internal_lib_setup_helper(lofar_udp_io_write_config *config, lofar_udp_reader *reader, int32_t iter) {
	if (config == NULL || reader == NULL) {
		fprintf(stderr, "ERROR %s: passed null input configuration (config: %p, meta: %p), exiting.\n", __func__, config, reader);
		return -1;
	}

	if (reader->packetsPerIteration < 1) {
		fprintf(stderr, "ERROR %s: Input packetsPerIteration is not initialised (%ld), exiting.\n", __func__, reader->packetsPerIteration);
		return -2;
	}
	for (int8_t outp = 0; outp < config->numOutputs; outp++) {
		if (reader->meta->packetOutputLength[outp] < 1) {
			fprintf(stderr, "ERROR %s: packetOutputLength[%d] is not initialised (%d), exiting.\n", __func__, outp, reader->meta->packetOutputLength[outp]);
			return -3;
		}
		config->writeBufSize[outp] = reader->packetsPerIteration * reader->meta->packetOutputLength[outp];
	}
	int64_t fakeOutputLength[1] = { LONG_MIN };

	return lofar_udp_io_write_setup_helper(config, fakeOutputLength, reader->meta->numOutputs, iter, reader->meta->lastPacket);
}


// Cleanup functions

/**
 * @brief      Perform cleanup of allocated memory and open references for a reader struct
 *
 * @param      input      The input
 */
void lofar_udp_io_read_cleanup(lofar_udp_io_read_config *input) {
	if (input == NULL) {
		return;
	}

	const int8_t ports = input->numInputs;
	for (int8_t port = 0; port < ports; port++) {
		switch (input->readerType) {
			// Normal files and FIFOs use the same interface
			case NORMAL:
			case FIFO:
				_lofar_udp_io_read_cleanup_FILE(input, port);
				break;

			case ZSTDCOMPRESSED:
			case ZSTDCOMPRESSED_INDIRECT:
				_lofar_udp_io_read_cleanup_ZSTD(input, port);
				break;

			case DADA_ACTIVE:
				_lofar_udp_io_read_cleanup_DADA(input, port);
				break;

			case HDF5:
				_lofar_udp_io_read_cleanup_HDF5(input, port);
				break;

			default:
				fprintf(stderr, "ERROR: Unknown reader (%d) provided, exiting.\n", input->readerType);
				break;
		}
	}

	FREE_NOT_NULL(input);
}

/**
 * @brief      Perform cleanup of allocated memory and open references for a writer struct
 *
 * @param      config     The configuration
 * @param[in]  fullClean  bool: Fully cleanup the allocated memory (including config) / close all references in the cases of shared memory writers
 */
void lofar_udp_io_write_cleanup(lofar_udp_io_write_config *config, int8_t fullClean) {
	if (config == NULL) {
		return;
	}

	const int8_t numOutputs = config->numOutputs;
	for (int8_t outp = 0; outp < numOutputs; outp++) {
		switch (config->readerType) {
			// Normal files and FIFOs use the same interface
			case NORMAL:
			case FIFO:
				_lofar_udp_io_write_cleanup_FILE(config, outp);
				break;

			case ZSTDCOMPRESSED:
			case ZSTDCOMPRESSED_INDIRECT:
				_lofar_udp_io_write_cleanup_ZSTD(config, outp, fullClean);
				break;

			case DADA_ACTIVE:
				_lofar_udp_io_write_cleanup_DADA(config, outp, fullClean);
				break;

			case HDF5:
				_lofar_udp_io_write_cleanup_HDF5(config, outp, fullClean);
				break;

			default:
				fprintf(stderr, "ERROR: Unknown type (%d) provided, exiting.\n", config->readerType);
				return;
		}
	}

	if (fullClean) {
		FREE_NOT_NULL(config);
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
reader_t lofar_udp_io_parse_type_optarg(const char *optargc, char *fileFormat, int32_t *baseVal, int16_t *stepSize, int8_t *offsetVal) {
	reader_t reader = NO_ACTION;
	if (optargc == NULL || fileFormat == NULL || baseVal == NULL || stepSize == NULL || offsetVal == NULL) {
		fprintf(stderr, "ERROR %s: Passed null ptr (optargc: %p, fileFormat: %p, baseVal: %p, stepSize: %p, offsetVal: %p), exiting.\n", __func__, optargc, fileFormat, baseVal, stepSize, offsetVal);
		return reader;
	}
	if (!strnlen(optargc, DEF_STR_LEN)) {
		fprintf(stderr, "ERROR %s: Passed empty string, exiting.\n", __func__);
		return -1;
	}

	if (strstr(optargc, "%") != NULL) {
		fprintf(stderr, "\n\nWARNING: A %% was detected in your input, UPM v0.7+ no longer uses these for indexing.\n");
		fprintf(stderr, "WARNING: Use [[port]], [[idx]], [[pack]] and [[iter]] instead, check the docs for more details.\n\n");
	}

	VERBOSE(printf("a: %s: %d, %d, %d\n", __func__, *baseVal, *stepSize, *offsetVal));
	// Check if we have a prefix name
	if (optargc[4] == ':') {
		sscanf(optargc, "%*[^:]:%[^,],%d,%hd,%hhd", fileFormat, baseVal, stepSize, offsetVal);
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
	}
	// Otherwise attempt to find a substring hint
	if (reader == NO_ACTION) {
		if (strstr(optargc, ".zst") != NULL) {
			VERBOSE(printf("%s, COMPRESSED\n", optargc));
			reader = ZSTDCOMPRESSED;
		} else if (strstr(optargc, ".hdf5") != NULL || strstr(optargc, ".h5") != NULL) {
			VERBOSE(printf("%s, HDF5\n", optargc));
			reader = HDF5;
		} else {
			fprintf(stderr, "WARNING %s: No filename hints found, assuming input is a normal file.\n", __func__);
			reader = NORMAL;
		}

		// Consume base/offset vals if present
		sscanf(optargc, "%[^,],%d,%hd,%hhd", fileFormat, baseVal, stepSize, offsetVal);
		VERBOSE(printf("c: %s: %d, %d, %d\n", __func__, *baseVal, *stepSize, *offsetVal));

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
int32_t lofar_udp_io_parse_format(char *dest, const char format[], int32_t port, int32_t iter, int32_t idx, int64_t pack) {
	if (dest == NULL || format == NULL) {
		fprintf(stderr, "ERROR: Passed null input, (dest: %p, format: %p), exiting.\n", dest, format);
		return -1;
	}

	// We might not always want to parse port/etc, flag that for invalid values
	int8_t parsePort = 1, parseIter = 1, parseIdx = 1;
	if (port < 0) {
		parsePort = 0;
	}
	if (iter < 0) {
		parseIter = 0;
	}
	if (idx < 0) {
		parseIdx = 0;
	}

	const int32_t formatStrLen = (int32_t) strnlen(format, DEF_STR_LEN);
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



	char *startSubStr, *workingPtr;
	// Please don't ever bring up how disgusting this loop is.
	// TODO: Macro-ify?
	int32_t notrigger = 1;
	while ((workingPtr = strstr(formatCopySrc, "[["))) {
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
				fprintf(stderr, "ERROR %s: Input string (%s) contains [[iter]], but iter index is invalid (%d), exiting.\n", __func__, format, iter);
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
				char *tmpwork;
				if ((tmpwork = strstr(workingPtr, "]]"))) {
					fprintf(stderr, "WARNING %s: Failed to detect keyword in input format %s, but unknown key %.*s is still present.\n", __func__,
					        formatCopySrc, (int32_t) (tmpwork - workingPtr) + 2, workingPtr);
				}
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

// TODO: int32_t lofar_udp_io_read_parse_optarg(lofar_udp_io_read_config *config, const char optargc[])

/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  optargc  The optarg
 *
 * @return     0: Success, <0: Failure
 */
int32_t _lofar_udp_io_read_internal_lib_parse_optarg(lofar_udp_config *config, const char optargc[]) {
	if (config == NULL || optargc == NULL) {
		fprintf(stderr, "ERROR %s: Input pointer is null (config: %p, optargc: %p), exiting.\n", __func__, config, optargc);
		return -1;
	}

	char fileFormat[DEF_STR_LEN];
	config->readerType = lofar_udp_io_parse_type_optarg(optargc, fileFormat, &(config->basePort), &(config->stepSizePort), &(config->offsetPortCount));


	if (config->readerType <= NO_ACTION) {
		fprintf(stderr, "ERROR: Failed to parse input pattern (%s), exiting.\n", optargc);
		return -2;
	}

	switch (config->readerType) {
		case NORMAL:
		case FIFO:
		case ZSTDCOMPRESSED:
		case ZSTDCOMPRESSED_INDIRECT:
		case HDF5:
			for (int8_t i = 0; i < (MAX_NUM_PORTS - config->offsetPortCount); i++) {
				int32_t port = (config->basePort + config->offsetPortCount * config->stepSizePort) + i * config->stepSizePort;

				if (lofar_udp_io_parse_format(config->inputLocations[i], fileFormat, port, -1, i, -1) < 0) {
					return -3;
				}

				VERBOSE(printf("%s\n", config->inputLocations[i]));
			}
			break;

		case DADA_ACTIVE:
			config->stepSizePort = (int16_t) config->basePort;
			// Swap values, default value is in the fileFormat for ringbuffers
			// Parse the base value from the input
			char *endPtr;
			config->basePort = internal_strtoi(fileFormat, &endPtr);
			if (!(fileFormat != endPtr && *(endPtr) == '\0')) {
				fprintf(stderr,"ERROR: Failed to parse base port number (%s), exiting.\n", fileFormat);
				return -4;
			}

			if (config->basePort < IPC_PRIVATE || config->basePort > INT16_MAX) {
				fprintf(stderr, "ERROR %s: Invalid base PSRDADA key provided (%d) correctly, exiting.\n", __func__, config->basePort);
				return -5;
			}

			// Minimum offset between ringbuffers of 2 to account for header ringbuffers.
			if (config->stepSizePort < 2 && config->stepSizePort > -2) {
				if (config->stepSizePort == 0) config->stepSizePort = 1;
				config->stepSizePort *= 2;
				fprintf(stderr, "WARNING %s: Doubling ringbuffer offset to %d prevent overlaps with headers.\n", __func__,
						config->stepSizePort);
			}

			// Populate the dada keys
			for (int8_t i = 0; i < (MAX_NUM_PORTS - config->offsetPortCount); i++) {
				config->inputDadaKeys[i] = (config->basePort + config->offsetPortCount * config->stepSizePort) + i * config->stepSizePort;
			}
			break;

		default:
			__builtin_unreachable();
			fprintf(stderr, "ERROR: Unhandled reader %d, exiting.\n", config->readerType);
			return -6;
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

	int8_t dummyInt = 0;
	config->readerType = lofar_udp_io_parse_type_optarg(optargc, config->outputFormat, &(config->baseVal),
	                                                    &(config->stepSize), &(dummyInt));

	if (config->readerType <= NO_ACTION) {
		fprintf(stderr, "ERROR: Failed to parse input pattern (%s), exiting.\n", optargc);
		return -2;
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

			config->stepSize = (int16_t) config->baseVal;
			// Swap values, default value is in the fileFormat for ringbuffers
			// Parse the base value from the input
			char *endPtr;
			config->baseVal = internal_strtoi(config->outputFormat, &endPtr);
			if (!(config->outputFormat != endPtr && *(endPtr) == '\0')) {
				fprintf(stderr,"ERROR: Failed to parse base port number (%s), exiting.\n", config->outputFormat);
				return -3;
			}

			if (config->baseVal < IPC_PRIVATE || config->baseVal > INT16_MAX) {
				fprintf(stderr, "ERROR %s: Invalid base PSRDADA key provided (%d) correctly, exiting.\n", __func__, config->baseVal);
				return -4;
			}

			// Minimum offset between ringbuffers of 2 to account for header ringbuffers.
			if (config->stepSize < 2 && config->stepSize > -2) {
				if (config->stepSize == 0) config->stepSize = 1;
				config->stepSize *= 2;
				fprintf(stderr, "WARNING %s: Doubling ringbuffer offset to %d prevent overlaps with headers.\n", __func__,
				        config->stepSize);
			}
			break;

		default:
			fprintf(stderr, "ERROR: Unhandled reader %d, exiting.\n", config->readerType);
			return -5;
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

	if (port < 0 || port >= MAX_NUM_PORTS) {
		fprintf(stderr, "ERROR: Invalid port index (%d)\n, exiting.", port);
		return -1;
	}

	if (nchars > input->readBufSize[port]) {
		fprintf(stderr, "ERROR: Requested read of %ld is larger than buffer size %ld, exiting.\n", nchars, input->readBufSize[port]);
		return -1;
	}

	if (input == NULL || targetArray == NULL) {
		fprintf(stderr, "ERROR: Inputs were nulled at some point, cannot read new data, exiting.\n");
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

	if (outp < 0 || outp >= MAX_OUTPUT_DIMS) {
		fprintf(stderr, "ERROR: Invalid port index (%d)\n, exiting.", outp);
		return -1;
	}

	if (config == NULL || src == NULL) {
		fprintf(stderr, "ERROR: Target was nulled at some point, cannot write new data, exiting.\n");
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
		return -2;
	}

	if ((outConfig->readerType == HDF5 && metadata->type != HDF5_META)
		|| (outConfig->readerType != HDF5 && metadata->type == HDF5_META)) {
		fprintf(stderr, "ERROR %s: Only HDF5 metadata can only be written to a HDF5 output (outp: %d, meta: %d), exiting.\n", __func__, outConfig->readerType, metadata->type);
		return -3;
	}

	if (outConfig->readerType != HDF5 && (headerBuffer == NULL || headerLength < 1)) {
		fprintf(stderr, "ERROR %s: Header buffer is null is unsized (%p, %ld), exiting.\n", __func__, headerBuffer, headerLength);
		return -4;
	}

	int64_t outputHeaderSize = (int64_t) strnlen((const char *) headerBuffer, headerLength);

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
		return -2;
	}

	if (num < 1 || size < 1) {
		if (!(num *size)) return 0;
		fprintf(stderr, "ERROR: Invalid number of elements to read (%ld * %ld), exiting.\n", num, size);
		return -3;
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

	// Expand the buffer up to the next page length
	// Add an extra page for small reads when shifting packets for alignment / extreme packet loss
	const int64_t bufferMul = bufferSize / zstdAlignedSize + 1 * ((bufferSize % zstdAlignedSize) > 0) + 1;
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
