/**
 * @brief Update the metadata struct to prepare for a DADA header write
 *
 * @param metadata	The struct to configure
 *
 * @return 0: Success, <0: Failure
 */
int32_t _lofar_udp_metadata_setup_DADA(__attribute__((unused)) lofar_udp_metadata *metadata) {
	return 0;
}

/**
 * @brief Update the metadata struct to prepare for a DADA header write
 *
 * @param metadata	The struct to configure
 *
 * @return 0: Success, <0: Failure
 */
int32_t _lofar_udp_metadata_update_DADA(__attribute__((unused)) lofar_udp_metadata *const metadata, __attribute__((unused)) int8_t newObs) {
	// update_BASE covers all updates, just return
	return 0;
}


/**
 * @brief      Write a DADA header to the specified buffer
 *
 * @param[in]      	hdr   The header
 * @param[out]     	headerBuffer The output buffer
 * @param[in]		headerLength The output buffer length
 *
 * @return     >0: Output header size in bytes, <0: Failure
 */
int64_t _lofar_udp_metadata_write_DADA(const lofar_udp_metadata *hdr, int8_t *const headerBuffer, int64_t headerLength) {
	if (headerBuffer == NULL || hdr == NULL) {
		fprintf(stderr, "ERROR %s: Null buffer provided (hdr: %p, headerBuFFer: %p), exiting.\n", __func__, hdr, headerBuffer);
		return -1;
	}

	headerLength /= sizeof(char) / sizeof(int8_t);

	const int32_t fileoutp = hdr->output_file_number % hdr->upm_num_outputs;
	const int32_t keyfmtLen = 16;
	char keyfmt[keyfmtLen];
	int32_t returnVal = 0;

	// At time of initial implementation the approx. maximum input size here is
	// (2 + MAX_NUM_PORTS) * DEF_STR_LEN -- beamctl, rawdatafiles
	// (20 + MAX_OUTPUT_DIMS) * META_STR_LEN -- everything str-based
	// 40 * (64/entry on the worst end) -- everything non-str
	// Approximate our warning as 16,000 characters

	const int32_t expectedDadaLength = 4096 * 4;
	if (headerLength < expectedDadaLength) {
		fprintf(stderr, "WARNING %s: headerBuffer may be too short (%ld characters provided, %d recommended), returning.\n", __func__, headerLength, expectedDadaLength);
		return -1;
	}

	char *workingBuffer = (char*) headerBuffer;

	// TODO: Concern: no easy way to check length of write against remaining buffer.
	// Maybe predict output length based on max(strlen, 64)?
	returnVal += _writeDouble_DADA(workingBuffer, "HDR_VERSION", hdr->hdr_version, 0);
	// Lovely chicken and the egg problem, we'll update it again later.
	returnVal += _writeLong_DADA(workingBuffer, "HDR_SIZE", (int64_t) strnlen(workingBuffer, headerLength));
	returnVal += _writeStr_DADA(workingBuffer, "INSTRUMENT", "CASPSR"); // TODO: ANYTHING ELSE, psrchive exits early if the machine isn't recognised.
	returnVal += _writeStr_DADA(workingBuffer, "TELESCOPE", hdr->telescope);
	returnVal += _writeInt_DADA(workingBuffer, "TELESCOPE_RSP", hdr->telescope_rsp_id);
	returnVal += _writeStr_DADA(workingBuffer, "RECEIVER", hdr->receiver);
	returnVal += _writeStr_DADA(workingBuffer, "OBSERVER", hdr->observer);
	returnVal += _writeStr_DADA(workingBuffer, "HOSTNAME", hdr->hostname);
	returnVal += _writeInt_DADA(workingBuffer, "BASEPORT", hdr->baseport);
	returnVal += _writeInt_DADA(workingBuffer, "FILE_NUMBER", hdr->output_file_number / hdr->upm_num_inputs);
	returnVal += _writeInt_DADA(workingBuffer, "RAW_FILE_NUMBER", hdr->output_file_number);

	for (int port = 0; port < hdr->upm_num_inputs; port++) {
		// The buffer will likely run out before we get anywhere near a number large enough to fail this snprintf
		if (snprintf(keyfmt, keyfmtLen - 1, "RAWFILE%d", port) < 0) {
			fprintf(stderr, "ERROR %s: Failed to create key for raw file %d, exiting.\n", __func__, port);
			return -1;
		}
		returnVal += _writeStr_DADA(workingBuffer, keyfmt, hdr->rawfile[port]);
	}

	returnVal += _writeStr_DADA(workingBuffer, "SOURCE", hdr->source);
	returnVal += _writeStr_DADA(workingBuffer, "COORD_BASIS", hdr->coord_basis);
	returnVal += _writeStr_DADA(workingBuffer, "RA", hdr->ra);
	returnVal += _writeStr_DADA(workingBuffer, "DEC", hdr->dec);
	returnVal += _writeStr_DADA(workingBuffer, "RA_ANALOG", hdr->ra_analog);
	returnVal += _writeStr_DADA(workingBuffer, "DEC_ANALOG", hdr->dec_analog);
	returnVal += _writeDouble_DADA(workingBuffer, "RA_RAD", hdr->ra_rad, 0);
	returnVal += _writeDouble_DADA(workingBuffer, "DEC_RAD", hdr->dec_rad, 0);
	returnVal += _writeDouble_DADA(workingBuffer, "RA_RAD_ANALOG", hdr->ra_rad_analog, 0);
	returnVal += _writeDouble_DADA(workingBuffer, "DEC_RAD_ANALOG", hdr->dec_rad_analog, 0);
	returnVal += _writeStr_DADA(workingBuffer, "OBS_ID", hdr->obs_id);

	// DSPSR requires strptime-parsable input %Y-%m-%D-%H:%M:%S, so it doesn't accept UTC_START with isot format / fractional seconds.
	char tmpUtc[META_STR_LEN + 1] = "";
	// Remove the fractional seconds
	if (sscanf(hdr->obs_utc_start, "%[^.]s%*s", tmpUtc) < 0) {
		fprintf(stderr, "ERROR %s: Failed to modify UTC_START for DADA header, exiting.\n", __func__);
		return -1;
	}
	char *workingPtr;
	// Convert the T to a '-'
	while((workingPtr = strstr(tmpUtc, "T")) != NULL) {
		*(workingPtr) = '-';
	}
	returnVal += _writeStr_DADA(workingBuffer, "UTC_START", tmpUtc);

	returnVal += _writeDouble_DADA(workingBuffer, "MJD_START", hdr->obs_mjd_start, 0);
	returnVal += _writeLong_DADA(workingBuffer, "OBS_OFFSET", hdr->obs_offset);
	returnVal += _writeLong_DADA(workingBuffer, "OBS_OVERLAP", hdr->obs_overlap);
	returnVal += _writeStr_DADA(workingBuffer, "BASIS", hdr->basis);
	returnVal += _writeStr_DADA(workingBuffer, "MODE", hdr->mode);


	returnVal += _writeDouble_DADA(workingBuffer, "FREQ", hdr->freq_raw, 0);
	returnVal += _writeDouble_DADA(workingBuffer, "BW", hdr->bw, 1);
	returnVal += _writeDouble_DADA(workingBuffer, "CHAN_BW", hdr->subband_bw, 1);
	returnVal += _writeDouble_DADA(workingBuffer, "FTOP", hdr->ftop, 0);
	returnVal += _writeDouble_DADA(workingBuffer, "FBOT", hdr->fbottom, 0);
	returnVal += _writeInt_DADA(workingBuffer, "NCHAN", hdr->nchan);
	returnVal += _writeInt_DADA(workingBuffer, "NRCU", hdr->nrcu);
	returnVal += _writeInt_DADA(workingBuffer, "NPOL", hdr->npol);
	// DSPSR front end: -32 is a float, dspsr backend: -32 is a very large number because this is stored in an unsigned integer.
	// Why?
	returnVal += _writeInt_DADA(workingBuffer, "NBIT", hdr->nbit > 0 ? hdr->nbit : -1 * hdr->nbit);
	returnVal += _writeInt_DADA(workingBuffer, "RESOLUTION", hdr->resolution);
	returnVal += _writeInt_DADA(workingBuffer, "NDIM", hdr->ndim);
	returnVal += _writeDouble_DADA(workingBuffer, "TSAMP",
								   hdr->tsamp != -1.0 ? hdr->tsamp * 1e6 : -1.0, // tsamp s -> us, check if unset before scaling.
								   0);
	returnVal += _writeStr_DADA(workingBuffer, "STATE", hdr->state);

	returnVal += _writeStr_DADA(workingBuffer, "COMMENT", "Further contents are metadata from UPM");

	returnVal += _writeStr_DADA(workingBuffer, "UPM_VERSION", hdr->upm_version);
	returnVal += _writeStr_DADA(workingBuffer, "UPM_REC_VERSION", hdr->rec_version);
	returnVal += _writeStr_DADA(workingBuffer, "UPM_DAQ", hdr->upm_daq);
	returnVal += _writeStr_DADA(workingBuffer, "UPM_BEAMCTL", hdr->upm_beamctl);

	if (snprintf(keyfmt, 15, "UPM_OUTPFMT%d", fileoutp) < 0) {
		fprintf(stderr, "ERROR %s: Failed to create key for UPM_OUTPFMT / %d, exiting.\n", __func__, fileoutp);
		return -1;
	}
	returnVal += _writeStr_DADA(workingBuffer, keyfmt, hdr->upm_outputfmt[fileoutp]);

	returnVal += _writeStr_DADA(workingBuffer, "UPM_OUTPFMTCMT", hdr->upm_outputfmt_comment);
	returnVal += _writeInt_DADA(workingBuffer, "UPM_NUM_INPUTS", hdr->upm_num_inputs);
	returnVal += _writeInt_DADA(workingBuffer, "UPM_READER", hdr->upm_reader);
	returnVal += _writeInt_DADA(workingBuffer, "UPM_MODE", hdr->upm_procmode);
	returnVal += _writeInt_DADA(workingBuffer, "UPM_BANDFLIP", hdr->upm_bandflip);
	returnVal += _writeInt_DADA(workingBuffer, "UPM_REPLAY", hdr->upm_replay);
	returnVal += _writeInt_DADA(workingBuffer, "UPM_CALIBRATED", hdr->upm_calibrated);
	returnVal += _writeLong_DADA(workingBuffer, "UPM_BLOCKSIZE", hdr->upm_blocksize);
	returnVal += _writeLong_DADA(workingBuffer, "UPM_PKTPERITR", hdr->upm_pack_per_iter);
	returnVal += _writeLong_DADA(workingBuffer, "UPM_PROCPKT", hdr->upm_processed_packets);
	returnVal += _writeLong_DADA(workingBuffer, "UPM_DRPPKT", hdr->upm_dropped_packets);
	returnVal += _writeInt_DADA(workingBuffer, "UPM_LSTDRPPKT", hdr->upm_last_dropped_packets);
	returnVal += _writeInt_DADA(workingBuffer, "UPM_BITMODE", hdr->upm_input_bitmode);
	returnVal += _writeInt_DADA(workingBuffer, "UPM_RCUCLOCK", hdr->upm_rcuclock);
	returnVal += _writeInt_DADA(workingBuffer, "UPM_RAWBEAMLETS", hdr->upm_rawbeamlets);
	returnVal += _writeInt_DADA(workingBuffer, "UPM_UPPERBEAMLET", hdr->upm_upperbeam);
	returnVal += _writeInt_DADA(workingBuffer, "UPM_LOWERBEAMLET", hdr->upm_lowerbeam);

	// ascii_header_set should overwrite values if they already exist, and the length -shouldn't- change between iterations given that the value is padded
	// As a result, this should only change the buffer length for the first write to disk, assuming the buffer is re-used
	returnVal += _writeLong_DADA(workingBuffer, "HDR_SIZE", (long) strnlen(workingBuffer, headerLength));

	if (returnVal < 0) {
		fprintf(stderr, "ERROR: %d errors occurred while preparing the header, exiting.\n", -1 * returnVal);
		return -1;
	}
	return strnlen(workingBuffer, headerLength);
}

