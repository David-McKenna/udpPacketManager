
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
int lofar_udp_io_read_temp_ZSTD(void *outbuf, const size_t size, const int num, const char inputFile[],
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