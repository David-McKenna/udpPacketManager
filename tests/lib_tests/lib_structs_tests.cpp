#include "gtest/gtest.h"
#include "lofar_udp_reader.h"
#include "lofar_udp_io.h"


TEST(LibStructsTests, NormalAllocTests) {
	// lofar_udp_obs_meta *_lofar_udp_obs_meta_alloc()
	SCOPED_TRACE("lofar_udp_setup_allocs");

	lofar_udp_obs_meta *meta = _lofar_udp_obs_meta_alloc();
	EXPECT_NE(nullptr, meta);

	//lofar_udp_reader* _lofar_udp_reader_alloc(lofar_udp_obs_meta *meta)
	//lofar_udp_io_read_config* lofar_udp_io_read_alloc()
	lofar_udp_reader* reader = _lofar_udp_reader_alloc(meta);
	EXPECT_NE(nullptr, reader);
	lofar_udp_reader_cleanup(reader);

	//lofar_udp_config* lofar_udp_config_alloc()
	//lofar_udp_calibration* _lofar_udp_calibration_alloc()
	lofar_udp_config *config = lofar_udp_config_alloc();
	EXPECT_NE(nullptr, config);
	lofar_udp_config_cleanup(config);

	//lofar_udp_io_write_config* lofar_udp_io_write_alloc()
	lofar_udp_io_write_config *output = lofar_udp_io_write_alloc();
	EXPECT_NE(nullptr, output);
	FREE_NOT_NULL(output);

	lofar_udp_calibration *cal = _lofar_udp_calibration_alloc();
	EXPECT_NE(nullptr, cal);
	FREE_NOT_NULL(cal);

}

TEST(LibStructsTests, MetadataAllocTests) {

	SCOPED_TRACE("lofar_udp_metadata_alloc");
	//lofar_udp_metadata* lofar_udp_metadata_alloc();
	lofar_udp_metadata *meta = lofar_udp_metadata_alloc();
	EXPECT_NE(nullptr, meta);

	{
		SCOPED_TRACE("sigproc_hdr_alloc");
		//sigproc_hdr* sigproc_hdr_alloc();
		meta->output.sigproc = sigproc_hdr_alloc(64);
		EXPECT_NE(nullptr, meta->output.sigproc);
		EXPECT_NE(nullptr, meta->output.sigproc->fchannel);
	}
	{
		SCOPED_TRACE("guppi_hdr_alloc");
		//guppi_hdr* guppi_hdr_alloc();
		meta->output.guppi = guppi_hdr_alloc();
		EXPECT_NE(nullptr, meta->output.guppi);
	}

	lofar_udp_metadata_cleanup(meta);
}
