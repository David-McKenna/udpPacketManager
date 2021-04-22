#include "ascii_hdr_manager.h"


// Default values for the ASCII header
const ascii_hdr ascii_hdr_default = {
	.src_name = "J0000+0000",
	.ra_str = "00:00:00.0000",
	.dec_str = "+00:00:00.0000",
	.obsfreq = 149.90234375,
	.obsbw = 95.3125,
	.chan_bw = 0.1953125,
	.obsnchan = 488,
	.npol = 4,
	.nbits = 8,
	.tbin = 5.12e-6,
	.fd_poln = "LIN",
	.trk_mode = "TRACK",
	.obs_mode = "RAW",
	.cal_mode = "OFF",
	.scanlen = 0.,
	.projid = "UDP2RAW",
	.observer = "Unknown",
	.telescop = "ILT",
	.frontend = "LOFAR-RSP",
    .backend = "LOFAR-UDP",
    .datahost = "0.0.0.0",
    .dataport = 16130,
    .overlap = 0,
    .blocsize = 0,
    .daqpulse = "",
    .stt_imjd = 50000,
    .stt_smjd = 0,
    .pktidx = 0,
    .pktfmt = "1SFA",
    .stt_offs = 0.,
    .pktsize = 0,
    .dropblk = 0.,
    .droptot = 0.
};

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
int writeStr(char *headerBuffer, char key[], char val[]) {
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
int writeInt(char *headerBuffer, char key[], int val) {
	char tmpStr[81];
	char intStr[71];
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
int writeLong(char *headerBuffer, char key[], long val) {
	char tmpStr[81];
	char intStr[71];
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
int writeDouble(char *headerBuffer, char key[], double val) {
	char tmpStr[81];
	char intStr[71];
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
	char *fargv[4 * HDR_MEMBS + 1] = { "" };
	int fargc = 1;

	// Break up the input file into different words, store them in fargv
	fargv[fargc] = strtok(fileData, " \n\r");
	VERBOSE(printf("%d: %s\n", fargc, fargv[fargc]));

	while (fargc < (4 * HDR_MEMBS + 1) && fargv[fargc] != 0) {
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
				strncpy(header->src_name, optarg, ASCII_HDR_STR_LEN);
				break;
			case 'b':
				strncpy(header->ra_str, optarg, ASCII_HDR_STR_LEN);
				break;

			case 'c':
				strncpy(header->dec_str, optarg, ASCII_HDR_STR_LEN);
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
				strncpy(header->fd_poln, optarg, ASCII_HDR_STR_LEN);
				break;

			case 'l':
				strncpy(header->trk_mode, optarg, ASCII_HDR_STR_LEN);
				break;

			case 'm':
				strncpy(header->obs_mode, optarg, ASCII_HDR_STR_LEN);
				break;

			case 'n':
				strncpy(header->cal_mode, optarg, ASCII_HDR_STR_LEN);
				break;

			case 'o':
				header->scanlen = atof(optarg);
				break;

			case 'p':
				strncpy(header->projid, optarg, ASCII_HDR_STR_LEN);
				break;

			case 'q':
				strncpy(header->observer, optarg, ASCII_HDR_STR_LEN);
				break;

			case 'r':
				strncpy(header->telescop, optarg, ASCII_HDR_STR_LEN);
				break;

			case 's':
				strncpy(header->frontend, optarg, ASCII_HDR_STR_LEN);
				break;

			case 't':
				strncpy(header->backend, optarg, ASCII_HDR_STR_LEN);
				break;

			case 'u':
				strncpy(header->datahost, optarg, ASCII_HDR_STR_LEN);
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
				strncpy(header->daqpulse, optarg, ASCII_HDR_STR_LEN);
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
				strncpy(header->pktfmt, optarg, ASCII_HDR_STR_LEN);
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


__attribute_deprecated__ int lofar_udp_metadata_setup_GUPPI(lofar_udp_metadata *metadata) {



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
	if (headerLength <= (HDR_MEMBS * 80 + 1)) {
		fprintf(stderr, "ERROR: Passed header buffer is too small (%ld vs minimum of %d), exiting.", headerLength, (HDR_MEMBS * 80 + 1));
		return -1;
	}

	// Make sure we start appending at the start of the buffer
	if (headerBuffer[0] != '\0') {
		headerBuffer[0] = '\0';
	}

	int returnVal = 0;

	returnVal += writeStr(headerBuffer, "SRC_NAME", headerStruct->src_name);
	returnVal += writeStr(headerBuffer, "RA_STR", headerStruct->ra_str);
	returnVal += writeStr(headerBuffer, "DEC_STR", headerStruct->dec_str);

	returnVal += writeDouble(headerBuffer, "OBSFREQ", headerStruct->obsfreq);
	returnVal += writeDouble(headerBuffer, "OBSBW", headerStruct->obsbw);
	returnVal += writeDouble(headerBuffer, "CHAN_BW", headerStruct->chan_bw);
	returnVal += writeInt(headerBuffer, "OBSNCHAN", headerStruct->obsnchan);
	returnVal += writeInt(headerBuffer, "NPOL", headerStruct->npol);
	returnVal += writeInt(headerBuffer, "NBITS", headerStruct->nbits);
	returnVal += writeDouble(headerBuffer, "TBIN", headerStruct->tbin);

	returnVal += writeStr(headerBuffer, "FD_POLN", headerStruct->fd_poln);
	returnVal += writeStr(headerBuffer, "TRK_MODE", headerStruct->trk_mode);
	returnVal += writeStr(headerBuffer, "OBS_MODE", headerStruct->obs_mode);
	returnVal += writeStr(headerBuffer, "CAL_MODE", headerStruct->cal_mode);
	returnVal += writeDouble(headerBuffer, "SCANLEN", headerStruct->scanlen);

	returnVal += writeStr(headerBuffer, "PROJID", headerStruct->projid);
	returnVal += writeStr(headerBuffer, "OBSERVER", headerStruct->observer);
	returnVal += writeStr(headerBuffer, "TELESCOP", headerStruct->telescop);
	returnVal += writeStr(headerBuffer, "FRONTEND", headerStruct->frontend);
	returnVal += writeStr(headerBuffer, "BACKEND", headerStruct->backend);
	returnVal += writeStr(headerBuffer, "DATAHOST", headerStruct->datahost);
	returnVal += writeInt(headerBuffer, "DATAPORT", headerStruct->dataport);
	returnVal += writeInt(headerBuffer, "OVERLAP", headerStruct->overlap);

	returnVal += writeLong(headerBuffer, "BLOCSIZE", headerStruct->blocsize);
	returnVal += writeStr(headerBuffer, "DAQPULSE", headerStruct->daqpulse);

	returnVal += writeInt(headerBuffer, "STT_IMJD", headerStruct->stt_imjd);
	returnVal += writeInt(headerBuffer, "STT_SMJD", headerStruct->stt_smjd);
	returnVal += writeDouble(headerBuffer, "STT_OFFS", headerStruct->stt_offs);

	returnVal += writeLong(headerBuffer, "PKTIDX", headerStruct->pktidx);
	returnVal += writeStr(headerBuffer, "PKTFMT", headerStruct->pktfmt);
	returnVal += writeInt(headerBuffer, "PKTSIZE", headerStruct->pktsize);

	returnVal += writeDouble(headerBuffer, "DROPBLK", headerStruct->dropblk);
	returnVal += writeDouble(headerBuffer, "DROPTOT", headerStruct->droptot);



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

