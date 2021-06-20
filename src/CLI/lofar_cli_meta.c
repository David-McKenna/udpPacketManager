#include "lofar_cli_meta.h"


const char exitReasons[6][1024] = { "", "",
									"Reached the packet limit set at start-up",
									"Read less data than requested from file (near EOF or disk error)",
									"Metadata failed to write",
									"Output data failed to write"};

void processingModes(void) {
	printf("\n\nProcessing modes:\n");
	printf("0: Raw UDP to Single Output: read in a given set of packet captures, rewrite packets on each port with padding for dropped / out of order packets.\n");
	printf("1: Raw UDP to Single Header-less Output: (1), without the headers between each packet.\n");
	printf("2: Raw UDP to Split Pols: Rewrite each polarization to it's own output file (no headers).\n\n");


	printf("10: Raw UDP to Beamlet Major Output: Reverse the major order in the output (no headers).\n");
	printf("11: Raw UDP to Beamlet Major Split Pols Output: combine (2) and (10).\n\n");

	printf("20: Raw UDP to Beamlet Major, Frequency Reversed Output: Reverse the major order and beamlet order in the output\n");
	printf("21: Raw UDP to Beamlet Major, Frequency Reversed, Split Pols Output: combine (2) and (20).\n\n");

	printf("30: Raw UDP to Time Major Output: Time-continuous output\n");
	printf("31: Raw UDP to Time Major, Split Pols Output: Time-continuous output: combine (2) and (30)\n");
	printf("32: Raw UDP to Time Major, Dual Pols Output: Time-continuous output, to antenna polarisation output (X/Y Split, FFTW format)\n\n");
	printf("35: Raw UDP to Time Major, Dual Pols Floats Output: Mode 32, but output is always a float.\n\n");

	printf("100: Raw UDP to Stokes I: Form a 32-bit float Stokes I for the input.\n");
	printf("110: Raw UDP to Stokes Q: Form a 32-bit float Stokes Q for the input.\n");
	printf("120: Raw UDP to Stokes U: Form a 32-bit float Stokes U for the input.\n");
	printf("130: Raw UDP to Stokes V: Form a 32-bit float Stokes V for the input.\n\n");

	printf("150: Raw UDP to Full Stokes: Form a 32-bit float Stokes Vector for the input (I, Q, U, V output files)\n\n");
	printf("160: Raw UDP to Full Stokes: Form a 32-bit float Stokes Vector for the input (I, V output files)\n\n");

	printf("Stokes outputs can be decimated in orders of 2, up to 16x by adjusting the last digit of their processing mode.\n");
	printf("This is handled in orders of two, so 101 will give a Stokes I with 2x decimation, 102, will give 4x, 103 will give 8x and 104 will give 16x.\n\n");
	printf("Similarly, by default the beamlet order is reversed (negative frequency offset). By adding 100 to each Stokes output, e.g. changing mode 104 to 204,"
		"the frequency order will be changed to be increasing (positive frequency offset, matching the telescope configuration).\n");
}

int checkOpt(char opt, char* inp, char* endPtr) {
	if (inp != endPtr && *(endPtr) == '\0') {
		return 0;
	}

	fprintf(stderr, "ERROR: Failed to parse flag %c with value %s, exiting.\n", opt, inp);
	return 1;
}
