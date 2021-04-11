#include "dada_hdr_manager.h"


// Default for metadata struct
const lofar_udp_metadata lofar_udp_metadata_default{
	.dadaHeader = "",
	.hdr_version = 1.0,

	.instrument = "",
	.telescope = "ILT",
	.reciever = "LOFAR-RSP",
	.primary = "",
	.hostname = "",
	.file_name = "",
	.file_number = -1,


	.source = "",
	.ra = "",
	.dec = "",
	.obs_id = "",
	.utc_start = "",
	.mjd_start = -1.0,
	.obs_offset = -1,
	.obs_overlap = -1,
	.basis = "LINEAR",
	.mode = "",


	.freq = -1.0,
	.bw = -1.0,
	.nchan = -1,
	.npol = -1,
	.nbit = -1,
	.resolution = -1,
	.ndim = -1,
	.tsamp = -1.0,
	.state = "",

	// UPM Extra values
	.upm_version = UPM_VERSION,
	.rec_version = "",
	.upm_proc = "",
	.upm_reader = -1,
	.upm_mode = -1,
	.upm_replay = -1,
	.upm_calibrated = -1,

	.upm_bitmode = -1,
	.upm_rawbeamlets = -1,
	.upm_upperbeam = -1,
	.upm_lowerbeam = -1,
	.upm_clockbit = -1,
	.upm_stn = -1

};


