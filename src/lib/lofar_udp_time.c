#include "lofar_udp_time.h"

// Time steps per second in each clock mode
const double clock200MHzSteps = CLOCK200MHZ;
const double clock160MHzSteps = CLOCK160MHZ;


// Beam samples per second in each clock mode
const double clock200MHzSampleTime = 1.0 / CLOCK200MHZ;
const double clock160MHzSampleTime = 1.0 / CLOCK160MHZ;

// Packets per second in each clock mode
const double clock200MHzPacketRate = CLOCK200MHZ / UDPNTIMESLICE;
const double clock160MHzPacketRate = CLOCK160MHZ / UDPNTIMESLICE;

// Thanks GCC 7!
#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ <= 7)
const double clockStepsDelta =  CLOCK200MHZ - CLOCK160MHZ;
const double clockSampleTimeDelta = (1.0/ CLOCK200MHZ) - (1.0 / CLOCK160MHZ);
const double clockPacketRateDelta = (CLOCK200MHZ / UDPNTIMESLICE) - (CLOCK160MHZ / UDPNTIMESLICE);

// Validated to be the same as below in the lib_time_tests.cpp file, can't use static_assert as they "aren't static", same reaason they need to be specially defined...
#else
const double clockStepsDelta = clock200MHzSteps - clock160MHzSteps;
const double clockSampleTimeDelta = clock200MHzSampleTime - clock160MHzSampleTime;
const double clockPacketRateDelta = clock200MHzPacketRate - clock160MHzPacketRate;

#include <assert.h>
static_assert(clockStepsDelta > 0, "Require Step Delta is positive");
static_assert(clockSampleTimeDelta < 0, "Require Sample Time Delta ia negative");
static_assert(clockPacketRateDelta > 0, "Require Packet Rate Delta is positive");
#endif

/**
 * @brief      Gets the starting packet for a given Unix time
 *
 * @param[in]  inputTime    The input time (ISO format string YYYY-MM-DDTHH:MM:SS)
 * @param[in]  clock200MHz  bool: 0 for 160MHz clock, 1 for 200Mhz clock
 *
 * @return     The starting packet
 */
int64_t lofar_udp_time_get_packet_from_isot(const char *inputTime, const uint8_t clock200MHz) {
	struct tm unixTm;
	time_t unixEpoch = 0;

	const char *lastParsedChar = strptime(inputTime, "%Y-%m-%dT%H:%M:%S", &unixTm);
	if ((lastParsedChar != NULL) && (lastParsedChar[0] == '\0')) {
		unixEpoch = timegm(&unixTm);
		if (unixEpoch != -1) {
			return lofar_udp_time_beamformed_packno((int32_t) unixEpoch, 0, clock200MHz); // 2036 bug long -> int narrowed
		}
	}

	fprintf(stderr, "ERROR: Invalid time string, %s / %lu, exiting.\n", inputTime, unixEpoch);
	return 1;

}

/**
 * @brief      Gets the number of packets generated in a given amount of seconds
 *
 * @param[in]  seconds      The amount of seconds to use
 * @param[in]  clock200MHz  bool: 0 for 160MHz clock, 1 for 200Mhz clock
 *
 * @return     The number of packets generated
 */
int64_t lofar_udp_time_get_packets_from_seconds(const double seconds, const uint8_t clock200MHz) {
	return (long) (seconds * (clock160MHzPacketRate + clockPacketRateDelta * clock200MHz));
}


/**
 * @brief      Convert the current packet to an ISOT-style string
 *
 * @param[in]  reader		The active reader
 * @param[out] stringBuff 	The string buffer
 * @param[in]  strlen		The output string buffer length
 */
void lofar_udp_time_get_current_isot(const lofar_udp_reader *reader, char *stringBuff, const int64_t strlen) {
	// Get the current time information
	const double startTime = lofar_udp_time_get_packet_time(reader->meta->inputData[0]);
	const time_t startTimeUnix = (uint32_t) (startTime); // 2036 bug

	struct tm *startTimeStruct;
	startTimeStruct = gmtime(&startTimeUnix);

	// Write the output to a temporary buffer
	char localBuff[32];
	strftime(localBuff, VAR_ARR_SIZE(localBuff), "%Y-%m-%dT%H:%M:%S", startTimeStruct);

	// Re-write with fractional seconds to the given output buffer
	snprintf(stringBuff, strlen,  "%s.%06ld", localBuff, (int64_t) ((startTime - (double) startTimeUnix) * 1e6 + 0.5));
}

/**
 * @brief      Get the unix timestamp for a given packet
 *
 * @param[in]  inputData  The input data pointer
 *
 * @return     Unix time double
 */
double lofar_udp_time_get_packet_time(const int8_t *inputData) {
	return (double) *((uint32_t *) &(inputData[CEP_HDR_TIME_OFFSET])) +
		   ((double) *((int32_t *) &(inputData[CEP_HDR_SEQ_OFFSET])) /
			(clock160MHzSteps + clockStepsDelta * ((lofar_source_bytes *) &(inputData[CEP_HDR_SRC_OFFSET]))->clockBit));
}

/**
 * @brief      Get the MJD time for a given packet
 *
 * WARNING: Does not take into account leap seconds // TODO: Spice?
 *
 * @param[in]  inputData  The input data pointer
 *
 * @return     MJD double
 */
double lofar_udp_time_get_packet_time_mjd(const int8_t *inputData) {
	double unixTime = lofar_udp_time_get_packet_time(inputData);

	// 86400 seconds per day
	// MJD is offset from the unix epoch by 40587 days
	return (unixTime / 86400.0) + 40587.0;
}

/**
 * @brief      Emulate the GUPPI DAQ time string from the current data timestamp
 *
 * "DAQPULSE" key in https://safe.nrao.edu/wiki/pub/Main/JoeBrandt/guppi_status_shmem.pdf
 *
 * @param[in]      reader		The lofar_udp_reader
 * @param[out]     stringBuff	The output time string buffer
 * @param[in]      strlen		The output string buffer length
 */
void lofar_udp_time_get_daq(const lofar_udp_reader *reader, char *stringBuff, const int32_t strlen) {
	// Get the current time information
	const double startTime = lofar_udp_time_get_packet_time(reader->meta->inputData[0]);
	const time_t startTimeUnix = (unsigned int) startTime;

	struct tm *startTimeStruct;
	startTimeStruct = gmtime(&startTimeUnix);

	// Write the time to the output buffer in the DAQ format
	strftime(stringBuff, strlen, "%a %b %e %H:%M:%S %Y", startTimeStruct);
}