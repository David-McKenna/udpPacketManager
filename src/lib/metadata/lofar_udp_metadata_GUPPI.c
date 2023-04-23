int _lofar_udp_metadata_setup_GUPPI(lofar_udp_metadata *metadata) {

	metadata->output.guppi = guppi_hdr_alloc();
	CHECK_ALLOC_NOCLEAN(metadata->output.guppi, -1);

	int returnVal = 0;

	// Make these pointers to each other?
	returnVal += (metadata->output.guppi->src_name != strncpy(metadata->output.guppi->src_name, metadata->source, META_STR_LEN));
	returnVal += (metadata->output.guppi->ra_str != strncpy(metadata->output.guppi->ra_str, metadata->ra, META_STR_LEN));
	returnVal += (metadata->output.guppi->dec_str != strncpy(metadata->output.guppi->dec_str, metadata->dec, META_STR_LEN));
	returnVal += (metadata->output.guppi->projid != strncpy(metadata->output.guppi->projid, metadata->obs_id, META_STR_LEN));
	returnVal += (metadata->output.guppi->observer != strncpy(metadata->output.guppi->observer, metadata->observer, META_STR_LEN));
	returnVal += (metadata->output.guppi->telescop != strncpy(metadata->output.guppi->telescop, metadata->telescope, META_STR_LEN));
	returnVal += (metadata->output.guppi->backend != strncpy(metadata->output.guppi->backend, metadata->receiver, META_STR_LEN));
	returnVal += (metadata->output.guppi->datahost != strncpy(metadata->output.guppi->datahost, metadata->hostname, META_STR_LEN));

	// GUPPI constants
	returnVal += (metadata->output.guppi->fd_poln != strncpy(metadata->output.guppi->fd_poln, "LIN", META_STR_LEN));
	returnVal += (metadata->output.guppi->trk_mode != strncpy(metadata->output.guppi->trk_mode, "TRACK", META_STR_LEN));
	returnVal += (metadata->output.guppi->obs_mode != strncpy(metadata->output.guppi->obs_mode, "RAW", META_STR_LEN));
	returnVal += (metadata->output.guppi->cal_mode != strncpy(metadata->output.guppi->cal_mode, "OFF", META_STR_LEN));
	returnVal += (metadata->output.guppi->pktfmt != strncpy(metadata->output.guppi->pktfmt, "1SFA", META_STR_LEN));
	returnVal += (metadata->output.guppi->frontend != strncpy(metadata->output.guppi->frontend, "LOFAR-RSP", META_STR_LEN));
	returnVal += (metadata->output.guppi->daqpulse != strncpy(metadata->output.guppi->daqpulse, "", META_STR_LEN));

	if (returnVal > 0) {
		fprintf(stderr, "ERROR: Failed to copy str data to guppi_hdr, exiting.\n");
		return -1;
	}

	// Copy over normal values
	metadata->output.guppi->obsfreq = metadata->freq;
	metadata->output.guppi->obsbw = metadata->bw;
	metadata->output.guppi->chan_bw = metadata->channel_bw;
	metadata->output.guppi->obsnchan = metadata->nchan;
	metadata->output.guppi->npol = metadata->npol;
	metadata->output.guppi->nbits = metadata->nbit > 0 ? metadata->nbit : -1 * metadata->nbit;
	metadata->output.guppi->tbin = metadata->tsamp;
	metadata->output.guppi->dataport = metadata->baseport;
	metadata->output.guppi->stt_imjd = (int) metadata->obs_mjd_start;
	// TODO: Leap seconds
	metadata->output.guppi->stt_smjd = (int) ((metadata->obs_mjd_start - (int) metadata->obs_mjd_start) * 86400);
	metadata->output.guppi->stt_offs = ((metadata->obs_mjd_start - (int) metadata->obs_mjd_start) * 86400) - metadata->output.guppi->stt_smjd;

	metadata->output.guppi->pktidx = 0;

	return 0;
}

