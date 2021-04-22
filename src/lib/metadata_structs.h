#ifndef METADATA_STRUCTS_H
#define METADATA_STRUCTS_H

#include "lofar_udp_general.h"

// Define a struct of variables for a fake ASCII header
//
// We likely don't need half as many keys as provided here, see
// https://github.com/UCBerkeleySETI/rawspec/blob/master/rawspec_rawutils.c#L155
//
// char values are 68(+\0) in length as each entry is 80 chars, each key is up to 8
// chars long, and parsing requires '= ' between the key and value. Two bytes
// are left as a buffer.
//
#define ASCII_HDR_STR_LEN 69
// Naming conventions all over the place to match the actual header key names
// @formatter:off
typedef struct ascii_hdr {
	char src_name[ASCII_HDR_STR_LEN];
	char ra_str[ASCII_HDR_STR_LEN];
	char dec_str[ASCII_HDR_STR_LEN];

	double obsfreq;
	double obsbw;
	double chan_bw;
	int obsnchan;
	int npol;
	int nbits;
	double tbin;

	char fd_poln[ASCII_HDR_STR_LEN];
	char trk_mode[ASCII_HDR_STR_LEN];
	char obs_mode[ASCII_HDR_STR_LEN];
	char cal_mode[ASCII_HDR_STR_LEN];
	double scanlen;

	char projid[ASCII_HDR_STR_LEN];
	char observer[ASCII_HDR_STR_LEN];
	char telescop[ASCII_HDR_STR_LEN];
	char frontend[ASCII_HDR_STR_LEN];
	char backend[ASCII_HDR_STR_LEN];
	char datahost[ASCII_HDR_STR_LEN];
	int dataport;
	int overlap;

	long blocsize;
	char daqpulse[ASCII_HDR_STR_LEN];
	int stt_imjd;
	int stt_smjd;
	long pktidx;
	char pktfmt[ASCII_HDR_STR_LEN];
	double stt_offs;
	int pktsize;
	double dropblk;
	double droptot;

} ascii_hdr;

typedef struct sigproc_hdr {
	// Observatory information
	int telescope_id;
	int machine_id;

	// File information
	int data_type;
	char rawdatafile[DEF_STR_LEN];

	// Observation parameters
	char source_name[64];
	int barycentric;
	int pulsarcentric;

	double az_start;
	double za_start;
	double src_raj;
	double src_dej;

	double tstart;
	double tsamp;

	int nbits;
	int nsamples;

	double fch1;
	double foff;
	double *fchannel;
	int nchans;
	int nifs;

	// Pulsar information
	double refdm;
	double period;
} sigproc_hdr;
// @formatter:on


// The DADA header format is the standard output format
// 64 + \0
#define META_STR_LEN 65
typedef struct lofar_udp_metadata {

	// DADA + DSPSR Defined header values
	double hdr_version; // Lib

	char instrument[META_STR_LEN]; // Standard
	char telescope[META_STR_LEN]; // Lib
	char reciever[META_STR_LEN]; // Lib
	char primary[META_STR_LEN]; // Lib
	char hostname[META_STR_LEN]; // Lib
	char file_name[DEF_STR_LEN]; // Lib
	int file_number; // Lib


	char source[META_STR_LEN]; // External
	char ra[META_STR_LEN]; // beamctl
	char dec[META_STR_LEN]; // beamctl
	double ra_rad; // beamctl
	double dec_rad; // beamctl
	char coord_basis[META_STR_LEN]; // beamctl
	char obs_id[META_STR_LEN]; // External
	char utc_start[META_STR_LEN]; // Lib
	double mjd_start; // Lib
	long obs_offset; // Lib
	long obs_overlap; // Lib
	char basis[META_STR_LEN]; // Standard
	char mode[META_STR_LEN]; // Standard


	double freq; // beamctl
	double bw; // beamctl
	int nchan; // beamctl
	int npol; // Standard
	int nbit; // Lib
	int resolution; // Standard?
	int ndim; // Lib
	double tsamp; // Lib
	char state[META_STR_LEN]; // Lib

	// UPM Extra values
	char upm_version[META_STR_LEN];
	char rec_version[META_STR_LEN];
	char upm_proc[META_STR_LEN];
	char upm_daq[META_STR_LEN];
	char upm_beamctl[4 * DEF_STR_LEN]; // beamctl command(s) can be long
	int upm_reader;
	int upm_mode;
	int upm_replay;
	int upm_calibrated;

	int upm_bitmode;
	int upm_rawbeamlets;
	int upm_upperbeam;
	int upm_lowerbeam;

	size_t outputSize;
	struct output {
		ascii_hdr *ascii;
		sigproc_hdr *sigproc;
	} output;

} lofar_udp_metadata;
extern const lofar_udp_metadata lofar_udp_metadata_default;

#endif //End of METADATA_STRUCTS_H
