#include "ascii_hdr_manager.h"

// Default values for the ASCII header
ascii_hdr ascii_hdr_default = {	"J0000+0000", "00:00:00.0000", "+00:00:00.0000", 149.90234375, 95.3125, 0.1953125, 488, 4, 8, 5.12e-6, \
								"LIN", "TRACK", "RAW", "OFF", 0., "UDP2RAW", "Unknown", "ILT", "LOFAR-RSP", "LOFAR-UDP", "0.0.0.0", \
								16130, 0, 0, "", 50000, 0, 0, "1SFA", 0., 0, 0., 0.};

// Setup the getopt_long keys + short keys
// https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
static struct option long_options[] = {
	{ "src_name", required_argument, NULL, 'a'},
	{ "ra_str", required_argument, NULL, 'b'},
	{ "dec_str", required_argument, NULL, 'c'},
	{ "obsfreq", required_argument, NULL, 'd'},
	{ "obsbw", required_argument, NULL, 'e'},
	{ "chan_bw", required_argument, NULL, 'f'},
	{ "obsnchan", required_argument, NULL, 'g'},
	{ "npol", required_argument, NULL, 'h'},
	{ "nbits", required_argument, NULL, 'i'},
	{ "tbin", required_argument, NULL, 'j'},
	{ "fd_poln", required_argument, NULL, 'k'},
	{ "trk_mode", required_argument, NULL, 'l'},
	{ "obs_mode", required_argument, NULL, 'm'},
	{ "cal_mode", required_argument, NULL, 'n'},
	{ "scanlen", required_argument, NULL, 'o'},
	{ "projid", required_argument, NULL, 'p'},
	{ "observer", required_argument, NULL, 'q'},
	{ "telescop", required_argument, NULL, 'r'},
	{ "frontend", required_argument, NULL, 's'},
	{ "backend", required_argument, NULL, 't'},
	{ "datahost", required_argument, NULL, 'u'},
	{ "dataport", required_argument, NULL, 'v'},
	{ "overlap", required_argument, NULL, 'w'},
	{ "blocsize", required_argument, NULL, 'x'},
	{ "daqpulse", required_argument, NULL, 'y'},
	{ "stt_imjd", required_argument, NULL, 'z'},
	{ "stt_smjd", required_argument, NULL, 'A'},
	{ "pktidx", required_argument, NULL, 'B'},
	{ "pktfmt", required_argument, NULL, 'C'},
	{ "stt_offs", required_argument, NULL, 'D'},
	{ "pktsize", required_argument, NULL, 'E'},
	{ "dropblk", required_argument, NULL, 'F'},
	{ "droptot", required_argument, NULL, 'G'},
	{0, 0, NULL, 0}
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
int parseHdrFile(char inputFile[], ascii_hdr *header) {
	int returnVal = 0;


	// Open the header file, find it's length and load it into memory
	FILE* fileRef = fopen(inputFile, "r");

	fseek(fileRef, 0, SEEK_END);
	unsigned long fileSize = ftell(fileRef);
	fseek(fileRef, 0, SEEK_SET);

	char *fileData = calloc(fileSize, sizeof(char));
	unsigned long readlen = fread(fileData, 1, fileSize, fileRef);

	// Sanity check the old vs new length
	if (readlen != fileSize) {
		fprintf(stderr, "Header file size changed during read operation, continuing with caution...\n");
	}
	
	fclose(fileRef);

	VERBOSE(printf("%s\n", fileData));

	// Overall method modified from https://stackoverflow.com/posts/39841506/ 
	// 
	// I have no idea about the 10 offset though. getopt_long just seems to be
	// refusing to parse the first 9-10 inputs. Just empty-string pad to work
	// around it.
	char *fargv[4 * HEADER_ARGS + 1] = { "" };
	int fargc = 1;

	// Break up the input file into different words, store them in fargv
	fargv[fargc] = strtok(fileData, " \n\r");
	VERBOSE(printf("%d: %s\n", fargc, fargv[fargc]));

	while (fargc < (4 * HEADER_ARGS + 1) && fargv[fargc] != 0) {
		fargc++;
		fargv[fargc] = strtok(0, " \n\r");
		VERBOSE(printf("%d: %s\n", fargc, fargv[fargc]));
	}


	// Reset optind as we likely used it in the GUPPI CLI and it needs to be reinitialised
	optind = 1;
	// Iterate over each work and update the header as needed.
	int optIdx = 0;
	int charVal;
	while ((charVal = getopt_long(fargc, &(fargv[0]), "+", long_options, &optIdx)) != -1) {
		VERBOSE(printf("%c= %s \n", charVal, optarg));
		switch (charVal) {
			case 'a':
				strcpy(header->src_name, optarg);
				break;
			case 'b':
				strcpy(header->ra_str, optarg);
				break;

			case 'c':
				strcpy(header->dec_str, optarg);
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
				strcpy(header->fd_poln, optarg);
				break;

			case 'l':
				strcpy(header->trk_mode, optarg);
				break;

			case 'm':
				strcpy(header->obs_mode, optarg);
				break;

			case 'n':
				strcpy(header->cal_mode, optarg);
				break;

			case 'o':
				header->scanlen = atof(optarg);
				break;

			case 'p':
				strcpy(header->projid, optarg);
				break;

			case 'q':
				strcpy(header->observer, optarg);
				break;

			case 'r':
				strcpy(header->telescop, optarg);
				break;

			case 's':
				strcpy(header->frontend, optarg);
				break;

			case 't':
				strcpy(header->backend, optarg);
				break;

			case 'u':
				strcpy(header->datahost, optarg);
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
				strcpy(header->daqpulse, optarg);
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
				strcpy(header->pktfmt, optarg);
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


/**
 * @brief      Writes an ASCII header to disk based on the current values of the
 *             struct
 *
 * @param      fileRef  The file reference
 * @param      header   The ASCII header struct
 */
void writeHdr(FILE *fileRef, ascii_hdr *header) {
	writeStr(fileRef, "SRC_NAME", header->src_name);
	writeStr(fileRef, "RA_STR", header->ra_str);
	writeStr(fileRef, "DEC_STR", header->dec_str);

	writeDouble(fileRef, "OBSFREQ", header->obsfreq);
	writeDouble(fileRef, "OBSBW", header->obsbw);
	writeDouble(fileRef, "CHAN_BW", header->chan_bw);
	writeInt(fileRef, "OBSNCHAN", header->obsnchan);
	writeInt(fileRef, "NPOL", header->npol);
	writeInt(fileRef, "NBITS", header->nbits);
	writeDouble(fileRef, "TBIN", header->tbin);

	writeStr(fileRef, "FD_POLN", header->fd_poln);
	writeStr(fileRef, "TRK_MODE", header->trk_mode);
	writeStr(fileRef, "OBS_MODE", header->obs_mode);
	writeStr(fileRef, "CAL_MODE", header->cal_mode);
	writeDouble(fileRef, "SCANLEN", header->scanlen);

	writeStr(fileRef, "PROJID", header->projid);
	writeStr(fileRef, "OBSERVER", header->observer);
	writeStr(fileRef, "TELESCOP", header->telescop);
	writeStr(fileRef, "FRONTEND", header->frontend);
	writeStr(fileRef, "BACKEND", header->backend);
	writeStr(fileRef, "DATAHOST", header->datahost);
	writeInt(fileRef, "DATAPORT", header->dataport);
	writeInt(fileRef, "OVERLAP", header->overlap);

	writeLong(fileRef, "BLOCSIZE", header->blocsize);
	writeStr(fileRef, "DAQPULSE", header->daqpulse);

	writeInt(fileRef, "STT_IMJD", header->stt_imjd);
	writeInt(fileRef, "STT_SMJD", header->stt_smjd);
	writeDouble(fileRef, "STT_OFFS", header->stt_offs);

	writeLong(fileRef, "PKTIDX", header->pktidx);
	writeStr(fileRef, "PKTFMT", header->pktfmt);
	writeInt(fileRef, "PKTSIZE", header->pktsize);

	writeDouble(fileRef, "DROPBLK", header->dropblk);
	writeDouble(fileRef, "DROPTOT", header->droptot);

	// All headers are terminated with "END" followed by 77 spaces.
	const char end[4] = "END";
	fprintf(fileRef, "%-80s", end);
}

/**
 * @brief      Writes a string value to disk, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param      val      The value
 */
void writeStr(FILE *fileRef, char key[], char val[]) {
	char tmpStr[81];
	sprintf(tmpStr, "%-8s= '%-8s'", key, val);
	fprintf(fileRef, "%-80s", tmpStr);
}

/**
 * @brief      Writes an integer value to disk, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param[in]  val      The value
 */
void writeInt(FILE *fileRef, char key[], int val) {
	char tmpStr[81];
	char intStr[21];
	sprintf(intStr, "%d", val);
	sprintf(tmpStr, "%-8s= %20s", key, intStr);
	fprintf(fileRef, "%-80s", tmpStr);
}

/**
 * @brief      Writes a long value to disk, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param[in]  val      The value
 */
void writeLong(FILE *fileRef, char key[], long val) {
	char tmpStr[81];
	char intStr[21];
	sprintf(intStr, "%ld", val);
	sprintf(tmpStr, "%-8s= %20s", key, intStr);
	fprintf(fileRef, "%-80s", tmpStr);
}

/**
 * @brief      Writes a double value to disk, padded to 80 chars long
 *
 * @param      fileRef  The file reference
 * @param      key      The key
 * @param[in]  val      The value
 */
void writeDouble(FILE *fileRef, char key[], double val) {
	char tmpStr[81];
	char intStr[21];
	sprintf(intStr, "%.9lf", val);
	sprintf(tmpStr, "%-8s= %20s", key, intStr);
	fprintf(fileRef, "%-80s", tmpStr);
}
