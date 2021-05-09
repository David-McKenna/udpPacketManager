#include "lofar_udp_time.h"

const double clock200MHzSteps = CLOCK200MHZ;
const double clock160MHzSteps = CLOCK160MHZ;
const double clockStepsDelta = CLOCK200MHZ - CLOCK160MHZ;

const double clock200MHzSampleRate = 1.0 / CLOCK200MHZ;
const double clock160MHzSampleRate = 1.0 / CLOCK160MHZ;

// 1024 voltage samples per beam sample
const double clock200MHzSampleTime = 1024.0 / CLOCK200MHZ;
const double clock160MHzSampleTime = 1024.0 / CLOCK160MHZ;


const double clock200MHzPacketRate = CLOCK200MHZ / 16;
const double clock160MHzPacketRate = CLOCK160MHZ / 16;

/**
 * @brief      Gets the starting packet for a given Unix time
 *
 * @param      inputTime    The input time (ISO format string YYYY-MM-DDTHH:MM:SS)
 * @param[in]  clock200MHz  bool: 0 for 160MHz clock, 1 for 200Mhz clock
 *
 * @return     The starting packet.
 */
long lofar_udp_time_get_packet_from_isot(const char *inputTime, const unsigned int clock200MHz) {
	struct tm unixTm;
	time_t unixEpoch;

	if (strptime(inputTime, "%Y-%m-%dT%H:%M:%S", &unixTm) != NULL) {
		unixEpoch = timegm(&unixTm);
		return lofar_udp_time_beamformed_packno(unixEpoch, 0, clock200MHz);
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
long lofar_udp_time_get_packets_from_seconds(double seconds, const unsigned int clock200MHz) {
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
 *  TODO: Use a dfferent input, this does not account for packet loss.
 *
 * @param      reader      The UDP reader
 * @param      stringBuff  The string buffer
 */
void lofar_udp_time_get_current_isot_offset(const lofar_udp_reader *reader, char *stringBuff, double offsetSeconds) {
	double startTime;
	time_t startTimeUnix;
	struct tm *startTimeStruct;

	startTime = lofar_udp_time_get_packet_time(reader->meta->inputData[0]);
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
void lofar_udp_time_get_current_isot(const lofar_udp_reader *reader, char *stringBuff) {
	lofar_udp_time_get_current_isot_offset(reader, stringBuff, 0.0);
}
/**
 * @brief      Get the unix timestamp for a given packet
 *
 * @param      inputData  The input data pointer
 *
 * @return     Unix time double
 */
double lofar_udp_time_get_packet_time(const char *inputData) {
	return (double) *((unsigned int *) &(inputData[CEP_HDR_TIME_OFFSET])) +
		   ((double) *((unsigned int *) &(inputData[CEP_HDR_SEQ_OFFSET])) /
			(clock160MHzSteps + clockStepsDelta * ((lofar_source_bytes *) &(inputData[CEP_HDR_SRC_OFFSET]))->clockBit));
}
/**
 * @brief      Get the MJD time for a given packet
 *
 * @param      inputData  The input data pointer
 *
 * @return     MJD double
 */
double lofar_udp_time_get_packet_time_mjd(const char *inputData) {
	double unixTime = lofar_udp_time_get_packet_time(inputData);

	return (unixTime / 86400.0) + 40587.0;
}


/**
 * @brief      The the number of packets between a Unix timestamp and a given
 *             packet number
 *
 * @param[in]  ts            The reference unix time
 * @param[in]  packetNumber  The reference packet number
 * @param[in]  clock200MHz   bool: 0 for 160MHz clock, 1 for 200MHz clock
 *
 * @return     Packet delta
 */
long lofar_get_packet_difference(unsigned int ts, long packetNumber, unsigned int clock200MHz) {
	return lofar_udp_time_beamformed_packno(ts, 0, clock200MHz) - packetNumber;
}

/**
 * @brief      Emulate the GUPPI DAQ time string from the current data timestamp
 *
 * @param      reader      The lofar_udp_reader
 * @param      stringBuff  The output time string buffer
 */
void lofar_udp_time_get_daq(const lofar_udp_reader *reader, char *stringBuff) {
	double startTime;
	time_t startTimeUnix;
	struct tm *startTimeStruct;

	startTime = lofar_udp_time_get_packet_time(reader->meta->inputData[0]);
	startTimeUnix = (unsigned int) startTime;
	startTimeStruct = gmtime(&startTimeUnix);

	strftime(stringBuff, 24, "%a %b %e %H:%M:%S %Y", startTimeStruct);
}