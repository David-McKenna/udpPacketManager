/**
 * @brief Update the metadata struct to prepare for a SigProc header write
 *
 * @param metadata	The struct to configure
 *
 * @return 0: Success, <0: Failure
 */
int32_t _lofar_udp_metadata_setup_SIGPROC(lofar_udp_metadata *metadata) {
	if (metadata == NULL) {
		fprintf(stderr, "ERROR %s: Passed null metadata struct, exiting.\n", __func__);
		return -1;
	}
	FREE_NOT_NULL(metadata->output.sigproc);
	metadata->output.sigproc = sigproc_hdr_alloc(0); // Disable fchannels for now. TODO: Optional to enable
	CHECK_ALLOC_NOCLEAN(metadata->output.sigproc, -1);

	// Copy over strings
	if (strncpy(metadata->output.sigproc->source_name, metadata->source, META_STR_LEN) != metadata->output.sigproc->source_name) {
		fprintf(stderr, "ERROR: Failed to copy source name to sigproc_hdr, exiting.\n");
		return -1;
	}

	if (strncpy(metadata->output.sigproc->rawdatafile, metadata->rawfile[0], META_STR_LEN) != metadata->output.sigproc->rawdatafile) {
		fprintf(stderr, "ERROR: Failed to copy raw data file to sigproc_hdr, exiting.\n");
		return -1;
	}

	// Copy over assigned metadata
	metadata->output.sigproc->telescope_id = _sigprocStationID(metadata->telescope_rsp_id);
	//metadata->output.sigproc->machine_id = sigprocMachineID(metadata->machine);
	metadata->output.sigproc->tstart = metadata->obs_mjd_start;
	metadata->output.sigproc->tsamp = metadata->tsamp;
	metadata->output.sigproc->fch1 = metadata->ftop + 0.5 * metadata->channel_bw; // fch1 is half a channel offset from the top of the bandwidth
	metadata->output.sigproc->foff = metadata->channel_bw;
	metadata->output.sigproc->nchans = metadata->nchan;

	// Sigproc accepts positive int 32 as a float, unlike other specifications
	metadata->output.sigproc->nbits = metadata->nbit > 0 ? metadata->nbit : -1 * metadata->nbit;

	// Constants for what we produce
	// TODO: Modify if we want support for more than just single(-Stokes) outputs
	metadata->output.sigproc->nifs = 1;
	metadata->output.sigproc->data_type = 1;

	// Sigproc pointing variables take the form of a double, which is parsed as ddmmss.ss
	metadata->output.sigproc->src_raj = _sigprocStr2Pointing(metadata->ra);
	metadata->output.sigproc->src_decj = _sigprocStr2Pointing(metadata->dec);

	return 0;
}

int32_t _lofar_udp_metadata_update_SIGPROC(lofar_udp_metadata *const metadata, int8_t newObs) {
	if (metadata == NULL || metadata->output.sigproc == NULL) {
		fprintf(stderr, "ERROR %s: Input metadata struct is null, exiting.\n", __func__);
		return -1;
	}

	// Sigproc headers are only really meant to be used once at the start of the file, so they don't really change during processing.

	if (newObs) {
		metadata->output.sigproc->tstart = metadata->obs_mjd_start;
	}

	return 0;
}

/**
 * @brief      Write a SigProc header to the specified buffer
 *
 * @param[in]      	hdr   The header
 * @param[out]     	headerBuffer The output buffer
 * @param[in]		headerLength The output buffer length
 *
 * @return     >0: Output header size in bytes, <0: Failure
 */