int _lofar_udp_metadata_update_GUPPI(lofar_udp_metadata *metadata, int newObs) {

	if (metadata == NULL || metadata->output.guppi == NULL) {
		fprintf(stderr, "ERROR %s: Input metadata struct is null, exiting.\n", __func__);
		return -1;
	}

	if (metadata->upm_pack_per_iter == 0 || metadata->upm_num_inputs == 0 || metadata->upm_processed_packets == 0) {
		fprintf(stderr, "ERROR %s: Input data is badly initialised and would cause division by 0 (pack_per_iter: %ld, num_inputs: %d, processed_packets: %ld), exiting.\n", __func__, metadata->upm_pack_per_iter, metadata->upm_num_inputs, metadata->upm_processed_packets);
		return -1;
	}

	metadata->output.guppi->blocsize = metadata->upm_blocksize;

	// Should this be processing time, rather than the recording time?
	if (strncpy(metadata->output.guppi->daqpulse, metadata->upm_daq, META_STR_LEN) != metadata->output.guppi->daqpulse) {
		fprintf(stderr, "ERROR: Failed to copy DAQ string to GUPPI header, exiting.\n");
		return -1;
	}

	metadata->output.guppi->dropblk = (float) metadata->upm_last_dropped_packets / (float) metadata->upm_pack_per_iter / (float) metadata->upm_num_inputs;
	metadata->output.guppi->droptot = metadata->upm_dropped_packets / metadata->upm_processed_packets;
	metadata->output.guppi->pktidx = (metadata->upm_processed_packets / metadata->upm_num_inputs) - metadata->upm_pack_per_iter;

	if (newObs) {
		metadata->output.guppi->stt_imjd = (int) metadata->obs_mjd_start;
		metadata->output.guppi->stt_smjd = (int) ((metadata->obs_mjd_start - (int) metadata->obs_mjd_start) * 86400);
		metadata->output.guppi->stt_offs = ((metadata->obs_mjd_start - (int) metadata->obs_mjd_start) * 86400) - metadata->output.guppi->stt_smjd;
		metadata->output.guppi->pktidx = 0;
	}

	return 0;
}

/**
 * @brief      Writes an ASCII header to disk based on the current values of the
 *             struct
 *
 * @param      fileRef  The file reference
 * @param      header   The ASCII header struct
 */
