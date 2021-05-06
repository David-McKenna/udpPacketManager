#include <sys/stat.h>

int parseHdrFile(char inputFile[], ascii_hdr *header);

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
//@formatter:on



/**
 * @brief      Writes a string value to the buffer, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param      val      The value
 */
int writeStr_GUPPI(char *headerBuffer, char *key, char *val) {
	char tmpStr[81];
	// 80 base length - 10 for key / separator - 2 for wrapping ' - length of string
	int parseChars = sprintf(tmpStr, "%-8s= '%s'%-*.s", key, val, 80 - 10 - 2 - (int) strlen(val), " ");
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
	int parseChars = sprintf(tmpStr, "%-8s= %-70.s", key, intStr);
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
	char intStr[META_STR_LEN + 2];
	sprintf(intStr, "%ld", val);
	int parseChars = sprintf(tmpStr, "%-8s= %-70.s", key, intStr);
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
	char intStr[META_STR_LEN + 2];
	sprintf(intStr, "%lf", val);
	int parseChars = sprintf(tmpStr, "%-8s= %-70.s", key, intStr);
	return (strcat(headerBuffer, tmpStr) != headerBuffer) + (parseChars != 80);
}


/**
 * @brief      Parse a header file and populate an ASCII header struct with the
 *             new values
 *
 * @param      inputFile  The input metadata file location
 * @param      header     The ASCII header struct
 *
 * @return     Success (0), Failure (>0), Unknown Flag Parsed (-1)
 */
