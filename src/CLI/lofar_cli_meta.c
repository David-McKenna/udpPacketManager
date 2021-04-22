#include "lofar_cli_meta.h"


const char exitReasons[4][1024] = { "", "",
									"Reached the packet limit set at start-up",
									"Read less data than requested from file (near EOF or disk error)" };

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


/**
 * @brief      Gets the starting packet for a given Unix time
 *
 * @param      inputTime    The input time (ISO format string YYYY-MM-DDTHH:MM:SS)
 * @param[in]  clock200MHz  bool: 0 for 160MHz clock, 1 for 200Mhz clock
 *
 * @return     The starting packet.
 */
long getStartingPacket(char inputTime[], const unsigned int clock200MHz) {
	struct tm unixTm;
	time_t unixEpoch;

	if (strptime(inputTime, "%Y-%m-%dT%H:%M:%S", &unixTm) != NULL) {
		unixEpoch = timegm(&unixTm);
		return beamformed_packno(unixEpoch, 0, clock200MHz);
	} else {
		fprintf(stderr, "Invalid time string, %s, exiting.\n", inputTime);

		return 1;
	}

}

/**
 * @brief      Gets the number of packets generated in a given amount of seconds
 *
 * @param[in]  seconds      The amount of seconds to use
 * @param[in]  clock200MHz  bool: 0 for 160MHz clock, 1 for 200Mhz clock
 *
 * @return     The number of packets generated
 */
long getSecondsToPacket(double seconds, const unsigned int clock200MHz) {
	return (long) (seconds * (clock200MHz * clock200MHzSteps + (float) (1 - clock200MHz) * clock160MHzSteps) /
				   (float) UDPNTIMESLICE);
}

/**
 * @brief      (WARNING:CURRENTLY BROKEN?) Calculate the number of seconds a given amount of packets cover
 *
 * @param[in]  packetCount  The amount of packets
 * @param[in]  clock200MHz  bool: 0 for 160MHz clock, 1 for 200Mhz clock
 *
 * @return     The amount of seconds passed

float getPacketsToSeconds(long packetCount, const int clock200MHz) {
	return (float) packetCount / ((clock200MHz * clock200MHzSteps + (1 - clock200MHz) * clock160MHzSteps)) * 16;
}
*/

/**
 * @brief      Convert the current packet to an ISOT string
 *
 * @param      reader      The UDP reader
 * @param      stringBuff  The string buffer
 */
void getStartTimeStringOffset(lofar_udp_reader *reader, char stringBuff[], int offsetSeconds) {
	double startTime;
	time_t startTimeUnix;
	struct tm *startTimeStruct;

	startTime = lofar_get_packet_time(reader->meta->inputData[0]);
	startTimeUnix = (unsigned int) (startTime + offsetSeconds);
	startTimeStruct = gmtime(&startTimeUnix);

	char localBuff[32];
	strftime(localBuff, sizeof(localBuff), "%Y-%m-%dT%H:%M:%S", startTimeStruct);
	sprintf(stringBuff, "%s.%06d", localBuff, (int) ((startTime - startTimeUnix) * 1e6));
}

/**
 * @brief      Convert the current packet to an ISOT string
 *
 * @param      reader      The UDP reader
 * @param      stringBuff  The string buffer
 */
void getStartTimeString(lofar_udp_reader *reader, char stringBuff[]) {
	getStartTimeStringOffset(reader, stringBuff, 0);
}