#include "lofar_udp_structs_metadata.h"

// Default for global metadata struct
const lofar_udp_metadata lofar_udp_metadata_default = {

	.type = NO_META,

	.hdr_version = 1.0,

	.instrument = "",
	.telescope = "ILT",
	.telescope_rsp_id = -1,
	.receiver = "LOFAR-RSP",
	.observer = "",
	.hostname = "",
	.baseport = -1,
	.rawfile = { "" },
	.output_file_number = -1,


	.source = "",
	.ra = "",
	.dec = "",
	.ra_rad = -1.0,
	.dec_rad = -1.0,
	.ra_analog = "",
	.dec_analog = "",
	.ra_rad_analog = -1.0,
	.dec_rad_analog = -1.0,
	.coord_basis = "",
	.obs_id = "",
	.utc_start = "",
	.obs_mjd_start = -1.0,
	.obs_offset = -1,
	.obs_overlap = -1,
	.basis = "LINEAR",
	.mode = "",


	.freq = -1.0,
	.bw = -0.0,
	.channel_bw = -0.0,
	.ftop = -1.0,
	.fbottom = -1.0,
	.subbands = { -1 },
	.nchan = -1,
	.nrcu = -1,
	.npol = -1,
	.nbit = -1,
	.resolution = -1,
	.ndim = -1,
	.tsamp = -1.0,
	.state = "",

	// UPM Extra values
	.upm_version = UPM_VERSION,
	.rec_version = "",
	.upm_daq = "",
	.upm_beamctl = "",
	.upm_outputfmt = { "" },
	.upm_outputfmt_comment = "",
	.upm_num_inputs = -1,
	.upm_num_outputs = -1,
	.upm_reader = -1,
	.upm_procmode = -1,
	.upm_bandflip = -1,
	.upm_replay = -1,
	.upm_calibrated = -1,
	.upm_blocksize = -1,
	.upm_pack_per_iter = -1,
	.upm_processed_packets = -1,
	.upm_dropped_packets = -1,
	.upm_last_dropped_packets = -1,

	.upm_input_bitmode = -1,
	.upm_rcuclock = -1,
	.upm_rawbeamlets = -1,
	.upm_upperbeam = -1,
	.upm_lowerbeam = -1,

	.output = { NULL }
};

// Default values for the ASCII header
const guppi_hdr guppi_hdr_default = {
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