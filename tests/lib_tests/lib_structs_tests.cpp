#include "gtest/gtest.h"
#include "lofar_udp_structs.h"
#include "lofar_udp_structs_metadata.h"


TEST(LibStructsTests, NormalAllocTests) {
	// lofar_udp_obs_meta *lofar_udp_obs_meta_alloc()
	lofar_udp_obs_meta *meta = lofar_udp_obs_meta_alloc();
	EXPECT_NE(nullptr, meta);

	//lofar_udp_reader* lofar_udp_reader_alloc(lofar_udp_obs_meta *meta)
	//lofar_udp_io_read_config* lofar_udp_io_alloc_read()
	lofar_udp_reader* reader = lofar_udp_reader_alloc(meta);
	EXPECT_NE(nullptr, reader);
	free(meta);
	free(reader->input);
	free(reader);

	//lofar_udp_config* lofar_udp_config_alloc()
	//lofar_udp_calibration* lofar_udp_calibration_alloc()
	lofar_udp_config *config = lofar_udp_config_alloc();
	EXPECT_NE(nullptr, config);
	lofar_udp_config_cleanup(config);

	//lofar_udp_io_write_config* lofar_udp_io_alloc_write()
	lofar_udp_io_write_config *output = lofar_udp_io_alloc_write();
	EXPECT_NE(nullptr, output);
	free(output);
}

TEST(LibStructsTests, MetadataAllocTests) {
	//lofar_udp_metadata* lofar_udp_metadata_alloc();
	lofar_udp_metadata *meta = lofar_udp_metadata_alloc();
	EXPECT_NE(nullptr, meta);

	//sigproc_hdr* sigproc_hdr_alloc();
	meta->output.sigproc = sigproc_hdr_alloc();
	EXPECT_NE(nullptr, meta->output.sigproc);
	//guppi_hdr* guppi_hdr_alloc();
	meta->output.guppi = guppi_hdr_alloc();
	EXPECT_NE(nullptr, meta->output.guppi);

	free(meta->output.sigproc);
	free(meta->output.guppi);
	free(meta);
}
