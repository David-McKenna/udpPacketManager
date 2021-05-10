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

long lofar_udp_time_get_packet_from_isot(const char *inputTime, const unsigned int clock200MHz);
long lofar_udp_time_get_packets_from_seconds(double seconds, const unsigned int clock200MHz);
void lofar_udp_time_get_current_isot_offset(const lofar_udp_reader *reader, char *stringBuff, double offsetSeconds);
void lofar_udp_time_get_current_isot(const lofar_udp_reader *reader, char *stringBuff);
void lofar_udp_time_get_daq(const lofar_udp_reader *reader, char *stringBuff);
double lofar_udp_time_get_packet_time(const char *inputData);
double lofar_udp_time_get_packet_time_mjd(const char *inputData);
inline long lofar_udp_time_beamformed_packno(unsigned int timestamp, unsigned int sequence, unsigned int clock200MHz);
inline long lofar_udp_time_get_packet_number(const char *inputData);
inline unsigned int lofar_udp_time_get_next_packet_sequence(const char *inputData);


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
inline long lofar_udp_time_beamformed_packno(unsigned int timestamp, unsigned int sequence, unsigned int clock200MHz) {
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
inline long lofar_udp_time_get_packet_number(const char *inputData) {
	return lofar_udp_time_beamformed_packno(*((unsigned int *) &(inputData[CEP_HDR_TIME_OFFSET])),
	                                        *((unsigned int *) &(inputData[CEP_HDR_SEQ_OFFSET])),
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
inline unsigned int lofar_udp_time_get_next_packet_sequence(const char *inputData) {
	return (unsigned int) ((16 * \
            (lofar_udp_time_beamformed_packno(*((unsigned int *) &(inputData[CEP_HDR_TIME_OFFSET])),
	                                          *((unsigned int *) &(inputData[CEP_HDR_SEQ_OFFSET])),
	                                          ((lofar_source_bytes *) &(inputData[CEP_HDR_SRC_OFFSET]))->clockBit) + 1))
	                       - (*((unsigned int *) &(inputData[CEP_HDR_TIME_OFFSET])) * 1000000l * 200 + 512) / 1024);
}

#ifdef __cplusplus
};
#endif

#endif // End of LOFAR_UDP_TIME_H