int parseHdrFile(char inputFile[], ascii_hdr *header) {
	int returnVal = 0;


	// Open the header file, find it's length and load it into memory
	FILE *fileRef = fopen(inputFile, "r");

	size_t fileSize = FILE_file_size(fileRef);

	char *fileData = calloc(fileSize, sizeof(char));
	size_t readlen = fread(fileData, 1, fileSize, fileRef);

	// Sanity check the old vs new length
	if (readlen != fileSize) {
		fprintf(stderr, "Header file size changed during read operation, continuing with caution...\n");
	}

	fclose(fileRef);

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


	// Reset optind as we likely used it in the GUPPI CLI and it needs to be reinitialised
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


int lofar_udp_metadata_setup_GUPPI(lofar_udp_metadata *metadata) {

	if (metadata->output.ascii == NULL) {
		if ((metadata->output.ascii = calloc(1, sizeof(ascii_hdr))) == NULL) {
			fprintf(stderr, "ERROR: Failed to allocate memory for ascii_hdr struct, exiting.\n");
			return -1;
		}
	}

	int returnVal = 0;

	// Make these pointers to each other?
	// META_STR_LEN > META_STR_LEN
	returnVal += (metadata->output.ascii->src_name != strncpy(metadata->output.ascii->src_name, metadata->source, META_STR_LEN));
	returnVal += (metadata->output.ascii->ra_str != strncpy(metadata->output.ascii->ra_str, metadata->ra, META_STR_LEN));
	returnVal += (metadata->output.ascii->dec_str != strncpy(metadata->output.ascii->dec_str, metadata->dec, META_STR_LEN));
	returnVal += (metadata->output.ascii->projid != strncpy(metadata->output.ascii->projid, metadata->obs_id, META_STR_LEN));
	returnVal += (metadata->output.ascii->observer != strncpy(metadata->output.ascii->observer, metadata->observer, META_STR_LEN));
	returnVal += (metadata->output.ascii->telescop != strncpy(metadata->output.ascii->telescop, metadata->telescope, META_STR_LEN));
	returnVal += (metadata->output.ascii->backend != strncpy(metadata->output.ascii->backend, metadata->machine, META_STR_LEN)); // TODO: Is that right?
	returnVal += (metadata->output.ascii->datahost != strncpy(metadata->output.ascii->datahost, metadata->hostname, META_STR_LEN));

	// GUPPI constants
	returnVal += (metadata->output.ascii->fd_poln != strncpy(metadata->output.ascii->fd_poln, "LIN", META_STR_LEN));
	returnVal += (metadata->output.ascii->trk_mode != strncpy(metadata->output.ascii->trk_mode, "TRACK", META_STR_LEN));
	returnVal += (metadata->output.ascii->obs_mode != strncpy(metadata->output.ascii->obs_mode, "RAW", META_STR_LEN));
	returnVal += (metadata->output.ascii->cal_mode != strncpy(metadata->output.ascii->cal_mode, "OFF", META_STR_LEN));
	returnVal += (metadata->output.ascii->pktfmt != strncpy(metadata->output.ascii->pktfmt, "1SFA", META_STR_LEN));
	returnVal += (metadata->output.ascii->frontend != strncpy(metadata->output.ascii->frontend, "LOFAR-RSP", META_STR_LEN));
	returnVal += (metadata->output.ascii->daqpulse != strncpy(metadata->output.ascii->daqpulse, "", META_STR_LEN));

	if (returnVal > 0) {
		fprintf(stderr, "ERROR: Failed to copy data to ascii_hdr, exiting.\n");
		return -1;
	}

	// Copy over normal values
	metadata->output.ascii->obsfreq = metadata->freq;
	metadata->output.ascii->obsbw = metadata->bw;
	metadata->output.ascii->chan_bw = metadata->channel_bw;
	metadata->output.ascii->obsnchan = metadata->nchan;
	metadata->output.ascii->npol = metadata->npol;
	metadata->output.ascii->nbits = metadata->nbit;
	metadata->output.ascii->tbin = metadata->tsamp;
	metadata->output.ascii->dataport = metadata->baseport;
	metadata->output.ascii->pktidx = 0;

	metadata->output.ascii->stt_imjd = 0;
	metadata->output.ascii->stt_smjd = 0;
	metadata->output.ascii->stt_offs = 0;

	return 0;
}

/**
 * @brief      Emulate the DAQ time string from the current data timestamp
 *
 * @param      reader      The lofar_udp_reader
 * @param      stringBuff  The output time string buffer
 */
void getStartTimeStringDAQ(lofar_udp_reader *reader, char stringBuff[24]) {
	double startTime;
	time_t startTimeUnix;
	struct tm *startTimeStruct;

	startTime = lofar_get_packet_time(reader->meta->inputData[0]);
	startTimeUnix = (unsigned int) startTime;
	startTimeStruct = gmtime(&startTimeUnix);

	strftime(stringBuff, 24, "%a %b %e %H:%M:%S %Y", startTimeStruct);
}

int lofar_udp_metadata_update_GUPPI(lofar_udp_reader *reader, lofar_udp_metadata *metadata, int newBlock) {

	metadata->output.ascii->blocsize = reader->meta->packetsPerIteration * reader->meta->packetOutputLength[0];

	// Should this be processing time, rather than the recording time?
	char timeStr[META_STR_LEN];
	getStartTimeStringDAQ(reader, timeStr);
	if (strcpy(metadata->output.ascii->daqpulse, timeStr) != metadata->output.ascii->daqpulse) {
		fprintf(stderr, "ERROR: Failed to copy DAQ string to GUPPI header, exiting.\n");
		return -1;
	}

	int blockDropped = 0;
	for (int port = 0; port < reader->meta->numPorts; port++) {
		if (reader->meta->portLastDroppedPackets[port] != 0) {
			blockDropped += reader->meta->portLastDroppedPackets[port];
		}
	}
	metadata->output.ascii->dropblk = metadata->upm_last_dropped_packets / reader->meta->packetsPerIteration / reader->meta->numPorts;
	metadata->output.ascii->droptot = metadata->upm_dropped_packets / metadata->upm_processed_packets;
	metadata->output.ascii->pktidx = metadata->upm_processed_packets - reader->meta->packetsPerIteration;

	if (newBlock) {
		metadata->output.ascii->stt_imjd = (int) metadata->obs_mjd_start;
		metadata->output.ascii->stt_smjd = (int) ((metadata->obs_mjd_start - (int) metadata->obs_mjd_start) * 86400);
		metadata->output.ascii->stt_offs = ((metadata->obs_mjd_start - (int) metadata->obs_mjd_start) * 86400) - metadata->output.ascii->stt_smjd;
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
int lofar_udp_metadata_write_GUPPI(char *headerBuffer, size_t headerLength, ascii_hdr *headerStruct) {
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

	returnVal += writeStr_GUPPI(headerBuffer, "SRC_NAME", headerStruct->src_name);
	returnVal += writeStr_GUPPI(headerBuffer, "RA_STR", headerStruct->ra_str);
	returnVal += writeStr_GUPPI(headerBuffer, "DEC_STR", headerStruct->dec_str);

	returnVal += writeDouble_GUPPI(headerBuffer, "OBSFREQ", headerStruct->obsfreq);
	returnVal += writeDouble_GUPPI(headerBuffer, "OBSBW", headerStruct->obsbw);
	returnVal += writeDouble_GUPPI(headerBuffer, "CHAN_BW", headerStruct->chan_bw);
	returnVal += writeInt_GUPPI(headerBuffer, "OBSNCHAN", headerStruct->obsnchan);
	returnVal += writeInt_GUPPI(headerBuffer, "NPOL", headerStruct->npol);
	returnVal += writeInt_GUPPI(headerBuffer, "NBITS", headerStruct->nbits);
	returnVal += writeDouble_GUPPI(headerBuffer, "TBIN", headerStruct->tbin);

	returnVal += writeStr_GUPPI(headerBuffer, "FD_POLN", headerStruct->fd_poln);
	returnVal += writeStr_GUPPI(headerBuffer, "TRK_MODE", headerStruct->trk_mode);
	returnVal += writeStr_GUPPI(headerBuffer, "OBS_MODE", headerStruct->obs_mode);
	returnVal += writeStr_GUPPI(headerBuffer, "CAL_MODE", headerStruct->cal_mode);
	returnVal += writeDouble_GUPPI(headerBuffer, "SCANLEN", headerStruct->scanlen);

	returnVal += writeStr_GUPPI(headerBuffer, "PROJID", headerStruct->projid);
	returnVal += writeStr_GUPPI(headerBuffer, "OBSERVER", headerStruct->observer);
	returnVal += writeStr_GUPPI(headerBuffer, "TELESCOP", headerStruct->telescop);
	returnVal += writeStr_GUPPI(headerBuffer, "FRONTEND", headerStruct->frontend);
	returnVal += writeStr_GUPPI(headerBuffer, "BACKEND", headerStruct->backend);
	returnVal += writeStr_GUPPI(headerBuffer, "DATAHOST", headerStruct->datahost);
	returnVal += writeInt_GUPPI(headerBuffer, "DATAPORT", headerStruct->dataport);
	returnVal += writeInt_GUPPI(headerBuffer, "OVERLAP", headerStruct->overlap);

	returnVal += writeLong_GUPPI(headerBuffer, "BLOCSIZE", headerStruct->blocsize);
	returnVal += writeStr_GUPPI(headerBuffer, "DAQPULSE", headerStruct->daqpulse);

	returnVal += writeInt_GUPPI(headerBuffer, "STT_IMJD", headerStruct->stt_imjd);
	returnVal += writeInt_GUPPI(headerBuffer, "STT_SMJD", headerStruct->stt_smjd);
	returnVal += writeDouble_GUPPI(headerBuffer, "STT_OFFS", headerStruct->stt_offs);

	returnVal += writeLong_GUPPI(headerBuffer, "PKTIDX", headerStruct->pktidx);
	returnVal += writeStr_GUPPI(headerBuffer, "PKTFMT", headerStruct->pktfmt);
	returnVal += writeInt_GUPPI(headerBuffer, "PKTSIZE", headerStruct->pktsize);

	returnVal += writeDouble_GUPPI(headerBuffer, "DROPBLK", headerStruct->dropblk);
	returnVal += writeDouble_GUPPI(headerBuffer, "DROPTOT", headerStruct->droptot);



	// All headers are terminated with "END" followed by 77 spaces.
	const char end[4] = "END";
	char tmpStr[81];
	int parseChars = sprintf(tmpStr, "%-80s", end);
	returnVal += (strcat(headerBuffer, tmpStr) != headerBuffer) + (parseChars != 80);

	if (returnVal > 0) {
		fprintf(stderr, "ERROR: %d errors occurred while preparing the header, exiting.\n", returnVal);
		return -1;
	}

	return 0;
}

