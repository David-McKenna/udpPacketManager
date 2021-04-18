#include "lofar_udp_io.h"


// Internal functions
void lofar_udp_io_cleanup_DADA_loop(ipcbuf_t *buff, float timeout);
int lofar_udp_io_write_setup_DADA_ringbuffer(ipcio_t **ringbuffer, int dadaKey, uint64_t nbufs, long bufSize, unsigned int numReaders, int appendExisting);

// Setup functions
//
// @param      input   The input
// @param[in]  config  The configuration
// @param[in]  meta    The meta
// @param[in]  port    The port
//
// @return     { description_of_the_return_value }
//
int lofar_udp_io_read_setup(lofar_udp_io_read_config *input, int port) {

	switch (input->readerType) {
		// Normal files and FIFOs use the same read interface
		case NORMAL:
		case FIFO:
			return lofar_udp_io_read_setup_FILE(input, input->inputLocations[port], port);


		case ZSTDCOMPRESSED:
			return lofar_udp_io_read_setup_ZSTD(input, input->inputLocations[port], port);


		case DADA_ACTIVE:
			return lofar_udp_io_read_setup_DADA(input, input->dadaKeys[port], port);


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
int lofar_udp_io_write_setup(lofar_udp_io_write_config *config, int iter) {
	int returnVal = 0;
	for (int outp = 0; outp < config->numOutputs; outp++) {
		switch (config->readerType) {
			case NORMAL:
			case FIFO:
				returnVal = (returnVal == -1) ? returnVal : lofar_udp_io_write_setup_FILE(config, outp, iter);
				break;

			case ZSTDCOMPRESSED:
				returnVal = (returnVal == -1) ? returnVal : lofar_udp_io_write_setup_ZSTD(config, outp, iter);
				break;


			case DADA_ACTIVE:
				returnVal = (returnVal == -1) ? returnVal : lofar_udp_io_write_setup_DADA(config, outp);
				break;


			default:
				fprintf(stderr, "ERROR: Unknown reader (%d) provided, exiting.\n", config->readerType);
				return -1;
		}
	}

	return returnVal;
}


int lofar_udp_io_read_setup_helper(lofar_udp_io_read_config *input, const lofar_udp_config *config, const lofar_udp_meta *meta,
                                   int port) {
	if (input->readerType == NO_INPUT) {
		input->readerType = config->readerType;
	}

	if (input->readerType != config->readerType) {
		fprintf(stderr, "ERROR: Mismatch between reader types on port %d (%d vs %d), exiting.\n", port, input->readerType, config->readerType);
		return -1;
	}

	input->portPacketLength[port] = meta->portPacketLength[port];
	input->readBufSize[port] = meta->packetsPerIteration * meta->portPacketLength[port];

	if (input->readerType == ZSTDCOMPRESSED) {
		input->decompressionTracker[port].dst = meta->inputData[port];
	}

	if (input->readerType == DADA_ACTIVE) {
		input->dadaKeys[port] = config->dadaKeys[port];
	}

	if (strcpy(input->inputLocations[port], config->inputLocations[port]) != input->inputLocations[port]) {
		fprintf(stderr, "ERROR: Failed to copy input location to reader on port %d, exiting.\n", port);
		return -1;
	}

	return lofar_udp_io_read_setup(input, port);
}

int lofar_udp_io_write_setup_helper(lofar_udp_io_write_config *config, const lofar_udp_meta *meta, int iter) {
	config->numOutputs = meta->numOutputs;
	for (int outp = 0; outp < config->numOutputs; outp++) {
		config->writeBufSize[outp] = meta->packetsPerIteration * meta->packetOutputLength[outp];
	}

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
int lofar_udp_io_read_cleanup(lofar_udp_io_read_config *input, const int port) {

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
			return lofar_udp_io_write_cleanup_FILE(config, outp);


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
 * @param[in]  optargc     The optarg
 * @param      fileFormat  The file format
 * @param      baseVal     The base value
 * @param      offsetVal   The offset value
 *
 * @return     { description_of_the_return_value }
 */
reader_t lofar_udp_io_parse_type_optarg(const char optargc[], char *fileFormat, int *baseVal, int *offsetVal) {

	reader_t reader;

	if (strstr(optargc, "%") != NULL) {
		fprintf(stderr, "\n\nWARNING: A %% was detected in your input, UPM v0.7+ no longer uses these for indexing.\n");
		fprintf(stderr, "WARNING: Use [[port]], [[outp]], [[pack]] and [[iter]] instead, check the docs for more details.\n\n");
	}

	if (optargc[4] == ':') {
		sscanf(optargc, "%*[^:]:%[^,],%d,%d", fileFormat, baseVal, offsetVal);

		if (strstr(optargc, "FILE:") != NULL) {
			reader = NORMAL;
		} else if (strstr(optargc, "FIFO:") != NULL) {
			reader = FIFO;
		} else if (strstr(optargc, "DADA:") != NULL) {
			reader = DADA_ACTIVE;
		} else {
			// Unknown prefix
			reader = NO_INPUT;
		}
	} else {
		reader = NORMAL;
		sscanf(optargc, "%[^,],%d,%d", fileFormat, baseVal, offsetVal);
	}


	if (strstr(fileFormat, ".zst") != NULL) {
		VERBOSE(printf("%s, COMPRESSED\n", fileFormat));
		reader = ZSTDCOMPRESSED;
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
int lofar_udp_io_parse_format(char *dest, const char format[], int port, int iter, int outp, long pack) {

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

	if (config == NULL) {
		fprintf(stderr, "ERROR: Input config is NULL, exiting.\n");
		return -1;
	}

	char fileFormat[DEF_STR_LEN];
	int baseVal = 0;
	int offsetVal = 1;
	config->readerType = lofar_udp_io_parse_type_optarg(optargc, fileFormat, &baseVal, &offsetVal);

	if (config->readerType == -1) {
		fprintf(stderr, "ERROR: Failed to parse input pattern (%s), exiting.\n", optargc);
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
 * @param[in]  optargc  The optarg
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_parse_optarg(lofar_udp_io_write_config *config, const char optargc[]) {

	if (config == NULL) {
		fprintf(stderr, "ERROR: Input config is NULL, exiting.\n");
		return -1;
	}

	config->readerType = lofar_udp_io_parse_type_optarg(optargc, config->outputFormat, &(config->baseVal),
														&(config->offsetVal));

	if (config->readerType == -1) {
		fprintf(stderr, "ERROR: Failed to parse input pattern (%s), exiting.\n", optargc);
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


/**
 * @brief      { function_description }
 *
 * @param      input   The input
 * @param[in]  config  The configuration
 * @param[in]  port    The port
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_read_setup_FILE(lofar_udp_io_read_config *input, const char *inputLocation, const int port) {
	input->fileRef[port] = fopen(inputLocation, "rb");
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
lofar_udp_io_read_setup_ZSTD(lofar_udp_io_read_config *input, const char *inputLocation, const int port) {
	// Copy the file reference into input
	// Leave in boilerplate return encase the function gets more complicated in future
	int returnVal;
	if ((returnVal = lofar_udp_io_read_setup_FILE(input, inputLocation, port)) != 0) {
		return returnVal;
	}

	// Find the file size (needed for mmap)
	long fileSize = FILE_file_size(input->fileRef[port]);
	// Ensure there wasn't an error
	if (fileSize < 0) {
		fprintf(stderr, "ERROR: Failed to get size of file at %s, exiting.\n", inputLocation);
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
		fprintf(stderr, "ERROR: Failed to advise the kernel on mmap read strategy on port %d. Errno: %d. Exiting.\n",
				port, errno);
		return -3;

	}

	// Setup the decompressed data buffer/struct
	long bufferSize = input->readBufSize[port];
	VERBOSE(printf("reader_setup: expending decompression buffer by %ld bytes\n",
				   (long) ZSTD_DStreamOutSize() - (bufferSize % (long) ZSTD_DStreamOutSize())));
	bufferSize += (long) ZSTD_DStreamOutSize() - (bufferSize % (long) ZSTD_DStreamOutSize());
	input->decompressionTracker[port].size = bufferSize;
	input->decompressionTracker[port].pos = 0; // Initialisation for our step-by-step reader

	input->readBufSize[port] = bufferSize;

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
lofar_udp_io_read_setup_DADA(lofar_udp_io_read_config *input, const int dadaKey, const int port) {
#ifndef NODADA
	// Init the logger and HDU
	input->multilog[port] = multilog_open("udpPacketManager", 0);
	input->dadaReader[port] = dada_hdu_create(input->multilog[port]);

	if (input->multilog[port] == NULL || input->dadaReader[port] == NULL) {
		fprintf(stderr, "ERROR: Unable to initialsie PSRDADA logger on port %d. Exiting.\n", port);
		return -1;
	}

	// If successful, connect to the ingbuffer as a given reader type
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
	if ((ipcio_tell(input->dadaReader[port]->data_block) % input->portPacketLength[port]) != 0) {
		if (ipcio_seek(input->dadaReader[port]->data_block,
					   7824 - (int64_t) (ipcio_tell(input->dadaReader[port]->data_block) % 7824), SEEK_CUR) < 0) {
			return -5;
		}
	}

	input->dadaPageSize[port] = (long) ipcbuf_get_bufsz((ipcbuf_t *) input->dadaReader[port]->data_block);
	if (input->dadaPageSize[port] < 1) {
		fprintf(stderr, "ERROR: Failed to get PSRDADA buffer size on ringbuffer %d for port %d, exiting.\n", dadaKey,
				port);
		return -6;
	}

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
 * @param[in]  outp    The outp
 * @param[in]  iter    The iterator
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_setup_FILE(lofar_udp_io_write_config *config, int outp, int iter) {
	char outputLocation[DEF_STR_LEN];
	if (lofar_udp_io_parse_format(outputLocation, config->outputFormat, -1, iter, outp, config->firstPacket) < 0) {
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
 * @param[in]  outp    The outp
 * @param[in]  iter    The iterator
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_setup_ZSTD(lofar_udp_io_write_config *config, int outp, int iter) {
	if (lofar_udp_io_write_setup_FILE(config, outp, iter) < 0) {
		return -1;
	}

	if (config->cparams == NULL) {
		config->cparams = ZSTD_createCCtxParams();
		if (config->cparams == NULL || ZSTD_isError(ZSTD_CCtxParams_init(config->cparams, 3))) {
			fprintf(stderr, "ERROR: Failed to initialised compression parameters, exiting.\n");
			return -1;
		}
	}

	if (config->zstdWriter[outp].cstream == NULL) {
		size_t zstdReturn;

		config->zstdWriter[outp].cstream = ZSTD_createCStream();
		if (config->zstdWriter[outp].cstream == NULL) {
			fprintf(stderr, "ERROR: Failed to create compression stream on output %d, exiting.\n", outp);
			return -1;
		}
		zstdReturn = ZSTD_initCStream(config->zstdWriter[outp].cstream, 3); //ZSTD_COMP_LEVEL);

		if (ZSTD_isError(zstdReturn)) {
			fprintf(stderr,
					"ZSTD encountered an error while creating compression stream for [%d] (code %ld, %s), exiting.\n",
					outp, zstdReturn, ZSTD_getErrorName(zstdReturn));
			lofar_udp_io_write_cleanup(config, outp, 1);
			return -1;
		}

		long inputSize = config->writeBufSize[outp];
		if (inputSize % ZSTD_CStreamInSize() != 0) {
			inputSize += (long) ZSTD_CStreamInSize() - (inputSize % (long) ZSTD_CStreamInSize());
		}


		if (inputSize % ZSTD_CStreamOutSize() != 0) {
			inputSize += (long) ZSTD_CStreamInSize() - (inputSize % (long) ZSTD_CStreamOutSize());
		}
		config->zstdWriter[outp].compressionBuffer.dst = calloc(inputSize, sizeof(char));

		if (config->zstdWriter[outp].compressionBuffer.dst == NULL) {
			fprintf(stderr, "ERROR: Failed to allocate memory for ZTSD compression, exiting.\n");
		}
		config->zstdWriter[outp].compressionBuffer.size = inputSize;
		config->writeBufSize[outp] = inputSize;


		ZSTD_CCtx_setParameter(config->zstdWriter[outp].cstream, ZSTD_c_nbWorkers, 4);
	}

	if (ZSTD_isError(ZSTD_CCtx_reset(config->zstdWriter[outp].cstream, ZSTD_reset_session_only))) {
		fprintf(stderr, "ERROR: Failed to reset ZSTD context; exiting.\n");
		return -1;
	}
	if (ZSTD_isError(ZSTD_CCtx_setParametersUsingCCtxParams(config->zstdWriter[outp].cstream, config->cparams))) {
		fprintf(stderr, "ERROR: Failed to initialise compression parameters, exiting.\n");
		return -1;
	}

	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_setup_DADA(lofar_udp_io_write_config *config, int outp) {
#ifndef NODADA

	if (config->dadaWriter[outp].multilog == NULL && config->enableMultilog == 1) {
		config->dadaWriter[outp].multilog = multilog_open(config->dadaConfig.programName, config->dadaConfig.syslog);

		if (config->dadaWriter[outp].multilog == NULL) {
			fprintf(stderr, "ERROR: Failed to initialise multilog struct on output %d, exiting.\n", outp);
			return -1;
		}

		multilog_add(config->dadaWriter[outp].multilog, stdout);
	}

	if (config->dadaWriter[outp].ringbuffer == NULL) {
		if (lofar_udp_io_write_setup_DADA_ringbuffer(&(config->dadaWriter[outp].ringbuffer), config->outputDadaKeys[outp],
											   config->dadaConfig.nbufs, config->writeBufSize[outp], config->dadaConfig.num_readers,
											   config->appendExisting) < 0) {
			return -1;
		}

		if (sprintf(config->outputLocations[outp], "PSRDADA:%d", config->outputDadaKeys[outp])) {
			fprintf(stderr, "ERROR: Failed to copy output file path (PSRDADA:%d), exiting.\n",
			        config->outputDadaKeys[outp]);
			return -1;
		}
	}

	if (config->dadaWriter[outp].header == NULL) {
		if (lofar_udp_io_write_setup_DADA_ringbuffer(&(config->dadaWriter[outp].header), config->outputDadaKeys[outp] + 1,
		                                             1, config->dadaConfig.header_size, config->dadaConfig.num_readers,
		                                             config->appendExisting) < 0) {
			return -1;
		}
	}

	return 0;
#endif
	// If PSRDADA was disabled at compile time, error	
	return -1;
}

int lofar_udp_io_write_setup_DADA_ringbuffer(ipcio_t **ringbuffer, int dadaKey, uint64_t nbufs, long bufSize, unsigned int numReaders, int appendExisting) {
	(*ringbuffer) = calloc(1, sizeof(ipcio_t));
	**(ringbuffer) = IPCIO_INIT;

	if (ipcio_create(*ringbuffer, dadaKey, nbufs, bufSize, numReaders) < 0) {
		// ipcio_create(...) prints error to stderr, so we just need to exit.
		if (appendExisting) {
			fprintf(stderr, "WARNING: Failed to create ringbuffer, but appendExisting int is set, attempting to connect to given ringbuffer %d...\n", dadaKey);
			if (ipcio_connect(*ringbuffer, dadaKey) < 0) {
				return -1;
			}
		} else {
			return -1;
		}
	}

	// Open the ringbuffer instance as the primary writer
	if (ipcio_open(*ringbuffer, 'W') < 0) {
		// ipcio_open(...) prints error to stderr, so we just need to exit.
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
long lofar_udp_io_read(lofar_udp_io_read_config *input, int port, char *targetArray, long nchars) {

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
long lofar_udp_io_read_FILE(lofar_udp_io_read_config *input, const int port, char *targetArray, const long nchars) {
	// Decompressed file: Read and return the data as needed
	VERBOSE(printf("reader_nchars: Entering read request (normal): %d, %ld\n", port, nchars));
	return (long) fread(targetArray, sizeof(char), nchars, input->fileRef[port]);
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
long lofar_udp_io_read_ZSTD(lofar_udp_io_read_config *input, int port, long nchars) {
	// Compressed file: Perform streaming decompression on a zstandard compressed file
	VERBOSE(printf("reader_nchars: Entering read request (compressed): %d, %ld\n", port, nchars));

	long dataRead = 0, byteDelta;
	size_t previousDecompressionPos;
	size_t returnVal;

	VERBOSE(printf("reader_nchars %d: start of ZSTD read loop, %ld, %ld, %ld, %ld, %ld\n", port, input->readingTracker[port].pos,
				   input->readingTracker[port].size, input->decompressionTracker[port].pos, dataRead, nchars););

	// Loop across while decompressing the data (zstd decompressed in frame iterations, so it may take a few iterations)
	while (input->readingTracker[port].pos < input->readingTracker[port].size) {
		previousDecompressionPos = input->decompressionTracker[port].pos;
		// zstd streaming decompression + check for errors
		returnVal = ZSTD_decompressStream(input->dstream[port], &(input->decompressionTracker[port]),
										  &(input->readingTracker[port]));
		if (ZSTD_isError(returnVal)) {
			fprintf(stderr, "ZSTD encountered an error decompressing a frame (code %ld, %s), exiting data read early.\n",
					returnVal, ZSTD_getErrorName(returnVal));
			break;
		}

		// Determine how much data we just added to the buffer
		byteDelta = ((long) input->decompressionTracker[port].pos - (long) previousDecompressionPos);

		// Update the total data read + check if we have reached our goal
		dataRead += byteDelta;
		VERBOSE(if (dataRead >= nchars) {
			printf("Reader terminating: %ld read, %ld requested, overflow %ld\n", dataRead, nchars, dataRead - nchars);
		});

		if (dataRead >= nchars) {
			break;
		}

		if (input->decompressionTracker[port].pos == input->decompressionTracker[port].size) {
			fprintf(stderr,
					"Failed to read %ld/%ld chars on port %d before filling the buffer. Attempting to continue...\n",
					dataRead, nchars, port);
			break;
		}
	}

	// Completed or EOF: unmap used memory and return everything we read
	if (madvise(((void *) input->readingTracker[port].src), input->readingTracker[port].pos, MADV_DONTNEED) < 0) {
		fprintf(stderr,
				"ERROR: Failed to apply MADV_DONTNEED after read operation on port %d (errno %d: %s).\n", port,
				errno, strerror(errno));
	}


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
long lofar_udp_io_read_DADA(lofar_udp_io_read_config *input, const int port, char *targetArray, const long nchars) {
#ifndef NODADA

	VERBOSE(printf("reader_nchars: Entering read request (dada): %d, %d, %ld\n", port, input->dadaKeys[port], nchars));

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
 * @param      src     The source (non-const due to ipcio_write being non-const)
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
long lofar_udp_io_write_FILE(lofar_udp_io_write_config *config, int outp, const char *src, const long nchars) {
	return (long) fwrite(src, sizeof(char), nchars, config->outputFiles[outp]);
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
lofar_udp_io_write_ZSTD(lofar_udp_io_write_config *config, int outp, const char *src, const long nchars) {

	ZSTD_inBuffer input = { src, nchars, 0 };
	ZSTD_outBuffer output = { config->zstdWriter[outp].compressionBuffer.dst, config->zstdWriter[outp].compressionBuffer.size, 0 };
	while (!(input.size == input.pos)) {
		size_t returnVal = ZSTD_compressStream2(config->zstdWriter[outp].cstream, &output, &input, ZSTD_e_end);
		if (ZSTD_isError(returnVal)) {
			fprintf(stderr, "ERROR: Failed to compressed data with ZSTD (%ld, %s)\n", returnVal,
					ZSTD_getErrorName(returnVal));
			return -1;
		}
	}

	config->zstdWriter[outp].compressionBuffer.pos = output.pos;

	if (lofar_udp_io_write_FILE(config, outp, config->zstdWriter[outp].compressionBuffer.dst, (long) output.pos) < 0) {
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

	long writtenBytes = ipcio_write(config->dadaWriter[outp].ringbuffer, src, nchars);
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

	size_t readlen = fread(outbuf, size, num, inputFilePtr);

	if (resetSeek) {
		if (fseek(inputFilePtr, (long) size * -num, SEEK_CUR) != 0) {
			fprintf(stderr, "Failed to reset seek head, exiting.\n");
			fclose(inputFilePtr);
			return -2;
		}
	}

	fclose(inputFilePtr);

	if (readlen != (size * num)) {
		fprintf(stderr, "Unable to read %ld elements from file, exiting.\n", size * num);
		return -1;
	}

	return readlen;
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
	FILE *inputFilePtr = fopen(inputFile, "rb");
	if (inputFilePtr == NULL) {
		fprintf(stderr, "ERROR: Unable to open normal file at %s, exiting.\n", inputFile);
		return -1;
	}

	// Build the decompression stream
	ZSTD_DStream *dstreamTmp = ZSTD_createDStream();
	int readFactor = ((size * num) / ZSTD_DStreamInSize()) + 1;
	size_t minRead = ZSTD_DStreamInSize() * readFactor;
	size_t minOut = ZSTD_DStreamOutSize() * readFactor;
	ZSTD_initDStream(dstreamTmp);

	// Allocate the buffer memory, build the buffer structs
	char *inBuff = calloc(minRead, sizeof(char));
	char *localOutBuff = calloc(minOut, sizeof(char));

	ZSTD_inBuffer tmpRead = { inBuff, minRead * sizeof(char), 0 };
	ZSTD_outBuffer tmpDecom = { localOutBuff, minOut * sizeof(char), 0 };

	// Read in the compressed data
	long readlen = (long) fread(inBuff, sizeof(char), minRead, inputFilePtr);
	if (readlen != (long) minRead) {
		fprintf(stderr, "Unable to read in data from file; exiting.\n");
		fclose(inputFilePtr);
		ZSTD_freeDStream(dstreamTmp);
		FREE_NOT_NULL(inBuff);
		FREE_NOT_NULL(localOutBuff);
		return -1;
	}

	// Move the read head back to the start of the packer
	if (resetSeek) {
		if (fseek(inputFilePtr, -(long) minRead, SEEK_CUR) != 0) {
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
		fprintf(stderr, "ZSTD encountered an error while doing temp read (code %ld, %s), exiting.\n", output,
				ZSTD_getErrorName(output));
		ZSTD_freeDStream(dstreamTmp);
		FREE_NOT_NULL(inBuff);
		FREE_NOT_NULL(localOutBuff);
		return -1;
	}

	// Cap the return value of the data
	readlen = (long) tmpDecom.pos;
	if (readlen > (long) size * num) { readlen = size * num; }

	// Copy the output and cleanup
	memcpy(outbuf, localOutBuff, size * num);
	ZSTD_freeDStream(dstreamTmp);
	FREE_NOT_NULL(inBuff);
	FREE_NOT_NULL(localOutBuff);

	return readlen;

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

	return (int) returnlen;
#else
	fprintf(stderr, "ERROR: PSRDADA not enabled at compile time, exiting.\n");
			return -1;
#endif
}


/**
 * @brief      { function_description }
 *
 * @param      input  The input
 * @param[in]  port   The port
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_read_cleanup_FILE(lofar_udp_io_read_config *input, const int port) {
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
int lofar_udp_io_read_cleanup_ZSTD(lofar_udp_io_read_config *input, const int port) {
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
int lofar_udp_io_read_cleanup_DADA(lofar_udp_io_read_config *input, const int port) {
#ifndef NODADA
	if (input->dadaReader[port] != NULL) {
		if (dada_hdu_unlock_read(input->dadaReader[port]) < 0) {
			fprintf(stderr, "ERROR: Failed to close PSRDADA buffer %d on port %d.\n", input->dadaKeys[port], port);
		}

		if (dada_hdu_disconnect(input->dadaReader[port]) < 0) {
			fprintf(stderr, "ERROR: Failed to disconnect from PSRDADA buffer %d on port %d.\n", input->dadaKeys[port],
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
int lofar_udp_io_write_cleanup_FILE(lofar_udp_io_write_config *config, int outp) {
	if (config->outputFiles[outp] != NULL) {
		fclose(config->outputFiles[outp]);
		config->outputFiles[outp] = NULL;
	}

	if (config->readerType == FIFO) {
		if (remove(config->outputLocations[outp]) != 0) {
			fprintf(stderr, "WARNING: Failed to remove old FIFO at %s, continuing.\n",
					config->outputLocations[outp]);
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
	lofar_udp_io_write_cleanup_FILE(config, outp);

	if (fullClean && config->zstdWriter[outp].cstream != NULL) {
		ZSTD_freeCStream(config->zstdWriter[outp].cstream);
		config->zstdWriter[outp].cstream = NULL;
		
		if (config->zstdWriter[outp].compressionBuffer.dst != NULL) {
			FREE_NOT_NULL(config->zstdWriter[outp].compressionBuffer.dst);
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
int lofar_udp_io_write_cleanup_DADA(lofar_udp_io_write_config *config, int outp, int fullClean) {
	// Close the ringbuffer writers
	if (fullClean) {
		if (config->dadaWriter[outp].ringbuffer != NULL) {
			ipcio_stop(config->dadaWriter[outp].ringbuffer);
			ipcio_close(config->dadaWriter[outp].ringbuffer);
		}

		if (config->dadaWriter[outp].header != NULL) {
			ipcio_stop(config->dadaWriter[outp].header);
			ipcio_close(config->dadaWriter[outp].header);
		}

		// Wait for readers to finish up and exit, or timeout (ipcio_read can hang if it requests data past the EOD point)
		if (config->dadaWriter[outp].ringbuffer != NULL) {
			lofar_udp_io_cleanup_DADA_loop((ipcbuf_t*) config->dadaWriter[outp].ringbuffer, config->dadaConfig.cleanup_timeout);
			ipcio_destroy(config->dadaWriter[outp].ringbuffer);
		}

		if (config->dadaWriter[outp].ringbuffer != NULL) {
			lofar_udp_io_cleanup_DADA_loop((ipcbuf_t*) config->dadaWriter[outp].ringbuffer, config->dadaConfig.cleanup_timeout);
			ipcio_destroy(config->dadaWriter[outp].header);
		}

		// Destroy the ringbuffer instances
		FREE_NOT_NULL(config->dadaWriter[outp].ringbuffer);
		FREE_NOT_NULL(config->dadaWriter[outp].header);
	}

	return 0;
}

void lofar_udp_io_cleanup_DADA_loop(ipcbuf_t *buff, float timeout) {
	float totalSleep = 0.0f;

	while (totalSleep < timeout) {
		if (ipcbuf_get_reader_conn(buff) != 0) {
			break;
		}

		if (((int) totalSleep * 1000) % 5000 == 0) {
			printf("Waiting on DADA writer readers to exit (%f / %fs before timing out).\n", totalSleep, 30.0f);
		}

		usleep(1000 * 5);
		totalSleep += 0.005;
	}

}

// Metadata functions


void swapCharPtr(char **a, char **b) {
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