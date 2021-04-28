#include "lofar_udp_metadata.h"


int parseList(const char inputStr[], float results[3]);
int countList(const char inputStr[]);
int getCharValue(const char inputStr[], const char keyword[], char result[]);
int parseBeamctlCmd(lofar_udp_metadata *metadata, const char inputLine[], size_t lineLength);
int lofar_udp_metadata_parse_input_file(lofar_udp_metadata *metadata, const char inputFile[]);
int lofar_udp_metadata_parse_subbands(lofar_udp_metadata *metadata, float subbands[3]);
int lofar_udp_metadata_parse_pointing(lofar_udp_metadata *metadata, const char inputStr[], int digi);

metadata_t lofar_udp_metadata_string_to_meta(char input[]) {
	if (strstr(input, "GUPPI") != NULL)  {
		return GUPPI;
	} else if (strstr(input, "DADA") != NULL) {
		return DADA;
	} else if (strstr(input, "SIGPROC") != NULL)  {
		return SIGPROC;
	} else {
		fprintf(stderr, "ERROR: Unknown metadata format '%s', exiting.\n", input);
		return NO_META;
	}
}

int lofar_udp_setup_metadata(lofar_udp_metadata *metadata, metadata_t meta, char inputFile[]) {

	if (meta == NO_META) {
		fprintf(stderr, "ERROR: No metadata type specified, exiting.\n");
		return -1;
	}

	metadata->type = meta;

	if (lofar_udp_metadata_parse_input_file(metadata, inputFile) < 0) {
		return -1;
	}

	switch(metadata->type) {
		case GUPPI:
			return lofar_udp_metadata_setup_GUPPI(metadata);

		// The DADA header is formed from the reference metadata struct
		case DADA:
			return 0;

		case SIGPROC:
			return lofar_udp_metadata_setup_SIGPROC(metadata);

		default:
			fprintf(stderr, "ERROR: Unknown metadata type %d, exiting.\n", meta);
			return -1;
	}
}

int lofar_udp_metadata_parse_input_file(lofar_udp_metadata *metadata, const char inputFile[]) {
	char *inputLine = NULL, *strPtr = NULL;
	FILE *input = fopen(inputFile, "r");
	
	int lineLength;
	size_t buffLen = 0;

	if (input == NULL) {
		fprintf(stderr, "ERROR: Failed to open file at '%s'm exiting.\n", inputFile);
		return -1;
	}

	while ((lineLength = getline(&inputLine, &buffLen, input)) != -1) {
		if ((strPtr = strstr(inputLine, "beamctl")) != NULL) {
			if (parseBeamctlCmd(metadata, strPtr, lineLength) < 0) {
				return -1;
			}
		} else if ((strPtr = strstr(inputLine, "TARGET")) != NULL) {
			if (getCharValue(strPtr, "TARGET\t", metadata->source) < 0) {
				return -1;
			}
		}
		/* else if ((strPtr = strstr()) != NULL) {

		}
		*/
 	}

	return 0;
}

int parseBeamctlCmd(lofar_udp_metadata *metadata, const char inputLine[], size_t lineLength) {
	char *workingPtr;
	char workingBuff[64];
	int workingInt;

	if ((workingPtr = strstr(inputLine, "--rcus=")) != NULL) {
		metadata->nrcu = countList(workingPtr);

		if (metadata->nrcu == -1) {
			return -1;
		}
	} else {
		metadata->nrcu = 0;
	}

	if ((workingPtr = strstr(inputLine, "--band=")) != NULL) {
		if (sscanf(workingPtr, "--band=%d_%*s", &workingInt) < 0) {
			return -1;
		}

		switch (workingInt) {
			case 10:
				metadata->upm_rcumode = 3;
				break;

			case 30:
				metadata->upm_rcumode = 4;
				break;

			case 110:
				metadata->upm_rcumode = 5;
				break;

			case 160:
				metadata->upm_rcumode = 6;
				break;

			case 210:
				metadata->upm_rcumode = 7;
				break;

			default:
				fprintf(stderr, "ERROR: Failed to parse RCU band mode (base int of %d), exiting.\n", workingInt);
				return -1;
		}
	} else if ((workingPtr = strstr(inputLine, "--rcumode"))) {
		if (sscanf(workingPtr, "--rcumode=%d", &(metadata->upm_rcumode)) < 0) {
			return -1;
		}
	} else {
		fprintf(stderr, "ERROR: Insufficient information in beamctl command to determine observing frequency (no band/rcumode), exiting.\n");
		return -1;
	}

	float subbands[3];
	if ((workingPtr = strstr(inputLine, "--subbands=")) != NULL) {
		if ((metadata->nchan = parseList(workingPtr, subbands)) < 0) {
			return -1;
		}

		if (lofar_udp_metadata_parse_subbands(metadata, subbands) < 0) {
			return -1;
		}
	} else {
		fprintf(stderr, "ERROR: Insufficient information in beamctl command to determine observing frequency (no subbands), exiting.\n");
		return -1;
	}

	if ((workingPtr = strstr(inputLine, "--digidir=")) != NULL) {
		if (lofar_udp_metadata_parse_pointing(metadata, workingPtr, 1) < 0) {
			return -1;
		}
	} else {
		fprintf(stderr, "ERROR: beamctl command doesn't have a digidir -- something has gone badly wrong. Exiting.\n");
		return -1;
	}

	if ((workingPtr = strstr(inputLine, "--anadir=")) != NULL) {
		if (lofar_udp_metadata_parse_pointing(metadata, workingPtr, 0) < 0) {
			return -1;
		}
	}


	return 0;
}

