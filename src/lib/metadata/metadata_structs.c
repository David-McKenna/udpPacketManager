#include "metadata_structs.h"

// Default for global metadata struct
const lofar_udp_metadata lofar_udp_metadata_default = {
	.hdr_version = 1.0,

	.instrument = "",
	.telescope = "ILT",
	.reciever = "LOFAR-RSP",
	.primary = "",
	.hostname = "",
	.file_name = "",
	.file_number = -1,


	.source = "",
	.ra = "",
	.dec = "",
	.ra_rad = -1.0,
	.dec_rad = -1.0,
	.obs_id = "",
	.utc_start = "",
	.mjd_start = -1.0,
	.obs_offset = -1,
	.obs_overlap = -1,
	.basis = "LINEAR",
	.mode = "",


	.freq = -1.0,
	.bw = -1.0,
	.nchan = -1,
	.npol = -1,
	.nbit = -1,
	.resolution = -1,
	.ndim = -1,
	.tsamp = -1.0,
	.state = "",

	// UPM Extra values
	.upm_version = UPM_VERSION,
	.rec_version = "",
	.upm_proc = "",
	.upm_reader = -1,
	.upm_mode = -1,
	.upm_replay = -1,
	.upm_calibrated = -1,

	.upm_bitmode = -1,
	.upm_rawbeamlets = -1,
	.upm_upperbeam = -1,
	.upm_lowerbeam = -1,

	.outputSize = -1,
	.output = { NULL }
};