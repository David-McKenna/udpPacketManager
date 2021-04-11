#include "lofar_udp_general.h"
#include <ascii_hdr.h>

// gethostname(2)
#include <unistd.h>


typedef struct lofar_udp_metadata {

	char dadaHeader[8192];

	// DADA + DSPSR Defined header values
	double hdr_version;

	char instrument[64];
	char telescope[64];
	char reciever[64];
	char primary[64];
	char hostname[64];
	char file_name[128];
	int file_number;


	char source[64];
	char ra[64];  // J2000
	char dec[64]; // J2000
	char obs_id[64];
	char utc_start[64];
	double mjd_start;
	long obs_offset;
	long obs_overlap;
	char basis[64];
	char mode[64];


	double freq;
	double bw;
	int nchan;
	int npol;
	int nbit;
	// int resolution; // define at dump time
	int ndim;
	double tsamp;
	char state[64];

	// UPM Extra values
	char upm_version;
	char rec_version;
	char upm_proc[64];
	int upm_reader;
	int upm_mode;
	int upm_replay;
	int upm_calibrated;

	int upm_bitmode;
	int upm_rawbeamlets;
	int upm_upperbeam;
	int upm_lowerbeam;
	int upm_clockbit;
	int upm_stn;

} lofar_udp_metadata;
extern const lofar_udp_metadata lofar_udp_metadata_default;