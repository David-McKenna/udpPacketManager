
// Read Interface


/**
 * @brief      Setup the read I/O struct to handle zstandard data
 *
 * @param      input   The input
 * @param[in]  inputLocation    The input file location
 * @param[in]  port    The index offset from the base file
 *
 * @return     0: Success, <0: Failure
 */
int32_t
_lofar_udp_io_read_setup_ZSTD(lofar_udp_io_read_config *const input, const char *inputLocation, const int8_t port) {
	// Copy the file reference into input
	// Leave in boilerplate return encase the function gets more complicated in future
	int32_t returnVal;
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

	// mmap the input file for better buffered reading
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

	if (madvise(tmpPtr, fileSize, MADV_SEQUENTIAL | MADV_WILLNEED) == -1) {
		fprintf(stderr, "ERROR: Failed to advise the kernel on mmap read strategy on port %d. Errno: %d. Exiting.\n",
				port, errno);
		return -3;

	}

	// Setup the decompressed data buffer/struct
	// If the size is pre-set, we have already resize the true buffer as required
	int64_t bufferSize = input->decompressionTracker[port].size != 0 ?
						 (int64_t) input->decompressionTracker[port].size : input->readBufSize[port];
	if (bufferSize % ZSTD_DStreamOutSize() && !(input->readerType == ZSTDCOMPRESSED_INDIRECT)) {
		fprintf(stderr, "ERROR %s: Zstandard buffer was not initialised correctly, exiting.\n", __func__);
		return -1;
	} else {
		bufferSize += _lofar_udp_io_read_ZSTD_fix_buffer_size(bufferSize, 1);
		VERBOSE(printf("reader_setup: expanding decompression buffer by %ld bytes\n",
					   bufferSize - input->readBufSize[port]));
	}

	input->decompressionTracker[port].pos = 0; // Initialisation for our step-by-step reader

	// If we are using the indirect reader, allocate our own intermediate buffer
	if (input->readerType == ZSTDCOMPRESSED_INDIRECT) {
		FREE_NOT_NULL(input->decompressionTracker[port].dst);
		//fprintf(stderr, "WARNING: ZSTDCOMPRESSED_INDIRECT is enabled but a buffer wasn't provided, allocating %ld bytes for you.\n", bufferSize);
		input->decompressionTracker[port].dst = calloc(bufferSize, sizeof(int8_t));
		input->decompressionTracker[port].size = bufferSize;
	} else if (input->readerType == ZSTDCOMPRESSED && input->decompressionTracker[port].size == 0) {
		input->decompressionTracker[port].size = bufferSize;
	}

	return 0;
}

/**
 * @brief      Perform a data read for a normal file
 *
 * @param      input        The input
 * @param[in]  port         The index offset from the base file
 * @param      targetArray  The output array
 * @param[in]  nchars       The number of bytes to read
 *
 * @return     <=0: Failure, >0 Characters read
 */
