
/**
 * @brief      Writes a string value to the buffer, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param      val      The value
 */
int writeStr_GUPPI(char *headerBuffer, const char *key, const char *val) {
	char tmpStr[81];
	// 80 base length - 10 for key / separator - 2 for wrapping ' - length of string
	int parseChars = sprintf(tmpStr, "%-8s= '%s'%-*.s", key, val, 80 - 10 - 2 - (int) strlen(val), " ");
	VERBOSE(printf("GUPPI %s: %s\t %s\n", key, val, tmpStr));
	return (strcat(headerBuffer, tmpStr) != headerBuffer) + (parseChars != 80);
}

/**
 * @brief      Writes an integer value to the buffer, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param[in]  val      The value
 */
int writeInt_GUPPI(char *headerBuffer, char *key, int val) {
	char tmpStr[81];
	char intStr[META_STR_LEN + 2];
	sprintf(intStr, "%d", val);
	int parseChars = sprintf(tmpStr, "%-8s= %-70s", key, intStr);
	VERBOSE(printf("GUPPI %s: %d, %s\t%s\n", key, val, intStr, tmpStr));
	return (strcat(headerBuffer, tmpStr) != headerBuffer) + (parseChars != 80);
}

/**
 * @brief      Writes a long value to the buffer, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param[in]  val      The value
 */
int writeLong_GUPPI(char *headerBuffer, char *key, long val) {
	char tmpStr[81];
	char longStr[META_STR_LEN + 2];
	sprintf(longStr, "%ld", val);
	int parseChars = sprintf(tmpStr, "%-8s= %-70s", key, longStr);
	VERBOSE(printf("GUPPI %s: %ld, %s\t%s\n", key, val, longStr, tmpStr));
	return (strcat(headerBuffer, tmpStr) != headerBuffer) + (parseChars != 80);
}

/**
 * @brief      Writes a float value to the buffer, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param[in]  val      The value
 */
__attribute__((unused)) int writeFloat_GUPPI(char *headerBuffer, char *key, float val) {
	char tmpStr[81];
	char floatStr[META_STR_LEN + 2];
	sprintf(floatStr, "%.12f", val);
	int parseChars = sprintf(tmpStr, "%-8s= %-70s", key, floatStr);
	VERBOSE(printf("GUPPI %s: %f, %s\t%s\n", key, val, floatStr, tmpStr));
	return (strcat(headerBuffer, tmpStr) != headerBuffer) + (parseChars != 80);
}

/**
 * @brief      Writes a double value to the buffer, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param[in]  val      The value
 */
int writeDouble_GUPPI(char *headerBuffer, char *key, double val) {
	char tmpStr[81];
	char doubleStr[META_STR_LEN + 2];
	sprintf(doubleStr, "%.17f", val);
	int parseChars = sprintf(tmpStr, "%-8s= %-70s", key, doubleStr);
	VERBOSE(printf("GUPPI %s: %lf, %s\t%s\n", key, val, doubleStr, tmpStr));
	return (strcat(headerBuffer, tmpStr) != headerBuffer) + (parseChars != 80);
}

int lofar_udp_metadata_setup_GUPPI(lofar_udp_metadata *metadata) {

	if (metadata->output.guppi == NULL) {
		if ((metadata->output.guppi = calloc(1, sizeof(guppi_hdr))) == NULL) {
			fprintf(stderr, "ERROR: Failed to allocate memory for guppi_hdr struct, exiting.\n");
			return -1;
		}
	}
	*(metadata->output.guppi) = guppi_hdr_default;

	int returnVal = 0;

	// Make these pointers to each other?
	// META_STR_LEN > META_STR_LEN
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
		fprintf(stderr, "ERROR: Failed to copy data to guppi_hdr, exiting.\n");
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
	metadata->output.guppi->stt_smjd = (int) ((metadata->obs_mjd_start - (int) metadata->obs_mjd_start) * 86400);
	metadata->output.guppi->stt_offs = ((metadata->obs_mjd_start - (int) metadata->obs_mjd_start) * 86400) - metadata->output.guppi->stt_smjd;

	metadata->output.guppi->pktidx = 0;

	return 0;
}

