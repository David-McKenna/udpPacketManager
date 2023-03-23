#ifndef __LOFAR_SAMPLING_CONSTS
#define __LOFAR_SAMPLING_CONSTS
extern const double clock200MHzSteps;
extern const double clock160MHzSteps;
extern const double clockStepsDelta;

// Time steps per second in each clock mode
extern const double clock200MHzSampleTime;
extern const double clock160MHzSampleTime;

// Beam samples per second in each clock mode
extern const double clock200MHzSampleTime;
extern const double clock160MHzSampleTime;

// Packets per second in each clock mode
extern const double clock200MHzPacketRate;
extern const double clock160MHzPacketRate;
#endif // End of __LOFAR_SAMPLING_CONSTS


#ifndef LOFAR_UDP_TIME_H
#define LOFAR_UDP_TIME_H

// Needs to be included before time?
#include <unistd.h>

// XOPEN -> strptime requirement
#define __USE_XOPEN 1
#include <time.h>

#include "lofar_udp_structs.h"


#ifdef __cplusplus
extern "C" {
#endif

long lofar_udp_time_get_packet_from_isot(const char *inputTime, const uint32_t clock200MHz);
long lofar_udp_time_get_packets_from_seconds(double seconds, const uint32_t clock200MHz);
void lofar_udp_time_get_current_isot(const lofar_udp_reader *reader, char *stringBuff, int strlen);
void lofar_udp_time_get_daq(const lofar_udp_reader *reader, char *stringBuff, int strlen);
double lofar_udp_time_get_packet_time(const int8_t *inputData);
double lofar_udp_time_get_packet_time_mjd(const int8_t *inputData);
inline int64_t lofar_udp_time_beamformed_packno(uint32_t timestamp, uint32_t sequence, uint32_t clock200MHz);
inline long lofar_udp_time_get_packet_number(const int8_t *inputData);
inline uint32_t lofar_udp_time_get_next_packet_sequence(const int8_t *inputData);


// Define inlines in the header

// Taken from Olaf Wucknitz' VLBI recorder, with modifications for arbitrary
// input data
//
// @param[in]  timestamp    The timestamp
// @param[in]  sequence     The sequence
// @param[in]  clock200MHz  The clock 200 m hz
//
// @return     { description_of_the_return_value }
//
inline int64_t lofar_udp_time_beamformed_packno(uint32_t timestamp, uint32_t sequence, uint32_t clock200MHz) {
	// VERBOSE(printf("Packetno: %d, %d, %d\n", timestamp, sequence,
	// clock200MHz););
	return ((timestamp * 1000000l * (160 + 40 * clock200MHz) + 512) / 1024 + sequence) / 16;
}


/**
 * @brief      Get the packet number corresponding to the data of an input
 *             packet
 *
 * @param      inputData  The input data pointer
 *
 * @return     The packet number
 */
inline long lofar_udp_time_get_packet_number(const int8_t *inputData) {
	return lofar_udp_time_beamformed_packno(*((uint32_t *) &(inputData[CEP_HDR_TIME_OFFSET])),
	                                        *((uint32_t *) &(inputData[CEP_HDR_SEQ_OFFSET])),
	                                        ((lofar_source_bytes *) &(inputData[CEP_HDR_SRC_OFFSET]))->clockBit);
}

/**
 * @brief      Get the sequence value corresponding to the next packet
 *             (overflows on end of time samples rather than incrementing the
 *             time sample)
 *
 * @param      inputData  The input data pointer
 *
 * @return     The suggested sequence value
 */
inline uint32_t lofar_udp_time_get_next_packet_sequence(const int8_t *inputData) {
	return (uint32_t) (*((int32_t *) &(inputData[CEP_HDR_SEQ_OFFSET])) + 16);

}

#ifdef __cplusplus
};
#endif

#endif // End of LOFAR_UDP_TIME_H