int64_t _lofar_udp_io_read_ZSTD(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars) {
	VERBOSE(printf("reader_nchars: Entering read request (compressed): %d, %ld\n", port, nchars));

	if (input->decompressionTracker[port].dst == NULL) {
		fprintf(stderr, "ERROR: ZSTD decompression array was not initialised, exiting.\n");
		return -1;
	}

	int64_t dataRead, byteDelta;
	size_t previousDecompressionPos;
	size_t returnVal;

	// Move the trailing end of the buffer to the start of the target read
	const int8_t *dest = input->decompressionTracker[port].dst;
	dataRead = input->decompressionTracker[port].pos - input->zstdLastRead[port];

	// Copy the preread position to optimise madvise calls later
	const size_t previousReadPos = input->readingTracker[port].pos;

	VERBOSE(printf("reader_nchars %d: start of ZSTD read loop, %ld, %ld, %ld, %ld, %ld\n",
				   port,
				   input->readingTracker[port].pos,
				   input->readingTracker[port].size,
				   input->decompressionTracker[port].pos,
				   dataRead,
				   nchars););

	if (input->readerType == ZSTDCOMPRESSED) {
		dest = targetArray;
		const int64_t ptrByteDifference = targetArray - (int8_t *) input->decompressionTracker[port].dst;
		if (ptrByteDifference < 0 || ptrByteDifference > (int64_t) input->decompressionTracker[port].size) {
			fprintf(stderr, "ERROR %s: Passed buffer is not within pre-set buffer range (delta %ld), exiting.\n", __func__, ptrByteDifference);
			return -1;
		}
	}

	// memmove as we can't use memcpy for the ZSTANDARD moe due to potential overlapping buffer components
	if (memmove((int8_t *) dest, &(((int8_t *) input->decompressionTracker[port].dst)[input->zstdLastRead[port]]), dataRead) != dest) {
		fprintf(stderr, "ERROR: Failed to copy end of ZSTD buffer, exiting.\n");
		return -1;
	}


	if ((int64_t) input->decompressionTracker[port].pos > input->zstdLastRead[port]) {
		// Update the position marker
		input->decompressionTracker[port].pos = dest - (int8_t *) input->decompressionTracker[port].dst + dataRead;

		VERBOSE(printf("ZSTD READ %d, base offset, read: %ld, %ld\n", port, input->decompressionTracker[port].pos, dataRead));
	} else {
		// Update the position marker
		input->decompressionTracker[port].pos = dest - ((int8_t *) input->decompressionTracker[port].dst);
		VERBOSE(printf("ZSTD READ%d, base offset: %ld\n", port, input->decompressionTracker[port].pos));
	}

	VERBOSE(printf("ZSTD Read%hhd: starting loop with %ld/%ld\n", port, dataRead, nchars));

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

	VERBOSE(
		printf("Reader terminating %hhd: %ld read, %ld requested, overflow %ld\n", port, dataRead, nchars, dataRead - nchars);
	);

	if (nchars > dataRead) {
		nchars = dataRead;
	}

	// Completed or EOF: unmap used memory and return everything we read
	// TODO: MADV_DONTNEED causes significant slow down for multi-hour observations, test performance of MADV_FREE vs MADV_DONTNEED
	// Previously commented out at it wasn't thought the be needed, unfortunately it is.
	// Large observations were holding > 50% of a systems memory, as compared to < 3GB in 0.6 builds.
	//if (madvise(((void *) input->readingTracker[port].src), input->readingTracker[port].pos, MADV_DONTNEED) < 0) {
	if (previousReadPos < input->readingTracker[port].pos) {
		const size_t pageSize = getpagesize();
		const size_t previousReadMadviseOffset = previousReadPos - (previousReadPos % pageSize);
		void *startAddress = ((void *) input->readingTracker[port].src) + previousReadMadviseOffset;
		size_t memoryPages = (input->readingTracker[port].pos - previousReadMadviseOffset) / pageSize;

		// Don't try to apply madvise to too much data
		if ((previousReadMadviseOffset + memoryPages * pageSize) > input->readingTracker[port].size) {
			memoryPages -= 1;
		}

		if (memoryPages > 0) {
			// madvise calls must be page aligned, determine the offsets to apply to all of the current page at the start, but not the end of the page at the end
			if (madvise((void *) startAddress, memoryPages * pageSize, input->zstdMadvise) < 0) {
				fprintf(stderr,
				        "WARNING: Failed to apply %d after read operation on port %d (errno %d: %s).\n", input->zstdMadvise, port,
				        errno, strerror(errno));
				if (input->zstdMadvise == MADV_FREE) {
					fprintf(stderr,
					        "WARNING: This was previously using MADV_FREE, your kernel may not be recent enough to support it (> 4.5.0), swapping to MADV_DONTNEED.\n");
					#pragma omp atomic write
					input->zstdMadvise = MADV_DONTNEED;
				}

				if (madvise(((void *) input->readingTracker[port].src) + previousReadPos, input->readingTracker[port].pos - previousReadPos,
				            input->zstdMadvise) <
				    0) {
					fprintf(stderr, "ERROR: Fallback madvise %d failed to apply, exiting.\n", input->zstdMadvise);
					return -1;
				}
			}
		}
	}
	input->zstdLastRead[port] = (dest - (int8_t*) input->decompressionTracker[port].dst) + nchars;

	// Copy data for the indirect reader
	if (input->readerType == ZSTDCOMPRESSED_INDIRECT) {
		if (memcpy(targetArray, input->decompressionTracker[port].dst, nchars) != targetArray) {
			fprintf(stderr, "ERROR: Failed to copy ZSTD decompress output to array, exiting.\n");
			return -1;
		}
	}

	// Cap the read data; the structs will remember the true read count
	if (dataRead > nchars) {
		dataRead = nchars;
	}

	return dataRead;
}

