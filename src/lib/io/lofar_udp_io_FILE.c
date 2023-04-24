
// Read interface

/**
 * @brief      { function_description }
 *
 * @param      input   The input
 * @param[in]  config  The configuration
 * @param[in]  port    The port
 *
 * @return     { description_of_the_return_value }
 */
int32_t _lofar_udp_io_read_setup_FILE(lofar_udp_io_read_config *const input, const char *inputLocation, int8_t port) {
	VERBOSE(printf("Opening file at %s for port %d\n", inputLocation, port));

	input->fileRef[port] = fopen(inputLocation, "rb");

	if (input->fileRef[port] == NULL) {
		fprintf(stderr, "ERROR: Failed to open file at %s: errno %d, %s.\n", inputLocation, errno, strerror(errno));
		return -1;
	}

	return 0;
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
int64_t _lofar_udp_io_read_FILE(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars) {
	// Decompressed file: Read and return the data as needed
	VERBOSE(printf("reader_nchars: Entering read request (normal): %d, %ld\n", port, nchars));
	return (int64_t) fread(targetArray, sizeof(int8_t), nchars, input->fileRef[port]);
}

/**
 * @brief      { function_description }
 *
 * @param      input  The input
 * @param[in]  port   The port
 *
 * @return     { description_of_the_return_value }
 */
void _lofar_udp_io_read_cleanup_FILE(lofar_udp_io_read_config *const input, int8_t port) {
	if (input == NULL) {
		return;
	}

	if (input->fileRef[port] != NULL) {
		VERBOSE(printf("On port: %d closing file\n", port))
		fclose(input->fileRef[port]);
		input->fileRef[port] = NULL;
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
int64_t _lofar_udp_io_read_temp_FILE(void *outbuf, int64_t size, int64_t num, const char inputFile[],
                                     int8_t resetSeek) {
	FILE *inputFilePtr = fopen(inputFile, "rb");
	if (inputFilePtr == NULL) {
		fprintf(stderr, "ERROR: Unable to open normal file at %s, exiting.\n", inputFile);
		return -1;
	}

	int64_t readlen = fread(outbuf, size, num, inputFilePtr);

	if (readlen != num) {
		fprintf(stderr, "Unable to read %ld elements from file, exiting.\n", size * num);
		return -1;
	}

	if (resetSeek) {
		// This will raise an error if we are dealing with a FIFO/pipe  
		if (fseek(inputFilePtr, (long) size * -num, SEEK_CUR) != 0) {
			fprintf(stderr, "Failed to reset seek head, exiting.\n");
			fclose(inputFilePtr);
			return -2;
		}
	}

	fclose(inputFilePtr);

	return readlen;
}



// Write Interface

/**
 * @brief      Determines if lofar udp i/o write setup check exists.
 *
 * @param      filePath    The file path
 * @param[in]  appendMode  The append mode
 *
 * @return     True if lofar udp i/o write setup check exists, False otherwise.
 */
int lofar_udp_io_write_setup_check_exists(char filePath[], int appendMode) {
	if (appendMode == 1 && access(filePath, F_OK) == 0 && access(filePath, W_OK) != 0) {
		fprintf(stderr, "ERROR: Unable to write to file at %s, exiting.\n", filePath);
		return -1;
	} else if (appendMode != 1 && access(filePath, F_OK) == 0) {
		fprintf(stderr, "ERROR: Output file at %s already exists; exiting.\n", filePath);
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
int32_t _lofar_udp_io_write_setup_FILE(lofar_udp_io_write_config *const config, int8_t outp, int32_t iter) {
	char outputLocation[DEF_STR_LEN];

	if (config->outputFiles[outp] != NULL) {
		lofar_udp_io_write_cleanup(config, outp, 1);
	}

	if (lofar_udp_io_parse_format(outputLocation, config->outputFormat, -1, iter, outp, config->firstPacket) < 0) {
		return -1;
	}

	if (lofar_udp_io_write_setup_check_exists(outputLocation, config->progressWithExisting)) {
		return -1;
	}

	if (strncpy(config->outputLocations[outp], outputLocation, DEF_STR_LEN) != config->outputLocations[outp]) {
		fprintf(stderr, "ERROR: Failed to copy output file path (%s), exiting.\n", outputLocation);
		return -1;
	}

	if (config->readerType == FIFO) {
		struct stat fileStat;

		if (mkfifo(outputLocation, 0664) < 0) {
			printf("ERROR: Failed to create FIFO (%d, %s)\n", errno, strerror(errno));
			if (!config->progressWithExisting) {
				fprintf(stderr, "ERROR: Failed to create FIFO at %s (errno %d: %s), exiting.\n", outputLocation, errno, strerror(errno));
				return -1;
			} else if (!(stat(outputLocation, &fileStat) == 0 && S_ISFIFO(fileStat.st_mode))) {
				fprintf(stderr, "WARNING: Normal file already exists at %s when a FIFO was requested, attempting to replace with a FIFO..\n", outputLocation);
				if (!remove(outputLocation)) {
					fprintf(stderr, "ERROR: Failed to remove non-FIFO file at %s (errno %d: %s), exiting.\n", outputLocation, errno, strerror(errno));
					return -1;
				}
				return _lofar_udp_io_write_setup_FILE(config, outp, iter);
			}
		}
	}

	FILE *tmpPtr = fopen(outputLocation, "wb");
	if (tmpPtr == NULL) {
		fprintf(stderr, "ERROR: Failed to open output (type %d. location %s, errno %d, (%s)), exiting.\n", config->readerType, outputLocation, errno, strerror(errno));
		return -1;
	}

	config->outputFiles[outp] = tmpPtr;

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
int64_t _lofar_udp_io_write_FILE(lofar_udp_io_write_config *const config, int8_t outp, const int8_t *src, int64_t nchars) {
	if (config->outputFiles[outp] != NULL) {
		return (int64_t) fwrite(src, sizeof(int8_t), nchars, config->outputFiles[outp]);
	}
	fprintf(stderr, "ERROR %s: Output file pointer is null on outp %d, exiting.\n", __func__, outp);
	return -1;
}

int64_t _check_FIFO_status(lofar_udp_io_write_config *config, int8_t outp, int returnReaders) {
	struct stat fifoStat;
	if (config->readerType != FIFO) {
		fprintf(stderr, "ERROR %s: Input config is not FIFO, exiting.\n", __func__);
		return -1;
	}

	if (-1 == stat(config->outputLocations[outp], &fifoStat)) {
		fprintf(stderr, "ERROR: Failed to check FIFO status while writing, exiting.\n");
		return -1;
	} else if (!S_ISFIFO(fifoStat.st_mode)) {
		fprintf(stderr, "ERROR: Writer struct claims to be FIFO, but a FIFO is not present at the output location (%s: %d\n)", config->outputLocations[outp],
		        fifoStat.st_mode);
		return -1;
	}

	if (returnReaders) {
		return (int64_t) fifoStat.st_nlink;
	}

	if (fifoStat.st_nlink < 1) {
		fprintf(stderr, "WARNING: Write request for FIFO, but it has no readers, the process may hang.\n");
	}

	return 0;
}

int64_t _lofar_udp_io_write_FIFO(lofar_udp_io_write_config *const config, int8_t outp, const int8_t *src, int64_t nchars) {
	if (_check_FIFO_status(config, outp, 0)) {
		return -1;
	}
	return _lofar_udp_io_write_FILE(config, outp, src, nchars);
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
void _lofar_udp_io_write_cleanup_FILE(lofar_udp_io_write_config *const config, int8_t outp) {
	if (config == NULL) {
		return;
	}

	if (config->outputFiles[outp] != NULL) {
		if (config->readerType == FIFO) {
			int64_t numReaders;
			if ((numReaders = _check_FIFO_status(config, outp, 1))) {
				fprintf(stderr, "WARNING %s: FIFO at %s still has %ld reader(s) and will not be removed from file system.\n", __func__, config->outputLocations[outp], numReaders);
			} else if (remove(config->outputLocations[outp]) != 0) {
				fprintf(stderr, "WARNING %s: Failed to remove old FIFO at %s (errno: %d, %s), continuing.\n",
				        __func__, config->outputLocations[outp], errno, strerror(errno));
			}
		}

		fclose(config->outputFiles[outp]);
		config->outputFiles[outp] = NULL;

	}

}
