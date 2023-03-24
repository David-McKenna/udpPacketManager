#include "gtest/gtest.h"
#include "lofar_udp_reader.h"

#define MODIFY_AND_RESET(target, inter, val, execute) \
	inter = target;                             \
	target = val;                               \
    execute;                                          \
	target = inter;

TEST(LibReaderTests, SetupReader) {
	lofar_udp_config *config = lofar_udp_config_alloc();
	int32_t tmpVal = 0;

	EXPECT_NE(nullptr, config);
	const int32_t numPorts = 3;
	lofar_udp_calibration *cal = config->calibrationConfiguration;


	//int lofar_udp_reader_config_check(lofar_udp_config *config)
	config->numPorts = numPorts;
	config->ompThreads = 10;

	config->packetsPerIteration = 16;
	config->packetsReadMax = 32;

	config->beamletLimits[0] = -1;
	config->beamletLimits[1] = -1;

	config->calibrateData = NO_CALIBRATION;
	config->processingMode = 0;
	config->startingPacket = -1;

	config->replayDroppedPackets = 0;

	EXPECT_EQ(0, lofar_udp_reader_config_check(config));

	// (config->numPorts > MAX_NUM_PORTS || config->numPorts < 1)
	MODIFY_AND_RESET(config->numPorts, tmpVal, MAX_NUM_PORTS + 1, EXPECT_EQ(-1, lofar_udp_reader_config_check(config)););
	MODIFY_AND_RESET(config->numPorts, tmpVal, -1, EXPECT_EQ(-1, lofar_udp_reader_config_check(config)););

	// (config->packetsPerIteration < 1)
	MODIFY_AND_RESET(config->packetsPerIteration, tmpVal, 0, EXPECT_EQ(-1, lofar_udp_reader_config_check(config)););

	// (config->beamletLimits[0] > 0 && config->beamletLimits[1] > 0) &&
	//      (config->beamletLimits[0] > config->beamletLimits[1]) || (config->processingMode < 2)
	config->beamletLimits[0] = 3;
	config->beamletLimits[1] = 2;
	EXPECT_EQ(-1, lofar_udp_reader_config_check(config));
	config->beamletLimits[1] = 4;
	EXPECT_EQ(-1, lofar_udp_reader_config_check(config));
	config->processingMode = 2;
	EXPECT_EQ(0, lofar_udp_reader_config_check(config));

	// (config->calibrateData < NO_CALIBRATION || config->calibrateData > APPLY_CALIBRATION)
	config->calibrateData = (calibrate_t) 16;
	EXPECT_EQ(-1, lofar_udp_reader_config_check(config));
	config->calibrateData = APPLY_CALIBRATION;

	// (config->calibrateData > NO_CALIBRATION && config->calibrationConfiguration == NULL)
	config->calibrateData = APPLY_CALIBRATION;
	config->calibrationConfiguration = nullptr;
	EXPECT_EQ(-1, lofar_udp_reader_config_check(config));
	config->calibrationConfiguration = cal;

	// (config->calibrateData > NO_CALIBRATION && config->calibrationConfiguration != NULL) &&  (strcmp(config->calibrationConfiguration->calibrationFifo, "") == 0)
	config->calibrationConfiguration->calibrationFifo[0] = '\0';
	EXPECT_EQ(-1, lofar_udp_reader_config_check(config));
	config->calibrateData = NO_CALIBRATION;

	// (config->processingMode < 0)
	MODIFY_AND_RESET(config->processingMode, tmpVal, -1, EXPECT_EQ(-1, lofar_udp_reader_config_check(config)););

	// (config->startingPacket > 0 && config->startingPacket < LFREPOCH)
	MODIFY_AND_RESET(config->startingPacket, tmpVal, 1, EXPECT_EQ(-1, lofar_udp_reader_config_check(config)););

	// (config->packetsReadMax < 1)
	MODIFY_AND_RESET(config->packetsReadMax, tmpVal, 0, EXPECT_EQ(-1, lofar_udp_reader_config_check(config)););

	// (config->packetsReadMax < config->packetsPerIteration) -> config->packetsReadMax = config->packetsPerIteration
	MODIFY_AND_RESET(config->packetsReadMax, tmpVal, 0, EXPECT_EQ(-1, lofar_udp_reader_config_check(config)););

	// ((2 * config->ompThreads) < config->numPorts) && (config->ompThreads < 1) -> config->ompThreads = OMP_THREADS
	MODIFY_AND_RESET(config->ompThreads, tmpVal, -1, \
		lofar_udp_reader_config_check(config); \
		EXPECT_EQ(OMP_THREADS, config->ompThreads)
	 );

	// (config->replayDroppedPackets != 0 && config->replayDroppedPackets != 1)
	MODIFY_AND_RESET(config->replayDroppedPackets, tmpVal, 2, EXPECT_EQ(-1, lofar_udp_reader_config_check(config)););
	EXPECT_EQ(0, lofar_udp_reader_config_check(config));


	//lofar_udp_reader *lofar_udp_reader_setup(lofar_udp_config *config)
	MODIFY_AND_RESET(config->replayDroppedPackets, tmpVal, 2, EXPECT_EQ(nullptr, lofar_udp_reader_setup(config)););
	MODIFY_AND_RESET(config->packetsReadMax, tmpVal, INT_MAX, EXPECT_EQ(nullptr, lofar_udp_reader_setup(config)););

	std::string inputLocations[numPorts] = {
		"",
		"",
		""
	};
	tmpVal = 0;
	for (const std::string str : inputLocations) {
		memcpy(config->inputLocations[tmpVal++], str.c_str(), str.length());
	}

	//int lofar_udp_file_reader_reuse(lofar_udp_reader *reader, const long startingPacket, const long packetsReadMax)

};

