#include "lofar_udp_misc.h"

const float clock200MHzSteps = CLOCK200MHZ;
const float clock160MHzSteps = CLOCK160MHZ;
const float clockStepsDelta = CLOCK200MHZ - CLOCK160MHZ;

const double clock200MHzSample = 1.0 / CLOCK200MHZ;
const double clock160MHzSample = 1.0 / CLOCK160MHZ;


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

	ts.c[0] = inputData[8]; ts.c[1] = inputData[9]; ts.c[2] = inputData[10]; ts.c[3] = inputData[11];
	seq.c[0] = inputData[12]; seq.c[1] = inputData[13]; seq.c[2] = inputData[14]; seq.c[3] = inputData[15];

	//VERBOSE(printf("Packet search: %d %d %d\n", ts.ui, seq.ui, ((lofar_source_bytes*) &(inputData[1]))->clockBit));
	return beamformed_packno(ts.ui, seq.ui, ((lofar_source_bytes*) &(inputData[1]))->clockBit);
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

	ts.c[0] = inputData[8]; ts.c[1] = inputData[9]; ts.c[2] = inputData[10]; ts.c[3] = inputData[11];
	seq.c[0] = inputData[12]; seq.c[1] = inputData[13]; seq.c[2] = inputData[14]; seq.c[3] = inputData[15];

	return (unsigned int) ((16 * (beamformed_packno(ts.ui, seq.ui, ((lofar_source_bytes*) &(inputData[1]))->clockBit) + 1)) - (ts.ui*1000000l*200+512)/1024);
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

	ts.c[0] = inputData[8]; ts.c[1] = inputData[9]; ts.c[2] = inputData[10]; ts.c[3] = inputData[11];
	seq.c[0] = inputData[12]; seq.c[1] = inputData[13]; seq.c[2] = inputData[14]; seq.c[3] = inputData[15];


	return (double) ts.ui + ((double) seq.ui / (clock160MHzSteps + clockStepsDelta * ((lofar_source_bytes*) &(inputData[1]))->clockBit));
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


/*
const char stationCodes[] = {
	"CS001", "CS002", "CS003", "CS004", "CS005", "CS006", "CS007", "CS011", "CS013", 
	"CS017", "CS021", "CS024", "CS026", "CS028", "CS030", "CS031", "CS032", "CS101", 
	"CS103", "RS106", "CS201", "RS205", "RS208", "RS210", "CS301", "CS302", "RS305", 
	"RS306", "RS307", "RS310", "CS401", "RS406", "RS407", "RS409", "CS501", "RS503", 
	"RS508", "RS509", "DE601", "DE602", "DE603", "DE604", "DE605", "FR606", "SE607", 
	"UK608", "DE609", "PL610", "PL611", "PL612", "IE613", "LV614"
};
*/

/**
 * @brief      Conver the station ID to the station code
 *
 * @param[in]  stationID    The station id
 * @param      stationCode  The output station code (min size: 5 bytes)
 *
 * @return     0: Success, 1: Failure
 */
int lofar_get_station_code(int stationID, char *stationCode) {

	switch (stationID) {
		// Core Stations
		case 1 ... 7:
		case 11:
		case 13:
		case 17:
		case 21:
		case 24:
		case 26:
		case 28:
		case 30:
		case 31:
		case 32:
		case 101:
		case 103:
		case 201:
		case 301 ... 302:
		case 401:
		case 501:

			sprintf(stationCode, "CS%03d", stationID);
			break;

		// Remote Stations
		case 106:
		case 205:
		case 208:
		case 210:
		case 305 ... 307:
		case 310:
		case 406 ... 407:
		case 409:
		case 503:
		case 508 ... 509:
			sprintf(stationCode, "RS%03d", stationID);
			break;


		// Intl Stations
		// DE
		case 601 ... 605:
		case 609:
			sprintf(stationCode, "DE%03d", stationID);
			break;

		// FR
		case 606:
			sprintf(stationCode, "FR%03d", stationID);
			break;

		// SE
		case 607:
			sprintf(stationCode, "SE%03d", stationID);
			break;

		// UK
		case 608:
			sprintf(stationCode, "UK%03d", stationID);
			break;

		// PL
		case 610 ... 612:
			sprintf(stationCode, "PL%03d", stationID);
			break;

		// IE
		case 613:
			sprintf(stationCode, "IE%03d", stationID);
			break;

		// LV
		case 614:
			sprintf(stationCode, "LV%03d", stationID);
			break;

		default:
			fprintf("Unknown telescope ID %d. Was a new station added to the array? Update lofar_udp_misc.c\n", stationID);
			return 1;
			break;
	}

	return 0;
}