#define HDR_LEN_LEFT headerLength - (workingBuffer - (char *) headerBuffer)
int64_t _lofar_udp_metadata_write_SIGPROC(const sigproc_hdr *hdr, int8_t *const headerBuffer, int64_t headerLength) {
	if (headerBuffer == NULL || hdr == NULL) {
		fprintf(stderr, "ERROR %s: Null buffer provided (hdr: %p, headerBuFFer: %p), exiting.\n", __func__, hdr, headerBuffer);
		return -1;
	}

	headerLength /= sizeof(char) / sizeof(int8_t);

	int64_t estimatedLength = 380;
	estimatedLength += strlen(hdr->rawdatafile) + strlen(hdr->source_name);

	if (headerLength < estimatedLength) {
		fprintf(stderr, "WARNING: Buffer is short (<%ld chars), we may overflow this buffer. Continuing with caution...\n", estimatedLength);
	}

	char *workingBuffer = (char *) headerBuffer;
	int64_t workingBufferLen = HDR_LEN_LEFT;
	workingBuffer = _writeKey_SIGPROC(workingBuffer, &workingBufferLen, "HEADER_START");
	workingBuffer = _writeInt_SIGPROC(workingBuffer, HDR_LEN_LEFT, "telescope_id", hdr->telescope_id);
	workingBuffer = _writeInt_SIGPROC(workingBuffer, HDR_LEN_LEFT, "machine_id", hdr->machine_id);
	workingBuffer = _writeInt_SIGPROC(workingBuffer, HDR_LEN_LEFT, "data_type", hdr->data_type);
	workingBuffer = _writeStr_SIGPROC(workingBuffer, HDR_LEN_LEFT, "rawdatafile", hdr->rawdatafile);
	workingBuffer = _writeStr_SIGPROC(workingBuffer, HDR_LEN_LEFT, "source_name", hdr->source_name);
	workingBuffer = _writeInt_SIGPROC(workingBuffer, HDR_LEN_LEFT, "barycentric", hdr->barycentric);
	workingBuffer = _writeInt_SIGPROC(workingBuffer, HDR_LEN_LEFT, "pulsarcentric", hdr->pulsarcentric);
	workingBuffer = _writeDouble_SIGPROC(workingBuffer, HDR_LEN_LEFT, "az_start", hdr->az_start, 0);
	workingBuffer = _writeDouble_SIGPROC(workingBuffer, HDR_LEN_LEFT, "za_start", hdr->za_start, 0);
	workingBuffer = _writeDouble_SIGPROC(workingBuffer, HDR_LEN_LEFT, "src_raj", hdr->src_raj, 0);// Maybe exception? Though ra/dec of  -1 arcsec seems less likely than 0
	workingBuffer = _writeDouble_SIGPROC(workingBuffer, HDR_LEN_LEFT, "src_dej", hdr->src_decj, 0);// Maybe exception?
	workingBuffer = _writeDouble_SIGPROC(workingBuffer, HDR_LEN_LEFT, "tstart", hdr->tstart, 0);
	workingBuffer = _writeDouble_SIGPROC(workingBuffer, HDR_LEN_LEFT, "tsamp", hdr->tsamp, 0);
	workingBuffer = _writeInt_SIGPROC(workingBuffer, HDR_LEN_LEFT, "nbits", hdr->nbits);
	//workingBuffer = writeInt_SIGPROC(workingBuffer, "nsamples", hdr->nsamples); // Unfortunately we can't predict the future yet :(
	workingBuffer = _writeDouble_SIGPROC(workingBuffer, HDR_LEN_LEFT, "fch1", hdr->fch1, 0);
	workingBuffer = _writeDouble_SIGPROC(workingBuffer, HDR_LEN_LEFT, "foff", hdr->foff, 1);
	workingBuffer = _writeInt_SIGPROC(workingBuffer, HDR_LEN_LEFT, "nchans", hdr->nchans);
	workingBuffer = _writeInt_SIGPROC(workingBuffer, HDR_LEN_LEFT, "nifs", hdr->nifs);
	workingBuffer = _writeDouble_SIGPROC(workingBuffer, HDR_LEN_LEFT, "refdm", hdr->refdm, 0);
	workingBuffer = _writeDouble_SIGPROC(workingBuffer, HDR_LEN_LEFT, "period", hdr->period, 0);

	// Don't implement frequency tables for now
	// if (hdr->fchannel != NULL) workingBuffer = writeDoubleArray_SIGPROC(workingBuffer, header->fchannel);

	workingBufferLen = HDR_LEN_LEFT;
	workingBuffer = _writeKey_SIGPROC(workingBuffer, &workingBufferLen, "HEADER_END");

	if (workingBuffer == NULL) {
		fprintf(stderr, "ERROR: Failed to generate sigproc header exiting.\n");
		return -1;
	}

	return ((int8_t*) workingBuffer + (sizeof(char) - sizeof(int8_t))) - headerBuffer;

}
#undef HDR_LEN_LEFT


