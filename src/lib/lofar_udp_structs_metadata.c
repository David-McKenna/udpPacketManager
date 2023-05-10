#include "lofar_udp_structs_metadata.h"

// Default for global metadata struct
const lofar_udp_metadata lofar_udp_metadata_default = {

	.type = NO_META,
	.headerBuffer = NULL,

	.hdr_version = 1.0,

	.instrument = "",
	.telescope = "ILT",
	.telescope_rsp_id = -1,
	.receiver = "LOFAR-RSP",
	.observer = "",
	.hostname = "",
	.baseport = -1,
	.rawfile = {""}, // NEEDS FULL RUNTIME INITIALISATION
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
	.obs_utc_start = "",
	.obs_mjd_start = -1.0,
	.obs_offset = -1,
	.obs_overlap = -1,
	.basis = "LINEAR",
	.mode = "",


	.freq_raw = -1.0,
	.freq = -1.0,
	.bw = -0.0,
	.bw_raw = -0.0,
	.subband_bw = -0.0,
	.channel_bw = -0.0,
	.ftop = -1.0,
	.ftop_raw = -1.0,
	.fbottom = -1.0,
	.fbottom_raw = -1.0,
	.subbands = { -1 }, // NEEDS FULL RUNTIME INITIALISATION
	.lowerBeamlet = -1,
	.upperBeamlet = -1,
	.nsubband = -1,
	.nchan = -1,
	.nrcu = -1,
	.npol = -1,
	.nbit = -1,
	.resolution = -1,
	.ndim = -1,
	.tsamp_raw = -1.0,
	.tsamp = -1.0,
	.state = "",

	// UPM Extra values
	.upm_version = UPM_VERSION,
	.rec_version = "",
	.upm_daq = "",
	.upm_beamctl = "",
	.upm_outputfmt = {""}, // NEEDS FULL RUNTIME INITIALISATION
	.upm_outputfmt_comment = "",
	.upm_num_inputs = -1,
	.upm_num_outputs = -1,
	.upm_reader = -1,
	.upm_procmode = UNSET_MODE,
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

	.external_channelisation = -1,
	.external_downsampling = -1,

	.output = { NULL , NULL }
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
	.rawdatafile = "",

	// Observation parameters
	.source_name = "",
	.barycentric = -1,
	.pulsarcentric = -1,

	.az_start = -1.0,
	.za_start = -1.0,
	.src_raj = -1.0,
	.src_decj = -1.0,

	.tstart = -1.0,
	.tsamp = -1.0,

	.nbits = -1,
	.nsamples = -1,

	.fch1 = -1.0,
	.foff = 0.0,
	.fchannel = NULL,
	.fchannels = -1,
	.nchans = -1,
	.nifs = -1,

	// Pulsar information
	.refdm = -1.0,
	.period = -1.0
};

/**
 * @brief Allocate a standard metadata struct and perform all initialisation needed.
 *
 * @return Allocated and initialised struct, or NULL
 */
lofar_udp_metadata* lofar_udp_metadata_alloc() {
	DEFAULT_STRUCT_ALLOC(lofar_udp_metadata, meta, lofar_udp_metadata_default, ;, NULL);
	ARR_INIT(meta->subbands, MAX_NUM_PORTS * UDPMAXBEAM, -1);
	STR_INIT(meta->rawfile, MAX_NUM_PORTS);
	STR_INIT(meta->upm_outputfmt, MAX_NUM_PORTS);

	return meta;
}

/**
 * @brief Cleanup a metadata struct
 *
 * @param meta	The struct to cleanup
 */
void lofar_udp_metadata_cleanup(lofar_udp_metadata *meta) {
	if (meta != NULL) {
		if (meta->output.sigproc != NULL) {
			sigproc_hdr_cleanup(meta->output.sigproc);
		}
		FREE_NOT_NULL(meta->output.guppi);
	}
	FREE_NOT_NULL(meta);
}

/**
 * @brief Allocate a Sigproc metadata struct and perform all initialisation needed.
 *
 * @return Allocated and initialised struct, or NULL
 */
sigproc_hdr* sigproc_hdr_alloc(const int32_t fchannels) {
	DEFAULT_STRUCT_ALLOC(sigproc_hdr, hdr, sigproc_hdr_default, ;, NULL);

	if (fchannels) {
		hdr->fchannel = calloc(fchannels, sizeof(double));
		CHECK_ALLOC(hdr->fchannel, NULL, free(hdr));
		hdr->fchannels = fchannels;
		ARR_INIT(hdr->fchannel, fchannels, (double) -1);
	}

	return hdr;
}

/**
 * @brief Free a Sigproc metadata struct pointer
 */
void sigproc_hdr_cleanup(sigproc_hdr *hdr) {
	if (hdr) {
		FREE_NOT_NULL(hdr->fchannel);
	}
	FREE_NOT_NULL(hdr);
}

/**
 * @brief Allocate a GUPPI metadata struct and perform all initialisation needed.
 *
 * @return Allocated and initialised struct, or NULL
 */
guppi_hdr* guppi_hdr_alloc() {
	DEFAULT_STRUCT_ALLOC(guppi_hdr, hdr, guppi_hdr_default, ;, NULL);

	return hdr;
}


/**
 * Copyright (C) 2023 David McKenna
 * This file is part of udpPacketManager <https://github.com/David-McKenna/udpPacketManager>.
 *
 * udpPacketManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * udpPacketManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with udpPacketManager.  If not, see <http://www.gnu.org/licenses/>.
 **/
