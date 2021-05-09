#include "lofar_udp_io.h"


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
			input->numInputs++;
			return lofar_udp_io_read_setup_FILE(input, input->inputLocations[port], port);


		case ZSTDCOMPRESSED:
			input->numInputs++;
			return lofar_udp_io_read_setup_ZSTD(input, input->inputLocations[port], port);


		case DADA_ACTIVE:
			input->numInputs++;
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


int lofar_udp_io_read_setup_helper(lofar_udp_io_read_config *input, const lofar_udp_config *config, const lofar_udp_input_meta *meta,
                                   int port) {

	// If this is the first initialisation call, copy over the specified reader type
	if (input->readerType == NO_INPUT) {
		if (config->readerType == NO_INPUT) {
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
		input->dadaKeys[port] = config->dadaKeys[port];
	}

	// Copy over the raw input locations
	if (strcpy(input->inputLocations[port], config->inputLocations[port]) != input->inputLocations[port]) {
		fprintf(stderr, "ERROR: Failed to copy input location to reader on port %d, exiting.\n", port);
		return -1;
	}

	// Call the main setup function
	return lofar_udp_io_read_setup(input, port);
}

int lofar_udp_io_write_setup_helper(lofar_udp_io_write_config *config, const lofar_udp_input_meta *meta, int iter) {
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
reader_t lofar_udp_io_parse_type_optarg(const char optargc[], char *fileFormat, int *baseVal, int *stepSize, int *offsetVal) {

	reader_t reader;

	if (strstr(optargc, "%") != NULL) {
		fprintf(stderr, "\n\nWARNING: A %% was detected in your input, UPM v0.7+ no longer uses these for indexing.\n");
		fprintf(stderr, "WARNING: Use [[port]], [[outp]], [[pack]] and [[iter]] instead, check the docs for more details.\n\n");
	}

	if (optargc[4] == ':') {
		sscanf(optargc, "%*[^:]:%[^,],%d,%d,%d", fileFormat, baseVal, stepSize, offsetVal);

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
int lofar_udp_io_parse_format(char *dest, const char format[], int port, int iter, int idx, long pack) {

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

		if ((startSubStr = strstr(formatCopySrc, "[[idx]]"))) {
			(*startSubStr) = '\0';
			sprintf(formatCopyDst, "%s%d%s", formatCopySrc, idx, startSubStr + sizeof("[idx]]"));
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
	config->readerType = lofar_udp_io_parse_type_optarg(optargc, fileFormat, &(config->basePort), &(config->stepSizePort), &(config->offsetPortCount));


	if (config->readerType == -1) {
		fprintf(stderr, "ERROR: Failed to parse input pattern (%s), exiting.\n", optargc);
		return -1;
	}


	switch (config->readerType) {
		case NORMAL:
		case FIFO:
		case ZSTDCOMPRESSED:
			for (int i = 0; i < (MAX_NUM_PORTS - config->offsetPortCount); i++) {
				lofar_udp_io_parse_format(config->inputLocations[i], fileFormat, (config->basePort + config->offsetPortCount * config->stepSizePort) + i * config->stepSizePort, -1, i, -1);
			}
			break;

		case DADA_ACTIVE:
			// Swap values, default value is in the fileFormat for ringbuffers
			config->stepSizePort = config->basePort;

			// Parse the base value from the input
			config->basePort = atoi(fileFormat);
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
				config->dadaKeys[i] = (config->basePort + config->offsetPortCount * config->stepSizePort) + i * config->stepSizePort;
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

	int dummyInt;
	config->readerType = lofar_udp_io_parse_type_optarg(optargc, config->outputFormat, &(config->baseVal),
	                                                    &(config->stepSize), &(dummyInt));

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
			config->stepSize = config->baseVal;
			config->baseVal = atoi(config->outputFormat);
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
			return lofar_udp_io_read_ZSTD(input, port, targetArray, nchars);


		case DADA_ACTIVE:
			return lofar_udp_io_read_DADA(input, port, targetArray, nchars);


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
			return lofar_udp_io_write_DADA(config->dadaWriter[outp].ringbuffer, outp, src, nchars);


		default:
			fprintf(stderr, "ERROR: Unknown reader %d, exiting.\n", config->readerType);
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
int
lofar_udp_io_read_temp(const lofar_udp_config *config, const int port, void *outbuf, const size_t size, const int num,
						const int resetSeek) {
	if (num < 1 || size < 1) {
		fprintf(stderr, "ERROR: Invalid number of elements to read (%d * %ld), exiting.\n", num, size);
	}

	switch (config->readerType) {
		case NORMAL:
		case FIFO:
			return lofar_udp_io_read_temp_FILE(outbuf, size, num, config->inputLocations[port], resetSeek);


		case ZSTDCOMPRESSED:
			return lofar_udp_io_read_temp_ZSTD(outbuf, size, num, config->inputLocations[port], resetSeek);


		case DADA_ACTIVE:
			return lofar_udp_io_read_temp_DADA(outbuf, size, num, config->dadaKeys[port], resetSeek);


		default:
			fprintf(stderr, "ERROR: Unknown reader type (%d), exiting.\n", config->readerType);
			return -1;
	}

}

#include "./io/lofar_udp_io_FILE.c"
#include "./io/lofar_udp_io_ZSTD.c"
#include "./io/lofar_udp_io_DADA.c"



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