TEST(LibReaderTests, Preprocessing) {
	//int lofar_udp_reader_malformed_header_checks(const int8_t header[UDPHDRLEN])
	int8_t header[UDPHDRLEN] = {{0}};
	lofar_source_bytes *source = (lofar_source_bytes *) &(header[CEP_HDR_SRC_OFFSET]);
	int32_t tmpVal = 0;

	header[CEP_HDR_RSP_VER_OFFSET] = UDPCURVER;
	*((int32_t *) &(header[CEP_HDR_TIME_OFFSET])) = LFREPOCH;
	*((int32_t *) &(header[CEP_HDR_SEQ_OFFSET])) = RSPMAXSEQ;
	header[CEP_HDR_NBEAM_OFFSET] = (uint8_t) UDPMAXBEAM;
	header[CEP_HDR_NTIMESLICE_OFFSET] = UDPNTIMESLICE;
	source->padding0 = 0;
	source->errorBit = 0;
	source->bitMode = 2;
	source->padding1 = 0;

	EXPECT_EQ(0, lofar_udp_reader_malformed_header_checks(header));

	// ((uint8_t) header[CEP_HDR_RSP_VER_OFFSET] < UDPCURVER)
	MODIFY_AND_RESET(header[CEP_HDR_RSP_VER_OFFSET], tmpVal, (uint8_t) UDPCURVER - 1, EXPECT_EQ(-1, lofar_udp_reader_malformed_header_checks(header)););

	// (*((int32_t *) &(header[CEP_HDR_TIME_OFFSET])) < LFREPOCH)
	MODIFY_AND_RESET(*((int32_t *) &(header[CEP_HDR_TIME_OFFSET])), tmpVal, LFREPOCH - 1, EXPECT_EQ(-1, lofar_udp_reader_malformed_header_checks(header)););

	// (*((int32_t *) &(header[CEP_HDR_SEQ_OFFSET])) > RSPMAXSEQ)
	MODIFY_AND_RESET(*((int32_t *) &(header[CEP_HDR_SEQ_OFFSET])), tmpVal, RSPMAXSEQ + 1, EXPECT_EQ(-1, lofar_udp_reader_malformed_header_checks(header)););

	// ((uint8_t) header[CEP_HDR_NBEAM_OFFSET] > UDPMAXBEAM)
	MODIFY_AND_RESET(header[CEP_HDR_NBEAM_OFFSET], tmpVal, (uint8_t) UDPMAXBEAM + 1, EXPECT_EQ(-1, lofar_udp_reader_malformed_header_checks(header)););

	// ((uint8_t) header[CEP_HDR_NTIMESLICE_OFFSET] != UDPNTIMESLICE)
	MODIFY_AND_RESET(header[CEP_HDR_NTIMESLICE_OFFSET], tmpVal, (uint8_t) UDPNTIMESLICE - 1, EXPECT_EQ(-1, lofar_udp_reader_malformed_header_checks(header)););

	// (source->padding0 != (uint32_t) 0)
	MODIFY_AND_RESET(source->padding0, tmpVal, (uint8_t) 1, EXPECT_EQ(-1, lofar_udp_reader_malformed_header_checks(header)););

	// (source->errorBit != (uint32_t) 0)
	MODIFY_AND_RESET(source->errorBit, tmpVal, (uint8_t) 1, EXPECT_EQ(-1, lofar_udp_reader_malformed_header_checks(header)););

	// (source->bitMode == (uint32_t) 3)
	MODIFY_AND_RESET(source->bitMode, tmpVal, (uint8_t) 3, EXPECT_EQ(-1, lofar_udp_reader_malformed_header_checks(header)););

	// (source->padding1 > (uint32_t) 1)
	MODIFY_AND_RESET(source->padding1, tmpVal, (uint32_t) 2, EXPECT_EQ(-1, lofar_udp_reader_malformed_header_checks(header)););

	// (source->padding1 == (uint32_t) 1)
	MODIFY_AND_RESET(source->padding1, tmpVal, (uint32_t) 1, EXPECT_EQ(0, lofar_udp_reader_malformed_header_checks(header)););

	EXPECT_EQ(0, lofar_udp_reader_malformed_header_checks(header));


	//int lofar_udp_parse_headers(lofar_udp_obs_meta *meta, const int8_t header[MAX_NUM_PORTS][UDPHDRLEN], const int16_t beamletLimits[2])
	//void lofar_udp_parse_extract_header_metadata(int port, lofar_udp_obs_meta *meta, const int8_t header[UDPHDRLEN], const int16_t beamletLimits[2])

	//int lofar_udp_skip_to_packet(lofar_udp_reader *reader)

	//int lofar_udp_get_first_packet_alignment(lofar_udp_reader *reader)

	//int lofar_udp_setup_processing(lofar_udp_obs_meta *meta)


};


TEST(LibReaderTests, ProcessingData) {

	//int lofar_udp_reader_calibration(lofar_udp_reader *reader)

	//int lofar_udp_reader_step_timed(lofar_udp_reader *reader, double timing[2])
	//int lofar_udp_reader_step(lofar_udp_reader *reader)
	//int lofar_udp_reader_internal_read_step(lofar_udp_reader *reader)

	//int lofar_udp_shift_remainder_packets(lofar_udp_reader *reader, const long shiftPackets[], const int handlePadding)

	//void lofar_udp_reader_cleanup(lofar_udp_reader *reader)
};