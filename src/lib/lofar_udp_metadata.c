#include "lofar_udp_metadata.h"


int lofar_udp_setup_metadata(lofar_udp_metadata *metadata, metadata_t meta) {

	switch(meta) {
		case GUPPI:
			return lofar_udp_metadata_setup_GUPPI(metadata);

		// The DADA header is the reference metadata struct
		case DADA:
			return 0;

		case SIGPROC:
			return lofar_udp_metadata_setup_SIGPROC(metadata);

		default:
			fprintf(stderr, "ERROR: Unknown metadata type %d, exiting.\n", meta);
			return -1;
	}
}