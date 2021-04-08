#include "lofar_udp_io.h"


/**
 * @brief      { function_description }
 *
 * @param[in]  optarg      The optarg
 * @param      fileFormat  The file format
 * @param      baseVal     The base value
 * @param      offsetVal   The offset value
 *
 * @return     { description_of_the_return_value }
 */
reader_t lofar_udp_io_parse_type_optarg(const char optarg[], char *fileFormat, int *baseVal, int *offsetVal) {

	reader_t reader;

	if (optarg[4] == ':') {
		sscanf(optarg, "%*[^:]:%[^,],%d,%d", fileFormat, baseVal, offsetVal);

		if (strstr(optarg, "FILE:") != NULL) {
			reader = NORMAL;
		} else if (strstr(optarg, "FIFO:") != NULL) {
			reader = FIFO;
		} else if (strstr(optarg, "DADA:") != NULL) {
			reader = DADA_ACTIVE;
		} else {
			// Unknown prefix
			reader = NONE;
		}
	} else {
		reader = NORMAL;
		sscanf(optarg, "%[^,],%d,%d", fileFormat, baseVal, offsetVal);
	}


	if (strstr(fileFormat, ".zst") != NULL) {
		VERBOSE(printf("%s, COMPRESSED\n", fileFormat));
		reader = ZSTDCOMPRESSED;
	}

	return reader;
}