int lofar_udp_metadata_update_GUPPI(lofar_udp_metadata *metadata, int newObs) {

	if (metadata == NULL || metadata->output.guppi == NULL) {
		fprintf(stderr, "ERROR %s: Input metadata struct is null, exiting.\n", __func__);
		return -1;
	}

	metadata->output.guppi->blocsize = metadata->upm_blocksize;

	// Should this be processing time, rather than the recording time?
	if (strcpy(metadata->output.guppi->daqpulse, metadata->upm_daq) != metadata->output.guppi->daqpulse) {
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
int lofar_udp_metadata_write_GUPPI(const guppi_hdr *hdr, char *headerBuffer, size_t headerLength) {
	// 35 * 80 + (1x \0) byte entries -> need headerLength to be larger than this or we'll have a bad time.
	if (headerLength <= (ASCII_HDR_MEMBS * 80 + 1)) {
		fprintf(stderr, "ERROR: Passed header buffer is too small (%ld vs minimum of %d), exiting.", headerLength, (ASCII_HDR_MEMBS * 80 + 1));
		return -1;
	}

	// Make sure we start appending at the start of the buffer
	if (headerBuffer[0] != '\0') {
		headerBuffer[0] = '\0';
	}

	int returnVal = 0;

	returnVal += writeStr_GUPPI(headerBuffer, "SRC_NAME", hdr->src_name);
	returnVal += writeStr_GUPPI(headerBuffer, "RA_STR", hdr->ra_str);
	returnVal += writeStr_GUPPI(headerBuffer, "DEC_STR", hdr->dec_str);

	returnVal += writeDouble_GUPPI(headerBuffer, "OBSFREQ", hdr->obsfreq);
	returnVal += writeDouble_GUPPI(headerBuffer, "OBSBW", hdr->obsbw);
	returnVal += writeDouble_GUPPI(headerBuffer, "CHAN_BW", hdr->chan_bw);
	returnVal += writeInt_GUPPI(headerBuffer, "OBSNCHAN", hdr->obsnchan);
	returnVal += writeInt_GUPPI(headerBuffer, "NPOL", hdr->npol);
	returnVal += writeInt_GUPPI(headerBuffer, "NBITS", hdr->nbits);
	returnVal += writeDouble_GUPPI(headerBuffer, "TBIN", hdr->tbin);

	returnVal += writeStr_GUPPI(headerBuffer, "FD_POLN", hdr->fd_poln);
	returnVal += writeStr_GUPPI(headerBuffer, "TRK_MODE", hdr->trk_mode);
	returnVal += writeStr_GUPPI(headerBuffer, "OBS_MODE", hdr->obs_mode);
	returnVal += writeStr_GUPPI(headerBuffer, "CAL_MODE", hdr->cal_mode);
	returnVal += writeDouble_GUPPI(headerBuffer, "SCANLEN", hdr->scanlen);

	returnVal += writeStr_GUPPI(headerBuffer, "PROJID", hdr->projid);
	returnVal += writeStr_GUPPI(headerBuffer, "OBSERVER", hdr->observer);
	returnVal += writeStr_GUPPI(headerBuffer, "TELESCOP", hdr->telescop);
	returnVal += writeStr_GUPPI(headerBuffer, "FRONTEND", hdr->frontend);
	returnVal += writeStr_GUPPI(headerBuffer, "BACKEND", hdr->backend);
	returnVal += writeStr_GUPPI(headerBuffer, "DATAHOST", hdr->datahost);
	returnVal += writeInt_GUPPI(headerBuffer, "DATAPORT", hdr->dataport);
	returnVal += writeInt_GUPPI(headerBuffer, "OVERLAP", hdr->overlap);

	returnVal += writeLong_GUPPI(headerBuffer, "BLOCSIZE", hdr->blocsize);
	returnVal += writeStr_GUPPI(headerBuffer, "DAQPULSE", hdr->daqpulse);

	returnVal += writeInt_GUPPI(headerBuffer, "STT_IMJD", hdr->stt_imjd);
	returnVal += writeInt_GUPPI(headerBuffer, "STT_SMJD", hdr->stt_smjd);
	returnVal += writeDouble_GUPPI(headerBuffer, "STT_OFFS", hdr->stt_offs);

	returnVal += writeLong_GUPPI(headerBuffer, "PKTIDX", hdr->pktidx);
	returnVal += writeStr_GUPPI(headerBuffer, "PKTFMT", hdr->pktfmt);
	returnVal += writeInt_GUPPI(headerBuffer, "PKTSIZE", hdr->pktsize);

	returnVal += writeDouble_GUPPI(headerBuffer, "DROPBLK", hdr->dropblk);
	returnVal += writeDouble_GUPPI(headerBuffer, "DROPTOT", hdr->droptot);



	// All headers are terminated with "END" followed by 77 spaces.
	const char end[4] = "END";
	char tmpStr[81];
	int parseChars = sprintf(tmpStr, "%-80s", end);
	returnVal += (strcat(headerBuffer, tmpStr) != headerBuffer) + (parseChars != 80);

	if (returnVal > 0) {
		fprintf(stderr, "ERROR: %d errors occurred while preparing the header, exiting.\n", returnVal);
		return -1;
	}

	return strnlen(headerBuffer, headerLength);
}





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

/**
 * @brief      Parse a header file and populate an ASCII header struct with the
 *             new values
 *
 * @param      inputFile  The input metadata file location
 * @param      header     The ASCII header struct
 *
 * @return     Success (0), Failure (>0), Unknown Flag Parsed (-1)
 */
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