int64_t _lofar_udp_metadata_write_GUPPI(const guppi_hdr *hdr, int8_t *const headerBuffer, int64_t headerLength) {
	if (headerBuffer == NULL || hdr == NULL) {
		fprintf(stderr, "ERROR: Null buffer provided to %s, exiting.\n", __func__);
	}

	headerLength /= sizeof(char) / sizeof(int8_t);

	// 35 * 80 + (1x \0) byte entries -> need headerLength to be larger than this or we'll have a bad time.
	if (headerLength <= (ASCII_HDR_MEMBS * 80 + 1)) {
		fprintf(stderr, "ERROR: Passed header buffer is too small (%ld vs minimum of %d), exiting.\n", headerLength, (ASCII_HDR_MEMBS * 80 + 1));
		return -1;
	}

	// Make sure we start appending at the start of the buffer
	if (headerBuffer[0] != '\0') {
		headerBuffer[0] = '\0';
	}

	char *workingBuffer = (void*) headerBuffer;
	workingBuffer = _writeStr_GUPPI(workingBuffer, "SRC_NAME", hdr->src_name);
	workingBuffer = _writeStr_GUPPI(workingBuffer, "RA_STR", hdr->ra_str);
	workingBuffer = _writeStr_GUPPI(workingBuffer, "DEC_STR", hdr->dec_str);

	workingBuffer = _writeDouble_GUPPI(workingBuffer, "OBSFREQ", hdr->obsfreq);
	workingBuffer = _writeDouble_GUPPI(workingBuffer, "OBSBW", hdr->obsbw);
	workingBuffer = _writeDouble_GUPPI(workingBuffer, "CHAN_BW", hdr->chan_bw);
	workingBuffer = _writeInt_GUPPI(workingBuffer, "OBSNCHAN", hdr->obsnchan);
	workingBuffer = _writeInt_GUPPI(workingBuffer, "NPOL", hdr->npol);
	workingBuffer = _writeInt_GUPPI(workingBuffer, "NBITS", hdr->nbits);
	workingBuffer = _writeDouble_GUPPI(workingBuffer, "TBIN", hdr->tbin);

	workingBuffer = _writeStr_GUPPI(workingBuffer, "FD_POLN", hdr->fd_poln);
	workingBuffer = _writeStr_GUPPI(workingBuffer, "TRK_MODE", hdr->trk_mode);
	workingBuffer = _writeStr_GUPPI(workingBuffer, "OBS_MODE", hdr->obs_mode);
	workingBuffer = _writeStr_GUPPI(workingBuffer, "CAL_MODE", hdr->cal_mode);
	workingBuffer = _writeDouble_GUPPI(workingBuffer, "SCANLEN", hdr->scanlen);

	workingBuffer = _writeStr_GUPPI(workingBuffer, "PROJID", hdr->projid);
	workingBuffer = _writeStr_GUPPI(workingBuffer, "OBSERVER", hdr->observer);
	workingBuffer = _writeStr_GUPPI(workingBuffer, "TELESCOP", hdr->telescop);
	workingBuffer = _writeStr_GUPPI(workingBuffer, "FRONTEND", hdr->frontend);
	workingBuffer = _writeStr_GUPPI(workingBuffer, "BACKEND", hdr->backend);
	workingBuffer = _writeStr_GUPPI(workingBuffer, "DATAHOST", hdr->datahost);
	workingBuffer = _writeInt_GUPPI(workingBuffer, "DATAPORT", hdr->dataport);
	workingBuffer = _writeInt_GUPPI(workingBuffer, "OVERLAP", hdr->overlap);

	workingBuffer = _writeLong_GUPPI(workingBuffer, "BLOCSIZE", hdr->blocsize);
	workingBuffer = _writeStr_GUPPI(workingBuffer, "DAQPULSE", hdr->daqpulse);

	workingBuffer = _writeInt_GUPPI(workingBuffer, "STT_IMJD", hdr->stt_imjd);
	workingBuffer = _writeInt_GUPPI(workingBuffer, "STT_SMJD", hdr->stt_smjd);
	workingBuffer = _writeDouble_GUPPI(workingBuffer, "STT_OFFS", hdr->stt_offs);

	workingBuffer = _writeLong_GUPPI(workingBuffer, "PKTIDX", hdr->pktidx);
	workingBuffer = _writeStr_GUPPI(workingBuffer, "PKTFMT", hdr->pktfmt);
	workingBuffer = _writeInt_GUPPI(workingBuffer, "PKTSIZE", hdr->pktsize);

	workingBuffer = _writeDouble_GUPPI(workingBuffer, "DROPBLK", hdr->dropblk);
	workingBuffer = _writeDouble_GUPPI(workingBuffer, "DROPTOT", hdr->droptot);

	if (workingBuffer == NULL) {
		fprintf(stderr, "ERROR: Failed to build GUPPI header, exiting.\n");
		return -1;
	}


	// All headers are terminated with "END" followed by 77 spaces.
	const char end[4] = "END";
	int parsedChars = snprintf(workingBuffer, headerLength - (workingBuffer - (char*) headerBuffer), "%-80s", end);

	if (parsedChars != 80) {
		fprintf(stderr, "ERROR: Failed to append end to GUPPI header (parsed %d chars), exiting.\n", parsedChars);
		return -1;
	}

	return strnlen((char*) headerBuffer, headerLength);
}



/**
 * @brief      Writes a string value to the buffer, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param      val      The value
 */
