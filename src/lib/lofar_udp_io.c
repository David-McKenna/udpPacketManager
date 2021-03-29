#include "lofar_udp_io.h"


// etup functions





// Read functions






// Write Functions









/// Temp read functions
int fread_temp_FILE(void *outbuf, const size_t size, int num, FILE* inputFile, const int resetSeek) {
	size_t readlen = fread(inputFile, size, num, outbuf);

	if (resetSeek) {
		if (fseek(inputFile, -size * num, SEEK_CUR) != 0) {
			fprintf(stderr, "Failed to reset seek head, exiting.\n");
			return -2;
		}
	}

	if (readlen != size) {
		fprintf(stderr, "Unsable to read %ld elements from file, exiting.\n", size * num);
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
int fread_temp_ZSTD(void *outbuf, const size_t size, int num, FILE* inputFile, const int resetSeek) {
	
	// Build the decompression stream
	ZSTD_DStream* dstreamTmp = ZSTD_createDStream();
	const size_t minRead = ZSTD_DStreamInSize();
	const size_t minOut = ZSTD_DStreamOutSize();
	ZSTD_initDStream(dstreamTmp);

	// Allocate the buffer memory, build the buffer structs
	char* inBuff = calloc(minRead, sizeof(char));
	char* localOutBuff = calloc(minOut , sizeof(char));

	ZSTD_inBuffer tmpRead = {inBuff, minRead * sizeof(char), 0};
	ZSTD_outBuffer tmpDecom = {localOutBuff, minOut * sizeof(char), 0};

	// Read in the compressed data
	size_t readLen = fread(inBuff, sizeof(char), minRead, inputFile);
	if (readLen != minRead) {
		fprintf(stderr, "Unable to read in header from file; exiting.\n");
		ZSTD_freeDStream(dstreamTmp);
		free(inBuff);
		free(localOutBuff);
		return 1;
	}

	// Move the read head back to the start of the packer
	if (resetSeek) {
		if (fseek(inputFile, -minRead, SEEK_CUR) != 0) {
			fprintf(stderr, "Failed to reset seek head, exiting.\n");
			return -2;
		}
	}

	// Decompressed the data, check for errors
	size_t output = ZSTD_decompressStream(dstreamTmp, &tmpDecom , &tmpRead);

	VERBOSE(printf("Header decompression code: %ld, %s\n", output, ZSTD_getErrorName(output)));
	
	if (ZSTD_isError(output)) {
		fprintf(stderr, "ZSTD enountered an error while doing temp read (code %ld, %s), exiting.\n", output, ZSTD_getErrorName(output));
		ZSTD_freeDStream(dstreamTmp);
		free(inBuff);
		free(localOutBuff);
		return -1;
	}

	// Cap the return value of the data
	readLen = tmpDecom.pos;
	if (readLen > size * num) readLen = size * num;

	// Copy the output and cleanup
	memcpy(outbuf, localOutBuff, size * num); 
	ZSTD_freeDStream(dstreamTmp);
	free(inBuff);
	free(localOutBuff);

	return readLen;

}


#ifndef NODADA
/**
 * @brief      Temporarily read in num bytes from a PSRDADA ringbuffer
 *
 * @param      outbuf     The output buffer pointer
 * @param[in]  size       The size of words ot read
 * @param[in]  num        The number of words to read
 * @param      dadaKey  The PSRDADA ringbuffer ID
 * @param[in]  resetSeek  Do (1) / Don't (0) reset back to the original location
 *                        in FILE* after performing a read operation
 *
 * @return     int 0: ZSTD error, 1: File error, other: data read length
 */
int fread_temp_dada(void *outbuf, const size_t size, int num, int dadaKey, const int resetSeek) {
	ipcio_t tmpReader = IPCIO_INIT;
	char readerChar = 'R';
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
	// Pasive reader currently removed from implementation, as I can't
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
	// TODO: read packet length form header rather than hard coding to 7824 (here and in initlaisation)
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
		fprintf(stderr, "Unsable to read %ld elements from ringbuffer %d, exiting.\n", size * num, dadaKey);
		return -1;
	}

	return returnlen;
}
#endif



// Metadata functions
/**
 * @brief      Get the size of a file ptr on disk
 *
 * @param[in]  fd    File descriptor
 *
 * @return     long: size of file on disk in bytes
 */
long FILE_file_size(FILE* fileptr) {
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