
// Read Interface


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
_lofar_udp_io_read_setup_ZSTD(lofar_udp_io_read_config *const input, const char *inputLocation, int8_t port) {
	// Copy the file reference into input
	// Leave in boilerplate return encase the function gets more complicated in future
	int returnVal;
	if ((returnVal = _lofar_udp_io_read_setup_FILE(input, inputLocation, port)) != 0) {
		return returnVal;
	}

	// Find the file size (needed for mmap)
	int64_t fileSize = _FILE_file_size(input->fileRef[port]);
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
	int64_t bufferSize = input->readBufSize[port];
	if (bufferSize % ZSTD_DStreamOutSize()) {
		bufferSize += _lofar_udp_io_read_ZSTD_fix_buffer_size(bufferSize, 1);
	}
	VERBOSE(printf("reader_setup: expending decompression buffer by %ld bytes\n",
	               bufferSize - input->readBufSize[port]));
	//input->readBufSize[port] = bufferSize;
	input->decompressionTracker[port].size = bufferSize;
	input->decompressionTracker[port].pos = 0; // Initialisation for our step-by-step reader

	if (input->readerType == ZSTDCOMPRESSED_INDIRECT) {
		FREE_NOT_NULL(input->decompressionTracker[port].dst);
		//fprintf(stderr, "WARNING: ZSTDCOMPRESSED_INDIRECT is enabled but a buffer wasn't provided, allocating %ld bytes for you.\n", bufferSize);
		input->decompressionTracker[port].dst = calloc(bufferSize, sizeof(int8_t));
	}

	return 0;
}

int64_t _lofar_udp_io_read_ZSTD_fix_buffer_size(int64_t bufferSize, int8_t deltaOnly) {
	int64_t zstdAlignedSize = ZSTD_DStreamOutSize();
	int64_t newBufferSize = (zstdAlignedSize - (bufferSize % zstdAlignedSize)) % zstdAlignedSize + zstdAlignedSize;
	// Extreme edge case: add an extra frame of data encase we need a small partial read at the end of a frame.
	// Only possible for very small packetsPerIteration, but it's still possible.
	if (!deltaOnly) {
		return newBufferSize;
	}

	return newBufferSize - bufferSize;
}


/**
 * @brief      { function_description }
 *
 * @param      input  The input
 * @param[in]  port   The port
 *
 * @return     { description_of_the_return_value }
 */