/**
 * @brief      { function_description }
 *
 * @param      output      The output
 * @param      header      The header
 * @param[in]  numHeaders  The number headers
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_dada_parse_multiple_headers(lofar_udp_metadata *output, char header[MAX_NUM_PORTS][], int numHeaders) {

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

	if (readCurrentHeader) {
		if (lofar_udp_dada_parse_header(hdr, hdr->header) < 0) { return -1; }
	}
	// Reset the header
	hdr->dadaHeader = "";

	// If we are passed a reader, provide some default values
	if (reader != NULL) {
		// Allow the instrument to be set manually (since the tempo codes don't normally match the staiton names)
		if (isEmpty(hdr->instrument)) {
			if (lofar_get_station_name(reader->stationID, hdr->instrument) < 0) {
				fprintf(stderr, "ERROR: Unable to find station code for station %d, exiting.\n",);
				return -1;
			}
		}


		if (isEmpty(hdr->primary)) {
			if (gethostname(hdr->primary, 64) < 0) { fprintf(stderr, "ERROR: gethostname failed, errno %d (%s), exiting.\n", errno, strerror(errno)); }
		}
		if (isEmpty(hdr->hostname)) {
			if (gethostname(hdr->hostname, 64) < 0) { fprintf(stderr, "ERROR: gethostname failed, errno %d (%s), exiting.\n", errno, strerror(errno)); }
		}

		if (doubleNotSet(hdr->tsamp)) {
			hdr->tsamp =
				((reader->meta->clockBit ? 6e-6 : 5.12e-6) * (reader->meta->processingMode % 4000) > 99 ? 2 * *(reader->meta->processingMode % 10) : 1);
		}


		if (isEmpty(hdr->utc_start)) {
			if (longNotSet(hdr->obs_offset)) {
				// Modulo by 2 as we have a fraction of data per-second, so we can't calculate the byte offset without rounding down to the nearest 2 seconds
				double packetTime = lofar_get_packet_time(reader->meta->inputData[0]);
				// Really avodiing trying to link math.h here....
				int timeRemainder = (int) ((long) packetTime) % 2;
				// (reader->meta->outputBitMode / 8) * totalProcBeamlets 	-- bytes per sample
				// (packetTime - ((long) packetTime) + timeRemainder) 		-- offset time
				hdr->utc_offset = (reader->meta->outputBitMode / 8) * totalProcBeamlets * ((packetTime - ((long) packetTime) + timeRemainder) / hdr->tsamp);
				getStartTimeString(reader, hdr->utc_start, -1 * timeRemainder);
			} else {
				getStartTimeString(reader, hdr->utc_start);
			}
		}
		if (doubleNotSet(hdr->mjd_start)) { hdr->mjd_start = lofar_get_packet_time_mjd(reader->meta->inputData[0]); }


		if (doubleNetSet(hdr->bw)) { hdr->bw = (reader->meta->clockBit ? 0.156250 : 0.1953125) * reader->meta->totalProcBeamlets; }
		if (intNotSet(hdr->nchan)) { hdr->nchan = reader->meta->totalProcBeamlets; }
		if (intNotSet(hdr->npol)) { hdr->npol = 2; }
		if (intnotSet(hdr->ndim)) { hdr->ndim = ((reader->meta->processingMode % 4000) > 99 ? 1 : 2); }
		if (intNotSet(hdr->nbit)) { hdr->nbit = reader->meta->outputBitMode; }
		if (intNotSet(hdr->resolution)) { hdr->resolution = reader->meta->outputBitMode / 8; }

		if (isEmpty(hdr->state)) {
			if (reader->meta->processingMode > 99) {
				hdr->state = "INTENSITY";
			} else {
				hdr->state = "NYQUIST";
			}
		}

		if (intNotSet(hdr->upm_reader)) { hdr->upm_reader = reader->readerType; }
		if (intNotSet(hdr->upm_mode)) { hdr->upm_mode = reader->meta->processingMode; }
		if (intNotSet(hdr->upm_replay)) { hdr->upm_replay = reader->meta->replayDroppedPackets; }
		if (intNotSet(hdr->upm_calibrated)) { hdr->upm_calibrated = reader->meta->calibrateData; }
		if (intNotSet(hdr->upm_bitmode)) { hdr->upm_bitmode = reader->meta->inputBitMode; }
		if (intNotSet(hdr->upm_rawbeamlets)) { hdr->upm_rawbeamlets = reader->meta->totalRawBeamlets; }
		if (intNotSet(hdr->upm_upperbeam)) { hdr->upm_upperbeam = reader->meta->upperBeamlets[reader->meta->numPorts - 1]; }
		if (intNotSet(hdr->upm_lowerbeam)) { hdr->upm_lowerbeam = reader->meta->lowerBeamlets[0]; }
		if (intNotSet(hdr->upm_clockbit)) { hdr->upm_clockbit = reader->meta->clockBit; }
		if (intNotSet(hdr->upm_stn)) { hdr->upm_stn = reader->meta->stationID; }
		if (isEmpty(hdr->upm_proc)) { sprintf(hdr->upm_proc, "UPM v%s-based", UPM_VERSION); }
	}

	// Populate the standard
	if (lofar_udp_generate_generic_hdr(hdr) < 0) { return -1; }

	if (reader != NULL) {
		if (lofar_udp_generate_upm_header(hdr, reader) < 0) { return -1; }
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
int lofar_udp_dada_generate_generic_hdr(lofar_udp_metadata *hdr) {
	int returnVal = 0;
	returnVal += wrap_ascii_header_set_double(hdr->header, "HDR_VERSION", hdr->hdr_version);
	returnVal += wrap_ascii_header_set_char(hdr->header, "INSTRUMENT", hdr->instrument);
	returnVal += wrap_ascii_header_set_char(hdr->header, "TELESCOPE", hdr->telescope);
	returnVal += wrap_ascii_header_set_char(hdr->header, "RECIEVER", hdr->reciever);
	returnVal += wrap_ascii_header_set_char(hdr->header, "PRIMARY", hdr->primary);
	returnVal += wrap_ascii_header_set_char(hdr->header, "HOSTNAME", hdr->hostname);
	returnVal += wrap_ascii_header_set_char(hdr->header, "FILE_NAME", hdr->file_name);
	returnVal += wrap_ascii_header_set_int(hdr->header, "FILE_NUMBER", hdr->file_number);

	returnVal += wrap_ascii_header_set_char(hdr->header, "SOURCE", hdr->source);
	returnVal += wrap_ascii_header_set_char(hdr->header, "RA", hdr->ra);
	returnVal += wrap_ascii_header_set_char(hdr->header, "DEC", hdr->dec);
	returnVal += wrap_ascii_header_set_char(hdr->header, "OBS_ID", hdr->obs_id);
	returnVal += wrap_ascii_header_set_char(hdr->header, "UTC_START", hdr->utc_start);
	returnVal += wrap_ascii_header_set_double(hdr->header, "MJD_START", hdr->mjd_start);
	returnVal += wrap_ascii_header_set_long(hdr->header, "UTC_OFFSET", hdr->utc_offset);
	returnVal += wrap_ascii_header_set_long(hdr->header, "OBS_OVERLAP", hdr->obs_overlap);
	returnVal += wrap_ascii_header_set_char(hdr->header, "BASIS", hdr->basis);
	returnVal += wrap_ascii_header_set_char(hdr->header, "MODE", hdr->mode);

	returnVal += wrap_ascii_header_set_double(hdr->header, "FREQ", hdr->freq);
	returnVal += wrap_ascii_header_set_double(hdr->header, "BW", hdr->bw);
	returnVal += wrap_ascii_header_set_int(hdr->header, "NCHAN", hdr->nchan);
	returnVal += wrap_ascii_header_set_int(hdr->header, "NPOL", hdr->npol);
	returnVal += wrap_ascii_header_set_int(hdr->header, "NBIT", hdr->nbit);
	returnVal += wrap_ascii_header_set_int(hdr->header, "RESOLUTION", hdr->resolution);
	returnVal += wrap_ascii_header_set_int(hdr->header, "NDIM", hdr->ndim);
	returnVal += wrap_ascii_header_set_double(hdr->header, "TSAMP", hdr->tsamp);
	returnVal += wrap_ascii_header_set_char(hdr->header, "STATE", hdr->state);

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
int lofar_udp_dada_generate_upm_header(lofar_udp_metadata *hdr, lofar_udp_reader *reader) {
	int returnVal = 0;
	returnVal += wrap_ascii_header_set_char(hdr->header, "UPM_VERSION", hdr->upm_version);
	returnVal += wrap_ascii_header_set_char(hdr->header, "REC_VERSION", hdr->rec_version);
	returnVal += wrap_ascii_header_set_char(hdr->header, "UPM_PROC", hdr->upm_proc);
	returnVal += wrap_ascii_header_set_int(hdr->header, "UPM_READER", hdr->upm_reader);
	returnVal += wrap_ascii_header_set_int(hdr->header, "UPM_MODE", hdr->upm_mode);
	returnVal += wrap_ascii_header_set_int(hdr->header, "UPM_REPLAY", hdr->upm_replay);
	returnVal += wrap_ascii_header_set_int(hdr->header, "UPM_CALIBRATED", hdr->upm_calibrated);
	returnVal += wrap_ascii_header_set_int(hdr->header, "UPM_BITMODE", hdr->upm_bitmode);
	returnVal += wrap_ascii_header_set_int(hdr->header, "UPM_RAWBEAMLETS", hdr->upm_rawbeamlets);
	returnVal += wrap_ascii_header_set_int(hdr->header, "UPM_UPPERBEAMLET", hdr->upm_upperbeam);
	returnVal += wrap_ascii_header_set_int(hdr->header, "UPM_LOWERBEAMLET", hdr->upm_lowerbeam);
	returnVal += wrap_ascii_header_set_int(hdr->header, "UPM_CLOCKBIT", hdr->upm_clockbit);
	returnVal += wrap_ascii_header_set_int(hdr->header, "UPM_STN", hdr->upm_stn);

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
int lofar_udp_dada_parse_header(lofar_udp_metadata *hdr, char header[]) {
	if (isEmpty(header)) { return 0; }

	int returnVal = 0;
	returnVal += wrap_ascii_header_get_double(hdr->header, "HDR_VERSION", &(hdr->hdr_version));
	returnVal += wrap_ascii_header_get_char(hdr->header, "INSTRUMENT", &(hdr->instrument[0]));
	returnVal += wrap_ascii_header_get_char(hdr->header, "TELESCOPE", &(hdr->telescope[0]));
	returnVal += wrap_ascii_header_get_char(hdr->header, "RECIEVER", &(hdr->reciever[0]));
	returnVal += wrap_ascii_header_get_char(hdr->header, "PRIMARY", &(hdr->primary[0]));
	returnVal += wrap_ascii_header_get_char(hdr->header, "HOSTNAME", &(hdr->hostname[0]));
	returnVal += wrap_ascii_header_get_char(hdr->header, "FILE_NAME", &(hdr->file_name[0]));
	returnVal += wrap_ascii_header_get_int(hdr->header, "FILE_NUMBER", &(hdr->file_number));

	returnVal += wrap_ascii_header_get_char(hdr->header, "SOURCE", &(hdr->source[0]));
	returnVal += wrap_ascii_header_get_char(hdr->header, "RA", &(hdr->ra[0]));
	returnVal += wrap_ascii_header_get_char(hdr->header, "DEC", &(hdr->dec[0]));
	returnVal += wrap_ascii_header_get_char(hdr->header, "OBS_ID", &(hdr->obs_id[0]));
	returnVal += wrap_ascii_header_get_char(hdr->header, "UTC_START", &(hdr->utc_start[0]));
	returnVal += wrap_ascii_header_get_double(hdr->header, "MJD_START", &(hdr->mjd_start));
	returnVal += wrap_ascii_header_get_long(hdr->header, "UTC_OFFSET", &(hdr->utc_offset));
	returnVal += wrap_ascii_header_get_long(hdr->header, "OBS_OVERLAP", &(hdr->obs_overlap));
	returnVal += wrap_ascii_header_get_char(hdr->header, "BASIS", &(hdr->basis[0]));
	returnVal += wrap_ascii_header_get_char(hdr->header, "MODE", &(hdr->mode[0]));

	returnVal += wrap_ascii_header_get_double(hdr->header, "FREQ", &(hdr->freq));
	returnVal += wrap_ascii_header_get_double(hdr->header, "BW", &(hdr->bw));
	returnVal += wrap_ascii_header_get_int(hdr->header, "NCHAN", &(hdr->nchan));
	returnVal += wrap_ascii_header_get_int(hdr->header, "NPOL", &(hdr->npol));
	returnVal += wrap_ascii_header_get_int(hdr->header, "NBIT", &(hdr->nbit));
	returnVal += wrap_ascii_header_get_int(hdr->header, "RESOLUTION", &(hdr->resolution));
	returnVal += wrap_ascii_header_get_int(hdr->header, "NDIM", &(hdr->ndim));
	returnVal += wrap_ascii_header_get_double(hdr->header, "TSAMP", &(hdr->tsamp));
	returnVal += wrap_ascii_header_get_char(hdr->header, "STATE", &(hdr->state[0]));

	returnVal += wrap_ascii_header_get_char(hdr->header, "UPM_VERSION", &(hdr->upm_version[0]));
	returnVal += wrap_ascii_header_get_char(hdr->header, "REC_VERSION", &(hdr->rec_version[0]));
	returnVal += wrap_ascii_header_get_char(hdr->header, "UPM_PROC", &(hdr->upm_proc[0]));
	returnVal += wrap_ascii_header_get_int(hdr->header, "UPM_READER", &(hdr->upm_reader));
	returnVal += wrap_ascii_header_get_int(hdr->header, "UPM_MODE", &(hdr->upm_mode));
	returnVal += wrap_ascii_header_get_int(hdr->header, "UPM_REPLAY", &(hdr->upm_replay));
	returnVal += wrap_ascii_header_get_int(hdr->header, "UPM_CALIBRATED", &(hdr->upm_calibrated));
	returnVal += wrap_ascii_header_get_int(hdr->header, "UPM_BITMODE", &(hdr->upm_bitmode));
	returnVal += wrap_ascii_header_get_int(hdr->header, "UPM_RAWBEAMLETS", &(hdr->upm_rawbeamlets));
	returnVal += wrap_ascii_header_get_int(hdr->header, "UPM_UPPERBEAMLET", &(hdr->upm_upperbeam));
	returnVal += wrap_ascii_header_get_int(hdr->header, "UPM_LOWERBEAMLET", &(hdr->upm_lowerbeam));
	returnVal += wrap_ascii_header_get_int(hdr->header, "UPM_CLOCKBIT", &(hdr->upm_clockbit));
	returnVal += wrap_ascii_header_get_int(hdr->header, "UPM_STN", &(hdr->upm_stn));

	return returnVal;
}


int isEmpty(char *string) { return hdr->instrument[0] == '\0'; }

int intNotSet(int input) { return input == -1; }

int longNotSet(long input) { return input == -1; }


int doubleNotSet(double input) { return input == -1.0; }

// Only set/get values that have been modified ascii_header_* can return 0 or 1
// on success, wrap this so that we always return 0 on success
//
// @param      header  The header
// @param[in]  key     The key
// @param[in]  value   The value
//
// @return     { description_of_the_return_value }
//
int wrap_ascii_header_set_char(char header[], const char key[], const char value[]) {
	if (!isEmpty(value)) { return (ascii_header_set(header, key, "%s", value) > -1) ? 0 : -1; }
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
int wrap_ascii_header_set_int(char header[], const char key[], const int value) {
	if (!intNotSet(value)) { return (ascii_header_set(header, key, "%d", value) > -1) ? 0 : -1; }
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
int wrap_ascii_header_set_long(char header[], const char key[], const long value) {
	if (!longNotSet(value)) { return (ascii_header_set(header, key, "%ld", value) > -1) ? 0 : -1; }
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
int wrap_ascii_header_set_double(char header[], const char key[], const double value) {
	if (!doubleNotSet(value)) { return (ascii_header_set(header, key, "%lf", value); > -1) ? 0 : -1 }
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
int wrap_ascii_header_get_char(char header[], const char key[], const char *value) { return (ascii_header_get(header, key, "%s", value) > -1) ? 0 : -1; }

/**
 * @brief      { function_description }
 *
 * @param      header  The header
 * @param[in]  key     The key
 * @param[in]  value   The value
 *
 * @return     { description_of_the_return_value }
 */
int wrap_ascii_header_get_int(char header[], const char key[], const int *value) { return (ascii_header_get(header, key, "%d", value) > -1) ? 0 : -1; }

/**
 * @brief      { function_description }
 *
 * @param      header  The header
 * @param[in]  key     The key
 * @param[in]  value   The value
 *
 * @return     { description_of_the_return_value }
 */
int wrap_ascii_header_get_long(char header[], const char key[], const long *value) { return (ascii_header_get(header, key, "%ld", value) > -1) ? 0 : -1; }

/**
 * @brief      { function_description }
 *
 * @param      header  The header
 * @param[in]  key     The key
 * @param[in]  value   The value
 *
 * @return     { description_of_the_return_value }
 */
int wrap_ascii_header_get_double(char header[], const char key[], const double *value) { return (ascii_header_get(header, key, "%lf", value); > -1) ? 0 : -1 }