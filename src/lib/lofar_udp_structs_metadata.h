#ifndef LOFAR_UDP_STRUCTS_METADATA_H
#define LOFAR_UDP_STRUCTS_METADATA_H

#include "lofar_udp_general.h"

typedef enum dataOrder {
	UNKNOWN = 0,
	TIME_MAJOR = 1,
	FREQUENCY_MAJOR = 2,
	PACKET_MAJOR = 4
} dataOrder;

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
typedef struct guppi_hdr {
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

} guppi_hdr;
extern const guppi_hdr guppi_hdr_default;

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
	//int nsamples;

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
	char *headerBuffer;

	// DADA + DSPSR Defined header values
	double hdr_version; // Lib

	char instrument[META_STR_LEN + 1]; // Standard
	char telescope[META_STR_LEN + 1]; // Lib
	int telescope_rsp_id;
	char receiver[META_STR_LEN + 1]; // Lib
	char observer[META_STR_LEN + 1]; // External
	char hostname[META_STR_LEN + 1]; // Lib
	int baseport; // Lib
	char rawfile[MAX_NUM_PORTS][DEF_STR_LEN];
	int output_file_number; // Lib


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
	char obs_utc_start[META_STR_LEN + 1]; // Lib
	double obs_mjd_start; // Lib
	long obs_offset; // Lib, always 0? "offset of the first sample in bytes recorded after UTC_START"
	long obs_overlap; // Lib, always 0? We never overlap
	char basis[META_STR_LEN + 1]; // Standard
	char mode[META_STR_LEN + 1]; // Standard, currently not set


	double freq; // beamctl
	double bw; // beamctl
	double subband_bw; // beamctl
	double channel_bw;
	double ftop; // beamctl
	double fbottom; // beamctl
	int subbands[MAX_NUM_PORTS * UDPMAXBEAM];
	int nsubband; // beamctl
	int nchan; // Lib
	int nrcu; // beamctl
	int npol; // Standard
	int nbit; // Lib
	int resolution; // Standard? DSPSR: minimum number of samples that can be parsed, always 1?
	int ndim; // Lib
	double tsamp_raw; // Lib
	double tsamp;
	char state[META_STR_LEN + 1]; // Lib
	int order; // Lib

	// UPM Extra values
	char upm_version[META_STR_LEN + 1];
	char rec_version[META_STR_LEN + 1]; // Currently unused
	char upm_daq[META_STR_LEN + 1];
	char upm_beamctl[2 * DEF_STR_LEN + 1]; // beamctl command(s) can be long, DADA headers are capped at 4096 per entry including the key
	char upm_outputfmt[MAX_OUTPUT_DIMS][META_STR_LEN + 1];
	char upm_outputfmt_comment[DEF_STR_LEN + 1];

	int upm_num_inputs;
	int upm_num_outputs;
	int upm_reader;
	int upm_procmode;
	int upm_replay;
	int upm_calibrated;
	long upm_blocksize;
	long upm_pack_per_iter;
	long upm_processed_packets;
	long upm_dropped_packets;
	long upm_last_dropped_packets;

	char upm_rel_outputs[MAX_OUTPUT_DIMS];
	int upm_bandflip;
	int upm_output_voltages;

	int upm_input_bitmode;
	int upm_rcuclock;
	int upm_rcumode;
	int upm_rawbeamlets;
	int upm_upperbeam;
	int upm_lowerbeam;

	int external_channelisation;
	int external_downsampling;

	struct output {
		guppi_hdr *guppi;
		sigproc_hdr *sigproc;
	} output;

} lofar_udp_metadata;
extern const lofar_udp_metadata lofar_udp_metadata_default;

#endif // End of LOFAR_UDP_STRUCTS_METADATA_H