int lofar_udp_metadata_parse_pointing(lofar_udp_metadata *metadata, const char inputStr[], int digi) {
	double lon, lon_work, lat, lat_work, ra_s, dec_s;
	int ra[2], dec[2];
	char basis[32];

	// 2pi / 24h -> 0.26179938779 rad/hr
	const double rad2HA = 0.26179938779;
	// 2pi / 360deg -> 0.01745329251 rad/deg
	const double rad2Deg = 0.01745329251;

	if (sscanf(inputStr, "%*[^=]=%lf,%lf,%[^ ]", &lon, &lat, basis) < 2) {
		fprintf(stderr, "ERROR: Failed to parse pointing string %s, exiting.\n", inputStr);
		return -1;
	}

	lon_work = lon;
	lat_work = lat;

	// Avoiding libmath imports, so manually manual modulo-s ahead.
	ra[0] = lon_work / rad2HA;
	lon_work -= ra[0] * rad2HA;
	ra[1] = lon_work / (rad2HA / 60);
	lon_work -= ra[1] * (rad2HA / 60);
	ra_s = lon_work / (rad2HA / 60 / 60);

	dec[0] = lat_work / rad2Deg;
	lat_work -= dec[0] * rad2Deg;
	dec[1] = lat_work / (rad2Deg / 60);
	lat_work -= dec[1] * (rad2Deg / 60);
	dec_s = lat_work / (rad2HA / 60 / 60);

	char *dest[2];
	if (digi) {
		dest[0] = metadata->ra;
		dest[1] = metadata->dec;
	} else {
		dest[0] = metadata->ra_analog;
		dest[1] = metadata->dec_analog;
	}

	if (sprintf(dest[0], "%02d:%02d:%06.3lf", ra[0], ra[1], ra_s) < 0) {
		fprintf(stderr, "ERROR: Failed to print RA/Lon to metadata, exiting.\n");
		return -1;
	}

	if (sprintf(dest[1], "%02d:%02d:%06.3lf", dec[0], dec[1], dec_s) < 0) {
		fprintf(stderr, "ERROR: Failed to print Dec/Lat to metadata, exiting.\n");
		return -1;
	}

	if (digi) {
		metadata->ra_rad = lon;
		metadata->dec_rad = lat;
	} else {
		metadata->ra_rad_analog = lon;
		metadata->dec_rad_analog = lat;
	}

	return 0;
}

int lofar_udp_metadata_parse_subbands(lofar_udp_metadata  *metadata, float subbands[3]) {
	int rcumode = metadata->upm_rcumode;

	if (rcumode == -1 || subbands == NULL) {
		fprintf(stderr, "ERROR: Input variables are uninitialised, exiting.\n");
		return -1;
	}

	int clock;
	double channelBw, baseBw;

	if (rcumode != 6) {
		clock = 200;

		switch (rcumode) {
			case 3:
			case 4:
				baseBw = 0.0;
				break;

			case 5:
				baseBw = 100.0;
				break;

			// case 6 -- 160MHz clock.

			case 7:
				baseBw = 200.0;
				break;

			default:
				fprintf(stderr, "ERROR: Unexpected rcumode %d, exiting.\n", rcumode);
		}
	} else {
		clock = 160;
		baseBw = 160.0;
	}

	channelBw = clock / 2 / 512;

	// Offset frequency by half a subband to get central frequencies rather than base frequencies
	metadata->fbottom = baseBw + channelBw * (0.5 + subbands[0]);
	metadata->freq = baseBw + channelBw * (0.5 + subbands[1]);
	metadata->ftop = baseBw + channelBw * (0.5 + subbands[2]);
	metadata->bw = metadata->nchan * channelBw;

	return 0;
}

int getCharValue(const char inputStr[], const char keyword[], char *result) {
	char format[64];
	sprintf(format, "%s%%s", keyword);

	if (sscanf(inputStr, format, result) < 0) {
		fprintf(stderr, "ERROR: Failed to get value for input '%s' keyword, exiting.\n", keyword);
		return -1;
	}

	return 0;
}

int countList(const char inputStr[]) {
	return parseList(inputStr, NULL);
}

int parseList(const char inputStr[], float results[3]) {
	char *workingPtr;


	int counter = 0, lower, upper;
	int minimum = INT_MAX, maximum = -1, sum = 0;
	size_t consumed = 0, last;

	workingPtr = strtok(inputStr, ",");
	do { //  while ((workingPtr = strtok(NULL, ",")) != NULL);

		if (strstr(workingPtr, ":") != NULL) {
			if (sscanf(workingPtr, "%d:%d", &lower, &upper) < 0) {
				fprintf(stderr, "Thing went wrong\n");
				return -1;
			}

			printf("Parse %d / %d\n", upper, lower);

			// Sum of a series
			sum += ((upper * (upper + 1) / 2) - ((lower - 1) * (lower) / 2));

			// LOFAR commands are inclusive lists, get the difference and add 1
			counter += (upper - lower) + 1;
		} else {
			sscanf(workingPtr, "%d", &lower);

			printf("Parsed %d\n", lower);
			upper = lower;
			sum += lower;

			// Single element, increment by 1
			counter += 1;
		}

		if (lower < minimum) {
			minimum = lower;
		}

		if (upper > maximum) {
			maximum = upper;
		}

		consumed += last;
	} while ((workingPtr = strtok(NULL, ",")) != NULL);

	if (results != NULL) {
		results[0] = minimum;
		results[1] = sum / counter;
		results[2] = maximum;
	}

	return counter;
}