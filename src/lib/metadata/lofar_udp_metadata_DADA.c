#include "lofar_udp_time.h"

// Internal writer prototypes
int writeStr_DADA(char *header, const char *key, const char *value);
int writeInt_DADA(char *header, const char *key, int value);
int writeLong_DADA(char *header, const char *key, long value);
int writeDouble_DADA(char *header, const char *key, double value, int exception);

// Define a float even if it isn't used
__attribute__((unused)) int writeFloat_DADA(char *header, const char *key, float value, int exception);



int lofar_udp_metadata_update_DADA(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, int newObs) {

	if (metadata == NULL) {
		fprintf(stderr, "ERROR %s: Input metadata struct is null, exiting.\n", __func__);
		return -1;
	}

	// DADA headers are only really meant to be used once at the start of the file, so they aren't meant to change too much
	// But since we use this struct as the base, we'll need to update the bulk of the contents on every iteration.

	if (newObs) {
		lofar_udp_time_get_current_isot(reader, metadata->utc_start);
		metadata->obs_mjd_start = lofar_udp_time_get_packet_time_mjd(reader->meta->inputData[0]);
		metadata->upm_processed_packets = 0;
		metadata->upm_dropped_packets = 0;
	}

	lofar_udp_time_get_daq(reader, metadata->upm_daq);
	metadata->upm_pack_per_iter = reader->meta->packetsPerIteration;
	metadata->upm_blocksize = metadata->upm_pack_per_iter * reader->meta->packetOutputLength[0];
	metadata->upm_processed_packets += metadata->upm_pack_per_iter * metadata->upm_num_inputs;

	metadata->upm_last_dropped_packets = 0;
	for (int port = 0; port < metadata->upm_num_inputs; port++) {
		metadata->upm_last_dropped_packets += reader->meta->portLastDroppedPackets[port];
	}
	metadata->upm_dropped_packets += metadata->upm_last_dropped_packets;

	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      hdr   The header
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_metadata_write_DADA(const lofar_udp_metadata *hdr, char *headerBuffer, size_t headerLength) {
	const int fileoutp = hdr->output_file_number % hdr->upm_num_outputs;
	char keyfmt[16];
	int returnVal = 0;

	// At time of initial implementation the approx. maximum input size here is
	// (2 + MAX_NUM_PORTS) * DEF_STR_LEN -- beamctl, rawdatafiles
	// (20 + MAX_OUTPUT_DIMS) * META_STR_LEN -- everything str-based
	// 40 * (64/entry on the worst end) -- everything non-str
	// Approximate our warning as 16kb

	if (headerLength < 4096 * 4) {
		fprintf(stderr, "WARNING %s: headerBuffer may be too short (%ld provided, %d recommended), attempting to continue, this may segfault...\n", __func__, headerLength, 4096 * 4);
	}



	returnVal += writeDouble_DADA(headerBuffer, "HDR_VERSION", hdr->hdr_version, 0);
	returnVal += writeStr_DADA(headerBuffer, "INSTRUMENT", "CASPSR");// TODO: ANYTHING ELSE, psrchive exits early if the machine isn't recognised.
	returnVal += writeStr_DADA(headerBuffer, "TELESCOPE", hdr->telescope);
	returnVal += writeInt_DADA(headerBuffer, "TELESCOPE_RSP", hdr->telescope_rsp_id);
	returnVal += writeStr_DADA(headerBuffer, "RECEIVER", hdr->receiver);
	returnVal += writeStr_DADA(headerBuffer, "OBSERVER", hdr->observer);
	returnVal += writeStr_DADA(headerBuffer, "HOSTNAME", hdr->hostname);
	returnVal += writeInt_DADA(headerBuffer, "BASEPORT", hdr->baseport);
	returnVal += writeInt_DADA(headerBuffer, "FILE_NUMBER", hdr->output_file_number / hdr->upm_num_inputs);
	returnVal += writeInt_DADA(headerBuffer, "RAW_FILE_NUMBER", hdr->output_file_number);

	for (int port = 0; port < hdr->upm_num_inputs; port++) {
		if (snprintf(keyfmt, 15, "RAWFILE%d", port) < 0) {
			fprintf(stderr, "ERROR %s: Failed to create key for raw file %d, exiting.\n", __func__, port);
			return -1;
		}
		returnVal += writeStr_DADA(headerBuffer, keyfmt, hdr->rawfile[port]);
	}

	returnVal += writeStr_DADA(headerBuffer, "SOURCE", hdr->source);
	returnVal += writeStr_DADA(headerBuffer, "COORD_BASIS", hdr->coord_basis);
	returnVal += writeStr_DADA(headerBuffer, "RA", hdr->ra);
	returnVal += writeStr_DADA(headerBuffer, "DEC", hdr->dec);
	returnVal += writeStr_DADA(headerBuffer, "RA_ANALOG", hdr->ra_analog);
	returnVal += writeStr_DADA(headerBuffer, "DEC_ANALOG", hdr->dec_analog);
	returnVal += writeDouble_DADA(headerBuffer, "RA_RAD", hdr->ra_rad, 0);
	returnVal += writeDouble_DADA(headerBuffer, "DEC_RAD", hdr->dec_rad, 0);
	returnVal += writeDouble_DADA(headerBuffer, "RA_RAD_ANALOG", hdr->ra_rad_analog, 0);
	returnVal += writeDouble_DADA(headerBuffer, "DEC_RAD_ANALOG", hdr->dec_rad_analog, 0);
	returnVal += writeStr_DADA(headerBuffer, "OBS_ID", hdr->obs_id);

	// DSPSR requires strptime-parsable input %Y-%m-%D-%H:%M:%S, so it doesn't accept UTC_START with isot format / fractional seconds.
	char tmpUtc[META_STR_LEN + 1] = "";
	if (sscanf(hdr->utc_start, "%[^.]s%*s", tmpUtc) < 0) {
		fprintf(stderr, "ERROR %s: Failed to modify UTC_START for DADA header, exiting.\n", __func__);
		return -1;
	}
	char *workingPtr;
	while((workingPtr = strstr(tmpUtc, "T")) != NULL) {
		*(workingPtr) = '-';
	}
	returnVal += writeStr_DADA(headerBuffer, "UTC_START", tmpUtc);

	returnVal += writeDouble_DADA(headerBuffer, "MJD_START", hdr->obs_mjd_start, 0);
	returnVal += writeLong_DADA(headerBuffer, "OBS_OFFSET", hdr->obs_offset);
	returnVal += writeLong_DADA(headerBuffer, "OBS_OVERLAP", hdr->obs_overlap);
	returnVal += writeStr_DADA(headerBuffer, "BASIS", hdr->basis);
	returnVal += writeStr_DADA(headerBuffer, "MODE", hdr->mode);


	returnVal += writeDouble_DADA(headerBuffer, "FREQ", hdr->freq, 0);
	returnVal += writeDouble_DADA(headerBuffer, "BW", hdr->bw, 1);
	returnVal += writeDouble_DADA(headerBuffer, "CHAN_BW", hdr->channel_bw, 1);
	returnVal += writeDouble_DADA(headerBuffer, "FTOP", hdr->ftop, 0);
	returnVal += writeDouble_DADA(headerBuffer, "FBOT", hdr->fbottom, 0);
	returnVal += writeInt_DADA(headerBuffer, "NCHAN", hdr->nchan);
	returnVal += writeInt_DADA(headerBuffer, "NRCU", hdr->nrcu);
	returnVal += writeInt_DADA(headerBuffer, "NPOL", hdr->npol);
	// DSPSR front end: -32 is a float, dspsr backend: -32 is a very large number because this is stored in an unsigned integer.
	// Why?
	returnVal += writeInt_DADA(headerBuffer, "NBIT", hdr->nbit > 0 ? hdr->nbit : -1 * hdr->nbit);
	returnVal += writeInt_DADA(headerBuffer, "RESOLUTION", hdr->resolution);
	returnVal += writeInt_DADA(headerBuffer, "NDIM", hdr->ndim);
	returnVal += writeDouble_DADA(headerBuffer, "TSAMP", hdr->tsamp, 0);
	returnVal += writeStr_DADA(headerBuffer, "STATE", hdr->state);


	returnVal += writeStr_DADA(headerBuffer, "COMMENT", "Further contents are metadata from UPM");

	returnVal += writeStr_DADA(headerBuffer, "UPM_VERSION", hdr->upm_version);
	returnVal += writeStr_DADA(headerBuffer, "UPM_REC_VERSION", hdr->rec_version);
	returnVal += writeStr_DADA(headerBuffer, "UPM_DAQ", hdr->upm_daq);
	returnVal += writeStr_DADA(headerBuffer, "UPM_BEAMCTL", hdr->upm_beamctl);

	if (snprintf(keyfmt, 15, "UPM_OUTPFMT%d", fileoutp) < 0) {
		fprintf(stderr, "ERROR %s: Failed to create key for UPM_OUTPFMT / %d, exiting.\n", __func__, fileoutp);
		return -1;
	}
	returnVal += writeStr_DADA(headerBuffer, keyfmt, hdr->upm_outputfmt[fileoutp]);

	returnVal += writeStr_DADA(headerBuffer, "UPM_OUTPFMTCMT", hdr->upm_outputfmt_comment);
	returnVal += writeInt_DADA(headerBuffer, "UPM_NUM_INPUTS", hdr->upm_num_inputs);
	returnVal += writeInt_DADA(headerBuffer, "UPM_READER", hdr->upm_reader);
	returnVal += writeInt_DADA(headerBuffer, "UPM_MODE", hdr->upm_procmode);
	returnVal += writeInt_DADA(headerBuffer, "UPM_BANDFLIP", hdr->upm_bandflip);
	returnVal += writeInt_DADA(headerBuffer, "UPM_REPLAY", hdr->upm_replay);
	returnVal += writeInt_DADA(headerBuffer, "UPM_CALIBRATED", hdr->upm_calibrated);
	returnVal += writeLong_DADA(headerBuffer, "UPM_BLOCKSIZE", hdr->upm_blocksize);
	returnVal += writeLong_DADA(headerBuffer, "UPM_PKTPERITR", hdr->upm_pack_per_iter);
	returnVal += writeLong_DADA(headerBuffer, "UPM_PROCPKT", hdr->upm_processed_packets);
	returnVal += writeLong_DADA(headerBuffer, "UPM_DRPPKT", hdr->upm_dropped_packets);
	returnVal += writeInt_DADA(headerBuffer, "UPM_LSTDRPPKT", hdr->upm_last_dropped_packets);
	returnVal += writeInt_DADA(headerBuffer, "UPM_BITMODE", hdr->upm_input_bitmode);
	returnVal += writeInt_DADA(headerBuffer, "UPM_RCUCLOCK", hdr->upm_rcuclock);
	returnVal += writeInt_DADA(headerBuffer, "UPM_RAWBEAMLETS", hdr->upm_rawbeamlets);
	returnVal += writeInt_DADA(headerBuffer, "UPM_UPPERBEAMLET", hdr->upm_upperbeam);
	returnVal += writeInt_DADA(headerBuffer, "UPM_LOWERBEAMLET", hdr->upm_lowerbeam);

	// Lovely chicken and the egg problem...
	returnVal += writeLong_DADA(headerBuffer, "HDR_SIZE", (long) strnlen(headerBuffer, headerLength));
	// ascii_header_set should overwrite values if they already exist, and the length -shouldn't- change between iterations given that the value is padded
	returnVal += writeLong_DADA(headerBuffer, "HDR_SIZE", (long) strnlen(headerBuffer, headerLength));

	if (returnVal < 0) {
		fprintf(stderr, "ERROR: %d errors occurred while preparing the header, exiting.\n", -1 * returnVal);
		return -1;
	}
	return strnlen(headerBuffer, headerLength);
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
	VERBOSE(printf("DADA HEADER %s: %s, %s\n", __func__, key, value));

	if (!isEmpty(value)) {
		return (ascii_header_set(header, key, "%s", value) > -1) ? 0 : -1;
	} else {
		VERBOSE(fprintf(stderr, "ERROR %s: %s UNSET\n", __func__, key));
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
int writeInt_DADA(char *header, const char *key, int value) {
	VERBOSE(printf("DADA HEADER %s: %s, %d\n", __func__, key, value));

	if (!intNotSet(value)) {
		return (ascii_header_set(header, key, "%d", value) > -1) ? 0 : -1;
	} else {
		VERBOSE(fprintf(stderr, "ERROR %s: %s UNSET\n", __func__, key));
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
int writeLong_DADA(char *header, const char *key, long value) {
	VERBOSE(printf("DADA HEADER %s: %s, %ld\n", __func__, key, value));

	if (!longNotSet(value)) {
		return (ascii_header_set(header, key, "%ld", value) > -1) ? 0 : -1;
	} else {
		VERBOSE(fprintf(stderr, "ERROR %s: %s UNSET\n", __func__, key));
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
__attribute__((unused)) int writeFloat_DADA(char *header, const char *key, float value, int exception)  {
	VERBOSE(printf("DADA HEADER %s: %s, %f\n", __func__, key, value));

	if (!doubleNotSet(value, exception)) {
		return ascii_header_set(header, key, "%.12f", value) > -1 ? 0 : -1;
	} else {
		VERBOSE(fprintf(stderr, "ERROR %s: %s UNSET\n", __func__, key));
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
int writeDouble_DADA(char *header, const char *key, double value, int exception) {
	VERBOSE(printf("DADA HEADER %s: %s, %lf\n", __func__, key, value));

	if (!doubleNotSet(value, exception)) {
		return ascii_header_set(header, key, "%.17lf", value) > -1 ? 0 : -1;
	} else {
		VERBOSE(fprintf(stderr, "ERROR %s: %s UNSET\n", __func__, key));
	}
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