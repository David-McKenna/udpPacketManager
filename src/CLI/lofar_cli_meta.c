#include "lofar_cli_meta.h"


const char exitReasons[8][DEF_STR_LEN] = { "", "",
									"Reached the packet limit set at start-up",
									"Read less data than requested from file (near EOF or disk error)",
									"Metadata failed to write",
									"Output data failed to write",
									"Failed to re-initialise reader for new event",
									"Failed to open new file",
};

void sharedFlags(void) {

	printf("-i: <format>	Input file name format\n");
	printf("-o: <format>	Output file name format\n");
	printf("-f:		        Append output files if they already exist (default: Exit if exists)\n");
	printf("-I: <str>		Input metadata file (default: '')\n");
	printf("-c:		        Calibrate the data using the provided metadata.\n");
	printf("-M: <str>		Override output metadata format\n");
	printf("-m: <numPack>	Number of packets to process in each read request (default: 65536)\n");
	printf("-u: <numPort>	Number of ports to combine (default: 4)\n");
	printf("-n: <baseNum>	Base value to iterate when choosing ports (default: 0)\n");
	printf("-b: <lo>,<hi>	Beamlets to extract from the input dataset. Lo is inclusive, hi is exclusive ( eg. 0,300 will return 300 beamlets, 0:299). (default: 0,0 === all)\n");
	printf("-t: <timeStr>	String of the time of the first requested packet, format YYYY-MM-DDTHH:mm:ss (default: '')\n");
	printf("-s: <numSec>	Maximum number of seconds of raw data to extract/process (default: all)\n");
	printf("-S: <iters>     Break into a new file every N given iterations (default: infinite, never break)\n");
	printf("-r:		        Replay the previous packet when a dropped packet is detected (default: pad with 0 values)\n");
	printf("-T: <threads>	OpenMP Threads to use during processing (8+ highly recommended, default: %d)\n", OMP_THREADS);

	printf("-q:		        Enable silent mode for the CLI, don't print any information outside of library error messages (default: False)\n");
	VERBOSE(printf("-v:		Enable verbose output (default: False)\n");
		        printf("-V:		Enable highly verbose output (default: False)\n"));
}

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

int checkOpt(int opt, char* inp, char* endPtr) {
	if (inp != endPtr && *(endPtr) == '\0') {
		return 0;
	}

	fprintf(stderr, "ERROR: Failed to parse flag %c with value %s (errno %d: %s), exiting.\n", opt, inp, errno, strerror(errno));
	return 1;
}

/**
 * Copyright (C) 2023 David McKenna
 * This file is part of udpPacketManager <https://github.com/David-McKenna/udpPacketManager>.
 *
 * udpPacketManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * udpPacketManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with udpPacketManager.  If not, see <http://www.gnu.org/licenses/>.
 **/