int32_t _sigprocStationID(int32_t telescopeId) {
	switch(telescopeId) {
		// IE613 RSP code
		case 214:
			return 1916;

		default:
			return 0;
	}
}

__attribute__((unused)) int32_t _sigprocMachineID(const char *machineName) {
	if (strcmp(machineName, "REALTA_ucc1") == 0) {
		return 1916;
	} else {
		return 0;
	}
}

double _sigprocStr2Pointing(const char *input) {
	int dd, mm;
	double ss;
	if (sscanf(input, "%d:%d:%lf", &dd, &mm, &ss) != 3) {
		fprintf(stderr, "ERROR: Failed to parse Sigproc RA/Dec pointing, exiting.\n");
		return -1;
	}

	// Flip the sign for minutes / seconds if needed
	int sign = (dd > 0) ? 1 : -1;

	return dd * 1e4 + sign * mm * 1e2 + sign * ss;
}

/**
 * @brief      Writes a string value to the buffer
 *
 * @param      buffer 	Buffer pointer
 * @param[in] 	   bufferLength		Buffer length
 * @param[in]      name      The string
 *
 * @return New buffer head
 */
char* _writeKey_SIGPROC(char *buffer, int64_t *bufferLength, const char *name) {
	if (buffer == NULL || name == NULL || bufferLength == NULL) {
		return NULL;
	}

	int32_t len = strlen(name);

	if (len == 0) {
		fprintf(stderr, "ERROR: Sigproc writer input string is empty, exiting.\n");
		return NULL;
	}

	int32_t sizeLen = sizeof(int32_t);
	if (*bufferLength < (sizeLen + len)) {
		fprintf(stderr, "ERROR %s: Insufficient buffer size to write key %s, exiting.\n", __func__, name);
		return NULL;
	}
	if (memcpy(buffer, &len, sizeLen) != buffer) {
		fprintf(stderr, "ERROR: Failed to write string length for string %s, exiting.\n", name);
		return NULL;
	}
	buffer += sizeLen;
	*bufferLength -= sizeLen;

	VERBOSE(printf("strlen %s: %d\n", name, len));
	if (memcpy(buffer, name, len) != buffer) { // TODO: Is strlen returning characters or bytes?
		fprintf(stderr, "ERROR: Failed to write sigproc header string %s to buffer, exiting.\n", name);
		return NULL;
	}
	*bufferLength -= len;

	return buffer + len;
}

/**
 * @brief      Writes a string key value pair value to the buffer
 *
 * @param      buffer 	Buffer pointer
 * @param[in] 	   bufferLength		Buffer length
 * @param[in]      name      The key string
 * @param[in]      value      The value string
 *
 * @return New buffer head
 */
char* _writeStr_SIGPROC(char *buffer, int64_t bufferLength, const char *name, const char *value) {
	if (buffer == NULL || name == NULL || value == NULL) {
		return NULL;
	}


	if (_isEmpty(value) || _isEmpty(name)) {
		return buffer;
	}

	if ((buffer = _writeKey_SIGPROC(buffer, &bufferLength, name)) == NULL) {
		return NULL;
	}

	return _writeKey_SIGPROC(buffer, &bufferLength, value);
}

/**
 * @brief      Writes a string/int key value pair value to the buffer
 *
 * @param      buffer 	Buffer pointer
 * @param[in] 	   bufferLength		Buffer length
 * @param[in]      name      The key string
 * @param[in]      value      The value int
 *
 * @return New buffer head
 */
char *_writeInt_SIGPROC(char *buffer, int64_t bufferLength, const char *name, int32_t value) {
	if (buffer == NULL || name == NULL) {
		return NULL;
	}

	if (_intNotSet(value) || _isEmpty(name)) {
		return buffer;
	}

	if ((buffer = _writeKey_SIGPROC(buffer, &bufferLength, name)) == NULL) {
		return NULL;
	}

	int32_t chars = sizeof(int32_t);
	if (bufferLength < chars) {
		fprintf(stderr, "ERROR %s: Insufficient buffer size to write key %s, exiting.\n", __func__, name);
		return NULL;
	}
	if (memcpy(buffer, &value, chars) != buffer) {
		fprintf(stderr, "ERROR: Failed to copy sigproc header int %s: %d to buffer, exiting.\n", name, value);
		return NULL;
	}

	return buffer + chars;
}