const int32_t outputLength = 81;
char* _writeStr_GUPPI(char *headerBuffer, const char *key, const char *val) {
	if (headerBuffer == NULL) {
		return NULL;
	}

	// 80 base length - 10 for key / separator - 2 for wrapping ' - length of string
	int32_t parseChars = snprintf(headerBuffer, outputLength, "%-8s= '%s'%-*.s", key, val, 80 - 10 - 2 - (int) strnlen(val, META_STR_LEN), " ");
	VERBOSE(printf("GUPPI %s: %s\t %s\n", key, val, headerBuffer));

	if (parseChars != (outputLength - 1)) {
		fprintf(stderr, "ERROR: Failed to build GUPPI header for key/value pair %s:%s (%d), exiting.\n", key, val, parseChars);
		return NULL;
	}

	return headerBuffer + parseChars;
}

/**
 * @brief      Writes an integer value to the buffer, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param[in]  val      The value
 */
char* _writeInt_GUPPI(char *headerBuffer, const char *key, int32_t val) {
	if (headerBuffer == NULL) {
		return NULL;
	}

	char intStr[META_STR_LEN + 2];
	if (snprintf(intStr, META_STR_LEN, "%d", val) < 0) {
		fprintf(stderr, "ERROR: Failed to stringify int %d for GUPPI header key %s, returning.\n", val, key);
		return NULL;
	}
	int32_t parseChars = snprintf(headerBuffer, outputLength, "%-8s= %-70s", key, intStr);
	VERBOSE(printf("GUPPI %s: %d, %s\t%s\n", key, val, intStr, headerBuffer));

	if (parseChars != (outputLength - 1)) {
		fprintf(stderr, "ERROR: Failed to build GUPPI header for key/value pair %s:%d (%d), exiting.\n", key, val, parseChars);
		return NULL;
	}

	return headerBuffer + parseChars;
}

/**
 * @brief      Writes a long value to the buffer, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param[in]  val      The value
 */
char* _writeLong_GUPPI(char *headerBuffer, const char *key, int64_t val) {
	if (headerBuffer == NULL) {
		return NULL;
	}

	char longStr[META_STR_LEN + 2];
	if (snprintf(longStr, META_STR_LEN, "%ld", val) < 0) {
		fprintf(stderr, "ERROR: Failed to stringify long %ld for GUPPI header key %s, returning.\n", val, key);
		return NULL;
	}

	int32_t parseChars = snprintf(headerBuffer, outputLength, "%-8s= %-70s", key, longStr);
	VERBOSE(printf("GUPPI %s: %ld, %s\t%s\n", key, val, longStr, headerBuffer));

	if (parseChars != (outputLength - 1)) {
		fprintf(stderr, "ERROR: Failed to build GUPPI header for key/value pair %s:%ld (%d), exiting.\n", key, val, parseChars);
		return NULL;
	}

	return headerBuffer + parseChars;
}

/**
 * @brief      Writes a float value to the buffer, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param[in]  val      The value
 */
__attribute__((unused)) char* _writeFloat_GUPPI(char *headerBuffer, const char *key, float val) {
	if (headerBuffer == NULL) {
		return NULL;
	}

	char floatStr[META_STR_LEN + 2];
	if (snprintf(floatStr, META_STR_LEN, "%.12f", val) < 0) {
		fprintf(stderr, "ERROR: Failed to stringify float %f for GUPPI header key %s, returning.\n", val, key);
		return NULL;
	}

	int32_t parseChars = snprintf(headerBuffer, outputLength, "%-8s= %-70s", key, floatStr);
	VERBOSE(printf("GUPPI %s: %f, %s\t%s\n", key, val, floatStr, headerBuffer));

	if (parseChars != (outputLength - 1)) {
		fprintf(stderr, "ERROR: Failed to build GUPPI header for key/value pair %s:%f (%d), exiting.\n", key, val, parseChars);
		return NULL;
	}

	return headerBuffer + parseChars;
}


/**
 * @brief      Writes a double value to the buffer, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param[in]  val      The value
 */
