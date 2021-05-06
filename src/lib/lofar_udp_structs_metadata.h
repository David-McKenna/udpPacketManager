#ifndef LOFAR_UDP_STRUCTS_METADATA_H
#define LOFAR_UDP_STRUCTS_METADATA_H

#include "lofar_udp_general.h"

// Define a struct of variables for an ASCII header
//
// We likely don't need half as many keys as provided here, see
// https://github.com/UCBerkeleySETI/rawspec/blob/master/rawspec_rawutils.c#L155
//
// char values are 68(+\0) in length as each entry is 80 chars, each key is up to 8
// chars long, and parsing requires '= ' between the key and value. Two bytes
// are left as a buffer.
//
#define META_STR_LEN 69
// Naming conventions all over the place to match the actual header key names
typedef struct ascii_hdr {
	char src_name[META_STR_LEN + 1];
	char ra_str[META_STR_LEN + 1];
	char dec_str[META_STR_LEN + 1];

	double obsfreq;
	double obsbw;
	double chan_bw;
	int obsnchan;
	int npol;
	int nbits;
	double tbin;

	char fd_poln[META_STR_LEN + 1];
	char trk_mode[META_STR_LEN + 1];
	char obs_mode[META_STR_LEN + 1];
	char cal_mode[META_STR_LEN + 1];
	double scanlen;

	char projid[META_STR_LEN + 1];
	char observer[META_STR_LEN + 1];
	char telescop[META_STR_LEN + 1];
	char frontend[META_STR_LEN + 1];
	char backend[META_STR_LEN + 1];
	char datahost[META_STR_LEN + 1];
	int dataport;
	int overlap;

	long blocsize;
	char daqpulse[META_STR_LEN + 1];
	int stt_imjd;
	int stt_smjd;
	long pktidx;
	char pktfmt[META_STR_LEN + 1];
	double stt_offs;
	int pktsize;
	double dropblk;
	double droptot;

} ascii_hdr;
extern const ascii_hdr ascii_hdr_default;

// Define a struct of variables for a sigproc header
//
// http://sigproc.sourceforge.net/sigproc.pdf
//
// Notable features:
//  - ra=dec values are double, where the digits describe the deg/minute/seconds, following ddmmss.sss
//  - fchannel array support is included in the headed, but not likely to get implemented as nobody supports it
typedef struct sigproc_hdr {
	// Observatory information
	int telescope_id;
	int machine_id;

	// File information
	int data_type;
	char rawdatafile[DEF_STR_LEN];

	// Observation parameters
	char source_name[META_STR_LEN + 1];
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
extern const sigproc_hdr sigproc_hdr_default;


// Define a struct of variables for a DADA header
//
// http://psrdada.sourceforge.net/manuals/Specification.pdf
// http://dspsr.sourceforge.net/manuals/dspsr/dada.shtml
// + our own extras, the spec can be expanded as needed, extra variables are ignored.
//
// The DADA header format is the standard output format, similar to that of the ASCII format but with different keywords and slightly sorter line lengths.
// 64 + \0
typedef struct lofar_udp_metadata {

	metadata_t type;
	int setup;

	// DADA + DSPSR Defined header values
	double hdr_version; // Lib

	char instrument[META_STR_LEN + 1]; // Standard
	char telescope[META_STR_LEN + 1]; // Lib
	int telescope_id; // Lib
	char reciever[META_STR_LEN + 1]; // Lib
	char machine[META_STR_LEN + 1]; // Lib
	char observer[META_STR_LEN + 1]; // External
	char hostname[META_STR_LEN + 1]; // Lib
	int baseport; // Lib
	char rawfile[DEF_STR_LEN];
	char file_name[DEF_STR_LEN]; // Lib
	int file_number; // Lib


	char source[META_STR_LEN + 1]; // External
	char ra[META_STR_LEN + 1]; // beamctl
	char dec[META_STR_LEN + 1]; // beamctl
	double ra_rad; // beamctl
	double dec_rad; // beamctl
	char ra_analog[META_STR_LEN + 1]; // beamctl -- HBA
	char dec_analog[META_STR_LEN + 1]; // beamctl -- HBA
	double ra_rad_analog; // beamctl -- HBA
	double dec_rad_analog; // beamctl -- HBA
	char coord_basis[META_STR_LEN + 1]; // beamctl
	char obs_id[META_STR_LEN + 1]; // External
	char utc_start[META_STR_LEN + 1]; // Lib
	double obs_mjd_start; // Lib
	double block_mjd_start; // Lib
	long obs_offset; // Lib
	long obs_overlap; // Lib
	char basis[META_STR_LEN + 1]; // Standard
	char mode[META_STR_LEN + 1]; // Standard


	double freq; // beamctl
	double bw; // beamctl
	double channel_bw; // beamctl
	double ftop; // beamctl
	double fbottom; // beamctl
	double subbands[MAX_NUM_PORTS * UDPMAXBEAM];
	int nchan; // beamctl / Library?
	int nrcu; // beamctl
	int npol; // Standard
	int nbit; // Lib
	int resolution; // Standard?
	int ndim; // Lib
	double tsamp; // Lib
	char state[META_STR_LEN + 1]; // Lib

	// UPM Extra values
	char upm_version[META_STR_LEN + 1];
	char rec_version[META_STR_LEN + 1];
	char upm_proc[META_STR_LEN + 1];
	char upm_daq[META_STR_LEN + 1];
	char upm_beamctl[4 * DEF_STR_LEN]; // beamctl command(s) can be long...
	int upm_reader;
	int upm_mode;
	int upm_replay;
	int upm_calibrated;
	long upm_processed_packets;
	long upm_dropped_packets;
	long upm_last_dropped_packets;

	int upm_bitmode;
	int upm_rcuclock;
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

#endif // End of LOFAR_UDP_STRUCTS_METADATA_H
