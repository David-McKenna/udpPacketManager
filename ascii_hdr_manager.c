#include "ascii_hdr_manager.h"

ascii_hdr ascii_hdr_default = {	"J0000+0000", "00:00:00.0000", "+00:00:00.0000", 149.90234375, 95.3125, 0.1953125, 488, 4, 8, 5.12e-6, \
								"LIN", "TRACK", "RAW", "OFF", 0., "UDP2RAW", "Unknown", "ILT", "LOFAR-RSP", "LOFAR-UDP", "0.0.0.0", \
								16130, 0, 0, "", 50000, 0, 0};

// https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
static struct option long_options[] = {
	{ "srcname", required_argument, NULL, 'a'},
	{ "rastr", required_argument, NULL, 'b'},
	{ "decstr", required_argument, NULL, 'c'},
	{ "obsfreq", required_argument, NULL, 'd'},
	{ "obsbw", required_argument, NULL, 'e'},
	{ "chanbw", required_argument, NULL, 'f'},
	{ "obsnchan", required_argument, NULL, 'g'},
	{ "npol", required_argument, NULL, 'h'},
	{ "nbits", required_argument, NULL, 'i'},
	{ "tbin", required_argument, NULL, 'j'},
	{ "fdpoln", required_argument, NULL, 'k'},
	{ "trkmode", required_argument, NULL, 'l'},
	{ "obsmode", required_argument, NULL, 'm'},
	{ "calmode", required_argument, NULL, 'n'},
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
	{ "sttimjd", required_argument, NULL, 'z'},
	{ "sttsmjd", required_argument, NULL, 'A'},
	{ "pktidx", required_argument, NULL, 'B'},
	{0, 0, NULL, 0}
};


int parseHdrFile(char inputFile[], ascii_hdr *header) {
	int returnVal = 0;

	FILE* fileRef = fopen(inputFile, "r");

	fseek(fileRef, 0, SEEK_END);
	int fileSize = ftell(fileRef);
	fseek(fileRef, 0, SEEK_SET);

	char *fileData = calloc(fileSize, sizeof(char));
	int readlen = fread(fileData, 1, fileSize, fileRef);

	if (readlen != fileSize) {
		fprintf(stderr, "Header file size changed during read operation, continuing with caution...\n");
	}
	
	fclose(fileRef);

	printf("%s\n", fileData);

	// https://stackoverflow.com/posts/39841506/
	char *fargv[HEADER_ARGS + 1];
	int fargc = 0;
	fargv[fargc++] = strtok(fileData, " \n\r");
	printf("%d: %s\n", fargc, fargv[fargc]);

	while (fargc < HEADER_ARGS && fargv[fargc] != 0) {
		if (fargc == 0) {
			fargv[fargc++] = strtok(0, " \n\r");
			printf("%d: %s\n", fargc, fargv[fargc]);
		}

		int optIdx = 0;
		char charVal;
		while ((charVal = getopt_long(fargc, fargv, "a:b:c:d:e:f:g:h:i:j:k:l:m:n:o:p:q:r:s:t:u:v:w:x:y:z:A:B:", long_options, &optIdx)) != -1) {
			printf("%c= %s \n", charVal, optarg);
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

				case '0':
				case '?':
				default:
					fprintf(stderr, "Unknown flag %d (%s), continuing with caution...\n", charVal, optarg);
					returnVal = -1;
					break;
			}
		}
		fargc = 0;
	}

	return returnVal;
}

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

	writeInt(fileRef, "BLOCSIZE", header->blocsize);
	writeStr(fileRef, "DAQPULSE", header->daqpulse);

	writeInt(fileRef, "STT_IMJD", header->stt_imjd);
	writeInt(fileRef, "STT_SMJD", header->stt_smjd);

	writeLong(fileRef, "PKTIDX", header->pktidx);

	const char end[3] = "END";
	fprintf(fileRef, "%-80s", end);
}

void writeStr(FILE *fileRef, char key[], char val[]) {
	char tmpStr[80];
	sprintf(tmpStr, "%-8s= '%-8s'", key, val);
	fprintf(fileRef, "%-80s", tmpStr);
}

void writeInt(FILE *fileRef, char key[], int val) {
	char tmpStr[80];
	char intStr[20];
	sprintf(intStr, "%d", val);
	sprintf(tmpStr, "%-8s= %20s", key, intStr);
	fprintf(fileRef, "%-80s", tmpStr);
}

void writeLong(FILE *fileRef, char key[], long val) {
	char tmpStr[80];
	char intStr[20];
	sprintf(intStr, "%ld", val);
	sprintf(tmpStr, "%-8s= %20s", key, intStr);
	fprintf(fileRef, "%-80s", tmpStr);
}

void writeDouble(FILE *fileRef, char key[], double val) {
	char tmpStr[80];
	char intStr[20];
	sprintf(intStr, "%lf", val);
	sprintf(tmpStr, "%-8s= %20s", key, intStr);
	fprintf(fileRef, "%-80s", tmpStr);
}