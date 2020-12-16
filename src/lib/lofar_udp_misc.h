// Normal includes
#include <stdio.h>

#include "lofar_udp_general.h"

#ifndef __LOFAR_SAMPLING_CONSTS
#define __LOFAR_SAMPLING_CONSTS
extern const float clock200MHzSteps;
extern const float clock160MHzSteps;
extern const float clockStepsDelta;

// Time steps per second in each clock mode
extern const double clock200MHzSample;
extern const double clock160MHzSample;
#endif



#ifndef __LOFAR_UDP_MISC_H
#define __LOFAR_UDP_MISC_H
#ifdef __cplusplus
extern "C" {
#endif

extern inline long beamformed_packno(unsigned int timestamp, unsigned int sequence, unsigned int clock200MHz);
extern inline long lofar_get_packet_number(char *inputData);
unsigned int lofar_get_next_packet_sequence(char *inputData);
double lofar_get_packet_time(char *inputData);
double lofar_get_packet_time_mjd(char *inputData);
int lofar_get_station_name(int stationID, char *stationCode);

#ifdef __cplusplus
}
#endif
#endif