/**
 * @brief      Wraps ascii_header_set to write a string to the header
 *
 * @param      headerBuffer 	Buffer pointer
 * @param[in]      key      The key
 * @param[in]      val      The value
 *
 * @return 0: success, other: failure
 */
int32_t _writeStr_DADA(char *header, const char *key, const char *value) {
	VERBOSE(printf("DADA HEADER %s: %s, %s\n", __func__, key, value));

	if (key == NULL || value == NULL || _isEmpty(key)) {
		fprintf(stderr, "ERROR: DADA string unset: %p: %p.\n", key, value);
		return -1;
	}

	if (!_isEmpty(value)) {
		return (ascii_header_set(header, key, "%s", value) > -1) ? 0 : -1;
	}

	VERBOSE(fprintf(stderr, "ERROR %s: %s UNSET\n", __func__, key));
	return 0;
}

/**
 * @brief      Wraps ascii_header_set to write an int to the header
 *
 * @param      headerBuffer 	Buffer pointer
 * @param[in]      key      The key
 * @param[in]      val      The value
 *
 * @return 0: success, other: failure
 */
int32_t _writeInt_DADA(char *header, const char *key, int32_t value) {
	VERBOSE(printf("DADA HEADER %s: %s, %d\n", __func__, key, value));
	if (key == NULL || _isEmpty(key)) {
		fprintf(stderr, "ERROR: DADA key unset: %p: %d.\n", key, value);
		return -1;
	}

	if (!_intNotSet(value)) {
		return (ascii_header_set(header, key, "%d", value) > -1) ? 0 : -1;
	}

	VERBOSE(fprintf(stderr, "ERROR %s: %s UNSET\n", __func__, key));
	return 0;
}

