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
	int32_t obsnchan;
	int32_t npol;
	int32_t nbits;
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
	int32_t dataport;
	int32_t overlap;

	int64_t blocsize;
	char daqpulse[META_STR_LEN + 1];
	int32_t stt_imjd;
	int32_t stt_smjd;
	int64_t pktidx;
	char pktfmt[META_STR_LEN + 1];
	double stt_offs;
	int32_t pktsize;
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
	int32_t telescope_id;
	int32_t machine_id;

	// File information
	int32_t data_type;
	char rawdatafile[DEF_STR_LEN];

	// Observation parameters
	char source_name[META_STR_LEN + 1];
	int32_t barycentric;
	int32_t pulsarcentric;

	double az_start;
	double za_start;
	double src_raj;
	double src_dej;

	double tstart;
	double tsamp;

	int32_t nbits;
	int32_t nsamples;

	double fch1;
	double foff;
	double *fchannel;
	int32_t nchans;
	int32_t nifs;

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
	int8_t *headerBuffer;

	// DADA + DSPSR Defined header values
	double hdr_version; // Lib

	char instrument[META_STR_LEN + 1]; // Standard
	char telescope[META_STR_LEN + 1]; // Lib
	int32_t telescope_rsp_id;
	char receiver[META_STR_LEN + 1]; // Lib
	char observer[META_STR_LEN + 1]; // External
	char hostname[META_STR_LEN + 1]; // Lib
	int32_t baseport; // Lib
	char rawfile[MAX_NUM_PORTS][DEF_STR_LEN];
	int32_t output_file_number; // Lib


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
	int64_t obs_offset; // Lib, always 0? "offset of the first sample in bytes recorded after UTC_START"
	int64_t obs_overlap; // Lib, always 0? We never overlap
	char basis[META_STR_LEN + 1]; // Standard
	char mode[META_STR_LEN + 1]; // Standard, currently not set


	double freq; // beamctl
	double bw; // beamctl
	double subband_bw; // beamctl
	double channel_bw;
	double ftop; // beamctl
	double fbottom; // beamctl
	int32_t subbands[MAX_NUM_PORTS * UDPMAXBEAM];
	int32_t lowerBeamlet;
	int32_t upperBeamlet;
	int32_t nsubband; // beamctl
	int32_t nchan; // Lib
	int32_t nrcu; // beamctl
	int32_t npol; // Standard
	int32_t nbit; // Lib
	int32_t resolution; // Standard? DSPSR: minimum number of samples that can be parsed, always 1?
	int32_t ndim; // Lib
	double tsamp_raw; // Lib
	double tsamp;
	char state[META_STR_LEN + 1]; // Lib
	int32_t order; // Lib

	// UPM Extra values
	char upm_version[META_STR_LEN + 1];
	char rec_version[META_STR_LEN + 1]; // Currently unused
	char upm_daq[META_STR_LEN + 1];
	char upm_beamctl[2 * DEF_STR_LEN + 1]; // beamctl command(s) can be long, DADA headers are capped at 4096 per entry including the key
	char upm_outputfmt[MAX_OUTPUT_DIMS][META_STR_LEN + 1];
	char upm_outputfmt_comment[DEF_STR_LEN + 1];

	int32_t upm_num_inputs;
	int32_t upm_num_outputs;
	int32_t upm_reader;
	int32_t upm_procmode;
	int32_t upm_replay;
	int32_t upm_calibrated;
	int64_t upm_blocksize;
	int64_t upm_pack_per_iter;
	int64_t upm_processed_packets;
	int64_t upm_dropped_packets;
	int64_t upm_last_dropped_packets;

	char upm_rel_outputs[MAX_OUTPUT_DIMS];
	int32_t upm_bandflip;
	int32_t upm_output_voltages;

	int32_t upm_input_bitmode;
	int32_t upm_rcuclock;
	int32_t upm_rcumode;
	int32_t upm_rawbeamlets;
	int32_t upm_upperbeam;
	int32_t upm_lowerbeam;

	int32_t external_channelisation;
	int32_t external_downsampling;

	struct output {
		guppi_hdr *guppi;
		sigproc_hdr *sigproc;
	} output;

} lofar_udp_metadata;
extern const lofar_udp_metadata lofar_udp_metadata_default;


#ifdef __cplusplus
extern "C" {
#endif

lofar_udp_metadata* lofar_udp_metadata_alloc();
sigproc_hdr* sigproc_hdr_alloc();
guppi_hdr* guppi_hdr_alloc();

#ifdef __cplusplus
}
#endif


#endif // End of LOFAR_UDP_STRUCTS_METADATA_H