char* _writeDouble_GUPPI(char *headerBuffer, const char *key, double val) {
	if (headerBuffer == NULL) {
		return NULL;
	}

	char doubleStr[META_STR_LEN + 2];
	if (snprintf(doubleStr, META_STR_LEN, "%.17f", val) < 0) {
		fprintf(stderr, "ERROR: Failed to stringify double %lf for GUPPI header key %s, returning.\n", val, key);
		return NULL;
	}

	int32_t parseChars = snprintf(headerBuffer, outputLength, "%-8s= %-70s", key, doubleStr);
	VERBOSE(printf("GUPPI %s: %lf, %s\t%s\n", key, val, doubleStr, headerBuffer));

	if (parseChars != (outputLength - 1)) {
		fprintf(stderr, "ERROR: Failed to build GUPPI header for key/value pair %s:%lf (%d), exiting.\n", key, val, parseChars);
		return NULL;
	}

	return headerBuffer + parseChars;
}



/*
// Old GUPPI parser / formatter using getopt_long file as an input

// Setup the getopt_long keys + short keys
// https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
//@formatter:off
static struct option long_options[] = {
	{ "src_name", required_argument, NULL, 'a' },
	{ "ra_str",   required_argument, NULL, 'b' },
	{ "dec_str",  required_argument, NULL, 'c' },
	{ "obsfreq",  required_argument, NULL, 'd' },
	{ "obsbw",    required_argument, NULL, 'e' },
	{ "chan_bw",  required_argument, NULL, 'f' },
	{ "obsnchan", required_argument, NULL, 'g' },
	{ "npol",     required_argument, NULL, 'h' },
	{ "nbits",    required_argument, NULL, 'i' },
	{ "tbin",     required_argument, NULL, 'j' },
	{ "fd_poln",  required_argument, NULL, 'k' },
	{ "trk_mode", required_argument, NULL, 'l' },
	{ "obs_mode", required_argument, NULL, 'm' },
	{ "cal_mode", required_argument, NULL, 'n' },
	{ "scanlen",  required_argument, NULL, 'o' },
	{ "projid",   required_argument, NULL, 'p' },
	{ "observer", required_argument, NULL, 'q' },
	{ "telescop", required_argument, NULL, 'r' },
	{ "frontend", required_argument, NULL, 's' },
	{ "backend",  required_argument, NULL, 't' },
	{ "datahost", required_argument, NULL, 'u' },
	{ "dataport", required_argument, NULL, 'v' },
	{ "overlap",  required_argument, NULL, 'w' },
	{ "blocsize", required_argument, NULL, 'x' },
	{ "daqpulse", required_argument, NULL, 'y' },
	{ "stt_imjd", required_argument, NULL, 'z' },
	{ "stt_smjd", required_argument, NULL, 'A' },
	{ "pktidx",   required_argument, NULL, 'B' },
	{ "pktfmt",   required_argument, NULL, 'C' },
	{ "stt_offs", required_argument, NULL, 'D' },
	{ "pktsize",  required_argument, NULL, 'E' },
	{ "dropblk",  required_argument, NULL, 'F' },
	{ "droptot",  required_argument, NULL, 'G' },
	{ 0, 0, NULL, 0 }
};


 * @brief      Parse a header file and populate an ASCII header struct with the
 *             new values
 *
 * @param      inputFile  The input metadata file location
 * @param      header     The ASCII header struct
 *
 * @return     Success (0), Failure (>0), Unknown Flag Parsed (-1)

  __attribute__((unused)) int lofar_udp_metdata_GUPPI_configure_from_file(const char *inputFile, guppi_hdr *header) {
	int returnVal = 0;

	// Open the header file, find it's length and load it into memory
	FILE *fileRef = fopen(inputFile, "r");
	size_t fileSize = FILE_file_size(fileRef);
	char *fileData = calloc(fileSize, sizeof(char));
	size_t readlen = fread(fileData, 1, fileSize, fileRef);
	fclose(fileRef);

	// Sanity check the old vs new length
	if (readlen != fileSize) {
		fprintf(stderr, "Header file size changed during read operation, continuing with caution...\n");
	}

	VERBOSE(printf("%s\n", fileData));

	// Overall method modified from https://stackoverflow.com/posts/39841506/
	char *fargv[4 * ASCII_HDR_MEMBS + 1] = { "" };
	int fargc = 1;

	// Break up the input file into different words, store them in fargv
	fargv[fargc] = strtok(fileData, " \n\r");
	VERBOSE(printf("%d: %s\n", fargc, fargv[fargc]));

	while (fargc < (4 * ASCII_HDR_MEMBS + 1) && fargv[fargc] != 0) {
		fargc++;
		fargv[fargc] = strtok(0, " \n\r");
		VERBOSE(printf("%d: %s\n", fargc, fargv[fargc]));
	}


	// Reset optind as we likely previously used it, so it needs to be re-initialised to prevent reading off a non-1 index
	// optind = 0 -> calling name, or "" for us as the above array is padded
	optind = 1;
	// Iterate over each work and update the header as needed.
	int optIdx = 0;
	int charVal;
	while ((charVal = getopt_long(fargc, &(fargv[0]), "+", long_options, &optIdx)) != -1) {
		VERBOSE(printf("%c= %s \n", charVal, optarg));
		switch (charVal) {
			case 'a':
				strncpy(header->src_name, optarg, META_STR_LEN);
				break;
			case 'b':
				strncpy(header->ra_str, optarg, META_STR_LEN);
				break;

			case 'c':
				strncpy(header->dec_str, optarg, META_STR_LEN);
				break;

			case 'd':
				header->obsfreq = atof(optarg);
				break;

			case 'e':
				header->obsbw = atof(optarg);
				break;

			case 'f':
				header->chan_bw = atof(optarg);
				break;

			case 'g':
				header->obsnchan = atoi(optarg);
				break;

			case 'h':
				header->npol = atoi(optarg);
				break;

			case 'i':
				header->nbits = atoi(optarg);
				break;

			case 'j':
				header->tbin = atof(optarg);
				break;

			case 'k':
				strncpy(header->fd_poln, optarg, META_STR_LEN);
				break;

			case 'l':
				strncpy(header->trk_mode, optarg, META_STR_LEN);
				break;

			case 'm':
				strncpy(header->obs_mode, optarg, META_STR_LEN);
				break;

			case 'n':
				strncpy(header->cal_mode, optarg, META_STR_LEN);
				break;

			case 'o':
				header->scanlen = atof(optarg);
				break;

			case 'p':
				strncpy(header->projid, optarg, META_STR_LEN);
				break;

			case 'q':
				strncpy(header->observer, optarg, META_STR_LEN);
				break;

			case 'r':
				strncpy(header->telescop, optarg, META_STR_LEN);
				break;

			case 's':
				strncpy(header->frontend, optarg, META_STR_LEN);
				break;

			case 't':
				strncpy(header->backend, optarg, META_STR_LEN);
				break;

			case 'u':
				strncpy(header->datahost, optarg, META_STR_LEN);
				break;

			case 'v':
				header->dataport = atoi(optarg);
				break;

			case 'w':
				header->overlap = atoi(optarg);
				break;

			case 'x':
				header->blocsize = atol(optarg);
				break;

			case 'y':
				strncpy(header->daqpulse, optarg, META_STR_LEN);
				break;

			case 'z':
				header->stt_imjd = atoi(optarg);
				break;

			case 'A':
				header->stt_smjd = atoi(optarg);
				break;

			case 'B':
				header->pktidx = atol(optarg);
				break;

			case 'C':
				strncpy(header->pktfmt, optarg, META_STR_LEN);
				break;

			case 'D':
				header->stt_offs = atof(optarg);
				break;

			case 'E':
				header->pktsize = atoi(optarg);
				break;

			case 'F':
				header->dropblk = atof(optarg);
				break;

			case 'G':
				header->droptot = atof(optarg);
				break;

			case '0':
			case '?':
			default:
				fprintf(stderr, "Unknown flag %d (%s), continuing with caution...\n", charVal, optarg);
				returnVal = -1;
				break;
		}
	}

	// Return 0 / -1 depending on the parsed flags
	return returnVal;
}
*/