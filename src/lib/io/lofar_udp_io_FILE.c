
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
int lofar_udp_io_read_setup_FILE(lofar_udp_io_read_config *input, const char *inputLocation, const int port) {
	VERBOSE(printf("Opening file at %s ofr port %d\n", inputLocation, port));
	input->fileRef[port] = fopen(inputLocation, "rb");
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
long lofar_udp_io_read_FILE(lofar_udp_io_read_config *input, const int port, char *targetArray, const long nchars) {
	// Decompressed file: Read and return the data as needed
	VERBOSE(printf("reader_nchars: Entering read request (normal): %d, %ld\n", port, nchars));
	return (long) fread(targetArray, sizeof(char), nchars, input->fileRef[port]);
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
 * @param      outbuf     The outbuf
 * @param[in]  size       The size
 * @param[in]  num        The number
 * @param[in]  inputFile  The input file
 * @param[in]  resetSeek  The reset seek
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_read_temp_FILE(void *outbuf, const size_t size, const int num, const char inputFile[],
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

	if (lofar_udp_io_write_setup_check_exists(outputLocation, config->progressWithExisting)) {
		return -1;
	}

	if (strcpy(config->outputLocations[outp], outputLocation) != config->outputLocations[outp]) {
		fprintf(stderr, "ERROR: Failed to copy output file path (%s), exiting.\n", outputLocation);
		return -1;
	}

	if (config->readerType == FIFO) {
		if (mkfifo(outputLocation, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH) < 0) {
			if (!config->progressWithExisting) {
				fprintf(stderr, "ERROR: Failed to create FIFO at %s, exiting.\n", outputLocation);
				return -1;
			} else {
				fprintf(stderr, "WARNING: Failed to create FIFO at %s, attempting to continue (we may end up writing to a file....\n", outputLocation);
			}
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
 * @param      src     The source
 * @param[in]  nchars  The nchars
 *
 * @return     { description_of_the_return_value }
 */
long lofar_udp_io_write_FILE(lofar_udp_io_write_config *config, const int outp, const int8_t *src, const long nchars) {
	return (long) fwrite(src, sizeof(char), nchars, config->outputFiles[outp]);
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