/**
 * @brief      Wraps ascii_header_set to write a long to the header
 *
 * @param      headerBuffer 	Buffer pointer
 * @param[in]      key      The key
 * @param[in]      val      The value
 *
 * @return 0: success, other: failure
 */
int32_t _writeLong_DADA(char *header, const char *key, int64_t value) {
	VERBOSE(printf("DADA HEADER %s: %s, %ld\n", __func__, key, value));
	if (key == NULL || _isEmpty(key)) {
		fprintf(stderr, "ERROR: DADA key unset: %p: %ld.\n", key, value);
		return -1;
	}

	if (!_longNotSet(value)) {
		return (ascii_header_set(header, key, "%ld", value) > -1) ? 0 : -1;
	}

	VERBOSE(fprintf(stderr, "ERROR %s: %s UNSET\n", __func__, key));
	return 0;
}

/**
 * @brief      Wraps ascii_header_set to write a float to the header
 *
 * @param      headerBuffer 	Buffer pointer
 * @param[in]      key      The key
 * @param[in]      val      The value
 * @param[in]	   exception Swaps the unset check from -1.0 to 0.0
 *
 * @return 0: success, other: failure
 */
__attribute__((unused)) int32_t _writeFloat_DADA(char *header, const char *key, float value, int32_t exception)  {
	VERBOSE(printf("DADA HEADER %s: %s, %f\n", __func__, key, value));
	if (key == NULL || _isEmpty(key)) {
		fprintf(stderr, "ERROR: DADA key unset: %p: %f.\n", key, value);
		return -1;
	}

	if (!_doubleNotSet(value, exception)) {
		return ascii_header_set(header, key, "%.12f", value) > -1 ? 0 : -1;
	}

	VERBOSE(fprintf(stderr, "ERROR %s: %s UNSET\n", __func__, key));
	return 0;
}

