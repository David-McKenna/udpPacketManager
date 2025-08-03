

// Internal functions
void _lofar_udp_io_cleanup_DADA_loop(ipcbuf_t *buff, float *timeout);
int32_t _lofar_udp_io_write_setup_DADA_ringbuffer(int32_t dadaKey, uint64_t nbufs, int64_t bufSize, uint32_t numReaders, int8_t reallocateExisting);

// Read Interface

/**
 * @brief      Setup the read I/O struct to handle PSRDADA data
 *
 * @param      input   The input
 * @param[in]  dadaKey	The target ringbuffer to attach to
 * @param[in]  port    The index offset from the base file
 *
 * @return     0: Success, <0: Failure
 */
int32_t
_lofar_udp_io_read_setup_DADA(lofar_udp_io_read_config *const input, const key_t dadaKey, const int8_t port) {
#ifndef NODADA
	// Init the logger and HDU
	input->multilog[port] = multilog_open("udpPacketManager", 0);
	input->dadaReader[port] = dada_hdu_create(input->multilog[port]);

	if (input->multilog[port] == NULL || input->dadaReader[port] == NULL) {
		fprintf(stderr, "ERROR: Unable to initialise PSRDADA logger on port %d. Exiting.\n", port);
		return -1;
	}

	// If successful, connect to the ringbuffer as a given reader type
	if (dadaKey < IPC_PRIVATE || dadaKey > INT32_MAX) {
		fprintf(stderr, "ERROR: Invalid PSRDADA ringbuffer key (%d) on port %d, exiting.\n", dadaKey, port);
		return -2;
	}
	dada_hdu_set_key(input->dadaReader[port], dadaKey);

	// Connect to the ringbuffer instance
	if (dada_hdu_connect(input->dadaReader[port])) {
		return -3;
	}

	// Register ourselfs as the active reader
	if (dada_hdu_lock_read(input->dadaReader[port])) {
		return -4;
	}

	// If we are restarting, align to the expected packet length
	if ((ipcio_tell(input->dadaReader[port]->data_block) % input->portPacketLength[port]) != 0) {
		if (ipcio_seek(input->dadaReader[port]->data_block,
		               input->portPacketLength[port] - (int64_t) (ipcio_tell(input->dadaReader[port]->data_block) % input->portPacketLength[port]), SEEK_CUR) < 0) {
			return -5;
		}
	}

	input->dadaPageSize[port] = (int64_t) ipcbuf_get_bufsz((ipcbuf_t *) input->dadaReader[port]->data_block);
	if (input->dadaPageSize[port] < 1) {
		fprintf(stderr, "ERROR: Failed to get PSRDADA buffer size on ringbuffer %d for port %d, exiting.\n", dadaKey,
				port);
		return -6;
	}

	// Remove our reader status until we need to perform a read
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
 * @brief      Perform a data read for a ringbuffer
 *
 * @param      input        The input
 * @param[in]  port         The index offset from the base file
 * @param      targetArray  The output array
 * @param[in]  nchars       The number of bytes to read
 *
 * @return     <=0: Failure, >0 Characters read
 */
int64_t _lofar_udp_io_read_DADA(lofar_udp_io_read_config *const input, const int8_t port, int8_t *const targetArray, const int64_t nchars) {
#ifndef NODADA

	VERBOSE(printf("reader_nchars: Entering read request (dada): %d, %d, %ld\n", port, input->inputDadaKeys[port], nchars));

	int64_t dataRead = 0, currentRead;

	// To prevent a lock-up when we call ipcio_stop, read the data in gulps
	int64_t readChars = ((dataRead + input->dadaPageSize[port]) > nchars) ? (nchars - dataRead)
																	   : input->dadaPageSize[port];
	// Register as the active reader
	if (dada_hdu_lock_read(input->dadaReader[port])) {
		return -1;
	}
	// Loop until we read the requested amount of data
	while (dataRead < nchars) {
		if ((currentRead = ipcio_read(input->dadaReader[port]->data_block, (char *) &(targetArray[dataRead]), readChars)) < 1) {
			fprintf(stderr, "ERROR: Failed to complete DADA read on port %d (%ld / %ld), returning partial data.\n",
					port, dataRead, nchars);
			// Return -1 if we haven't read anything
			//return (dataRead > 0) ? dataRead : -1;
			// If this read fails at the start of a read, return the error, otherwise return the data read so far.
			if (dataRead == 0) {
				dada_hdu_unlock_read(input->dadaReader[port]);
				return -1;
			} else {
				break;
			}
		}
		dataRead += currentRead;
		readChars = ((dataRead + input->dadaPageSize[port]) > nchars) ? (nchars - dataRead) : input->dadaPageSize[port];
	}

	// Unlock the ringbuffer
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
 * @brief      Cleanup ringbuffer references for the read I/O struct
 *
 * @param      input  The input
 * @param[in]  port   The index offset from the base file
 */
void _lofar_udp_io_read_cleanup_DADA(lofar_udp_io_read_config *const input, const int8_t port) {
#ifndef NODADA
	if (input == NULL) {
		return;
	}
	if (input->dadaReader[port] != NULL) {
		// Cleanup PSRDADA leaks
		FREE_NOT_NULL(input->dadaReader[port]->data_block->buf_ptrs);
		FREE_NOT_NULL(input->dadaReader[port]->data_block->buf.shm_addr);
		FREE_NOT_NULL(input->dadaReader[port]->header_block->shm_addr);
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
	input->dadaPageSize[port] = -1;
#endif
}



/**
 * @brief      Temporarily read in num bytes from a ringbuffer
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
int64_t
_lofar_udp_io_read_temp_DADA(void *outbuf, int64_t size, int64_t num, key_t dadaKey, int8_t resetSeek) {
#ifndef NODADA
	// Initialise a reader
	ipcio_t tmpReader;
	if (memcpy(&tmpReader, &(IPCIO_INIT), sizeof(ipcbuf_t)) != &tmpReader) {
		fprintf(stderr, "ERROR: Failed to initialise temp ringbuffer reader, exiting.\n");
		return -1;
	}
	// Only use an active reader for now, I just can't make the passive reader implementation stable.
	char readerChar = 'R';
	if (dadaKey < IPC_PRIVATE || dadaKey > INT32_MAX) {
		fprintf(stderr, "ERROR: Invalid PSRDADA ringbuffer key %d (%x), exiting.\n", dadaKey, dadaKey);
		return -1;
	}
	// All of these functions print their own error messages.

	// Connecting to an ipcio_t struct allows you to control it like any other file descriptor usng the ipcio_* functions
	// As a result, after connecting to the buffer...
	if (ipcio_connect(&tmpReader, dadaKey) < 0) {
		return -1;
	}

	// Open as the reader
	if (ipcio_open(&tmpReader, readerChar) < 0) {
		return -1;
	}

	// Cap reads at 1 buffer; if we go into the next buffer, we cannot go back.
	int64_t numRead = ipcbuf_get_bufsz(&(tmpReader.buf)) - 1;
	if (numRead < (size * num)) {
		fprintf(stderr, "WARNING: DADA temporary reads must be shorter than a buffer, reducing read from %ld to %ld.\n", (size * num), numRead);
		num = numRead / size;
	}

	// TODO: Consider seeking back to the start of the buffer before reading.
	if ((numRead = ipcio_read(&tmpReader, outbuf, num * size)) < 0) {
		return -1;
	}

	// Only the active reader needs an explicit seek + close, passive reader will raise an error here
#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantConditionsOC"
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
#pragma clang diagnostic pop


	// PSRDADA memory leak...
	FREE_NOT_NULL(tmpReader.buf_ptrs);
	if (ipcio_disconnect(&tmpReader) < 0) {
		return -1;
	}

	if (numRead != size * num) {
		fprintf(stderr, "Unable to read (%ld/)%ld elements from ringbuffer %d\n", numRead, size * num, dadaKey);
	}

	return numRead;
#else
	fprintf(stderr, "ERROR: PSRDADA not enabled at compile time, exiting.\n");
			return -1;
#endif
}


// Write Interface

/**
 * @brief      Setup the write I/O struct to handle ringbuffer data
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 *
 * @return     0: Success, <0: Failure
 */
int32_t _lofar_udp_io_write_setup_DADA(lofar_udp_io_write_config *const config, const int8_t outp) {
#ifndef NODADA

	// Initialise a multilog for the HDU
	if (config->dadaWriter[outp].multilog == NULL) {
		config->dadaWriter[outp].multilog = multilog_open(config->dadaConfig.programName, config->dadaConfig.syslog);

		if (config->dadaWriter[outp].multilog == NULL) {
			fprintf(stderr, "ERROR: Failed to initialise multilog struct on output %d, exiting.\n", outp);
			return -1;
		}

		multilog_add(config->dadaWriter[outp].multilog, stdout);
	}

	// Initialise a ringbuffer, followed by a HDU
	if (config->dadaWriter[outp].hdu == NULL) {
		// ringbuffer
		if (_lofar_udp_io_write_setup_DADA_ringbuffer(config->outputDadaKeys[outp],
		                                              config->dadaConfig.nbufs, config->writeBufSize[outp], config->dadaConfig.num_readers,
		                                              config->progressWithExisting) < 0 ||
	    // header ringbuffer
			_lofar_udp_io_write_setup_DADA_ringbuffer(config->outputDadaKeys[outp] + 1,
			                                          config->dadaConfig.nbufs, config->writeBufSize[outp], config->dadaConfig.num_readers,
			                                          config->progressWithExisting)) {
			return -1;
		}

		// Attach to the newly created ringbuffer
		config->dadaWriter[outp].hdu = dada_hdu_create(config->dadaWriter[outp].multilog);
		dada_hdu_set_key(config->dadaWriter[outp].hdu, config->outputDadaKeys[outp]);
		if (dada_hdu_connect(config->dadaWriter[outp].hdu) < 0) {
			return -1;
		}

		if (dada_hdu_lock_write(config->dadaWriter[outp].hdu) < 0) {
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

/**
 * @brief Setup a ringbuffer instance on a set key_t
 *
 * @param dadaKey The key_t of the ringbuffer
 * @param nbufs The number of buffers inthe ringbuffer
 * @param bufSize The size of each ringbuffer buffer
 * @param numReaders The number of expected readers for the ringbuffer
 * @param reallocateExisting Re-allocate a ringbuffer if it already exists
 *
 * @return 0: Success, <0: Failure
 */
int32_t _lofar_udp_io_write_setup_DADA_ringbuffer(const int32_t dadaKey, const uint64_t nbufs, const int64_t bufSize, const uint32_t numReaders, const int8_t reallocateExisting) {
#ifndef NODADA
	ipcio_t *ringbuffer = calloc(1, sizeof(ipcio_t));
	CHECK_ALLOC_NOCLEAN(ringbuffer, -1);
	*(ringbuffer) = IPCIO_INIT;

	// Create the ringbuffer
	if (ipcio_create(ringbuffer, dadaKey, nbufs, bufSize, numReaders) < 0) {
		// ipcio_create(...) prints error to stderr, so we just need to exit, or do something else.
		// If it exists, and we;re allowed to, re-allocate the ringbuffer
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

	// Close the ringbuffer, we will open it in a HDU
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
 * @brief      Perform a data write for a ringbuffer or it'sassociated header
 *
 * @param[in]  ringbuffer	The ringbuffer
 * @param[in]  src		The bytes to write
 * @param[in]  nchars	The number ofbytes to write
 * @param[in]  ipcbuf	Do a direct write to a ringbuffer with the ipcbuf interface (for headers)
 *
 * @return  >=0: Number of bytes written, <0: Failure
 */
int64_t _lofar_udp_io_write_DADA(ipcio_t *const ringbuffer, const int8_t *src, const int64_t nchars, const int8_t ipcbuf) {
#ifndef NODADA

	// If performing a data write,
	if (!ipcbuf) {
		// Open as the active writer
		// Now performed when we create the IO object
		// if (ipcio_open(ringbuffer, 'W') < 0) {
		// 	return -1;
		// }

		// Write the data
		int64_t writtenBytes = ipcio_write(ringbuffer, (char *) src, nchars);

		// Unlock the buffer and continue
		// if (ipcio_close(ringbuffer) < 0 || writtenBytes < 0) {
		if (writtenBytes < 0) {
			return -1;
		}

		return writtenBytes;
	} else {
		// If opening as a header, get the buffer and lock as a writer
		// This is possible as an ipcio_t starts with an ipcbuf_t, followed by extra data
		ipcbuf_t *buffer = (ipcbuf_t *) ringbuffer;
		// if (ipcbuf_lock_write(buffer) < 0) {
		// 	return -1;
		// }
		// While data in our buffer remains, fill up ringbuffer pages
		int64_t written = 0;
		while (nchars) {
			int64_t headerSize = (int64_t) ipcbuf_get_bufsz(buffer);
			int8_t *workingBuffer = (int8_t *) ipcbuf_get_next_write(buffer);
			int64_t toWrite = nchars > headerSize ? headerSize : nchars;

			if (memcpy(workingBuffer, &(src[written]), toWrite) != workingBuffer) {
				return -1;
			}

			written += toWrite;
			if (ipcbuf_mark_filled(buffer, toWrite) < 0) {
				return -1;
			}
		}

		// Unlock the buffer and continue
		// if (ipcbuf_unlock_write(buffer) < 0) {
		// 	return -1;
		// }
		return written;
	}
#else

	// If PSRDADA was disabled at compile time, error
	fprintf(stderr, "ERROR %s: PSRDADA was disable at compile time, exiting.\n", __func__);
	return -1;
#endif
}


/**
 * @brief      Cleanup ringbuffer references for the write I/O struct
 *
 * @param      config     The configuration
 * @param[in]  outp       The outp
 * @param[in]  fullClean  Perform a full clean, dealloc the ringbuffer
 */
void _lofar_udp_io_write_cleanup_DADA(lofar_udp_io_write_config *const config, const int8_t outp, const int8_t fullClean) {
#ifndef NODADA
	if (config == NULL) {
		return;
	}

	// PSRDADA assumes we still have the header open for writing, doing this will silence an error
	// ... but doing this causes the ringbuffer to not exist for the reader???
	if (config->dadaWriter[outp].hdu != NULL) {
		if (!ipcbuf_is_writer(config->dadaWriter[outp].hdu->header_block)) {
			ipcbuf_lock_write(config->dadaWriter[outp].hdu->header_block);
		}

		// Unlock write permissions
		dada_hdu_unlock_write(config->dadaWriter[outp].hdu);


		// Close the ringbuffer instances
		if (fullClean) {
			// Wait for readers to finish up and exit, or timeout (ipcio_read can hang if it requests data past the EOD point)
			if (config->dadaWriter[outp].hdu->data_block != NULL) {
				_lofar_udp_io_cleanup_DADA_loop((ipcbuf_t *) config->dadaWriter[outp].hdu->data_block, &(config->dadaConfig.cleanup_timeout));
				FREE_NOT_NULL(config->dadaWriter[outp].hdu->data_block->buf_ptrs); // Work around a bug in PSRDADA: free ipcio->buf_pts prior to destroying the ringbuffer to remove a memory leak
				FREE_NOT_NULL(config->dadaWriter[outp].hdu->data_block->buf.shm_addr); // Work around a bug in PSRDADA: free ipcio->buf_pts prior to destroying the ringbuffer to remove a memory leak
			}

			if (config->dadaWriter[outp].hdu->header_block != NULL) {
				_lofar_udp_io_cleanup_DADA_loop(config->dadaWriter[outp].hdu->header_block, &(config->dadaConfig.cleanup_timeout));
				FREE_NOT_NULL(config->dadaWriter[outp].hdu->header_block->shm_addr); // Work around a bug in PSRDADA: free ipcio->buf_pts prior to destroying the ringbuffer to remove a memory leak
			}

			if (config->dadaWriter[outp].multilog != NULL) {
				multilog_close(config->dadaWriter[outp].multilog);
				config->dadaWriter[outp].multilog = NULL;
			}

		}

		// Cleanup the writers
		// Part of this is a hangover from before we used the HDU interface, but it still works perfectly well
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
	}

#else
	// If PSRDADA was disabled at compile time, error
	fprintf(stderr, "ERROR %s: PSRDADA was disable at compile time, exiting.\n", __func__);
#endif
}

/**
 * @brief A ringbuffer may still have a reader when we want to destroy it, only destroy it after
 * 			the reader has complete it's work, it is hung on a read, or a timeout point has been reached
 * @param buff	The ringbuffer
 * @param timeoutPtr	The timeout time
 */
void _lofar_udp_io_cleanup_DADA_loop(ipcbuf_t *buff, float *timeoutPtr) {
#ifndef NODADA
	const int32_t sampleEveryNMilli = 5;

	float timeout = *timeoutPtr;
	float totalSleep = 0.001f * (float) sampleEveryNMilli;
	int64_t iters = 0;
	// Hold a reference to the last read buffer, we may be stuck within 1-2 blocks of the last write,
	//  in which case we will need to force an exit
	uint64_t previousBuffer = ipcbuf_get_read_count(buff);

	// Loop until we exit or timeout
	while (totalSleep < timeout) {
		if (ipcbuf_get_reader_conn(buff) != 0) {
			break;
		}

		// Notify the user of the current status every second
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
		totalSleep += 0.001f * (float) sampleEveryNMilli;
		iters += 1;
	}
#else
	// If PSRDADA was disabled at compile time, error
	fprintf(stderr, "ERROR %s: PSRDADA was disable at compile time, exiting.\n", __func__);
#endif
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