/**
 * @brief      Cleanup zstandard compressed file references for the read I/O struct
 *
 * @param      input  The input
 * @param[in]  port   The index offset from the base file
 */
void _lofar_udp_io_read_cleanup_ZSTD(lofar_udp_io_read_config *const input, const int8_t port) {
	if (input == NULL) {
		return;
	}

	// Free the decompression stream
	if (input->dstream[port] != NULL) {
		VERBOSE(printf("Freeing decompression buffers and ZSTD stream on port %d\n", port););
		ZSTD_freeDStream(input->dstream[port]);
		input->dstream[port] = NULL;

		munmap((void*) input->readingTracker[port].src, input->readingTracker[port].size);
	}

	// Cleanup the input file references
	_lofar_udp_io_read_cleanup_FILE(input, port);

	// Free the intermediate buffer
	if (input->readerType == ZSTDCOMPRESSED_INDIRECT) {
		FREE_NOT_NULL(input->decompressionTracker[port].dst);
	}

	// Reset the buffer size
	input->decompressionTracker[port].size = 0;
}

/**
 * @brief      Temporarily read in num bytes from a zstandard compressed file file
 *
 * @param      outbuf     The output buffer pointer
 * @param[in]  size       The size of words to read
 * @param[in]  num        The number of words to read
 * @param[in]  inputFile  The input file
 * @param[in]  resetSeek  Do (1) / Don't (0) reset back to the original location
 *                        in FILE* after performing a read operation
 *
 * @return     >0: bytes read, <=-1: Failure
 */
