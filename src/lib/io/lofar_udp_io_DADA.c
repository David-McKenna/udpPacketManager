

// Internal functions
void lofar_udp_io_cleanup_DADA_loop(ipcbuf_t *buff, float timeout);
int lofar_udp_io_write_setup_DADA_ringbuffer(ipcio_t **ringbuffer, int dadaKey, uint64_t nbufs, long bufSize, unsigned int numReaders, int appendExisting);

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
			//return (dataRead > 0) ? dataRead : -1;
			// If this read fails at the start of a read, return the error, otherwise return the data read so far.
			if (dataRead == 0) {
				return -1;
			} else {
				break;
			}
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
lofar_udp_io_read_temp_DADA(void *outbuf, const size_t size, const int num, const int dadaKey, const int resetSeek) {
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




// Write Interface


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


		if (sprintf(config->outputLocations[outp], "PSRDADA:%d", config->outputDadaKeys[outp]) < 0) {
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
lofar_udp_io_write_DADA(lofar_udp_io_write_config *config, ipcio_t *ringbuffer, int outp, char *src, const long nchars) {
	/*
	printf("1 %p ,%p\n", src, config->dadaWriter[outp].ringbufferptr);
	char *buffer = config->dadaWriter[outp].ringbufferptr ?: ipcbuf_get_next_write((ipcbuf_t*) config->dadaWriter[outp].ringbuffer);
	long writtenChars = 0, writeSize = config->writeBufSize[outp], offset = config->dadaWriter[outp].ringbufferOffset?: 0;

	printf("2 %p ,%p, %ld, %ld, %ld\n", src, buffer, buffer - src, src - buffer, nchars);
	while(writtenChars < nchars) {
		if (offset == config->writeBufSize[outp] || buffer == NULL) {
			buffer = ipcbuf_get_next_write((ipcbuf_t*) config->dadaWriter[outp].ringbuffer);
			offset = 0;
		}

		writeSize = (config->writeBufSize[outp] - offset) < nchars ? (config->writeBufSize[outp] - offset) : nchars;


	printf("3 %p ,%p, %ld, %ld\n", src, buffer, writeSize, config->writeBufSize[outp]);
		if (memcpy(&(buffer[offset]), &(src[writtenChars]), writeSize) != &(buffer[offset])) {
			fprintf(stderr, "ERROR: Failed to write to output ringbuffer, exiting.\n");
			return -1;
		}
	printf("4 %p ,%p\n", src, buffer);

		writtenChars += writeSize;
		printf("%ld, %ld\n", writeSize / 7824, writtenChars);
		offset += writeSize;

		if (offset == config->writeBufSize[outp]) {
			if (ipcbuf_mark_filled((ipcbuf_t*) config->dadaWriter[outp].ringbuffer, config->writeBufSize[outp]) < 0) {
				fprintf(stderr, "ERROR: Failed to mark buffer as filled, exiting.\n");
				return -1;
			}
		}
	}

	printf("Op complete, exiting.\n");

	config->dadaWriter[outp].ringbufferptr = buffer;
	config->dadaWriter[outp].ringbufferOffset = offset;

	return writtenChars;
	*/

	long writtenBytes = ipcio_write(ringbuffer, src, nchars);
	if (writtenBytes < 0) {
		return -1;
	}

	return writtenBytes;

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
		if (config->dadaWriter[outp].ringbuffer != NULL && ipcio_is_open(config->dadaWriter[outp].ringbuffer)) {
			ipcbuf_mark_filled((ipcbuf_t*) config->dadaWriter[outp].ringbuffer, config->writeBufSize[outp]);
			ipcbuf_mark_filled((ipcbuf_t*) config->dadaWriter[outp].ringbuffer, config->writeBufSize[outp]);
			ipcbuf_enable_eod((ipcbuf_t*) config->dadaWriter[outp].ringbuffer);
			ipcio_close(config->dadaWriter[outp].ringbuffer);
		}

		if (config->dadaWriter[outp].header != NULL && ipcio_is_open(config->dadaWriter[outp].header)) {
			ipcio_close(config->dadaWriter[outp].header);
		}

		// Wait for readers to finish up and exit, or timeout (ipcio_read can hang if it requests data past the EOD point)
		if (config->dadaWriter[outp].ringbuffer != NULL) {
			lofar_udp_io_cleanup_DADA_loop((ipcbuf_t*) config->dadaWriter[outp].ringbuffer, config->dadaConfig.cleanup_timeout);
			FREE_NOT_NULL(config->dadaWriter[outp].ringbuffer->buf_ptrs); // Work around a bug in PSRDADA: free ipcio->buf_pts prior to destroying the ringbuffer to remove a memory leak
			ipcio_destroy(config->dadaWriter[outp].ringbuffer);
		}

		if (config->dadaWriter[outp].ringbuffer != NULL) {
			FREE_NOT_NULL(config->dadaWriter[outp].header->buf_ptrs); // Work around a bug in PSRDADA: free ipcio->buf_pts prior to destroying the ringbuffer to remove a memory leak
			ipcio_destroy(config->dadaWriter[outp].header);
		}

		if (config->dadaWriter[outp].multilog != NULL) {
			multilog_close(config->dadaWriter[outp].multilog);
			config->dadaWriter[outp].multilog = NULL;
		}

		// Destroy the ringbuffer instances
		FREE_NOT_NULL(config->dadaWriter[outp].ringbuffer);
		FREE_NOT_NULL(config->dadaWriter[outp].header);
	}

	return 0;
}

void lofar_udp_io_cleanup_DADA_loop(ipcbuf_t *buff, float timeout) {
	float totalSleep = 0.0f;
	long previousBuffer = ipcbuf_get_read_count(buff), iters = 0;

	while (totalSleep < timeout) {
		if (ipcbuf_get_reader_conn(buff) != 0) {
			break;
		}

		if (iters % 1000 == 0) {
			previousBuffer = ipcbuf_get_read_count(buff);
			printf("Waiting on DADA writer readers to exit (%.2f / %.2fs before timing out).\n", totalSleep, 30.0f);
		}

		usleep(1000 * 5);
		totalSleep += 0.005;
		iters += 1;

		// If the reader is stuck on the last block, just exit early.
		if (previousBuffer == (long) ipcbuf_get_read_count(buff) && (ipcbuf_get_write_count(buff) - previousBuffer) < 2) {
			break;
		}
	}

}