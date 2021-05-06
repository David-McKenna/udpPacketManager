#include "lofar_udp_metadata.h"

int lofar_udp_setup_metadata(lofar_udp_metadata *metadata, metadata_t meta, lofar_udp_reader *reader, const char *inputFile) {

	if (meta == NO_META) {
		fprintf(stderr, "ERROR: No metadata type specified, exiting.\n");
		return -1;
	}

	metadata->type = meta;

	if (inputFile != NULL) {
		if (lofar_udp_metadata_parse_input_file(metadata, inputFile) < 0) {
			return -1;
		}
	}

	if (reader != NULL) {
		if (lofar_udp_metadata_parse_reader(metadata, reader) < 0) {
			return -1;
		}
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

	int beamctlData[] = { -1, INT_MAX, 0, -1, INT_MAX, 0, -1 };
	while ((lineLength = getline(&inputLine, &buffLen, input)) != -1) {
		if ((strPtr = strstr(inputLine, "beamctl")) != NULL) {
			if (lofar_udp_metadata_parse_beamctl(metadata, strPtr, beamctlData) < 0) {
				return -1;
			}

			strncat(metadata->upm_beamctl, inputLine, 4 * DEF_STR_LEN - 1 - strlen(metadata->upm_beamctl));

		} else if ((strPtr = strstr(inputLine, "TARGET")) != NULL) {
			if (lofar_udp_metadata_get_tsv(strPtr, "TARGET", metadata->source) < 0) {
				return -1;
			}
		}
		/* else if ((strPtr = strstr()) != NULL) {

		}
		*/
 	}

	if (beamctlData[0] == -1) {
		fprintf(stderr, "ERROR: Beamctl parsing did not detect the RCU mode, exiting.\n");
		return -1;
	}

	if (beamctlData[3] != beamctlData[5]) {
		fprintf(stderr, "ERROR: Number of subbands does not match number of beamlets, exiting.\n");
		return -1;
	}

	metadata->nchan = beamctlData[3];
	metadata->upm_lowerbeam = beamctlData[4];
	metadata->upm_upperbeam = beamctlData[6];
	metadata->upm_rcuclock = lofar_udp_metadata_get_clockmode(beamctlData[0]);

	// Remaining few bits
	if (strncpy(metadata->upm_version, UPM_VERSION, META_STR_LEN) != metadata->upm_version) {
		fprintf(stderr, "ERROR: Failed to copy UPM version into metadata struct, exiting.\n");
		return -1;
	}

	metadata->upm_rawbeamlets = beamctlData[5];

	return 0;
}

__attribute_deprecated__ lofar_udp_metadata_parse_reader(lofar_udp_metadata *metadata, lofar_udp_reader *reader) {

	// int baseBeamlet = reader->input->offsetVal;
	// // Parse the combined beamctl data to determine the frequency information
	// double channelBw = ((double) metadata->upm_rcuclock) / 1024;
	// int minSubband = beamctlData[1];
	// double meanSubband = (double) beamctlData[2] / (double) metadata->nchan;
	// int maxSubband = beamctlData[3];

	// // Offset frequency by half a subband to get central frequencies rather than base frequencies
	// metadata->fbottom = channelBw * (0.5 + minSubband);
	// metadata->freq = channelBw * (0.5 + meanSubband);
	// metadata->ftop = channelBw * (0.5 + maxSubband);
	// metadata->bw += metadata->nchan * channelBw;

	return 0;
}

int lofar_udp_metadata_parse_beamctl(lofar_udp_metadata *metadata, const char *inputLine, int *beamctlData) {
	char *workingPtr = (char*) inputLine;


	char *inputPtr = strtok(workingPtr, " ");
	if (inputPtr == NULL) {
		fprintf(stderr, "ERROR: Failed to find space in beamctl command, exiting.\n");
		return -1;
	}


	do {
		// Number of RCUs
		if ((workingPtr = strstr(inputPtr, "--rcus=")) != NULL) {
			metadata->nrcu = lofar_udp_metadata_count_csv(workingPtr + strlen("--rcus="));

			if (metadata->nrcu == -1) {
				return -1;
			}


		// RCU configuration
		} else if ((workingPtr = strstr(inputPtr, "--band=")) != NULL) {
			if (lofar_udp_metadata_parse_rcumode(metadata, workingPtr, beamctlData) < 0) {
				return -1;
			}

		} else if ((workingPtr = strstr(inputPtr, "--rcumode="))) {
			if (lofar_udp_metadata_parse_rcumode(metadata, workingPtr, beamctlData) < 0) {
				return -1;
			}


		// Pointing information
		} else if ((workingPtr = strstr(inputPtr, "--digidir=")) != NULL) {
			if (lofar_udp_metadata_parse_pointing(metadata, workingPtr, 1) < 0) {
				return -1;
			}
		} else if ((workingPtr = strstr(inputPtr, "--anadir=")) != NULL) {
			if (lofar_udp_metadata_parse_pointing(metadata, workingPtr, 0) < 0) {
				return -1;
			}
		}


	} while ((inputPtr = strtok(NULL, " ")) != NULL);


	workingPtr = (char*) inputLine;
	if (lofar_udp_metadata_parse_subbands(metadata, workingPtr, beamctlData) < 0) {
		return -1;
	}

	return 0;
}

int lofar_udp_metadata_parse_rcumode(lofar_udp_metadata *metadata, char *inputStr, int *beamctlData) {
	int workingInt = -1;
	if (sscanf(inputStr, "%*[^=]=%d", &(workingInt)) < 0) {
		return -1;
	}

	int lastClock = metadata->upm_rcuclock;
	metadata->upm_rcuclock = lofar_udp_metadata_get_clockmode(workingInt);
	beamctlData[0] = lofar_udp_metadata_get_rcumode(workingInt);

	if (lastClock != -1 && lastClock != metadata->upm_rcuclock) {
		fprintf(stderr, "ERROR: 160/200MHz clock mixing detected, this is not currently supported. Exiting.\n");
		return -1;
	}

	if (beamctlData[0] == -1) {
		fprintf(stderr, "ERROR: Failed to determine RCU mode, exiting.\n");
		return -1;
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

int lofar_udp_metadata_parse_subbands(lofar_udp_metadata *metadata, const char *inputLine, int *results) {

	if (results[0] == -1) {
		fprintf(stderr, "ERROR: RCU mode variable is uninitialised, exiting.\n");
		return -1;
	}

	int subbands[MAX_NUM_PORTS * UDPMAXBEAM] = { 0 };
	int beamlets[MAX_NUM_PORTS * UDPMAXBEAM] = { 0 };

	char *workingPtr = (char*) inputLine;
	char *inputPtr = strtok(workingPtr, " ");
	if (inputPtr == NULL) {
		fprintf(stderr, "ERROR: Failed to find space in beamctl command, exiting.\n");
		return -1;
	}

	int parsed = 0;
	do {
		if ((workingPtr = strstr(inputPtr, "--subbands=")) != NULL) {
			if ((lofar_udp_metadata_parse_csv(workingPtr + strlen("--subbands="), subbands, &(results[1]))) < 0) {
				return -1;
			}
			parsed |= 1;
		} else if ((workingPtr = strstr(inputPtr, "--beamlets=")) != NULL) {
			if ((lofar_udp_metadata_parse_csv(workingPtr + strlen("--beamlets="), beamlets, &(results[4]))) < 0) {
				return -1;
			}
			parsed |= 2;
		}
	} while ((inputPtr = strtok(NULL, " ")) != NULL);

	if (parsed != 3) {
		fprintf(stderr, "ERROR: Failed to parse both subbands and beamlets (%d), exiting.\n", parsed);
		return -1;
	}

	// Subband offset
	// Simpler way of calculating the frequencies -- use an offset to emulate the true frequency across several RCU modes
	// Mode 3/4 -- 0 offset, subband (0-511), 0.19..MHz bandwidth, 0 - 100MHz
	// Mode 5/6 -- 512 offset, subband (512 - 1023), 0.19 / 0.156..MHz bandwidth, 100 - 200MHz and 160 - 240MHz
	// Mode 7 -- 1024 offset, subband (1024 - 1535), 0.19 MHz bandwidth, 200 - 300MHz
	int subbandOffset = (512 * (results[0] - 3) / 2);

	// As a result, for mode 5, subbands 512 - 1023, we have (512 - 1023) * (0.19) = 100 - 200MHz
	// Similarly no offset for mode 3, double the offset for mode 7, etc.
	// This is why I raise an error when mixing mode 6 (160MHz clock) and anything else (all on the 200MHz clock)
	for (int i = 0; i < metadata->nchan; i++) {
		metadata->subbands[beamlets[i]] = subbandOffset + subbands[i];
	}

	return 0;
}

int lofar_udp_metadata_get_tsv(const char *inputStr, const char *keyword, char *result) {
	char format[64];
	sprintf(format, "%s\t%%s", keyword);

	if (sscanf(inputStr, format, result) < 0) {
		fprintf(stderr, "ERROR: Failed to get value for input '%s' keyword, exiting.\n", keyword);
		return -1;
	}

	return 0;
}

int lofar_udp_metadata_count_csv(const char *inputStr) {
	return lofar_udp_metadata_parse_csv(inputStr, NULL, NULL);
}

int lofar_udp_metadata_parse_csv(const char *inputStr, int *values, int *limits) {
	char *workingPtr = (char*) inputStr;


	int counter = 0, lower = -1, upper = -1;
	int minimum = INT_MAX, maximum = -1, sum = 0;

	workingPtr = strtok(workingPtr, ",");

	// If the input isn't a list, just point to the start of the list and parse the 1 element.
	if (workingPtr == NULL) {
		workingPtr = (char*) &(inputStr[0]);
	}

	do { //  while ((workingPtr = strtok(NULL, ",")) != NULL);

		if (strstr(workingPtr, ":") != NULL) {
			if (sscanf(workingPtr, "%d:%d", &lower, &upper) < 0) {
				fprintf(stderr, "Thing went wrong\n");
				return -1;
			}

			printf("Parse %d / %d\n", upper, lower);

			// Sum of a series
			sum += ((upper * (upper + 1) / 2) - ((lower - 1) * (lower) / 2));

			for (int i = lower; i < upper + 1; i++) {
				if (values != NULL) {
					values[counter] = i;
				}
				counter++;
			}

		} else {
			sscanf(workingPtr, "%d", &lower);

			if (lower == -1) {
				fprintf(stderr, "ERROR: Failed to parse csv list (input: %s), exiting.\n", inputStr);
				return -1;
			}

			printf("Parsed %d\n", lower);
			upper = lower;
			sum += lower;

			// Single element, increment by 1
			if (values != NULL) {
				values[counter] = lower;
			}
			counter += 1;
		}

		if (lower < minimum) {
			minimum = lower;
		}

		if (upper > maximum) {
			maximum = upper;
		}

	} while ((workingPtr = strtok(NULL, ",")) != NULL);

	if (limits != NULL) {
		limits[0] = (minimum < limits[0]) ? minimum : limits[0];
		limits[1] += sum;
		limits[2] = (maximum > limits[0]) ? maximum : limits[0];
	}

	return counter;
}


int lofar_udp_metadata_get_clockmode(int input) {
	switch (input) {
		// RCU modes
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 7:
		// Base values of bands
		case 10:
		case 30:
		case 110:
		case 210:
			return 200;

		case 6:
		case 160:
			return 160;


		default:
			fprintf(stderr, "ERROR: Failed to parse RCU clockmode (base int of %d), exiting.\n", input);
			return -1;
	}
}

int lofar_udp_metadata_get_rcumode(int input) {
	if (input < 1) {
		fprintf(stderr, "ERROR: Invalid input RCU mode %d, exiting.\n", input);
		return -1;
	}
	if (input < 8) {
		return input;
	}
	switch (input) {
		// Base values of bands
		case 10:
			return 3;
		case 30:
			return 4;

		case 110:
			return 5;
		case 210:
			return 7;

		case 160:
			return 6;


		default:
			fprintf(stderr, "ERROR: Failed to determine RCU mode (base int of %d), exiting.\n", input);
			return -1;
	}
}

metadata_t lofar_udp_metadata_string_to_meta(const char input[]) {
	if (strcasecmp(input, "GUPPI") == 0)  {
		return GUPPI;
	} else if (strcasecmp(input, "DADA") == 0) {
		return DADA;
	} else if (strcasecmp(input, "SIGPROC") == 0)  {
		return SIGPROC;
	} else {
		fprintf(stderr, "ERROR: Unknown metadata format '%s', exiting.\n", input);
		return NO_META;
	}
}

int lofar_udp_metadata_bitmode_to_beamlets(int bitmode) {
	switch(bitmode) {
		case 4:
			return 244;

		case 8:
			return 122;

		case 16:
			return 61;

		default:
			fprintf(stderr, "ERROR: Unknown bitmode %d, exiting.\n", bitmode);
			return -1;
	}
}


#include "./metadata/lofar_udp_metadata_GUPPI.c"
#include "./metadata/lofar_udp_metadata_DADA.c"
#include "./metadata/lofar_udp_metadata_SIGPROC.c"