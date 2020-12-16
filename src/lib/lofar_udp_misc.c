#include "lofar_udp_misc.h"

const float clock200MHzSteps = CLOCK200MHZ;
const float clock160MHzSteps = CLOCK160MHZ;
const float clockStepsDelta = CLOCK200MHZ - CLOCK160MHZ;

const double clock200MHzSample = 1.0 / CLOCK200MHZ;
const double clock160MHzSample = 1.0 / CLOCK160MHZ;


// Taken from Olaf Wucknitz' VBLI recorder, with modifiedcations for aribtrary input data
inline long beamformed_packno(unsigned int timestamp, unsigned int sequence, unsigned int clock200MHz) {
 	//VERBOSE(printf("Packetno: %d, %d, %d\n", timestamp, sequence, clock200MHz););
	return ((timestamp*1000000l*(160+40*clock200MHz)+512)/1024+sequence)/16;
}


// Shorthand note: 
//*((unsigned int*) &(inputData[8])) == unsigned int at 8 bytes from the header offset (packet number)
//*((unsigned int*) &(inputData[12])) == unsigned int at 12 bytes from the header offset (sequence ID)


/**
 * @brief      Get the packet number corresponding to the data of an input
 *             packet
 *
 * @param      inputData  The input data pointer
 *
 * @return     The packet number
 */
inline long lofar_get_packet_number(char *inputData) {
	return beamformed_packno(*((unsigned int*) &(inputData[UDPHDROFF + 8])), *((unsigned int*) &(inputData[UDPHDROFF + 12])), ((lofar_source_bytes*) &(inputData[UDPHDROFF + 1]))->clockBit);
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
	return (unsigned int) ((16 * \
			(beamformed_packno(*((unsigned int*) &(inputData[UDPHDROFF + 8])), *((unsigned int*) &(inputData[UDPHDROFF + 12])), ((lofar_source_bytes*) &(inputData[UDPHDROFF + 1]))->clockBit) + 1)) 
			- (*((unsigned int*) &(inputData[UDPHDROFF + 8]))*1000000l*200+512)/1024);
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
	return (double) *((unsigned int*) &(inputData[UDPHDROFF + 8])) + ((double) *((unsigned int*) &(inputData[UDPHDROFF + 12])) / (clock160MHzSteps + clockStepsDelta * ((lofar_source_bytes*) &(inputData[UDPHDROFF + 1]))->clockBit));
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


/**
 * @brief      Convert the station ID to the station code
 * 				RSP station ID != intll station ID. See
 * 				https://git.astron.nl/ro/lofar/-/raw/master/MAC/Deployment/data/StaticMetaData/StationInfo.dat
 * 				RSP hdr byte 4 reports 32 * ID code in result above. Divide by 32 on any port to round down to target code.
 *
 * @param[in]  stationID    The station id
 * @param      stationCode  The output station code (min size: 5 bytes)
 *
 * @return     0: Success, 1: Failure
 */
int lofar_get_station_name(int stationID, char *stationCode) {

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
			sprintf(stationCode, "CS%03d", stationID);
			break;

		case 121:
			sprintf(stationCode, "CS201");
			break;

		case 141 ... 142:
			sprintf(stationCode, "CS%03d", 301 + (stationID % 141));
			break;

		case 161:
			sprintf(stationCode, "CS401");
			break;

		case 181:
			sprintf(stationCode, "CS501");
			break;


		// Remote Stations
		case 106:
			sprintf(stationCode, "RS%03d", stationID);
			break;

		case 125:
		case 128:
		case 130:
			sprintf(stationCode, "RS%03d", 205 + (stationID % 125));
			break;

		case 145 ... 147:
		case 150:
			sprintf(stationCode, "RS%03d", 305 + (stationID % 145));
			break;

		case 166 ... 167:
		case 169:
			sprintf(stationCode, "RS%03d", 406 + (stationID % 166));
			break;

		case 183:
		case 188 ... 189:
			sprintf(stationCode, "RS%03d", 503 + (stationID % 183));
			break;

			break;


		// Intl Stations
		// DE
		case 201 ... 205:
			sprintf(stationCode, "DE%03d", 601 + (stationID % 201));
			break;

		case 210:
			sprintf(stationCode, "DE609");
			break;


		// FR
		case 206:
			sprintf(stationCode, "FR606");
			break;

		// SE
		case 207:
			sprintf(stationCode, "SE207");
			break;

		// UK
		case 208:
			sprintf(stationCode, "UK208");
			break;

		// PL
		case 211 ... 213:
			sprintf(stationCode, "PL%03d", 610 + (stationID % 211));
			break;

		// IE
		case 214:
			sprintf(stationCode, "IE613");
			break;

		// LV
		case 215:
			sprintf(stationCode, "LV614");
			break;

		// KAIRA
		case 901:
			sprintf(stationCode, "FI901");
			break;

		// LOFAR4SW test station
		case 902:
			sprintf(stationCode, "UK902");
			break;

		default:
			fprintf(stderr, "Unknown telescope ID %d. Was a new station added to the array? Update lofar_udp_misc.c\n", stationID);
			return 1;
			break;
	}

	return 0;
}