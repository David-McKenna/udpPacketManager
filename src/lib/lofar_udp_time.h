#ifndef LOFAR_SAMPLING_CONSTS
#define LOFAR_SAMPLING_CONSTS
// Time steps per second in each clock mode
extern const double clock200MHzSteps;
extern const double clock160MHzSteps;


// Beam samples per second in each clock mode
extern const double clock200MHzSampleTime;
extern const double clock160MHzSampleTime;

// Packets per second in each clock mode
extern const double clock200MHzPacketRate;
extern const double clock160MHzPacketRate;

// Difference of values; useful for non-branching calculations
extern const double clockStepsDelta;
extern const double clockSampleTimeDelta;
extern const double clockPacketRateDelta;

#endif // End of LOFAR_SAMPLING_CONSTS


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

int64_t lofar_udp_time_get_packet_from_isot(const char *inputTime, uint8_t clock200MHz);
int64_t lofar_udp_time_get_packets_from_seconds(double seconds, uint32_t clock200MHz);
void lofar_udp_time_get_current_isot(const lofar_udp_reader *reader, char *stringBuff, int strlen);
void lofar_udp_time_get_daq(const lofar_udp_reader *reader, char *stringBuff, int strlen);
double lofar_udp_time_get_packet_time(const int8_t *inputData);
double lofar_udp_time_get_packet_time_mjd(const int8_t *inputData);
static inline int64_t lofar_udp_time_beamformed_packno(int32_t timestamp, int32_t sequence, uint8_t clock200MHz);
static inline int64_t lofar_udp_time_get_packet_number(const int8_t *inputData);
static inline int32_t lofar_udp_time_get_next_packet_sequence(const int8_t *inputData);

// Taken from Olaf Wucknitz VLBI recorder, with modifications for arbitrary
// input data
//
// @param[in]  timestamp    The timestamp
// @param[in]  sequence     The sequence
// @param[in]  clock200MHz  The clock 200 m hz
//
// @return     { description_of_the_return_value }
//
static inline int64_t lofar_udp_time_beamformed_packno(int32_t timestamp, int32_t sequence, uint8_t clock200MHz) {
	//
	//      ((timestamp \                      // Unix Epoch time reference
	//          * 1000000l \                   // 1e6 To convert next line from MHz to Hz
	//          * (160 + 40 * clock200MHz) \   // Clock Rate, samples per second
	//          + 512) \                       // Offset halfway into the time sample (1024 samples per PFB output)
	//                                         // This could be swapped for a (sequence + 0.5), but would introduce FP uncertainty
	//                                         // Since the 200MHz samples/s is not an integer, this value isn't always rounded down
	//       ) / 1024                          // Collapse down to 1 PFB output
	//                                         // We now have the sample number that starts on the given second with sequence 0
	//       + sequence)                       // Offset to the true sequence
	//    / 16                                 // Divide by the number of samples per packet to get a packet number

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
static inline int64_t lofar_udp_time_get_packet_number(const int8_t *inputData) {
	return lofar_udp_time_beamformed_packno(*((int32_t *) &(inputData[CEP_HDR_TIME_OFFSET])),
	                                        *((int32_t *) &(inputData[CEP_HDR_SEQ_OFFSET])),
	                                        ((lofar_source_bytes *) &(inputData[CEP_HDR_SRC_OFFSET]))->clockBit);
}

/**
 * @brief      Get the sequence value corresponding to the next packet
 *             (overflows on end of time samples rather than incrementing the
 *             time sample, it would require a lot more effort to estimate the
 *             next true sequence value due to the 200MHz clock's fractional
 *             sequence cap)
 *
 * @param      inputData  The input data pointer
 *
 * @return     The suggested sequence value
 */
static inline int32_t lofar_udp_time_get_next_packet_sequence(const int8_t *inputData) {
	return (*((int32_t *) &(inputData[CEP_HDR_SEQ_OFFSET])) + 16);
}

#ifdef __cplusplus
};
#endif

#endif // End of LOFAR_UDP_TIME_H