/**
 * @brief      Wraps ascii_header_set to write a double to the header
 *
 * @param      headerBuffer 	Buffer pointer
 * @param[in]      key      The key
 * @param[in]      val      The value
 * @param[in]	   exception Swaps the unset check from -1.0 to 0.0
 *
 * @return 0: success, other: failure
 */
int32_t _writeDouble_DADA(char *header, const char *key, double value, int8_t exception) {
	VERBOSE(printf("DADA HEADER %s: %s, %lf\n", __func__, key, value));
	if (key == NULL || _isEmpty(key)) {
		fprintf(stderr, "ERROR: DADA key unset: %p: %lf.\n", key, value);
		return -1;
	}

	if (!_doubleNotSet(value, exception)) {
		return ascii_header_set(header, key, "%.17lf", value) > -1 ? 0 : -1;
	}

	VERBOSE(fprintf(stderr, "ERROR %s: %s UNSET\n", __func__, key));
	return 0;
}


// Old functions relating to parsing a DADA header, may be used with the recorder input in the future.
/*
int lofar_udp_dada_parse_header(lofar_udp_metadata *hdr, char *header);


 int lofar_udp_dada_parse_header(lofar_udp_metadata *hdr, char *header) {
	if (isEmpty(header)) { return 0; }

	int returnVal = 0;
	returnVal += getDouble_DADA(header, "HDR_VERSION", &(hdr->hdr_version));
	returnVal += getStr_DADA(header, "INSTRUMENT", &(hdr->instrument[0]));
	returnVal += getStr_DADA(header, "TELESCOPE", &(hdr->telescope[0]));
	returnVal += getStr_DADA(header, "RECEIVER", &(hdr->receiver[0]));
	returnVal += getStr_DADA(header, "HOSTNAME", &(hdr->hostname[0]));
	returnVal += getInt_DADA(header, "FILE_NUMBER", &(hdr->output_file_number));

	returnVal += getStr_DADA(header, "SOURCE", &(hdr->source[0]));
	returnVal += getStr_DADA(header, "RA", &(hdr->ra[0]));
	returnVal += getStr_DADA(header, "DEC", &(hdr->dec[0]));
	returnVal += getStr_DADA(header, "OBS_ID", &(hdr->obs_id[0]));
	returnVal += getStr_DADA(header, "UTC_START", &(hdr->obs_utc_start[0]));
	returnVal += getDouble_DADA(header, "MJD_START", &(hdr->obs_mjd_start));
	returnVal += getLong_DADA(header, "OBS_OFFSET", &(hdr->obs_offset));
	returnVal += getLong_DADA(header, "OBS_OVERLAP", &(hdr->obs_overlap));
	returnVal += getStr_DADA(header, "BASIS", &(hdr->basis[0]));
	returnVal += getStr_DADA(header, "MODE", &(hdr->mode[0]));

	returnVal += getDouble_DADA(header, "FREQ", &(hdr->freq));
	returnVal += getDouble_DADA(header, "BW", &(hdr->bw));
	returnVal += getInt_DADA(header, "NCHAN", &(hdr->nchan));
	returnVal += getInt_DADA(header, "NPOL", &(hdr->npol));
	returnVal += getInt_DADA(header, "NBIT", &(hdr->nbit));
	returnVal += getInt_DADA(header, "RESOLUTION", &(hdr->resolution));
	returnVal += getInt_DADA(header, "NDIM", &(hdr->ndim));
	returnVal += getDouble_DADA(header, "TSAMP", &(hdr->tsamp));
	returnVal += getStr_DADA(header, "STATE", &(hdr->state[0]));

	returnVal += getStr_DADA(header, "UPM_VERSION", &(hdr->upm_version[0]));
	returnVal += getStr_DADA(header, "REC_VERSION", &(hdr->rec_version[0]));
	returnVal += getInt_DADA(header, "UPM_READER", &(hdr->upm_reader));
	returnVal += getInt_DADA(header, "UPM_MODE", &(hdr->upm_procmode));
	returnVal += getInt_DADA(header, "UPM_REPLAY", &(hdr->upm_replay));
	returnVal += getInt_DADA(header, "UPM_CALIBRATED", &(hdr->upm_calibrated));
	returnVal += getInt_DADA(header, "UPM_BITMODE", &(hdr->upm_input_bitmode));
	returnVal += getInt_DADA(header, "UPM_RAWBEAMLETS", &(hdr->upm_rawbeamlets));
	returnVal += getInt_DADA(header, "UPM_UPPERBEAMLET", &(hdr->upm_upperbeam));
	returnVal += getInt_DADA(header, "UPM_LOWERBEAMLET", &(hdr->upm_lowerbeam));

	return returnVal;
}


int lofar_udp_dada_parse_multiple_headers(lofar_udp_metadata *output, char *header[MAX_NUM_PORTS], int numHeaders) {

	// Parse the first header as a reference
	lofar_udp_dada_parse_header(output, header[0]);

	// Ensure parameters are the same between observations
	for (int hdr = 1; hdr < numHeaders; hdr++) {}

	// Combine variable parameters
	for (int hdr = 0; hdr < numHeaders; hdr++) {}

	// Cap variable components
	for (int hdr = 0; hdr < numHeaders; hdr++) {}

	return 0;
}

int getStr_DADA(char *header, const char *key, const char *value);
int getInt_DADA(char *header, const char *key, const int *value);
int getLong_DADA(char *header, const char *key, const long *value);
int getDouble_DADA(char *header, const char *key, const double *value);

int getStr_DADA(char *header, const char *key, const char *value) {
	return ascii_header_get(header, key, "%s", value) > -1 ? 0 : -1;
}

int getInt_DADA(char *header, const char *key, const int *value) {
	return (ascii_header_get(header, key, "%d", value) > -1) ? 0 : -1;
}

int getLong_DADA(char *header, const char *key, const long *value) {
	return (ascii_header_get(header, key, "%ld", value) > -1) ? 0 : -1;
}

int getDouble_DADA(char *header, const char *key, const double *value) {
	return ascii_header_get(header, key, "%lf", value) > -1 ? 0 : -1;
}


*/