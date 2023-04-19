

// Internal functions
void _lofar_udp_io_cleanup_DADA_loop(ipcbuf_t *buff, float *timeout);
int _lofar_udp_io_write_setup_DADA_ringbuffer(int32_t dadaKey, uint64_t nbufs, int64_t bufSize, uint32_t numReaders, int reallocateExisting);

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
_lofar_udp_io_read_setup_DADA(lofar_udp_io_read_config *const input, int32_t dadaKey, int8_t port) {
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

	if (dada_hdu_unlock_read(input->dadaReader[port])) {
		return -7;
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
int64_t _lofar_udp_io_read_DADA(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars) {
#ifndef NODADA

	VERBOSE(printf("reader_nchars: Entering read request (dada): %d, %d, %ld\n", port, input->inputDadaKeys[port], nchars));

	int64_t dataRead = 0, currentRead;

	// To prevent a lock up when we call ipcio_stop, read the data in gulps
	int64_t readChars = ((dataRead + input->dadaPageSize[port]) > nchars) ? (nchars - dataRead)
																	   : input->dadaPageSize[port];
	// Loop until we read the requested amount of data
	if (dada_hdu_lock_read(input->dadaReader[port])) {
		return -1;
	}
	while (dataRead < nchars) {
		if ((currentRead = ipcio_read(input->dadaReader[port]->data_block, (char *) &(targetArray[dataRead]), readChars)) < 1) {
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

	if (dada_hdu_unlock_read(input->dadaReader[port])) {
		return -1;
	}

	return dataRead;

#else
	// Raise an error if PSRDADA wasn't available at compile time
	fprintf(stderr, "ERROR %s: PSRDADA was disable at compile time, exiting.\n", __func__);
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
void _lofar_udp_io_read_cleanup_DADA(lofar_udp_io_read_config *const input, int8_t port) {
#ifndef NODADA
	if (input == NULL) {
		return;
	}
	if (input->dadaReader[port] != NULL) {
		/*
		if (dada_hdu_unlock_read(input->dadaReader[port]) < 0) {
			fprintf(stderr, "ERROR: Failed to close PSRDADA buffer %d on port %d.\n", input->inputDadaKeys[port], port);
		}
		*/
		FREE_NOT_NULL(input->dadaReader[port]->data_block->buf_ptrs);
		if (dada_hdu_disconnect(input->dadaReader[port]) < 0) {
			fprintf(stderr, "ERROR: Failed to disconnect from PSRDADA buffer %d on port %d.\n", input->inputDadaKeys[port],
					port);
		}
		FREE_NOT_NULL(input->dadaReader[port]);
	}

	if (input->multilog[port] != NULL) {
		if (multilog_close(input->multilog[port]) < 0) {
			fprintf(stderr, "ERROR: Failed to close PSRDADA multilogger struct on port %d.\n", port);
		}
		input->multilog[port] = NULL; // multilog_close frees the ptr;
	}
#endif
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
int64_t
_lofar_udp_io_read_temp_DADA(void *outbuf, int64_t size, int64_t num, key_t dadaKey, int8_t resetSeek) {
#ifndef NODADA
	ipcio_t tmpReader;
	if (memcpy(&tmpReader, &(IPCIO_INIT), sizeof(ipcbuf_t)) != &tmpReader) {
		fprintf(stderr, "ERROR: Failed to initialise temp ringbuffer reader, exiting.\n");
		return -1;
	}
	// Only use an active reader for now, I just can't make the passive reader implementationstable.
	char readerChar = 'R';
	if (dadaKey < 1) {
		fprintf(stderr, "ERROR: Invalid PSRDADA ringbuffer key %d (%x), exiting.\n", dadaKey, dadaKey);
		return -1;
	}
	// All of these functions print their own error messages.

	// Connecting to an ipcio_t struct allows you to control it like any other file descriptor usng the ipcio_* functions
	// As a result, after connecting to the buffer...
	if (ipcio_connect(&tmpReader, dadaKey) < 0) {
		return -1;
	}

	if (ipcio_open(&tmpReader, readerChar) < 0) {
		return -1;
	}

	int64_t numRead = ipcbuf_get_bufsz(&(tmpReader.buf)) - 1;
	if (numRead < (size * num)) {
		fprintf(stderr, "WARNING: DADA temporary reads must be shorter than a buffer, reducing read from %ld to %ld.\n", (size * num), numRead);
		num = numRead / size;
	}

	if ((numRead = ipcio_read(&tmpReader, outbuf, num * size)) < 0) {
		return -1;
	}

	// Only the active reader needs an explicit seek + close, passive reader will raise an error here
	if (readerChar == 'R') {
		// fseek() if requested....
		if (resetSeek == 1) {
			if (ipcio_seek(&tmpReader, -1 * (size * num), SEEK_CUR) < 0) {
				return -1;
			}
		}
		if (ipcio_close(&tmpReader) < 0) {
			return -1;
		}
	}


	// PSRDADA memory leak...
	FREE_NOT_NULL(tmpReader.buf_ptrs);
	if (ipcio_disconnect(&tmpReader) < 0) {
		return -1;
	}

	if (numRead != size * num) {
		fprintf(stderr, "Unable to read %ld elements from ringbuffer %d, exiting.\n", size * num, dadaKey);
		return -1;
	}

	return numRead;
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
int _lofar_udp_io_write_setup_DADA(lofar_udp_io_write_config *const config, int8_t outp) {
#ifndef NODADA

	if (config->dadaWriter[outp].multilog == NULL) {
		config->dadaWriter[outp].multilog = multilog_open(config->dadaConfig.programName, config->dadaConfig.syslog);

		if (config->dadaWriter[outp].multilog == NULL) {
			fprintf(stderr, "ERROR: Failed to initialise multilog struct on output %d, exiting.\n", outp);
			return -1;
		}

		multilog_add(config->dadaWriter[outp].multilog, stdout);
	}

	if (config->dadaWriter[outp].hdu == NULL) {
		if (_lofar_udp_io_write_setup_DADA_ringbuffer(config->outputDadaKeys[outp],
		                                              config->dadaConfig.nbufs, config->writeBufSize[outp], config->dadaConfig.num_readers,
		                                              config->progressWithExisting) < 0 ||
			_lofar_udp_io_write_setup_DADA_ringbuffer(config->outputDadaKeys[outp] + 1,
			                                          config->dadaConfig.nbufs, config->writeBufSize[outp], config->dadaConfig.num_readers,
			                                          config->progressWithExisting)) {
			return -1;
		}

		config->dadaWriter[outp].hdu = dada_hdu_create(config->dadaWriter[outp].multilog);
		dada_hdu_set_key(config->dadaWriter[outp].hdu, config->outputDadaKeys[outp]);
		if (dada_hdu_connect(config->dadaWriter[outp].hdu) < 0) {
			return -1;
		}

		if (sprintf(config->outputLocations[outp], "PSRDADA:%d", config->outputDadaKeys[outp]) < 0) {
			fprintf(stderr, "ERROR: Failed to copy output file path (PSRDADA:%d), exiting.\n",
			        config->outputDadaKeys[outp]);
			return -1;
		}

	}

	return 0;
#else
	// If PSRDADA was disabled at compile time, error
	fprintf(stderr, "ERROR %s: PSRDADA was disable at compile time, exiting.\n", __func__);
	return -1;
#endif
}

int _lofar_udp_io_write_setup_DADA_ringbuffer(int32_t dadaKey, uint64_t nbufs, int64_t bufSize, uint32_t numReaders, int reallocateExisting) {
#ifndef NODADA
	ipcio_t *ringbuffer = calloc(1, sizeof(ipcio_t));
	CHECK_ALLOC_NOCLEAN(ringbuffer, -1);
	*(ringbuffer) = IPCIO_INIT;


	if (ipcio_create(ringbuffer, dadaKey, nbufs, bufSize, numReaders) < 0) {
		// ipcio_create(...) prints error to stderr, so we just need to exit, or do something else.
		if (reallocateExisting) {
			fprintf(stderr, "WARNING: Failed to create ringbuffer, but progressWithExisting int is set, attempting to destroy and re-create to given ringbuffer %d (%x)...\n", dadaKey, dadaKey);

			if (ipcio_connect(ringbuffer, dadaKey) < 0) {
				fprintf(stderr, "ERROR: Failed to connect to existing ringbuffer %d (%x), exiting.", dadaKey, dadaKey);
				return -1;
			}

			if (ipcio_destroy(ringbuffer) < 0) {
				fprintf(stderr, "ERROR: Failed to destroy existing ringbuffer %d (%x), exiting.\n", dadaKey, dadaKey);
				return -1;
			}

			*(ringbuffer) = IPCIO_INIT;
			if (ipcio_create(ringbuffer, dadaKey, nbufs, bufSize, numReaders) < 0) {
				fprintf(stderr, "ERROR: Failed to re-allocate ringbuffer %d (%x), exiting.\n", dadaKey, dadaKey);
				return -1;
			}

		} else {
			return -1;
		}
	}

	// Close the ringbuffer
	if (ipcio_disconnect(ringbuffer)) {
		// ipcio_disconnect(...) prints error to stderr, so we just need to exit.
		return -1;
	}

	free(ringbuffer);

	return 0;
#else
	// If PSRDADA was disabled at compile time, error
	fprintf(stderr, "ERROR %s: PSRDADA was disable at compile time, exiting.\n", __func__);
	return -1;
#endif
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
int64_t _lofar_udp_io_write_DADA(ipcio_t *const ringbuffer, const int8_t *src, int64_t nchars, int8_t ipcbuf) {
#ifndef NODADA

	if (!ipcbuf) {
		if (ipcio_open(ringbuffer, 'W') < 0) {
			return -1;
		}

		int64_t writtenBytes = ipcio_write(ringbuffer, (char *) src, nchars);

		if (writtenBytes < 0 || ipcio_close(ringbuffer) < 0) {
			return -1;
		}

		return writtenBytes;
	} else {
		ipcbuf_t *buffer = (ipcbuf_t *) ringbuffer;
		if (ipcbuf_lock_write(buffer) < 0) {
			return -1;
		}
		int64_t written = 0;
		while (nchars) {
			int64_t headerSize = (int64_t) ipcbuf_get_bufsz(buffer);
			int8_t *workingBuffer = (int8_t *) ipcbuf_get_next_write(buffer);
			int64_t toWrite = nchars > headerSize ? headerSize : nchars;

			if (memcpy(workingBuffer, &(src[written]), toWrite) != workingBuffer) {
				return -1;
			}

			written += toWrite;
			if (ipcbuf_mark_filled (buffer, toWrite) < 0) {
				return -1;
			}
		}

		if (ipcbuf_unlock_write(buffer) < 0) {
			return -1;
		}
		return written;
	}
#else

	// If PSRDADA was disabled at compile time, error
	fprintf(stderr, "ERROR %s: PSRDADA was disable at compile time, exiting.\n", __func__);
	return -1;
#endif
	// Attempts to work around buffers not being marked as filled by manually filling each block
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
void _lofar_udp_io_write_cleanup_DADA(lofar_udp_io_write_config *const config, int8_t outp, int8_t fullClean) {
#ifndef NODADA
	if (config == NULL) {
		return;
	}

	// PSRDADA assumes we still have the header open for writing, doing this will silence an error
	// ... but doing this causes the ringbuffer to not exist for the reader???
	if (!ipcbuf_is_writer(config->dadaWriter[outp].hdu->header_block)) {
		ipcbuf_lock_write(config->dadaWriter[outp].hdu->header_block);
	}

	if (dada_hdu_unlock_write(config->dadaWriter[outp].hdu) < 0) {
		return;
	}


	// Close the ringbuffer writers
	if (fullClean) {
		// Wait for readers to finish up and exit, or timeout (ipcio_read can hang if it requests data past the EOD point)
		if (config->dadaWriter[outp].hdu->data_block != NULL) {
			_lofar_udp_io_cleanup_DADA_loop((ipcbuf_t *) config->dadaWriter[outp].hdu->data_block, &(config->dadaConfig.cleanup_timeout));
			FREE_NOT_NULL(config->dadaWriter[outp].hdu->data_block->buf_ptrs); // Work around a bug in PSRDADA: free ipcio->buf_pts prior to destroying the ringbuffer to remove a memory leak
		}

		if (config->dadaWriter[outp].hdu->header_block != NULL) {
			_lofar_udp_io_cleanup_DADA_loop(config->dadaWriter[outp].hdu->header_block, &(config->dadaConfig.cleanup_timeout));
		}

		if (config->dadaWriter[outp].multilog != NULL) {
			multilog_close(config->dadaWriter[outp].multilog);
			config->dadaWriter[outp].multilog = NULL;
		}

	}

	if (config->dadaWriter[outp].hdu->data_block) {
		ipcio_destroy(config->dadaWriter[outp].hdu->data_block);
	}
	FREE_NOT_NULL(config->dadaWriter[outp].hdu->data_block);
	if (config->dadaWriter[outp].hdu->header_block) {
		ipcbuf_destroy(config->dadaWriter[outp].hdu->header_block);
	}
	FREE_NOT_NULL(config->dadaWriter[outp].hdu->header_block);
	if (config->dadaWriter[outp].hdu) {
		dada_hdu_destroy(config->dadaWriter[outp].hdu);
		config->dadaWriter[outp].hdu = NULL;
	}
	FREE_NOT_NULL(config->dadaWriter[outp].hdu);

#else
	// If PSRDADA was disabled at compile time, error
	fprintf(stderr, "ERROR %s: PSRDADA was disable at compile time, exiting.\n", __func__);
#endif
}

void _lofar_udp_io_cleanup_DADA_loop(ipcbuf_t *buff, float *timeoutPtr) {
#ifndef NODADA
	const int32_t sampleEveryNMilli = 5;

	float timeout = *timeoutPtr;
	float totalSleep = 0.001f * sampleEveryNMilli;
	long iters = 0;
	uint64_t previousBuffer = ipcbuf_get_read_count(buff);

	while (totalSleep < timeout) {
		if (ipcbuf_get_reader_conn(buff) != 0) {
			break;
		}

		// Notify the user every second
		if (iters % (1000 / sampleEveryNMilli) == 0) {
			// If the reader is stuck on the last block, just exit early.
			if (iters != 0 && previousBuffer == ipcbuf_get_read_count(buff) && (ipcbuf_get_write_count(buff) - previousBuffer) < 2) {
				printf("DADA reader(s) appears to have stalled at the end of the observation, cleaning up early.\n");
				(*timeoutPtr) = 0.0f;
				break;
			}

			previousBuffer = ipcbuf_get_read_count(buff);
			printf("Waiting on DADA readers to exit (%.2f / %.2fs before timing out).\n", totalSleep, timeout);

		}

		usleep(1000 * sampleEveryNMilli);
		totalSleep += 0.001f * sampleEveryNMilli;
		iters += 1;
	}
#else
	// If PSRDADA was disabled at compile time, error
	fprintf(stderr, "ERROR %s: PSRDADA was disable at compile time, exiting.\n", __func__);
#endif
}