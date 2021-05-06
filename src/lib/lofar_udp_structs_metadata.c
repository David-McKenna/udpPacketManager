#include "lofar_udp_structs_metadata.h"

// Default for global metadata struct
const lofar_udp_metadata lofar_udp_metadata_default = {
	.type = NO_META,
	.setup = -1,

	.hdr_version = 1.0,

	.instrument = "",
	.telescope = "ILT",
	.telescope_id = -1,
	.reciever = "LOFAR-RSP",
	.machine = "",
	.observer = "",
	.hostname = "",
	.baseport = -1,
	.file_name = "",
	.file_number = -1,


	.source = "",
	.ra = "",
	.dec = "",
	.ra_rad = -1.0,
	.dec_rad = -1.0,
	.obs_id = "",
	.utc_start = "",
	.obs_mjd_start = -1.0,
	.block_mjd_start = -1.0,
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
	.upm_processed_packets = -1,
	.upm_dropped_packets = -1,
	.upm_last_dropped_packets = -1,

	.upm_bitmode = -1,
	.upm_rcuclock = -1,
	.upm_rawbeamlets = -1,
	.upm_upperbeam = -1,
	.upm_lowerbeam = -1,

	.outputSize = -1,
	.output = { NULL }
};

// Default values for the ASCII header
const ascii_hdr ascii_hdr_default = {
	.src_name = "J0000+0000",
	.ra_str = "00:00:00.0000",
	.dec_str = "+00:00:00.0000",
	.obsfreq = 149.90234375,
	.obsbw = 95.3125,
	.chan_bw = 0.1953125,
	.obsnchan = 488,
	.npol = 4,
	.nbits = 8,
	.tbin = 5.12e-6,
	.fd_poln = "LIN",
	.trk_mode = "TRACK",
	.obs_mode = "RAW",
	.cal_mode = "OFF",
	.scanlen = 0.,
	.projid = "UDP2RAW",
	.observer = "Unknown",
	.telescop = "ILT",
	.frontend = "LOFAR-RSP",
	.backend = "LOFAR-UDP",
	.datahost = "0.0.0.0",
	.dataport = 16130,
	.overlap = 0,
	.blocsize = 0,
	.daqpulse = "",
	.stt_imjd = 50000,
	.stt_smjd = 0,
	.pktidx = 0,
	.pktfmt = "1SFA",
	.stt_offs = 0.,
	.pktsize = 0,
	.dropblk = 0.,
	.droptot = 0.
};

// Default values for the sigproc header
const sigproc_hdr sigproc_hdr_default = {
	// Observatory information
	.telescope_id = -1,
	.machine_id = -1,

	.data_type = -1,
	.rawdatafile = { "" },

	// Observation parameters
	.source_name = "",
	.barycentric = -1,
	.pulsarcentric = -1,

	.az_start = -1.0,
	.za_start = -1.0,
	.src_raj = -1.0,
	.src_dej = -1.0,

	.tstart = -1.0,
	.tsamp = -1.0,

	.nbits = -1,
	.nsamples = -1,

	.fch1 = -1.0,
	.foff = 0.0,
	.fchannel = NULL,
	.nchans = -1,
	.nifs = -1,

	// Pulsar information
	.refdm = -1.0,
	.period = -1.0
};