void _lofar_udp_io_read_cleanup_ZSTD(lofar_udp_io_read_config *const input, int8_t port) {
	if (input == NULL) {
		return;
	}

	_lofar_udp_io_read_cleanup_FILE(input, port);

	// Free the decompression stream
	if (input->dstream[port] != NULL) {
		VERBOSE(printf("Freeing decompression buffers and ZSTD stream on port %d\n", port););
		ZSTD_freeDStream(input->dstream[port]);
		void *tmpPtr = (void *) input->readingTracker[port].src;
		munmap(tmpPtr, input->readingTracker[port].size);
		input->dstream[port] = NULL;
	}

	if (input->readerType == ZSTDCOMPRESSED_INDIRECT) {
		FREE_NOT_NULL(input->decompressionTracker[port].dst);
	}
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
int64_t _lofar_udp_io_read_temp_ZSTD(void *outbuf, int64_t size, int64_t num, const char inputFile[],
                                     int8_t resetSeek) {
	VERBOSE(printf("%s: %s, %ld\n", __func__, inputFile, strlen(inputFile)));
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
	int8_t *inBuff = calloc(minRead, sizeof(int8_t));
	int8_t *localOutBuff = calloc(minOut, sizeof(int8_t));

	ZSTD_inBuffer tmpRead = { inBuff, minRead * sizeof(int8_t), 0 };
	ZSTD_outBuffer tmpDecom = { localOutBuff, minOut * sizeof(int8_t), 0 };

	// Read in the compressed data
	int64_t readlen = (int64_t) fread(inBuff, sizeof(int8_t), minRead, inputFilePtr);
	if (readlen != (int64_t) minRead) {
		fprintf(stderr, "Unable to read in data from file; exiting.\n");
		fclose(inputFilePtr);
		ZSTD_freeDStream(dstreamTmp);
		FREE_NOT_NULL(inBuff);
		FREE_NOT_NULL(localOutBuff);
		return -1;
	}

	// Move the read head back to the start of the packer
	if (resetSeek) {
		if (fseek(inputFilePtr, -(int64_t) minRead, SEEK_CUR) != 0) {
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
	readlen = (int64_t) tmpDecom.pos;
	if (readlen > (int64_t) size * num) { readlen = size * num; }

	// Copy the output and cleanup
	memcpy(outbuf, localOutBuff, size * num);
	ZSTD_freeDStream(dstreamTmp);
	FREE_NOT_NULL(inBuff);
	FREE_NOT_NULL(localOutBuff);

	return readlen;

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
int64_t _lofar_udp_io_read_ZSTD(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars) {
	// Compressed file: Perform streaming decompression on a zstandard compressed file
	VERBOSE(printf("reader_nchars: Entering read request (compressed): %d, %ld\n", port, nchars));

	if (input->decompressionTracker[port].dst == NULL) {
		fprintf(stderr, "ERROR: ZSTD decompression array was not initialised, exiting.\n");
		return -1;
	}

	int64_t dataRead = 0, byteDelta;
	size_t previousDecompressionPos;
	size_t returnVal;

	VERBOSE(printf("reader_nchars %d: start of ZSTD read loop, %ld, %ld, %ld, %ld, %ld\n", port, input->readingTracker[port].pos,
	               input->readingTracker[port].size, input->decompressionTracker[port].pos, dataRead, nchars););

	// Move the trailing end of the buffer to the start of the target read
	if (input->readerType == ZSTDCOMPRESSED) {
		if ((int64_t) input->decompressionTracker[port].pos > input->readBufSize[port]) {
			dataRead = input->decompressionTracker[port].pos - input->readBufSize[port];
			if (memmove(targetArray, &(((char *) input->decompressionTracker[port].dst)[input->readBufSize[port]]), dataRead) != targetArray) {
				fprintf(stderr, "ERROR: Failed to copy end of ZSTD buffer, exiting.\n");
				return -1;
			}
			// Update the position marker
			input->decompressionTracker[port].pos = targetArray - (int8_t *) input->decompressionTracker[port].dst + dataRead;

			VERBOSE(printf("ZSTD READ %d, base offset, read: %ld, %ld\n", port, input->decompressionTracker[port].pos, dataRead));
		} else {
			// Update the position marker
			input->decompressionTracker[port].pos = targetArray - ((int8_t *) input->decompressionTracker[port].dst);
			VERBOSE(printf("ZSTD READ%d, base offset: %ld\n", port, input->decompressionTracker[port].pos));
		}
	} else {
		if ((int64_t) input->decompressionTracker[port].pos > input->zstdLastRead[port]) {
			dataRead = input->decompressionTracker[port].pos - input->zstdLastRead[port];
			if (memmove(input->decompressionTracker[port].dst, &(((char *) input->decompressionTracker[port].dst)[input->zstdLastRead[port]]), dataRead) != targetArray) {
				fprintf(stderr, "ERROR: Failed to copy end of ZSTD buffer, exiting.\n");
				return -1;
			}
			// Update the position marker
			input->decompressionTracker[port].pos = dataRead;

			VERBOSE(printf("ZSTD READ %d, base offset, read: %ld, %ld\n", port, input->decompressionTracker[port].pos, dataRead));
		} else {
			// Update the position marker
			input->decompressionTracker[port].pos = 0;
			VERBOSE(printf("ZSTD READ%d, base offset: %ld\n", port, input->decompressionTracker[port].pos));
		}
	}

	VERBOSE(printf("ZSTD Read: starting loop with %ld/%ld\n", dataRead, nchars));

	// Loop across while decompressing the data (zstd decompressed in frame iterations, so it may take a few iterations)
	while (input->readingTracker[port].pos < input->readingTracker[port].size && dataRead < nchars) {
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
		byteDelta = ((int64_t) input->decompressionTracker[port].pos - (int64_t) previousDecompressionPos);

		// Update the total data read + check if we have reached our goal
		dataRead += byteDelta;

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

	VERBOSE(if (dataRead >= nchars) {
		printf("Reader terminating: %ld read, %ld requested, overflow %ld\n", dataRead, nchars, dataRead - nchars);
	});

	// Completed or EOF: unmap used memory and return everything we read
	if (madvise(((void *) input->readingTracker[port].src), input->readingTracker[port].pos, MADV_DONTNEED) < 0) {
		fprintf(stderr,
		        "ERROR: Failed to apply MADV_DONTNEED after read operation on port %d (errno %d: %s).\n", port,
		        errno, strerror(errno));
	}

	if (input->readerType == ZSTDCOMPRESSED_INDIRECT) {
		if (memcpy(targetArray, input->decompressionTracker[port].dst, nchars) != targetArray) {
			fprintf(stderr, "ERROR: Failed to copy ZSTD decompress output to array, exiting.\n");
			return -1;
		}
		input->zstdLastRead[port] = nchars;
	}

	if (dataRead > nchars) {
		dataRead = nchars;
	}

	return dataRead;
}





//Write Interface


/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 * @param[in]  iter    The iterator
 *
 * @return     { description_of_the_return_value }
 */
int _lofar_udp_io_write_setup_ZSTD(lofar_udp_io_write_config *const config, int8_t outp, int32_t iter) {
	if (_lofar_udp_io_write_setup_FILE(config, outp, iter) < 0) {
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

		int64_t inputSize = config->writeBufSize[outp];
		if (inputSize % ZSTD_CStreamInSize() != 0) {
			inputSize += (int64_t) ZSTD_CStreamInSize() - (inputSize % (int64_t) ZSTD_CStreamInSize());
		}


		if (inputSize % ZSTD_CStreamOutSize() != 0) {
			inputSize += (int64_t) ZSTD_CStreamInSize() - (inputSize % (int64_t) ZSTD_CStreamOutSize());
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
 * @param      src     The source
 * @param[in]  nchars  The nchars
 *
 * @return     { description_of_the_return_value }
 */
int64_t
_lofar_udp_io_write_ZSTD(lofar_udp_io_write_config *const config, int8_t outp, const int8_t *src, int64_t nchars) {

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

	if (_lofar_udp_io_write_FILE(config, outp, config->zstdWriter[outp].compressionBuffer.dst, (int64_t) output.pos) < 0) {
		return -1;
	}

	return nchars;
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
void _lofar_udp_io_write_cleanup_ZSTD(lofar_udp_io_write_config *const config, int8_t outp, int8_t fullClean) {
	if (config == NULL) {
		return;
	}
	_lofar_udp_io_write_cleanup_FILE(config, outp);

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

}