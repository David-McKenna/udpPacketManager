#include "lofar_udp_io.h"


// Setup functions

// @param      input   The input
// @param[in]  config  The configuration
// @param[in]  meta    The meta
// @param[in]  port    The port
//
// @return     { description_of_the_return_value }
//
int lofar_udp_io_read_setup(lofar_udp_io_read_config *input, int8_t port) {
	if (input == NULL) {
		fprintf(stderr, "ERROR %s: passed null input configuration, exiting.\n", __func__);
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
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  iter    The iterator
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_setup(lofar_udp_io_write_config *config, int32_t iter) {
	if (config == NULL) {
		fprintf(stderr, "ERROR %s: passed null input configuration, exiting.\n", __func__);
		return -1;
	}

	if (config->readerType == ZSTDCOMPRESSED_INDIRECT) {
		config->readerType = ZSTDCOMPRESSED;
	}
	int returnVal = 0;
	for (int8_t outp = 0; outp < config->numOutputs; outp++) {
		switch (config->readerType) {
			case NORMAL:
			case FIFO:
				returnVal = _lofar_udp_io_write_setup_FILE(config, outp, iter);
				break;

			case ZSTDCOMPRESSED:
				returnVal = _lofar_udp_io_write_setup_ZSTD(config, outp, iter);
				break;

			case DADA_ACTIVE:
				returnVal = _lofar_udp_io_write_setup_DADA(config, outp);
				break;

			case HDF5:
				returnVal = _lofar_udp_io_write_setup_HDF5(config, outp, iter);
				break;

			default:
				fprintf(stderr, "ERROR: Unknown reader (%d) provided, exiting.\n", config->readerType);
				return -1;
		}

		if (returnVal != 0) {
			return -1;
		}
	}

	return returnVal;
}


int _lofar_udp_io_read_setup_internal_lib_helper(lofar_udp_io_read_config *const input, const lofar_udp_config *config, const lofar_udp_obs_meta *meta,
                                                 int8_t port) {

	if (input == NULL || config == NULL || meta == NULL) {
		fprintf(stderr, "ERROR %s: passed null input configuration (input: %p, config: %p, meta: %p), exiting.\n", __func__, input, config, meta);
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
	// Set the maximum length read buffer
	input->readBufSize[port] = meta->packetsPerIteration * meta->portPacketLength[port];

	// Copy over the port parameters
	input->basePort = config->basePort;
	input->stepSizePort = config->stepSizePort;
	input->offsetPortCount = config->offsetPortCount;

	// ZSTD needs to write directly to a given output buffer
	if (input->readerType == ZSTDCOMPRESSED) {
		input->decompressionTracker[port].dst = meta->inputData[port];
	// Copy over the PSRDADA keys since they aren't parsed as strings
	} else if (input->readerType == DADA_ACTIVE) {
		input->inputDadaKeys[port] = config->inputDadaKeys[port];
	}

	// Copy over the raw input locations
	if (strcpy(input->inputLocations[port], config->inputLocations[port]) != input->inputLocations[port]) {
		fprintf(stderr, "ERROR: Failed to copy input location to reader on port %d, exiting.\n", port);
		return -1;
	}

	// Call the main setup function
	return lofar_udp_io_read_setup(input, port);
}

int lofar_udp_io_read_setup_helper(lofar_udp_io_read_config *input, const int8_t **outputArr, int64_t maxReadSize, int8_t port) {

	// If this is the first initialisation call, copy over the specified reader type
	if (input->readerType == NO_ACTION) {
		fprintf(stderr, "ERROR: Input reader configuration does not specify a reader type, exiting.\n");
		return -1;
	}

	// PSRDADA needs port packet length to do corrections if we reconnect to a partially processed ringbuffer
	input->portPacketLength[port] = 1;
	// Set the maximum length read buffer
	input->readBufSize[port] = maxReadSize;

	// ZSTD needs to write directly to a given output buffer
	if (input->readerType == ZSTDCOMPRESSED) {
		input->decompressionTracker[port].dst = outputArr;
	}

	// Call the main setup function
	return lofar_udp_io_read_setup(input, port);
}

int lofar_udp_io_write_setup_helper(lofar_udp_io_write_config *config, const lofar_udp_obs_meta *meta, int32_t iter) {
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
 * @brief      { function_description }
 *
 * @param      input      The input
 * @param[in]  port       The port
 *
 * @return     { description_of_the_return_value }
 */
void lofar_udp_io_read_cleanup(lofar_udp_io_read_config *input, int8_t port) {
	if (input == NULL) {
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
 * @brief      { function_description }
 *
 * @param      config     The configuration
 * @param[in]  outp       The outp
 * @param[in]  fullClean  The full clean
 *
 * @return     { description_of_the_return_value }
 */
void lofar_udp_io_write_cleanup(lofar_udp_io_write_config *config, int8_t outp, int8_t fullClean) {
	if (config == NULL) {
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
 * @brief      { function_description }
 *
 * @param[in]  optargc     The optarg
 * @param      fileFormat  The file format
 * @param      baseVal     The base value
 * @param      offsetVal   The offset value
 *
 * @return     { description_of_the_return_value }
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
	}

	VERBOSE(printf("a: %s: %d, %d, %d\n", __func__, *baseVal, *stepSize, *offsetVal));
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
 * @brief      { function_description }
 *
 * @param      dest    The destination
 * @param[in]  format  The format
 * @param[in]  port    The port
 * @param[in]  iter    The iterator
 * @param[in]  outp    The outp
 * @param[in]  pack    The pack
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_parse_format(char *dest, const char format[], int32_t port, int iter, int idx, long pack) {
	if (dest == NULL || format == NULL) {
		fprintf(stderr, "ERROR: Passed null input, (dest: %p, format: %p), exiting.\n", dest, format);
		return -1;
	}
	/*
	if ((sizeof(dest) / sizeof(dest[0])) < (sizeof(format) / sizeof(format[0]))) {
		fprintf(stderr, "ERROR: Destination is smaller than the input (%ld vs %ld), it is unsafe to continue, exiting.\n", sizeof(dest) / sizeof(dest[0]), sizeof(format) / sizeof(format[0]));
		return -1;
	}
	*/

	if (strlen(format) > (unsigned int) (0.9 * DEF_STR_LEN)) {
		fprintf(stderr, "WARNING: Input path is close to the internal string size (%ld vs %d), we may segfault here.\n",
				strlen(format), DEF_STR_LEN);
	}

	char *formatCopyOne = calloc(DEF_STR_LEN, sizeof(char));
	if (formatCopyOne == NULL || (strcpy(formatCopyOne, format) != formatCopyOne)) {
		fprintf(stderr, "ERROR: Failed to make a copy of the input format (1, %s), exiting.\n", format);
		FREE_NOT_NULL(formatCopyOne);
		return -1;
	}
	char *formatCopyTwo = calloc(DEF_STR_LEN, sizeof(char));
	if (formatCopyTwo == NULL || (strcpy(formatCopyTwo, format) != formatCopyTwo)) {
		fprintf(stderr, "ERROR: Failed to make a copy of the input format (2, %s), exiting.\n", format);
		FREE_NOT_NULL(formatCopyOne); FREE_NOT_NULL(formatCopyTwo);
		return -1;
	}

	char *formatCopyDst = formatCopyOne, *formatCopySrc = formatCopyTwo;


	char *prefix = calloc(DEF_STR_LEN, sizeof(char));
	char *suffix = calloc(DEF_STR_LEN, sizeof(char));
	if (prefix == NULL || suffix == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate memory for prefix / suffix, exiting.\n");
		FREE_NOT_NULL(prefix); FREE_NOT_NULL(suffix); FREE_NOT_NULL(formatCopyOne); FREE_NOT_NULL(formatCopyTwo);
	}
	char *startSubStr;
	// Please don't ever bring up how disgusting this loop is.
	int notrigger = 1;
	while (strstr(formatCopySrc, "[[")) {
		notrigger = notrigger ?: 1;
		if ((startSubStr = strstr(formatCopySrc, "[[port]]"))) {
			(*startSubStr) = '\0';
			sprintf(formatCopyDst, "%s%d%s", formatCopySrc, port, startSubStr + sizeof("[port]]"));
			_swapCharPtr(&formatCopyDst, &formatCopySrc);
			notrigger = 0;
		}

		if ((startSubStr = strstr(formatCopySrc, "[[iter]]"))) {
			(*startSubStr) = '\0';
			sprintf(formatCopyDst, "%s%04d%s", formatCopySrc, iter, startSubStr + sizeof("[iter]]"));
			_swapCharPtr(&formatCopyDst, &formatCopySrc);
			notrigger = 0;
		}

		if ((startSubStr = strstr(formatCopySrc, "[[idx]]"))) {
			(*startSubStr) = '\0';
			sprintf(formatCopyDst, "%s%d%s", formatCopySrc, idx, startSubStr + sizeof("[idx]]"));
			_swapCharPtr(&formatCopyDst, &formatCopySrc);
			notrigger = 0;
		}

		if ((startSubStr = strstr(formatCopySrc, "[[pack]]"))) {
			(*startSubStr) = '\0';
			sprintf(formatCopyDst, "%s%ld%s", formatCopySrc, pack, startSubStr + sizeof("[pack]]"));
			_swapCharPtr(&formatCopyDst, &formatCopySrc);
			notrigger = 0;
		}

		if (notrigger != 0) {
			if (notrigger != 1) {
				fprintf(stderr, "WARNING %s: Failed to detect keyword in input format %s, but key [[ is still present.\n", __func__, formatCopySrc);
				break;
			} else{
				notrigger += 1;
			}
		}
	}

	int returnBool = (dest == strcpy(dest, formatCopySrc));
	FREE_NOT_NULL(formatCopyOne); FREE_NOT_NULL(formatCopyTwo); FREE_NOT_NULL(prefix); FREE_NOT_NULL(suffix);
	return returnBool;

}

/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  optargc  The optarg
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_read_parse_optarg(lofar_udp_config *config, const char optargc[]) {
	if (config == NULL || optargc == NULL) {
		fprintf(stderr, "ERROR %s: Input pointer is null (config: %p, optargc: %p), exiting.\n", __func__, config, optargc);
		return -1;
	}

	char fileFormat[DEF_STR_LEN];
	config->readerType = lofar_udp_io_parse_type_optarg(optargc, fileFormat, &(config->basePort), &(config->stepSizePort), &(config->offsetPortCount));


	if (config->readerType == -1) {
		fprintf(stderr, "ERROR: Failed to parse input pattern (%s), exiting.\n", optargc);
		return -1;
	}

	char *endPtr;
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
			if (config->basePort > INT16_MIN && config->basePort < INT16_MAX) {
				fprintf(stderr, "ERROR: Failed to parse PSRDADA keys correctly, exiting.\n");
				return -1;
			}
			config->stepSizePort = (int16_t) config->basePort;

			// Parse the base value from the input
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
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  optargc  The optarg
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_parse_optarg(lofar_udp_io_write_config *config, const char optargc[]) {
	if (config == NULL || optargc == NULL) {
		fprintf(stderr, "ERROR %s: Input pointer is null (config: %p, optargc: %p), exiting.\n", __func__, config, optargc);
		return -1;
	}

	int16_t dummyInt = 0;
	config->readerType = lofar_udp_io_parse_type_optarg(optargc, config->outputFormat, &(config->baseVal),
	                                                    &(config->stepSize), &(dummyInt));

	if (config->readerType == -1) {
		fprintf(stderr, "ERROR: Failed to parse input pattern (%s), exiting.\n", optargc);
		return -1;
	}

	char *endPtr;
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
			if (config->baseVal > INT16_MIN && config->baseVal < INT16_MAX) {
				fprintf(stderr, "ERROR: Failed to parse PSRDADA keys correctly, exiting.\n");
				return -1;
			}
			config->stepSize = (int16_t) config->baseVal;
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


// Read functions /**
//  * { list_item_description }
// @brief      Read a set amount of data to a given pointer on a given port.
//  * Supports standard files and decompressed files when
//  * readerType== 1
//
// @param      input        The input
// @param[in]  port         The port (file) to read data from
//  * { list_item_description }
// @param      targetArray  The storage array
//  * { list_item_description }
// @param[in]  nchars       The number of chars (bytes) to read in
//  * { list_item_description }
// @param      reader       The lofar_udp_reader struct to process
// @param[in]  knownOffset  The compressed reader's known offset
//
// @return     long: bytes read */
//
int64_t lofar_udp_io_read(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars) {

	// Sanity check input
	if (nchars < 0) {
		fprintf(stderr, "ERROR: Requested negative read size %ld on port %d, exiting.\n", nchars, port);
		return -1;
	}

	if (nchars > input->readBufSize[port]) {
		fprintf(stderr, "ERROR: Request read of %ld is larger than buffer size %ld, exiting.\n", nchars, input->readBufSize[port]);
		return -1;
	}

	if (input == NULL || targetArray == NULL) {
		fprintf(stderr, "ERROR: Inputs were nulled at some point, cannot read new data, exiting.\n");
		return -1;
	}

	if (port < 0 || port > MAX_NUM_PORTS) {
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
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 * @param      src     The source (non-const due to ipcio_write being non-const)
 * @param[in]  nchars  The nchars
 *
 * @return     { description_of_the_return_value }
 */
int64_t lofar_udp_io_write(lofar_udp_io_write_config *const config, int8_t outp, const int8_t *src, int64_t nchars) {
	// Sanity check input
	if (nchars < 0) {
		fprintf(stderr, "ERROR: Requested negative write size %ld on output %d, exiting.\n", nchars, outp);
		return -1;
	}
	if (config == NULL || src == NULL) {
		fprintf(stderr, "ERROR: Target was nulled at some point, cannot write new data, exiting.\n");
		return -1;
	}
	if (outp < 0 || outp >= MAX_NUM_PORTS) {
		fprintf(stderr, "ERROR: Invalid port index (%d)\n, exiting.", outp);
		return -1;
	}

	switch (config->readerType) {
		case NORMAL:
			return _lofar_udp_io_write_FILE(config, outp, src, nchars);

		case FIFO:
			return _lofar_udp_io_write_FIFO(config, outp, src, nchars);

		case ZSTDCOMPRESSED:
			return _lofar_udp_io_write_ZSTD(config, outp, src, nchars);

		case DADA_ACTIVE:
			return _lofar_udp_io_write_DADA(config->dadaWriter[outp].hdu->data_block, src, nchars, 0);

		case HDF5:
			return _lofar_udp_io_write_HDF5(config, outp, src, nchars);

		default:
			fprintf(stderr, "ERROR: Unknown reader %d, exiting.\n", config->readerType);
			return -1;

	}
}


int64_t lofar_udp_io_write_metadata(lofar_udp_io_write_config *const outConfig, int8_t outp, const lofar_udp_metadata *metadata, const int8_t *headerBuffer, int64_t headerLength) {
	if (outConfig == NULL || metadata == NULL) {
		fprintf(stderr, "ERROR %s: Invalid struct (outConfig: %p, metadata: %p), exiting.\n", __func__, outConfig, metadata);
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

	ssize_t trueHeaderLen = (ssize_t) strnlen((const char *) headerBuffer, headerLength);

	switch (outConfig->readerType) {
		// Normal file writes
		case NORMAL:
		case FIFO:
		case ZSTDCOMPRESSED:
			return lofar_udp_io_write(outConfig, outp, headerBuffer, trueHeaderLen);

		case DADA_ACTIVE:
			return _lofar_udp_io_write_DADA((ipcio_t*) outConfig->dadaWriter[outp].hdu->header_block, headerBuffer, trueHeaderLen, 1);

		case HDF5:
			return _lofar_udp_io_write_metadata_HDF5(outConfig, metadata);

		default:
			fprintf(stderr, "ERROR: Unknown reader %d, exiting.\n", outConfig->readerType);
			return -1;
	}
}





/// Temp read functions (shortcuts to read + reverse read head/requested struct
/// allocation afterwards)
///
/// @param[in]  config     The configuration
/// @param[in]  port       The port
/// @param      outbuf     The outbuf
/// @param[in]  size       The size
/// @param[in]  num        The number
/// @param[in]  resetSeek  The reset seek
///
/// @return     { description_of_the_return_value }
///
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
		case FIFO:
			// -Wimplicit-fallthrough
			if (resetSeek) {
				fprintf(stderr, "ERROR %s: Cannot perform a temporary read on a FIFO and reset the pointer location, exiting.\n", __func__);
				return -1;
			}
		case NORMAL:
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

// Metadata functions

void _swapCharPtr(char **a, char **b) {
	char *tmp = *a;
	*a = *b;
	*b = tmp;
}

/**
//  * { list_item_description }
// @brief      Get the size of a file ptr on disk
//
// @param      fileptr  The fileptr
// @param[in]  fd    File descriptor
//
// @return     long: size of file on disk in bytes */
//
long _FILE_file_size(FILE *fileptr) {
	return _fd_file_size(fileno(fileptr));
}

/**
 * @brief      Get the size of a file descriptor on disk
 *
 * @param[in]  fd    File descriptor
 *
 * @return     long: size of file on disk in bytes
 */
long _fd_file_size(int fd) {
	struct stat stat_s;
	int status = fstat(fd, &stat_s);

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