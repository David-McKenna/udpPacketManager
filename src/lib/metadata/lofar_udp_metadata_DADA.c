int lofar_udp_dada_parse_header(lofar_udp_metadata *hdr, char *header);
int lofar_udp_generate_generic_hdr(char *header, lofar_udp_metadata *hdr);
int lofar_udp_generate_upm_header(char *header, lofar_udp_metadata *hdr, lofar_udp_reader *reader);


int writeStr_DADA(char *header, const char *key, const char *value);
int writeInt_DADA(char *header, const char *key, const int value);
int writeLong_DADA(char *header, const char *key, const long value);
int writeDouble_DADA(char *header, const char *key, const double value);
int getStr_DADA(char *header, const char *key, const char *value);
int getInt_DADA(char *header, const char *key, const int *value);
int getLong_DADA(char *header, const char *key, const long *value);
int getDouble_DADA(char *header, const char *key, const double *value);

int isEmpty(const char *string);
int intNotSet(int input);
int longNotSet(long input);
int doubleNotSet(double input);

/**
 * @brief      { function_description }
 *
 * @param      output      The output
 * @param      header      The header
 * @param[in]  numHeaders  The number headers
 *
 * @return     { description_of_the_return_value }
 */
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


/**
 * @brief      { function_description }
 *
 * @param      hdr                The header
 * @param      reader             The reader
 * @param[in]  readCurrentHeader  The read current header
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_dada_generate_header(lofar_udp_metadata *hdr, lofar_udp_reader *reader, int readCurrentHeader) {
	char header[4096];
	if (readCurrentHeader) {
		if (lofar_udp_dada_parse_header(hdr, header) < 0) { return -1; }
	}
	// Reset the header
	header[0] = '\0';

	// If we are passed a reader, provide some default values
	if (reader != NULL) {
		// Allow the instrument to be set manually (since the tempo codes don't normally match the staiton names)
		if (isEmpty(hdr->instrument)) {
			if (lofar_get_station_name(reader->meta->stationID, hdr->instrument) < 0) {
				return -1;
			}
		}


		if (isEmpty(hdr->machine)) {
			if (gethostname(hdr->machine, 64) < 0) { fprintf(stderr, "ERROR: gethostname failed, errno %d (%s), exiting.\n", errno, strerror(errno)); }
		}
		if (isEmpty(hdr->hostname)) {
			if (gethostname(hdr->hostname, 64) < 0) { fprintf(stderr, "ERROR: gethostname failed, errno %d (%s), exiting.\n", errno, strerror(errno)); }
		}

		if (doubleNotSet(hdr->tsamp)) {
			hdr->tsamp =
				((reader->meta->clockBit ? 6e-6 : 5.12e-6) * (reader->meta->processingMode % 4000) > 99 ? 2 * (reader->meta->processingMode % 10) : 1);
		}


		if (isEmpty(hdr->utc_start)) {
			if (longNotSet(hdr->obs_offset)) {
				// Modulo by 2 as we have a fraction of data per-second, so we can't calculate the byte offset without rounding down to the nearest 2 seconds
				double packetTime = lofar_get_packet_time(reader->meta->inputData[0]);
				// Really avodiing trying to link math.h here....
				int timeRemainder = (int) ((long) packetTime) % 2;
				// (reader->meta->outputBitMode / 8) * totalProcBeamlets 	-- bytes per sample
				// (packetTime - ((long) packetTime) + timeRemainder) 		-- offset time
				hdr->obs_offset =
					(reader->meta->outputBitMode / 8) * reader->meta->totalProcBeamlets * ((packetTime - ((long) packetTime) + timeRemainder) / hdr->tsamp);
			}

			getStartTimeStringDAQ(reader, hdr->utc_start);

		}

		if (doubleNotSet(hdr->obs_mjd_start)) { hdr->obs_mjd_start = lofar_get_packet_time_mjd(reader->meta->inputData[0]); }


		if (doubleNotSet(hdr->bw)) { hdr->bw = (reader->meta->clockBit ? 0.156250 : 0.1953125) * reader->meta->totalProcBeamlets; }
		if (intNotSet(hdr->nchan)) { hdr->nchan = reader->meta->totalProcBeamlets; }
		if (intNotSet(hdr->npol)) { hdr->npol = 2; }
		if (intNotSet(hdr->ndim)) { hdr->ndim = ((reader->meta->processingMode % 4000) > 99 ? 1 : 2); }
		if (intNotSet(hdr->nbit)) { hdr->nbit = reader->meta->outputBitMode; }
		if (intNotSet(hdr->resolution)) { hdr->resolution = reader->meta->outputBitMode / 8; }

		if (isEmpty(hdr->state)) {
			if (reader->meta->processingMode > 99) {
				strncpy(hdr->state, "INTENSITY", META_STR_LEN - 1);
			} else {
				strncpy(hdr->state, "NYQUIST", META_STR_LEN - 1);
			}
		}

		if (intNotSet(hdr->upm_reader)) { hdr->upm_reader = reader->input->readerType; }
		if (intNotSet(hdr->upm_mode)) { hdr->upm_mode = reader->meta->processingMode; }
		if (intNotSet(hdr->upm_replay)) { hdr->upm_replay = reader->meta->replayDroppedPackets; }
		if (intNotSet(hdr->upm_calibrated)) { hdr->upm_calibrated = reader->meta->calibrateData; }
		if (intNotSet(hdr->upm_bitmode)) { hdr->upm_bitmode = reader->meta->inputBitMode; }
		if (intNotSet(hdr->upm_rawbeamlets)) { hdr->upm_rawbeamlets = reader->meta->totalRawBeamlets; }
		if (intNotSet(hdr->upm_upperbeam)) { hdr->upm_upperbeam = reader->meta->upperBeamlets[reader->meta->numPorts - 1]; }
		if (intNotSet(hdr->upm_lowerbeam)) { hdr->upm_lowerbeam = reader->meta->baseBeamlets[0]; }
		if (isEmpty(hdr->upm_proc)) { sprintf(hdr->upm_proc, "UPM v%s-based", UPM_VERSION); }
	}

	// Populate the standard
	if (lofar_udp_generate_generic_hdr(header, hdr) < 0) { return -1; }

	if (reader != NULL) {
		if (lofar_udp_generate_upm_header(header, hdr, reader) < 0) { return -1; }
	}

	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      hdr   The header
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_generate_generic_hdr(char *header, lofar_udp_metadata *hdr) {
	int returnVal = 0;
	returnVal += writeDouble_DADA(header, "HDR_VERSION", hdr->hdr_version);
	returnVal += writeStr_DADA(header, "INSTRUMENT", hdr->instrument);
	returnVal += writeStr_DADA(header, "TELESCOPE", hdr->telescope);
	returnVal += writeStr_DADA(header, "RECIEVER", hdr->reciever);
	returnVal += writeStr_DADA(header, "PRIMARY", hdr->machine);
	returnVal += writeStr_DADA(header, "HOSTNAME", hdr->hostname);
	returnVal += writeStr_DADA(header, "FILE_NAME", hdr->file_name);
	returnVal += writeInt_DADA(header, "FILE_NUMBER", hdr->file_number);

	returnVal += writeStr_DADA(header, "SOURCE", hdr->source);
	returnVal += writeStr_DADA(header, "RA", hdr->ra);
	returnVal += writeStr_DADA(header, "DEC", hdr->dec);
	returnVal += writeStr_DADA(header, "OBS_ID", hdr->obs_id);
	returnVal += writeStr_DADA(header, "UTC_START", hdr->utc_start);
	returnVal += writeDouble_DADA(header, "MJD_START", hdr->obs_mjd_start);
	returnVal += writeLong_DADA(header, "obs_OFFSET", hdr->obs_offset);
	returnVal += writeLong_DADA(header, "OBS_OVERLAP", hdr->obs_overlap);
	returnVal += writeStr_DADA(header, "BASIS", hdr->basis);
	returnVal += writeStr_DADA(header, "MODE", hdr->mode);

	returnVal += writeDouble_DADA(header, "FREQ", hdr->freq);
	returnVal += writeDouble_DADA(header, "BW", hdr->bw);
	returnVal += writeInt_DADA(header, "NCHAN", hdr->nchan);
	returnVal += writeInt_DADA(header, "NPOL", hdr->npol);
	returnVal += writeInt_DADA(header, "NBIT", hdr->nbit);
	returnVal += writeInt_DADA(header, "RESOLUTION", hdr->resolution);
	returnVal += writeInt_DADA(header, "NDIM", hdr->ndim);
	returnVal += writeDouble_DADA(header, "TSAMP", hdr->tsamp);
	returnVal += writeStr_DADA(header, "STATE", hdr->state);

	return returnVal;
}


/**
 * @brief      { function_description }
 *
 * @param      hdr     The header
 * @param      reader  The reader
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_generate_upm_header(char *header, lofar_udp_metadata *hdr, lofar_udp_reader *reader) {
	int returnVal = 0;
	returnVal += writeStr_DADA(header, "UPM_VERSION", hdr->upm_version);
	returnVal += writeStr_DADA(header, "REC_VERSION", hdr->rec_version);
	returnVal += writeStr_DADA(header, "UPM_PROC", hdr->upm_proc);
	returnVal += writeInt_DADA(header, "UPM_READER", hdr->upm_reader);
	returnVal += writeInt_DADA(header, "UPM_MODE", hdr->upm_mode);
	returnVal += writeInt_DADA(header, "UPM_REPLAY", hdr->upm_replay);
	returnVal += writeInt_DADA(header, "UPM_CALIBRATED", hdr->upm_calibrated);
	returnVal += writeInt_DADA(header, "UPM_BITMODE", hdr->upm_bitmode);
	returnVal += writeInt_DADA(header, "UPM_RAWBEAMLETS", hdr->upm_rawbeamlets);
	returnVal += writeInt_DADA(header, "UPM_UPPERBEAMLET", hdr->upm_upperbeam);
	returnVal += writeInt_DADA(header, "UPM_LOWERBEAMLET", hdr->upm_lowerbeam);

	return returnVal;
}


/**
 * @brief      { function_description }
 *
 * @param      hdr     The header
 * @param      header  The header
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_dada_parse_header(lofar_udp_metadata *hdr, char *header) {
	if (isEmpty(header)) { return 0; }

	int returnVal = 0;
	returnVal += getDouble_DADA(header, "HDR_VERSION", &(hdr->hdr_version));
	returnVal += getStr_DADA(header, "INSTRUMENT", &(hdr->instrument[0]));
	returnVal += getStr_DADA(header, "TELESCOPE", &(hdr->telescope[0]));
	returnVal += getStr_DADA(header, "RECIEVER", &(hdr->reciever[0]));
	returnVal += getStr_DADA(header, "PRIMARY", &(hdr->machine[0]));
	returnVal += getStr_DADA(header, "HOSTNAME", &(hdr->hostname[0]));
	returnVal += getStr_DADA(header, "FILE_NAME", &(hdr->file_name[0]));
	returnVal += getInt_DADA(header, "FILE_NUMBER", &(hdr->file_number));

	returnVal += getStr_DADA(header, "SOURCE", &(hdr->source[0]));
	returnVal += getStr_DADA(header, "RA", &(hdr->ra[0]));
	returnVal += getStr_DADA(header, "DEC", &(hdr->dec[0]));
	returnVal += getStr_DADA(header, "OBS_ID", &(hdr->obs_id[0]));
	returnVal += getStr_DADA(header, "UTC_START", &(hdr->utc_start[0]));
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
	returnVal += getStr_DADA(header, "UPM_PROC", &(hdr->upm_proc[0]));
	returnVal += getInt_DADA(header, "UPM_READER", &(hdr->upm_reader));
	returnVal += getInt_DADA(header, "UPM_MODE", &(hdr->upm_mode));
	returnVal += getInt_DADA(header, "UPM_REPLAY", &(hdr->upm_replay));
	returnVal += getInt_DADA(header, "UPM_CALIBRATED", &(hdr->upm_calibrated));
	returnVal += getInt_DADA(header, "UPM_BITMODE", &(hdr->upm_bitmode));
	returnVal += getInt_DADA(header, "UPM_RAWBEAMLETS", &(hdr->upm_rawbeamlets));
	returnVal += getInt_DADA(header, "UPM_UPPERBEAMLET", &(hdr->upm_upperbeam));
	returnVal += getInt_DADA(header, "UPM_LOWERBEAMLET", &(hdr->upm_lowerbeam));

	return returnVal;
}


int isEmpty(const char *string) {
	return string[0] == '\0';
}

int intNotSet(int input) {
	return input == -1;
}

int longNotSet(long input) {
	return input == -1;
}

int doubleNotSet(double input) {
	return input == -1.0;
}

// Only set/get values that have been modified ascii_header_* can return 0 or 1
// on success, wrap this so that we always return 0 on success
//
// @param      header  The header
// @param[in]  key     The key
// @param[in]  value   The value
//
// @return     { description_of_the_return_value }
//
int writeStr_DADA(char *header, const char *key, const char *value) {
	if (!isEmpty(value)) {
		return (ascii_header_set(header, key, "%s", value) > -1) ? 0 : -1;
	}
	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      header  The header
 * @param[in]  key     The key
 * @param[in]  value   The value
 *
 * @return     { description_of_the_return_value }
 */
