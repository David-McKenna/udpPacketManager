#include "sigproc_hdr_manager.h"


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
	.foff = -1.0,
	.fchannel = NULL,
	.nchans = -1,
	.nifs = -1,

	// Pulsar information
	.refdm = -1.0,
	.period = -1.0
};


__attribute_deprecated__ int lofar_udp_metadata_setup_SIGPROC(lofar_udp_metadata *metadata) {



	return 0;
}

int lofar_udp_metadata_write_SIGPROC(char *headerBuffer, size_t headerLength, sigproc_hdr *headerStruct) {

	return 0;
}