// Spec doesn't have any floats, define the function anway
/**
 * @brief      Writes a string/long key value pair value to the buffer
 *
 * @param      buffer 	Buffer pointer
 * @param[in] 	   bufferLength		Buffer length
 * @param[in]      name      The key string
 * @param[in]      value      The value long
 *
 * @return New buffer head
 */
__attribute__((unused)) char *_writeLong_SIGPROC(char *buffer, int64_t bufferLength, const char *name, int64_t value) {
	if (buffer == NULL || name == NULL) {
		return NULL;
	}

	if (_longNotSet(value) || _isEmpty(name)) {
		return buffer;
	}

	if ((buffer = _writeKey_SIGPROC(buffer, &bufferLength, name)) == NULL) {
		return NULL;
	}

	int32_t chars = sizeof(int64_t);
	if (bufferLength < chars) {
		fprintf(stderr, "ERROR %s: Insufficient buffer size to write key %s, exiting.\n", __func__, name);
		return NULL;
	}
	if (memcpy(buffer, &value, chars) != buffer) {
		fprintf(stderr, "ERROR: Failed to copy sigproc header long %s: %ld to buffer, exiting.\n", name, value);
		return NULL;
	}

	return buffer + chars;
}

// Spec doesn't have any floats, define the function anyway
/**
 * @brief      Writes a string/float key value pair value to the buffer
 *
 * @param      buffer 	Buffer pointer
 * @param[in] 	   bufferLength		Buffer length
 * @param[in]      name      The key string
 * @param[in]      value      The value float
 * @param[in]      exception Swap -1.0 check for 0.0
 *
 * @return New buffer head
 */
__attribute__((unused)) char *_writeFloat_SIGPROC(char *buffer, int64_t bufferLength, const char *name, float value, int8_t exception) {
	if (buffer == NULL || name == NULL) {
		return NULL;
	}

	if (_floatNotSet(value, exception) || _isEmpty(name)) {
		return buffer;
	}

	if ((buffer = _writeKey_SIGPROC(buffer, &bufferLength, name)) == NULL) {
		return NULL;
	}

	int32_t chars = sizeof(float);
	if (bufferLength < chars) {
		fprintf(stderr, "ERROR %s: Insufficient buffer size to write key %s, exiting.\n", __func__, name);
		return NULL;
	}
	if (memcpy(buffer, &value, chars) != buffer) {
		fprintf(stderr, "ERROR: Failed to copy sigproc header float %s: %f to buffer, exiting.\n", name, value);
		return NULL;
	}

	return buffer + chars;
}

/**
 * @brief      Writes a string/double key value pair value to the buffer
 *
 * @param      buffer 	Buffer pointer
 * @param[in] 	   bufferLength		Buffer length
 * @param[in]      name      The key string
 * @param[in]      value      The value double
 * @param[in]      exception Swap -1.0 check for 0.0
 *
 * @return New buffer head
 */
char *_writeDouble_SIGPROC(char *buffer, int64_t bufferLength, const char *name, double value, int8_t exception) {
	if (buffer == NULL || name == NULL) {
		return NULL;
	}

	if (_doubleNotSet(value, exception) || _isEmpty(name)) {
		return buffer;
	}

	if ((buffer = _writeKey_SIGPROC(buffer, &bufferLength, name)) == NULL) {
		return NULL;
	}

	int32_t chars = sizeof(double);
	if (bufferLength < chars) {
		fprintf(stderr, "ERROR %s: Insufficient buffer size to write key %s, exiting.\n", __func__, name);
		return NULL;
	}
	if (memcpy(buffer, &value, chars) != buffer) {
		fprintf(stderr, "ERROR: Failed to copy sigproc header int %s: %lf to buffer, exiting.\n", name, value);
		return NULL;
	}

	return buffer + chars;
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