int writeInt_DADA(char *header, const char *key, const int value) {
	if (!intNotSet(value)) {
		return (ascii_header_set(header, key, "%d", value) > -1) ? 0 : -1;
	}
	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      header  The header
 * @param[in]  key     The key
 * @param[in]  value   The value
 *
 * @return     { description_of_the_return_value }
 */
int writeLong_DADA(char *header, const char *key, const long value) {
	if (!longNotSet(value)) {
		return (ascii_header_set(header, key, "%ld", value) > -1) ? 0 : -1;
	}
	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      header  The header
 * @param[in]  key     The key
 * @param[in]  value   The value
 *
 * @return     { description_of_the_return_value }
 */
int writeDouble_DADA(char *header, const char *key, const double value) {
	if (!doubleNotSet(value)) {
		return ascii_header_set(header, key, "%lf", value) > -1 ? 0 : -1;
	}
	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      header  The header
 * @param[in]  key     The key
 * @param[in]  value   The value
 *
 * @return     { description_of_the_return_value }
 */
int getStr_DADA(char *header, const char *key, const char *value) {
	return ascii_header_get(header, key, "%s", value) > -1 ? 0 : -1;
}

/**
 * @brief      { function_description }
 *
 * @param      header  The header
 * @param[in]  key     The key
 * @param[in]  value   The value
 *
 * @return     { description_of_the_return_value }
 */
int getInt_DADA(char *header, const char *key, const int *value) {
	return (ascii_header_get(header, key, "%d", value) > -1) ? 0 : -1;
}

/**
 * @brief      { function_description }
 *
 * @param      header  The header
 * @param[in]  key     The key
 * @param[in]  value   The value
 *
 * @return     { description_of_the_return_value }
 */
int getLong_DADA(char *header, const char *key, const long *value) {
	return (ascii_header_get(header, key, "%ld", value) > -1) ? 0 : -1;
}

/**
 * @brief      { function_description }
 *
 * @param      header  The header
 * @param[in]  key     The key
 * @param[in]  value   The value
 *
 * @return     { description_of_the_return_value }
 */
int getDouble_DADA(char *header, const char *key, const double *value) {
	return ascii_header_get(header, key, "%lf", value) > -1 ? 0 : -1;
}