void swapCharPtr(char **a, char **b) {
	char *tmp = *a;
	*a = *b;
	*b = tmp;
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
int lofar_udp_io_parse_format(char *dest, const char format[], int port, int iter, int outp, long pack) {

	/*
	if ((sizeof(dest) / sizeof(dest[0])) < (sizeof(format) / sizeof(format[0]))) {
		fprintf(stderr, "ERROR: Destrination is smaller than the input (%ld vs %ld), it is unsafe to continue, exiting.\n", sizeof(dest) / sizeof(dest[0]), sizeof(format) / sizeof(format[0]));
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
		free(formatCopyOne);
		return -1;
	}
	char *formatCopyTwo = calloc(DEF_STR_LEN, sizeof(char));
	if (formatCopyTwo == NULL || (strcpy(formatCopyTwo, format) != formatCopyTwo)) {
		fprintf(stderr, "ERROR: Failed to make a copy of the input format (2, %s), exiting.\n", format);
		free(formatCopyTwo);
		return -1;
	}

	char *formatCopyDst = formatCopyOne, *formatCopySrc = formatCopyTwo;


	char *prefix = calloc(DEF_STR_LEN, sizeof(char));
	char *suffix = calloc(DEF_STR_LEN, sizeof(char));
	char *startSubStr;
	// Please don't ever bring up how disgusting this loop is.
	while (strstr(formatCopySrc, "[[")) {
		if ((startSubStr = strstr(formatCopySrc, "[[port]]"))) {
			(*startSubStr) = '\0';
			sprintf(formatCopyDst, "%s%d%s", formatCopySrc, port, startSubStr + sizeof("[port]]"));
			swapCharPtr(&formatCopyDst, &formatCopySrc);
		}

		if ((startSubStr = strstr(formatCopySrc, "[[iter]]"))) {
			(*startSubStr) = '\0';
			sprintf(formatCopyDst, "%s%04d%s", formatCopySrc, iter, startSubStr + sizeof("[iter]]"));
			swapCharPtr(&formatCopyDst, &formatCopySrc);
		}

		if ((startSubStr = strstr(formatCopySrc, "[[outp]]"))) {
			(*startSubStr) = '\0';
			sprintf(formatCopyDst, "%s%d%s", formatCopySrc, outp, startSubStr + sizeof("[outp]]"));
			swapCharPtr(&formatCopyDst, &formatCopySrc);
		}

		if ((startSubStr = strstr(formatCopySrc, "[[pack]]"))) {
			(*startSubStr) = '\0';
			sprintf(formatCopyDst, "%s%ld%s", formatCopySrc, pack, startSubStr + sizeof("[pack]]"));
			swapCharPtr(&formatCopyDst, &formatCopySrc);
		}
	}

	int returnBool = (dest == strcpy(dest, formatCopySrc));
	free(formatCopyOne); free(formatCopyTwo);
	free(prefix);
	free(suffix);
	return returnBool;

}

/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  optarg  The optarg
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_read_parse_optarg(lofar_udp_config *config, const char optarg[]) {

	if (config == NULL) {
		fprintf(stderr, "ERROR: Input config is NULL, exiting.\n");
		return -1;
	}

	char fileFormat[DEF_STR_LEN];
	int baseVal = 0;
	int offsetVal = 1;
	config->readerType = lofar_udp_io_parse_type_optarg(optarg, fileFormat, &baseVal, &offsetVal);

	if (config->readerType == -1) {
		fprintf(stderr, "ERROR: Failed to parse input pattern (%s), exiting.\n", optarg);
		return -1;
	}

	switch (config->readerType) {
		case NORMAL:
		case FIFO:
		case ZSTDCOMPRESSED:
			for (int i = 0; i < MAX_NUM_PORTS; i++) {
				lofar_udp_io_parse_format(config->inputLocations[i], fileFormat, baseVal + i * offsetVal, -1, -1, -1);
			}
			break;

		case DADA_ACTIVE:
			// Swap values, default value is in the fileFormat for ringbuffers
			offsetVal = baseVal;

			// Parse the base value from the input
			baseVal = atoi(fileFormat);
			if (baseVal < 1) {
				fprintf(stderr,
						"ERROR: Failed to parse PSRDADA default value (given %s, parsed %d, must be > 0), exiting.\n",
						fileFormat, baseVal);
			}

			// Minimum offset between ringbuffers of 2 to account for header ringbuffers.
			if (offsetVal < 2 && offsetVal > -2) {
				offsetVal *= 2;
				fprintf(stderr, "WARNING: Doubling ringbuffer offset to %d prevent overlaps with headers.\n",
						offsetVal);
			}

			// Populate the dada keys
			for (int i = 0; i < MAX_NUM_PORTS; i++) {
				config->dadaKeys[i] = baseVal + i * offsetVal;
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
 * @param[in]  optarg  The optarg
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_parse_optarg(lofar_udp_io_write_config *config, const char optarg[]) {

	if (config == NULL) {
		fprintf(stderr, "ERROR: Input config is NULL, exiting.\n");
		return -1;
	}

	config->readerType = lofar_udp_io_parse_type_optarg(optarg, config->outputFormat, &(config->baseVal),
														&(config->offsetVal));

	if (config->readerType == -1) {
		fprintf(stderr, "ERROR: Failed to parse input pattern (%s), exiting.\n", optarg);
		return -1;
	}

	switch (config->readerType) {
		case NORMAL:
		case FIFO:
		case ZSTDCOMPRESSED:
			// Nothing needs to be done for normal files here
			break;

		case DADA_ACTIVE:
			// Swap values, default value is in the output format for ringbuffers
			config->offsetVal = config->baseVal;
			config->baseVal = atoi(config->outputFormat);
			if (config->baseVal < 1) {
				fprintf(stderr,
						"ERROR: Failed to parse PSRDADA default value (given %s, parsed %d, must be > 0), exiting.\n",
						config->outputFormat, config->baseVal);
			}

			// Minimum offset between ringbuffers of 2 to account for header ringbuffers.
			if (config->offsetVal < 2 && config->offsetVal > -2) {
				config->offsetVal *= 2;
				fprintf(stderr, "WARNING: Doubling ringbuffer offset to %d prevent overlaps with headers.\n",
						config->offsetVal);
			}
			break;

		default:
			fprintf(stderr, "ERROR: Unhandled reader %d, exiting.\n", config->readerType);
			return -1;
	}

	return 0;
}


// Setup functions
//
// @param      input   The input
// @param[in]  config  The configuration
// @param[in]  meta    The meta
// @param[in]  port    The port
//
// @return     { description_of_the_return_value }
//
int lofar_udp_io_read_setup(lofar_udp_reader_input *input, const lofar_udp_config *config, const lofar_udp_meta *meta,
							int port) {

	switch (config->readerType) {
		// Normal files and FIFOs use the same read interface
		case NORMAL:
		case FIFO:
			return lofar_udp_io_read_setup_FILE(input, config, port);


		case ZSTDCOMPRESSED:
			return lofar_udp_io_read_setup_ZSTD(input, config, meta, port);


		case DADA_ACTIVE:
			return lofar_udp_io_read_setup_DADA(input, config, meta, port);


		default:
			fprintf(stderr, "ERROR: Unknown reader (%d) provided, exiting.\n", config->readerType);
			return -1;
	}

}


/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  meta    The meta
 * @param[in]  iter    The iterator
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_setup(lofar_udp_io_write_config *config, const lofar_udp_meta *meta, int iter) {
	int returnVal = 0;
	for (int outp = 0; outp < meta->numOutputs; outp++) {
		printf("Writer %d/%d\n", outp, meta->numOutputs);
		switch (config->readerType) {
			case NORMAL:
			case FIFO:
				returnVal = (returnVal == -1) ? returnVal : lofar_udp_io_write_setup_FILE(config, meta, outp, iter);
				break;

			case ZSTDCOMPRESSED:
				returnVal = (returnVal == -1) ? returnVal : lofar_udp_io_write_setup_ZSTD(config, meta, outp, iter);
				break;


			case DADA_ACTIVE:
				returnVal = (returnVal == -1) ? returnVal : lofar_udp_io_write_setup_DADA(config, meta, outp);
				break;


			default:
				fprintf(stderr, "ERROR: Unknown reader (%d) provided, exiting.\n", config->readerType);
				return -1;
		}
	}

	return returnVal;
}


/**
 * @brief      { function_description }
 *
 * @param      input   The input
 * @param[in]  config  The configuration
 * @param[in]  port    The port
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_read_setup_FILE(lofar_udp_reader_input *input, const lofar_udp_config *config, const int port) {
	input->fileRef[port] = fopen(config->inputLocations[port], "rb");
	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      input   The input
 * @param[in]  config  The configuration
 * @param[in]  meta    The meta
 * @param[in]  port    The port
 *
 * @return     { description_of_the_return_value }
 */
int
lofar_udp_io_read_setup_ZSTD(lofar_udp_reader_input *input, const lofar_udp_config *config, const lofar_udp_meta *meta,
							 const int port) {
	// Copy the file reference into input
	// Leave in boilerplate return encase the function gets more complicated in future
	int returnVal;
	if ((returnVal = lofar_udp_io_read_setup_FILE(input, config, port)) != 0) {
		return returnVal;
	}

	// Find the file size (needed for mmap)
	long fileSize = FILE_file_size(input->fileRef[port]);
	// Ensure there wasn't an error
	if (fileSize < 0) {
		fprintf(stderr, "ERROR: Failed to get size of file at %s, exiting.\n", config->inputLocations[port]);
		return -1;
	}

	// Setup the decompression stream
	input->dstream[port] = ZSTD_createDStream();
	ZSTD_initDStream(input->dstream[port]);

	void *tmpPtr = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fileno(input->fileRef[port]), 0);

	// Setup the compressed data buffer/struct
	input->readingTracker[port].size = fileSize;
	input->readingTracker[port].pos = 0;
	input->readingTracker[port].src = tmpPtr;

	if (tmpPtr == MAP_FAILED) {
		fprintf(stderr, "ERROR: Failed to create memory mapping for file on port %d. Errno: %d (%s). Exiting.\n", port,
				errno, strerror(errno));
		return -2;
	}

	returnVal = madvise(tmpPtr, fileSize, MADV_SEQUENTIAL);

	if (returnVal == -1) {
		fprintf(stderr, "ERROR: Failed to advise the kernel on mmap read stratgy on port %d. Errno: %d. Exiting.\n",
				port, errno);
		return -3;

	}

	// Setup the decompressed data buffer/struct
	long bufferSize = meta->packetsPerIteration * meta->portPacketLength[port];
	VERBOSE(printf("reader_setup: expending decompression buffer by %ld bytes\n",
				   (long) ZSTD_DStreamOutSize() - (bufferSize % (long) ZSTD_DStreamOutSize())));
	bufferSize += (long) ZSTD_DStreamOutSize() - (bufferSize % (long) ZSTD_DStreamOutSize());
	input->decompressionTracker[port].size = bufferSize;
	input->decompressionTracker[port].pos = 0; // Initialisation for our step-by-step reader
	input->decompressionTracker[port].dst = meta->inputData[port];

	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      input   The input
 * @param[in]  config  The configuration
 * @param[in]  meta    The meta
 * @param[in]  port    The port
 *
 * @return     { description_of_the_return_value }
 */
int
lofar_udp_io_read_setup_DADA(lofar_udp_reader_input *input, const lofar_udp_config *config, const lofar_udp_meta *meta,
							 const int port) {
#ifndef NODADA
	// Init the logger and HDU
	input->multilog[port] = multilog_open("udpPacketManager", 0);
	input->dadaReader[port] = dada_hdu_create(input->multilog[port]);

	if (input->multilog[port] == NULL || input->dadaReader[port] == NULL) {
		fprintf(stderr, "ERROR: Unable to initialsie PSRDADA logger on port %d. Exiting.\n", port);
		return -1;
	}

	// If successful, connect to the ingbuffer as a given reader type
	int dadaKey = config->dadaKeys[port];
	if (dadaKey < 0) {
		fprintf(stderr, "ERROR: Invalid PSRDADA ringbuffer key (%d) on port %d, exiting.\n", dadaKey, port);
		return -2;
	}

	dada_hdu_set_key(input->dadaReader[port], dadaKey);
	if (dada_hdu_connect(input->dadaReader[port])) {
		return -3;
	}

	if (dada_hdu_lock_read(input->dadaReader[port])) {
		return -4;
	}

	// If we are restarting, align to the expected packet length
	if ((ipcio_tell(input->dadaReader[port]->data_block) % meta->portPacketLength[port]) != 0) {
		if (ipcio_seek(input->dadaReader[port]->data_block,
					   7824 - (int64_t) (ipcio_tell(input->dadaReader[port]->data_block) % 7824), SEEK_CUR) < 0) {
			return -5;
		}
	}

	input->dadaPageSize[port] = ipcbuf_get_bufsz((ipcbuf_t *) input->dadaReader[port]->data_block);
	if (input->dadaPageSize[port] < 1) {
		fprintf(stderr, "ERROR: Failed to get PSRDADA buffer size on ringbuffer %d for port %d, exiting.\n", dadaKey,
				port);
		return -6;
	}

	input->dadaKey[port] = dadaKey;

	return 0;

#else
	fprintf(stderr, "ERROR: PSRDADA was disabled at compile time, exiting.\n");
	return -1;
#endif
}


/**
 * @brief      Determines if lofar udp i/o write setup check exists.
 *
 * @param      filePath    The file path
 * @param[in]  appendMode  The append mode
 *
 * @return     True if lofar udp i/o write setup check exists, False otherwise.
 */
int lofar_udp_io_write_setup_check_exists(char filePath[], int appendMode) {
	if (appendMode != 1 && access(filePath, F_OK) != -1) {
		fprintf(stderr, "Output file at %s already exists; exiting.\n", filePath);
		return -1;
	}

	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  meta    The meta
 * @param[in]  outp    The outp
 * @param[in]  iter    The iterator
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_setup_FILE(lofar_udp_io_write_config *config, const lofar_udp_meta *meta, int outp, int iter) {
	char outputLocation[DEF_STR_LEN];
	if (lofar_udp_io_parse_format(outputLocation, config->outputFormat, -1, iter, outp, meta->lastPacket) < 0) {
		return -1;
	}

	if (lofar_udp_io_write_setup_check_exists(outputLocation, config->appendExisting)) {
		return -1;
	}

	if (strcpy(config->outputLocations[outp], outputLocation) != config->outputLocations[outp]) {
		fprintf(stderr, "ERROR: Failed to copy output file path (%s), exiting.\n", outputLocation);
		return -1;
	}

	if (config->readerType == FIFO) {
		if (mkfifo(outputLocation, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH) < 0) {
			fprintf(stderr, "ERROR: Failed to create FIFO at %s, exiting.\n", outputLocation);
		}
	}

	FILE *tmpPtr = fopen(outputLocation, "wb");
	if (tmpPtr == NULL) {
		fprintf(stderr, "ERROR: Failed to open output (%d) (%s), exiting.\n", config->readerType, outputLocation);
		return -1;
	}

	if (config->outputFiles[outp] != NULL) {
		lofar_udp_io_write_cleanup(config, outp, 0);
	}

	config->outputFiles[outp] = tmpPtr;

	return 0;
}


/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  meta    The meta
 * @param[in]  outp    The outp
 * @param[in]  iter    The iterator
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_setup_ZSTD(lofar_udp_io_write_config *config, const lofar_udp_meta *meta, int outp, int iter) {
	if (lofar_udp_io_write_setup_FILE(config, meta, outp, iter) < 0) {
		return -1;
	}

	if (config->cparams == NULL) {
		config->cparams = ZSTD_createCCtxParams();
		if (config->cparams == NULL || ZSTD_isError(ZSTD_CCtxParams_init(config->cparams, 3))) {
			fprintf(stderr, "ERROR: Failed to initialised compression parameters, exiting.\n");
			return -1;
		}
	}

	ZSTD_CCtx_setParameter(config->cstream[outp], ZSTD_c_nbWorkers, 4);

	if (config->cstream[outp] == NULL) {
		size_t zstdReturn;
		for (int port = 0; port < meta->numOutputs; port++) {
			zstdReturn = ZSTD_initCStream(config->cstream[port], ZSTD_COMP_LEVEL);

			if (ZSTD_isError(zstdReturn)) {
				fprintf(stderr,
						"ZSTD encountered an error while creating compression stream for [%d] (code %ld, %s), exiting.\n",
						port, zstdReturn, ZSTD_getErrorName(zstdReturn));
				lofar_udp_io_write_cleanup(config, outp, 1);
				return -1;
			}
		}

		long inputSize;
		for (int port = 0; port < meta->numOutputs; port++) {
			inputSize = meta->packetsPerIteration * meta->packetOutputLength[outp];
			if (inputSize % ZSTD_CStreamInSize() != 0) {
				inputSize += (long) ZSTD_CStreamInSize() - (inputSize % (long) ZSTD_CStreamInSize());
			}


			if (inputSize % ZSTD_CStreamOutSize() != 0) {
				inputSize += (long) ZSTD_CStreamInSize() - (inputSize % (long) ZSTD_CStreamOutSize());
			}
			config->outputBuffer[outp].dst = calloc(inputSize, sizeof(char));

			if (config->outputBuffer[outp].dst == NULL) {
				fprintf(stderr, "ERROR: Failed to allocate memory for ZTSD compression, exiting.\n");
			}
			config->outputBuffer[outp].size = inputSize;

		}
	}

	if (ZSTD_isError(ZSTD_CCtx_reset(config->cstream[outp], ZSTD_reset_session_only))) {
		fprintf(stderr, "ERROR: Failed to reset ZSTD context; exiting.\n");
		return -1;
	}
	if (ZSTD_isError(ZSTD_CCtx_setParametersUsingCCtxParams(config->cstream[outp], config->cparams))) {
		fprintf(stderr, "ERROR: Failed to initialise compression parameters, exiting.\n");
		return -1;
	}


	return -1;
	//return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  meta    The meta
 * @param[in]  outp    The outp
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_setup_DADA(lofar_udp_io_write_config *config, const lofar_udp_meta *meta, int outp) {
#ifndef NODADA

	if (config->dadaWriter[outp][0] == NULL) {
		config->dadaWriter[outp][0] = calloc(1, sizeof(ipcio_t));
		config->dadaWriter[outp][1] = calloc(1, sizeof(ipcio_t));
		*(config->dadaWriter[outp][0]) = IPCIO_INIT;
		*(config->dadaWriter[outp][1]) = IPCIO_INIT;
		// Create  and connect to the ringbuffer instance
		if (ipcio_create(config->dadaWriter[outp][0], config->outputDadaKeys[outp], 8,
						 meta->packetsPerIteration * meta->packetOutputLength[outp], 1) < 0) {
			// ipcio_create(...) prints error to stderr, so we just need to exit.
			return -1;
		}

		// Create  and connect to the header buffer instance
		if (ipcio_create(config->dadaWriter[outp][1], config->outputDadaKeys[outp] + 1, 1, DADA_DEFAULT_HEADER_SIZE,
						 1) < 0) {
			// ipcio_create(...) prints error to stderr, so we just need to exit.
			return -1;
		}

		if (sprintf(config->outputLocations[outp], "PSRDADA:%d", config->outputDadaKeys[outp])) {
			fprintf(stderr, "ERROR: Failed to copy output file path (PSRDADA:%d), exiting.\n",
					config->outputDadaKeys[outp]);
			return -1;
		}

		// Open the ringbuffer instance as the primary writer
		if (ipcio_open(config->dadaWriter[outp][0], 'W') < 0) {
			// ipcio_open(...) prints error to stderr, so we just need to exit.
			return -2;
		}

		// Open the header buffer instance as the primary writer
		if (ipcio_open(config->dadaWriter[outp][1], 'W') < 0) {
			// ipcio_open(...) prints error to stderr, so we just need to exit.
			return -2;
		}
	}
#else

#endif
	return -1;
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
long lofar_udp_io_read(lofar_udp_reader_input *input, int port, char *targetArray, long nchars) {

	// Sanity check input
	if (nchars < 0) {
		fprintf(stderr, "ERROR: Requested negative read size %ld on port %d, exiting.\n", nchars, port);
		return -1;
	}
	if (input == NULL) {
		fprintf(stderr, "ERROR: Inputs were nulled at some point, cannot read new data, exiting.\n");
		return -1;
	}

	switch (input->readerType) {
		case NORMAL:
		case FIFO:
			return lofar_udp_io_read_FILE(input, port, targetArray, nchars);


		case ZSTDCOMPRESSED:
			return lofar_udp_io_read_ZSTD(input, port, nchars);


		case DADA_ACTIVE:
			return lofar_udp_io_read_DADA(input, port, targetArray, nchars);


		default:
			fprintf(stderr, "ERROR: Unknown reader %d, exiting.\n", input->readerType);
			return -1;

	}
}

/**
 * @brief      { function_description }
 *
 * @param      input        The input
 * @param[in]  port         The port
 * @param      targetArray  The target array
 * @param[in]  nchars       The nchars
 *
 * @return     { description_of_the_return_value }
 */
long lofar_udp_io_read_FILE(lofar_udp_reader_input *input, const int port, char *targetArray, const long nchars) {
	// Decompressed file: Read and return the data as needed
	VERBOSE(printf("reader_nchars: Entering read request (normal): %d, %ld\n", port, nchars));
	return fread(targetArray, sizeof(char), nchars, input->fileRef[port]);
}

/**
 * @brief      { function_description }
 *
 * @param      input   The input
 * @param[in]  port    The port
 * @param[in]  nchars  The nchars
 *
 * @return     { description_of_the_return_value }
 */
long lofar_udp_io_read_ZSTD(lofar_udp_reader_input *input, int port, long nchars) {
	// Compressed file: Perform streaming decompression on a zstandard compressed file
	VERBOSE(printf("reader_nchars: Entering read request (compressed): %d, %ld\n", port, nchars));

	long dataRead = 0, byteDelta = 0;
	size_t previousDecompressionPos = 0;
	int returnVal = 0;

	VERBOSE(printf("reader_nchars: start of ZSTD read loop, %ld, %ld, %ld, %ld\n", input->readingTracker[port].pos,
				   input->readingTracker[port].size, input->decompressionTracker[port].pos, dataRead););

	// Loop across while decompressing the data (zstd decompressed in frame iterations, so it may take a few iterations)
	while (input->readingTracker[port].pos < input->readingTracker[port].size) {
		previousDecompressionPos = input->decompressionTracker[port].pos;
		// zstd streaming decompression + check for errors
		returnVal = ZSTD_decompressStream(input->dstream[port], &(input->decompressionTracker[port]),
										  &(input->readingTracker[port]));
		if (ZSTD_isError(returnVal)) {
			fprintf(stderr, "ZSTD encountered an error decompressing a frame (code %d, %s), exiting data read early.\n",
					returnVal, ZSTD_getErrorName(returnVal));
			return dataRead;
		}

		// Determine how much data we just added to the buffer
		byteDelta = ((long) input->decompressionTracker[port].pos - (long) previousDecompressionPos);

		// Update the total data read + check if we have reached our goal
		dataRead += byteDelta;
		VERBOSE(if (dataRead >= nchars) {
			printf("Reader terminating: %ld read, %ld requested, %ld\n", dataRead, nchars, nchars - dataRead);
		});

		if (dataRead >= nchars) {
			return dataRead;
		}

		if (input->decompressionTracker[port].pos == input->decompressionTracker[port].size) {
			fprintf(stderr,
					"Failed to read %ld/%ld chars on port %d before filling the buffer. Attempting to continue...\n",
					dataRead, nchars, port);
			return dataRead;
		}
	}

	// EOF: return everything we read
	return dataRead;
}

/**
 * @brief      { function_description }
 *
 * @param      input        The input
 * @param[in]  port         The port
 * @param      targetArray  The target array
 * @param[in]  nchars       The nchars
 *
 * @return     { description_of_the_return_value }
 */
long lofar_udp_io_read_DADA(lofar_udp_reader_input *input, const int port, char *targetArray, const long nchars) {
#ifndef NODADA

	VERBOSE(printf("reader_nchars: Entering read request (dada): %d, %d, %ld\n", port, input->dadaKey[port], nchars));

	long dataRead = 0, currentRead;

	// To prevent a lock up when we call ipcio_stop, read the data in gulps
	long readChars = ((dataRead + input->dadaPageSize[port]) > nchars) ? (nchars - dataRead)
																	   : input->dadaPageSize[port];
	// Loop until we read the requested amount of data
	while (dataRead < nchars) {
		if ((currentRead = ipcio_read(input->dadaReader[port]->data_block, &(targetArray[dataRead]), readChars)) < 0) {
			fprintf(stderr, "ERROR: Failed to complete DADA read on port %d (%ld / %ld), returning partial data.\n",
					port, dataRead, nchars);
			// Return -1 if we haven't read anything
			return (dataRead > 0) ? dataRead : -1;
		}
		dataRead += currentRead;
		readChars = ((dataRead + input->dadaPageSize[port]) > nchars) ? (nchars - dataRead) : input->dadaPageSize[port];
	}

	return dataRead;

#else
	// Raise an error if PSRDADA wasn't available at compile time
	fprintf(stderr, "ERROR: PSRDADA was disable at compile time, exiting.\n");
	return -1;

#endif
}






// Write Functions

/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 * @param      src     The source
 * @param[in]  nchars  The nchars
 *
 * @return     { description_of_the_return_value }
 */
long lofar_udp_io_write(lofar_udp_io_write_config *config, int outp, char *src, const long nchars) {
	// Sanity check input
	if (nchars < 0) {
		fprintf(stderr, "ERROR: Requested negative write size %ld on output %d, exiting.\n", nchars, outp);
		return -1;
	}
	if (config == NULL) {
		fprintf(stderr, "ERROR: Write config was nulled at some point, cannot read new data, exiting.\n");
		return -1;
	}

	switch (config->readerType) {
		case NORMAL:
		case FIFO:
			return lofar_udp_io_write_FILE(config, outp, src, nchars);


		case ZSTDCOMPRESSED:
			return lofar_udp_io_write_ZSTD(config, outp, src, nchars);


		case DADA_ACTIVE:
			return lofar_udp_io_write_DADA(config, outp, src, nchars);


		default:
			fprintf(stderr, "ERROR: Unknown reader %d, exiting.\n", config->readerType);
			return -1;

	}
}

/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 * @param      src     The source
 * @param[in]  nchars  The nchars
 *
 * @return     { description_of_the_return_value }
 */
long lofar_udp_io_write_FILE(lofar_udp_io_write_config *config, int outp, char *src, const long nchars) {
	return fwrite(src, sizeof(char), nchars, config->outputFiles[outp]);
}

/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 * @param      src     The source
 * @param[in]  nchars  The nchars
 *
 * @return     { description_of_the_return_value }
 */
long
lofar_udp_io_write_ZSTD(lofar_udp_io_write_config *config, int outp, char *src, const long nchars) {

	ZSTD_inBuffer input = { src, nchars, 0 };
	ZSTD_outBuffer output = { config->outputBuffer[outp].dst, config->outputBuffer[outp].size, 0 };
	while (!(input.size == input.pos)) {
		size_t returnVal = ZSTD_compressStream2(config->cstream[outp], &output, &input, ZSTD_e_continue);
		if (ZSTD_isError(returnVal)) {
			fprintf(stderr, "ERROR: Failed to compressed data with ZSTD (%ld, %s)\n", returnVal,
					ZSTD_getErrorName(returnVal));
			return -1;
		}
	}

	if (lofar_udp_io_write_FILE(config, outp, config->outputBuffer[outp].dst, config->outputBuffer[outp].pos) < 0) {
		return -1;
	}

	return nchars;
}


/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 * @param      src     The source
 * @param[in]  nchars  The nchars
 *
 * @return     { description_of_the_return_value }
 */
long
lofar_udp_io_write_DADA(lofar_udp_io_write_config *config, int outp, char *src, const long nchars) {

	long writtenBytes = ipcio_write(config->dadaWriter[outp][0], src, nchars);
	if (writtenBytes < 0) {
		return -1;
	}

	return writtenBytes;
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
int
lofar_udp_io_fread_temp(const lofar_udp_config *config, const int port, void *outbuf, const size_t size, const int num,
						const int resetSeek) {
	if (num < 1 || size < 1) {
		fprintf(stderr, "ERROR: Invalid number of elements to read (%d * %ld), exiting.\n", num, size);
	}

	switch (config->readerType) {
		case NORMAL:
		case FIFO:
			return lofar_udp_io_fread_temp_FILE(outbuf, size, num, config->inputLocations[port], resetSeek);


		case ZSTDCOMPRESSED:
			return lofar_udp_io_fread_temp_ZSTD(outbuf, size, num, config->inputLocations[port], resetSeek);


		case DADA_ACTIVE:
			return lofar_udp_io_fread_temp_DADA(outbuf, size, num, config->dadaKeys[port], resetSeek);


		default:
			fprintf(stderr, "ERROR: Unknown reader type (%d), exiting.\n", config->readerType);
			return -1;
	}

}

/**
 * @brief      { function_description }
 *
 * @param      outbuf     The outbuf
 * @param[in]  size       The size
 * @param[in]  num        The number
 * @param[in]  inputFile  The input file
 * @param[in]  resetSeek  The reset seek
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_fread_temp_FILE(void *outbuf, const size_t size, const int num, const char inputFile[],
								 const int resetSeek) {
	FILE *inputFilePtr = fopen(inputFile, "rb");
	if (inputFilePtr == NULL) {
		fprintf(stderr, "ERROR: Unable to open normal file at %s, exiting.\n", inputFile);
		return -1;
	}

	size_t readlen = fread(inputFilePtr, size, num, outbuf);

	if (resetSeek) {
		if (fseek(inputFilePtr, size * -num, SEEK_CUR) != 0) {
			fprintf(stderr, "Failed to reset seek head, exiting.\n");
			fclose(inputFilePtr);
			return -2;
		}
	}

	fclose(inputFilePtr);

	if (readlen != size) {
		fprintf(stderr, "Unable to read %ld elements from file, exiting.\n", size * num);
		return -1;
	}

	return 0;
}


/**
 * @brief      Temporarily read in num bytes from a zstandard compressed file
 *
 * @param      outbuf     The output buffer pointer
 * @param[in]  size       The size of words ot read
 * @param[in]  num        The number of words to read
 * @param      inputFile  The input file
 * @param[in]  resetSeek  Do (1) / Don't (0) reset back to the original location
 *                        in FILE* after performing a read operation
 *
 * @return     int 0: ZSTD error, 1: File error, other: data read length
 */
int lofar_udp_io_fread_temp_ZSTD(void *outbuf, const size_t size, const int num, const char inputFile[],
								 const int resetSeek) {
	printf("%s, %ld\n", inputFile, strlen(inputFile));
	FILE *inputFilePtr = fopen(inputFile, "r");
	if (inputFilePtr == NULL) {
		fprintf(stderr, "ERROR: Unable to open normal file at %s, exiting.\n", inputFile);
		return -1;
	}

	// Build the decompression stream
	ZSTD_DStream *dstreamTmp = ZSTD_createDStream();
	const size_t minRead = ZSTD_DStreamInSize();
	const size_t minOut = ZSTD_DStreamOutSize();
	ZSTD_initDStream(dstreamTmp);

	// Allocate the buffer memory, build the buffer structs
	char *inBuff = calloc(minRead, sizeof(char));
	char *localOutBuff = calloc(minOut, sizeof(char));

	ZSTD_inBuffer tmpRead = { inBuff, minRead * sizeof(char), 0 };
	ZSTD_outBuffer tmpDecom = { localOutBuff, minOut * sizeof(char), 0 };

	// Read in the compressed data
	size_t readLen = fread(inBuff, sizeof(char), minRead, inputFilePtr);
	if (readLen != minRead) {
		fprintf(stderr, "Unable to read in header from file; exiting.\n");
		fclose(inputFilePtr);
		ZSTD_freeDStream(dstreamTmp);
		free(inBuff);
		free(localOutBuff);
		return -1;
	}

	// Move the read head back to the start of the packer
	if (resetSeek) {
		if (fseek(inputFilePtr, -minRead, SEEK_CUR) != 0) {
			fprintf(stderr, "Failed to reset seek head, exiting.\n");
			fclose(inputFilePtr);
			return -2;
		}
	}

	fclose(inputFilePtr);

	// Decompressed the data, check for errors
	size_t output = ZSTD_decompressStream(dstreamTmp, &tmpDecom, &tmpRead);

	VERBOSE(printf("Header decompression code: %ld, %s\n", output, ZSTD_getErrorName(output)));

	if (ZSTD_isError(output)) {
		fprintf(stderr, "ZSTD enountered an error while doing temp read (code %ld, %s), exiting.\n", output,
				ZSTD_getErrorName(output));
		ZSTD_freeDStream(dstreamTmp);
		free(inBuff);
		free(localOutBuff);
		return -1;
	}

	// Cap the return value of the data
	readLen = tmpDecom.pos;
	if (readLen > size * num) { readLen = size * num; }

	// Copy the output and cleanup
	memcpy(outbuf, localOutBuff, size * num);
	ZSTD_freeDStream(dstreamTmp);
	free(inBuff);
	free(localOutBuff);

	return readLen;

}


/**
 * @brief      Temporarily read in num bytes from a PSRDADA ringbuffer
 *
 * @param      outbuf     The output buffer pointer
 * @param[in]  size       The size of words ot read
 * @param[in]  num        The number of words to read
 * @param      dadaKey    The PSRDADA ringbuffer ID
 * @param[in]  resetSeek  Do (1) / Don't (0) reset back to the original location
 *                        in FILE* after performing a read operation
 *
 * @return     int 0: ZSTD error, 1: File error, other: data read length
 */
int
lofar_udp_io_fread_temp_DADA(void *outbuf, const size_t size, const int num, const int dadaKey, const int resetSeek) {
#ifndef NODADA
	ipcio_t tmpReader = IPCIO_INIT;
	// Only use an active reader for now, I just don't understand how the passive reader works.
	char readerChar = 'R';
	if (dadaKey < 1) {
		fprintf(stderr, "ERROR: Invalid PSRDADA ringbuffer key %d (%x), exiting.\n", dadaKey, dadaKey);
	}
	// All of these functions print their own error messages.

	// Connecting to an ipcio_t struct allows you to control it like any other file descriptor usng the ipcio_* functions
	// As a result, after connecting to the buffer...
	if (ipcio_connect(&tmpReader, dadaKey) < 0) {
		return -1;
	}

	if (ipcbuf_get_reader_conn(&(tmpReader.buf)) == 0) {
		printf("ERROR: Reader already active on ringbuffer %d (%x). Exiting.\n", dadaKey, dadaKey);
		//readerChar = 'r';
	}

	// We can fopen()....
	if (ipcio_open(&tmpReader, 'R') < 0) {
		return -1;
	}

	// Passive reading needs at least 1 read before tell returns sane values
	// Passive reader currently removed from implementation, as I can't
	// figure out how it works for the life of me. Is it based on the writer?
	// Is it based on the reader? Why does it get blocked by the reader, but
	// updated by the writer?
	if (readerChar == 'r') {
		if (ipcio_read(&tmpReader, 0, 1) != 1) {
			return -1;
		}

		if (ipcio_seek(&tmpReader, -1, SEEK_CUR) < 0) {
			return -1;
		}
	}

	// Fix offset if we are joining in the middle of a run
	// TODO: read packet length form header rather than hard coding to 7824
	if (ipcio_tell(&tmpReader) != 0) {
		if (ipcio_seek(&tmpReader, 7824 - (int64_t) (ipcio_tell(&tmpReader) % 7824), SEEK_CUR) < 0) {
			return -1;
		}
	}

	// Then fread()...
	size_t returnlen = ipcio_read(&tmpReader, outbuf, size * num);

	// fseek() if requested....
	if (resetSeek == 1) {
		if (ipcio_seek(&tmpReader, -num, SEEK_CUR) < 0) {
			return -1;
		}
	}

	// Only the active reader needs an explicit close, passive reader will raise an error here
	if (readerChar == 'R') {
		if (ipcio_close(&tmpReader) < 0) {
			return -1;
		}
	}

	if (ipcio_disconnect(&tmpReader) < 0) {
		return -1;
	}

	if (returnlen != size * num) {
		fprintf(stderr, "Unable to read %ld elements from ringbuffer %d, exiting.\n", size * num, dadaKey);
		return -1;
	}

	return returnlen;
#else
	fprintf(stderr, "ERROR: PSRDADA not enabled at compile time, exiting.\n");
			return -1;
#endif
}


// Cleanup functions
//
// @param      input  The input
// @param[in]  port   The port
//
// @return     { description_of_the_return_value }
//
int lofar_udp_io_read_cleanup(lofar_udp_reader_input *input, const int port) {

	switch (input->readerType) {
		// Normal files and FIFOs use the same interface
		case NORMAL:
		case FIFO:
			return lofar_udp_io_read_cleanup_FILE(input, port);


		case ZSTDCOMPRESSED:
			return lofar_udp_io_read_cleanup_ZSTD(input, port);


		case DADA_ACTIVE:
			return lofar_udp_io_read_cleanup_DADA(input, port);


		default:
			fprintf(stderr, "ERROR: Unknown reader (%d) provided, exiting.\n", input->readerType);
			return -1;
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
int lofar_udp_io_write_cleanup(lofar_udp_io_write_config *config, const int outp, const int fullClean) {

	switch (config->readerType) {
		// Normal files and FIFOs use the same interface
		case NORMAL:
		case FIFO:
			return lofar_udp_io_write_cleanup_FILE(config, outp, fullClean);


		case ZSTDCOMPRESSED:
			return lofar_udp_io_write_cleanup_ZSTD(config, outp, fullClean);


		case DADA_ACTIVE:
			return lofar_udp_io_write_cleanup_DADA(config, outp, fullClean);


		default:
			fprintf(stderr, "ERROR: Unknown type (%d) provided, exiting.\n", config->readerType);
			return -1;
	}
}

/**
 * @brief      { function_description }
 *
 * @param      input  The input
 * @param[in]  port   The port
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_read_cleanup_FILE(lofar_udp_reader_input *input, const int port) {
	if (input->fileRef[port] != NULL) {
		VERBOSE(printf("On port: %d closing file\n", port))
		fclose(input->fileRef[port]);
		input->fileRef[port] = NULL;
	}

	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      input  The input
 * @param[in]  port   The port
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_read_cleanup_ZSTD(lofar_udp_reader_input *input, const int port) {
	if (lofar_udp_io_read_cleanup_FILE(input, port) != 0) {
		fprintf(stderr, "ERROR: Failed to close file on port %d\n", port);
	}

	// Free the decompression stream
	if (input->dstream[port] != NULL) {
		VERBOSE(printf("Freeing decompression buffers and ZSTD stream on port %d\n", port););
		ZSTD_freeDStream(input->dstream[port]);
		void *tmpPtr = (void *) input->readingTracker[port].src;
		munmap(tmpPtr, input->readingTracker[port].size);
		input->dstream[port] = NULL;
	}

	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      input  The input
 * @param[in]  port   The port
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_read_cleanup_DADA(lofar_udp_reader_input *input, const int port) {
#ifndef NODADA
	if (input->dadaReader[port] != NULL) {
		if (dada_hdu_unlock_read(input->dadaReader[port]) < 0) {
			fprintf(stderr, "ERROR: Failed to close PSRDADA buffer %d on port %d.\n", input->dadaKey[port], port);
		}

		if (dada_hdu_disconnect(input->dadaReader[port]) < 0) {
			fprintf(stderr, "ERROR: Failed to disconnect from PSRDADA buffer %d on port %d.\n", input->dadaKey[port],
					port);
		}
	}

	if (input->multilog[port] != NULL) {
		if (multilog_close(input->multilog[port]) < 0) {
			fprintf(stderr, "ERROR: Failed to close PSRDADA multilogger struct on port %d.\n", port);
		}
	}
#endif

	return 0;
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
int lofar_udp_io_write_cleanup_FILE(lofar_udp_io_write_config *config, int outp, int fullClean) {
	if (config->outputFiles[outp] != NULL) {
		fclose(config->outputFiles[outp]);

		if (fullClean && config->readerType == FIFO) {
			if (remove(config->outputLocations[outp]) != 0) {
				fprintf(stderr, "WARNING: Failed to remove old FIFO at %s, continuing.\n",
						config->outputLocations[outp]);
			}
		}
	}
	return 0;
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
int lofar_udp_io_write_cleanup_ZSTD(lofar_udp_io_write_config *config, int outp, int fullClean) {
	lofar_udp_io_write_cleanup_FILE(config, outp, fullClean);

	if (fullClean && config->cstream[outp] != NULL) {
		ZSTD_freeCStream(config->cstream[outp]);

		if (config->outputBuffer[outp].dst != NULL) {
			free(config->outputBuffer[outp].dst);
			config->outputBuffer[outp].dst = NULL;
		}
	}

	if (fullClean && config->cparams != NULL) {
		ZSTD_freeCCtxParams(config->cparams);
		config->cparams = NULL;
	}

	return 0;
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
__attribute__((deprecated)) int
lofar_udp_io_write_cleanup_DADA(lofar_udp_io_write_config *config, int outp, int fullClean) {
	return -1;
}

// Metadata functions /**
//  * { list_item_description }
// @brief      Get the size of a file ptr on disk
//
// @param      fileptr  The fileptr
// @param[in]  fd    File descriptor
//
// @return     long: size of file on disk in bytes */
//
long FILE_file_size(FILE *fileptr) {
	return fd_file_size(fileno(fileptr));
}

/**
 * @brief      Get the size of a file descriptor on disk
 *
 * @param[in]  fd    File descriptor
 *
 * @return     long: size of file on disk in bytes
 */
long fd_file_size(int fd) {
	struct stat stat_s;
	int status = fstat(fd, &stat_s);

	if (status == -1) {
		fprintf(stderr, "ERROR: Unable to stat input file for fd %d. Errno: %d. Exiting.\n", fd, errno);
		return -1;
	}

	return stat_s.st_size;
}