int64_t _lofar_udp_io_read_temp_ZSTD(void *outbuf, const int64_t size, const int64_t num, const char inputFile[],
									 const int8_t resetSeek) {
	VERBOSE(printf("%s: %s, %ld\n", __func__, inputFile, strlen(inputFile)));
	// Open the underlying file
	FILE *inputFilePtr = fopen(inputFile, "rb");
	if (inputFilePtr == NULL) {
		fprintf(stderr, "ERROR: Unable to open normal file at %s, exiting.\n", inputFile);
		return -1;
	}

	// Build the decompression stream
	ZSTD_DStream *dstreamTmp = ZSTD_createDStream();
	int64_t readFactor = (int64_t) ((size * num) / ZSTD_DStreamInSize()) + 1;
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
	if (readlen > (int64_t) minRead || readlen < 1) {
		fprintf(stderr, "Unable to read in data from file; exiting.\n");
		fclose(inputFilePtr);
		ZSTD_freeDStream(dstreamTmp);
		FREE_NOT_NULL(inBuff);
		FREE_NOT_NULL(localOutBuff);
		return -1;
	}

	// Move the read head back to the start of the packer
	if (resetSeek) {
		if (fseek(inputFilePtr, -(int64_t) readlen * sizeof(int8_t), SEEK_CUR) != 0) {
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
	if (memcpy(outbuf, localOutBuff, size * num) != outbuf) {
		fprintf(stderr, "ERROR: Failed to copy ZSTD temp read to output, exiting.\n");
		return -1;
	}
	ZSTD_freeDStream(dstreamTmp);
	FREE_NOT_NULL(inBuff);
	FREE_NOT_NULL(localOutBuff);

	return readlen;

}

//Write Interface

/**
 * @brief      Setup the write I/O struct to handle normal data
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 * @param[in]  iter    The iteration of the output file
 *
 * @return     0: Success, <0: Failure
 */
int32_t _lofar_udp_io_write_setup_ZSTD(lofar_udp_io_write_config *const config, const int8_t outp, const int32_t iter) {
	// Setup the normal file output
	if (_lofar_udp_io_write_setup_FILE(config, outp, iter) < 0) {
		return -1;
	}

	// Check if we have a compression configuration
	if (config->cparams == NULL) {
		config->cparams = ZSTD_createCCtxParams();
		if (config->cparams == NULL || ZSTD_isError(ZSTD_CCtxParams_init(config->cparams, config->zstdConfig.compressionLevel))) {
			fprintf(stderr, "ERROR: Failed to initialised compression parameters, exiting.\n");
			return -1;
		}
	}

	// Setup the compression stream
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
			lofar_udp_io_write_cleanup(config, 0);
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


		ZSTD_CCtx_setParameter(config->zstdWriter[outp].cstream, ZSTD_c_nbWorkers, config->zstdConfig.numThreads);
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
 * @brief      Perform a data write for a zstdcompressed file
 *
 * @param[in]  config  The configuration
 * @param[in]  outp    The outp
 * @param[in]  src     The source
 * @param[in]  nchars  The nchars
 *
 * @return  >=0: Number of bytes written, <0: Failure
 */
int64_t
_lofar_udp_io_write_ZSTD(lofar_udp_io_write_config *const config, const int8_t outp, const int8_t *src, const int64_t nchars) {

	ZSTD_inBuffer input = { src, nchars, 0 };
	ZSTD_outBuffer output = { config->zstdWriter[outp].compressionBuffer.dst, config->zstdWriter[outp].compressionBuffer.size, 0 };
	// Write the block of data
	size_t returnVal;
	while (!(input.size == input.pos)) {
		while((returnVal = ZSTD_compressStream2(config->zstdWriter[outp].cstream, &output, &input, ZSTD_e_continue))) {
			if (ZSTD_isError(returnVal)) {
				fprintf(stderr, "ERROR: Failed to compressed data with ZSTD (%ld, %s)\n", returnVal,
						ZSTD_getErrorName(returnVal));
				return -1;
			}
		}
	}

	// Flush the frame
	while((returnVal = ZSTD_compressStream2(config->zstdWriter[outp].cstream, &output, &input, ZSTD_e_flush))) {
		if (ZSTD_isError(returnVal)) {
			fprintf(stderr, "ERROR: Failed to finish frame for ZSTD (%ld, %s)\n", returnVal,
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
 * @brief      Cleanup zstandard file references for the write I/O struct
 *
 * @param      config     The configuration
 * @param[in]  outp       The outp
 * @param[in]  fullClean  Perform a full clean, dealloc the ringbuffer
 */
void _lofar_udp_io_write_cleanup_ZSTD(lofar_udp_io_write_config *const config, const int8_t outp, const int8_t fullClean) {
	if (config == NULL) {
		return;
	}

	// Cleanup output file references
	_lofar_udp_io_write_cleanup_FILE(config, outp);

	// Cleanup the compression stream
	if (fullClean && config->zstdWriter[outp].cstream != NULL) {
		ZSTD_freeCStream(config->zstdWriter[outp].cstream);
		config->zstdWriter[outp].cstream = NULL;

		if (config->zstdWriter[outp].compressionBuffer.dst != NULL) {
			FREE_NOT_NULL(config->zstdWriter[outp].compressionBuffer.dst);
		}
	}

	// Cleanup the compression configuration
	if (fullClean && config->cparams != NULL) {
		ZSTD_freeCCtxParams(config->cparams);
		config->cparams = NULL;
	}
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
