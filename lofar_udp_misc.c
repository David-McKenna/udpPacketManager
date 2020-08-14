#include "lofar_udp_misc.h"

const float clock200MHzSteps = CLOCK200MHZ;
const float clock160MHzSteps = CLOCK160MHZ;
const float clockStepsDelta = CLOCK200MHZ - CLOCK160MHZ;

// Taken from Olaf Wucknitz' VBLI recorder, with modifiedcations for aribtrary input data
long beamformed_packno(unsigned int timestamp, unsigned int sequence, unsigned int clock200MHz) {
 	//VERBOSE(printf("Packetno: %d, %d, %d\n", timestamp, sequence, clock200MHz););
	return ((timestamp*1000000l*(160+40*clock200MHz)+512)/1024+sequence)/16;
}



/**
 * @brief      Get the packet number corresponding to the data of an input
 *             packet
 *
 * @param      inputData  The input data pointer
 *
 * @return     The packet number
 */
long lofar_get_packet_number(char *inputData) {
	union char_unsigned_int ts;
	union char_unsigned_int seq;

	ts.c[0] = inputData[UDPHDROFFSET + 8]; ts.c[1] = inputData[UDPHDROFFSET + 9]; ts.c[2] = inputData[UDPHDROFFSET + 10]; ts.c[3] = inputData[UDPHDROFFSET + 11];
	seq.c[0] = inputData[UDPHDROFFSET + 12]; seq.c[1] = inputData[UDPHDROFFSET + 13]; seq.c[2] = inputData[UDPHDROFFSET + 14]; seq.c[3] = inputData[UDPHDROFFSET + 15];

	//VERBOSE(printf("Packet search: %d %d %d\n", ts.ui, seq.ui, ((lofar_source_bytes*) &(inputData[1]))->clockBit));
	return beamformed_packno(ts.ui, seq.ui, ((lofar_source_bytes*) &(inputData[UDPHDROFFSET + 1]))->clockBit);
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
unsigned int lofar_get_next_packet_sequence(char *inputData) {
	union char_unsigned_int ts;
	union char_unsigned_int seq;

	ts.c[0] = inputData[UDPHDROFFSET + 8]; ts.c[1] = inputData[UDPHDROFFSET + 9]; ts.c[2] = inputData[UDPHDROFFSET + 10]; ts.c[3] = inputData[UDPHDROFFSET + 11];
	seq.c[0] = inputData[UDPHDROFFSET + 12]; seq.c[1] = inputData[UDPHDROFFSET + 13]; seq.c[2] = inputData[UDPHDROFFSET + 14]; seq.c[3] = inputData[UDPHDROFFSET + 15];

	return (unsigned int) ((16 * (beamformed_packno(ts.ui, seq.ui, ((lofar_source_bytes*) &(inputData[UDPHDROFFSET + 1]))->clockBit) + 1)) - (ts.ui*1000000l*200+512)/1024);
}

/**
 * @brief      The the number of packet sbetween a Unix timestamp and a given
 *             packet number
 *
 * @param[in]  ts            The reference unix time
 * @param[in]  packetNumber  The reference packet number
 * @param[in]  clock200MHz   bool: 0 for 160MHz clock, 1 for 200MHz clock
 *
 * @return     Packet delta
 */
long lofar_get_packet_difference(unsigned int ts, long packetNumber, unsigned int clock200MHz) {

	return beamformed_packno(ts, 0, clock200MHz) - packetNumber;

}

/**
 * @brief      Get the unix timestamp for a given packet
 *
 * @param      inputData  The input data pointer
 *
 * @return     Unix time double
 */
double lofar_get_packet_time(char *inputData) {
	union char_unsigned_int ts;
	union char_unsigned_int seq;

	ts.c[0] = inputData[UDPHDROFFSET + 8]; ts.c[1] = inputData[UDPHDROFFSET + 9]; ts.c[2] = inputData[UDPHDROFFSET + 10]; ts.c[3] = inputData[UDPHDROFFSET + 11];
	seq.c[0] = inputData[UDPHDROFFSET + 12]; seq.c[1] = inputData[UDPHDROFFSET + 13]; seq.c[2] = inputData[UDPHDROFFSET + 14]; seq.c[3] = inputData[UDPHDROFFSET + 15];


	return (double) ts.ui + ((double) seq.ui / (clock160MHzSteps + clockStepsDelta * ((lofar_source_bytes*) &(inputData[UDPHDROFFSET + 1]))->clockBit));
}

/**
 * @brief      Get the MJD time for a given packet
 *
 * @param      inputData  The input data pointer
 *
 * @return     MJD double
 */
double lofar_get_packet_time_mjd(char *inputData) {
	double unixTime = lofar_get_packet_time(inputData);

	return (unixTime / 86400.0) + 40587.0;
}