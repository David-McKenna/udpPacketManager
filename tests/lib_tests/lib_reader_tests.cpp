#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"
#include "lofar_udp_reader.h"
#include "lib_reference_files.hpp"

#include <string>
#include <regex>
#include <valarray>
#include <complex>
#include <functional>

#define MODIFY_AND_RESET(target, inter, val, execute) \
	inter = target;                             \
	target = val;                               \
    execute;                                          \
	target = inter;



lofar_udp_config* config_setup(int32_t sampleMeta = 0, int32_t testNumber = 0, int32_t testPorts = 4, int32_t packetsReadMax = 32, calibrate_t calibration = NO_CALIBRATION) {
	lofar_udp_config *config = lofar_udp_config_alloc();
	EXPECT_NE(nullptr, config);
	assert(numPorts <= MAX_NUM_PORTS);
	config->calibrationDuration = 1.2;

	for (int32_t port = 0; port < testPorts; port++) {
		std::string workingString = std::regex_replace(inputLocations[testNumber], std::regex("portnum"), std::to_string(port));
		memcpy(config->inputLocations[port], workingString.c_str(), workingString.length());
	}

	config->readerType = strstr(config->inputLocations[0], ".zst") ? ZSTDCOMPRESSED : NORMAL;


	//int lofar_udp_reader_config_check(lofar_udp_config *config)
	config->numPorts = numPorts;
	config->ompThreads = 10;

	config->packetsPerIteration = 16;
	config->packetsReadMax = packetsReadMax;

	config->beamletLimits[0] = -1;
	config->beamletLimits[1] = -1;

	config->calibrateData = calibration;
	config->processingMode = 0;
	config->startingPacket = -1;

	config->replayDroppedPackets = 0;

	if (sampleMeta < 0 || sampleMeta > numMeta) {
		std::cerr << "Failed to provide accurate metadata index (" << numMeta << " valid, " << sampleMeta << " given)\n";
		return nullptr;
	}
	if (sampleMeta) {
		strncpy(config->metadata_config.metadataLocation, metadataLocation[sampleMeta].c_str(), DEF_STR_LEN);
	}

	return config;
}

lofar_udp_reader* reader_setup(int32_t processingMode) {
	lofar_udp_config *config = config_setup();
	config->processingMode = processingMode;

	return lofar_udp_reader_setup(config);
}


TEST(LibReaderTests, SetupReader) {
	lofar_udp_config *config = config_setup(0, 1, 4, 32);
	int32_t tmpVal;


	{
		SCOPED_TRACE("_lofar_udp_reader_config_check");
		EXPECT_EQ(0, _lofar_udp_reader_config_check(config));

		// (config->numPorts > MAX_NUM_PORTS || config->numPorts < 1)
		MODIFY_AND_RESET(config->numPorts, tmpVal, MAX_NUM_PORTS + 1, EXPECT_EQ(-1, _lofar_udp_reader_config_check(config)););
		MODIFY_AND_RESET(config->numPorts, tmpVal, -1, EXPECT_EQ(-1, _lofar_udp_reader_config_check(config)););

		// (!strlen(config->inputLocations[port]))
		char firstChar = config->inputLocations[0][0];
		config->inputLocations[0][0] = '\0';
		EXPECT_EQ(-1, _lofar_udp_reader_config_check(config));

		// (access(config->inputLocations[port], F_OK) != 0)
		config->inputLocations[0][0] = '!';
		EXPECT_EQ(-1, _lofar_udp_reader_config_check(config));
		config->inputLocations[0][0] = firstChar;
		EXPECT_EQ(0, _lofar_udp_reader_config_check(config));

		// (config->packetsPerIteration < 1)
		MODIFY_AND_RESET(config->packetsPerIteration, tmpVal, 0, EXPECT_EQ(-1, _lofar_udp_reader_config_check(config)););

		// (config->beamletLimits[0] > 0 && config->beamletLimits[1] > 0) &&
		//      (config->beamletLimits[0] > config->beamletLimits[1]) || (config->processingMode < 2)
		config->beamletLimits[0] = 3;
		config->beamletLimits[1] = 2;
		EXPECT_EQ(-1, _lofar_udp_reader_config_check(config));
		config->beamletLimits[1] = 4;
		EXPECT_EQ(-1, _lofar_udp_reader_config_check(config));
		config->processingMode = 2;
		EXPECT_EQ(0, _lofar_udp_reader_config_check(config));

		// (config->calibrateData < NO_CALIBRATION || config->calibrateData > APPLY_CALIBRATION)
		config->calibrateData = (calibrate_t) 16;
		EXPECT_EQ(-1, _lofar_udp_reader_config_check(config));
		config->calibrateData = APPLY_CALIBRATION;

		// (config->calibrateData > NO_CALIBRATION && config->calibrationConfiguration == NULL)

		config->calibrationDuration = -1.0f;
		EXPECT_EQ(-1, _lofar_udp_reader_config_check(config));
		config->calibrationDuration = 1.0f;
		config->calibrateData = NO_CALIBRATION;

		// (config->processingMode < 0)
		MODIFY_AND_RESET(config->processingMode, tmpVal, -1, EXPECT_EQ(-1, _lofar_udp_reader_config_check(config)););

		// (config->startingPacket > 0 && config->startingPacket < LFREPOCH)
		MODIFY_AND_RESET(config->startingPacket, tmpVal, 1, EXPECT_EQ(-1, _lofar_udp_reader_config_check(config)););

		// (config->packetsReadMax < 1)
		MODIFY_AND_RESET(config->packetsReadMax, tmpVal, 0, EXPECT_EQ(-1, _lofar_udp_reader_config_check(config)););

		// ((2 * config->ompThreads) < config->numPorts)
		MODIFY_AND_RESET(config->ompThreads, tmpVal, 1, _lofar_udp_reader_config_check(config); EXPECT_EQ(1, config->ompThreads));

		// (config->replayDroppedPackets != 0 && config->replayDroppedPackets != 1)
		MODIFY_AND_RESET(config->replayDroppedPackets, tmpVal, 2, EXPECT_EQ(-1, _lofar_udp_reader_config_check(config)););
		EXPECT_EQ(0, _lofar_udp_reader_config_check(config));


		// _lofar_udp_reader_config_patch

		// (config->packetsReadMax < 1) -> LONG_MAX
		MODIFY_AND_RESET(config->packetsReadMax, tmpVal, -1, _lofar_udp_reader_config_patch(config); EXPECT_EQ(LONG_MAX, config->packetsReadMax););

		// (config->packetsReadMax < config->packetsPerIteration) -> config->packetsReadMax = config->packetsPerIteration
		MODIFY_AND_RESET(config->packetsReadMax, tmpVal, 1, _lofar_udp_reader_config_patch(config); EXPECT_EQ(config->packetsPerIteration, config->packetsReadMax););

		// (config->ompThreads < 1) -> config->ompThreads = OMP_THREADS
		MODIFY_AND_RESET(config->ompThreads, tmpVal, -1, \
            _lofar_udp_reader_config_patch(config); \
            EXPECT_EQ(OMP_THREADS, config->ompThreads);
		);

		// (config->metadata_config.metadataType == NO_META && strnlen(config->metadata_config.metadataLocation, DEF_STR_LEN))
		snprintf(config->metadata_config.metadataLocation, 32, "thisisastring");
		MODIFY_AND_RESET(config->metadata_config.metadataType, *((metadata_t*) &tmpVal), NO_META, \
						 _lofar_udp_reader_config_patch(config); \
						EXPECT_EQ(DEFAULT_META, config->metadata_config.metadataType);
		 );
		config->metadata_config.metadataLocation[0] = '\0';


		//lofar_udp_reader *lofar_udp_reader_setup(lofar_udp_config *config)
		MODIFY_AND_RESET(config->replayDroppedPackets, tmpVal, 2, EXPECT_EQ(nullptr, lofar_udp_reader_setup(config)););
		//MODIFY_AND_RESET(config->packetsReadMax, tmpVal, LONG_MAX, EXPECT_NE(nullptr, lofar_udp_reader_setup(config)););

	}

	free(config);

};

TEST(LibReaderTests, PreprocessingRawData) {
	//int lofar_udp_reader_malformed_header_checks(const int8_t header[UDPHDRLEN])
	int8_t *header = (int8_t *) calloc(1, UDPHDRLEN * sizeof(int8_t));
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

	{
		SCOPED_TRACE("_lofar_udp_reader_malformed_header_checks");
		EXPECT_EQ(0, _lofar_udp_reader_malformed_header_checks(header));

		// ((uint8_t) header[CEP_HDR_RSP_VER_OFFSET] < UDPCURVER)
		MODIFY_AND_RESET(header[CEP_HDR_RSP_VER_OFFSET], tmpVal, (uint8_t) UDPCURVER - 1, EXPECT_EQ(-1, _lofar_udp_reader_malformed_header_checks(header)););

		// (*((int32_t *) &(header[CEP_HDR_TIME_OFFSET])) < LFREPOCH)
		MODIFY_AND_RESET(*((int32_t *) &(header[CEP_HDR_TIME_OFFSET])), tmpVal, LFREPOCH - 1, EXPECT_EQ(-1, _lofar_udp_reader_malformed_header_checks(header)););

		// (*((int32_t *) &(header[CEP_HDR_SEQ_OFFSET])) > RSPMAXSEQ)
		MODIFY_AND_RESET(*((int32_t *) &(header[CEP_HDR_SEQ_OFFSET])), tmpVal, RSPMAXSEQ + 1, EXPECT_EQ(-1, _lofar_udp_reader_malformed_header_checks(header)););

		// ((uint8_t) header[CEP_HDR_NBEAM_OFFSET] > UDPMAXBEAM)
		MODIFY_AND_RESET(*((uint8_t*) &header[CEP_HDR_NBEAM_OFFSET]), *((uint8_t*) &tmpVal), (uint8_t) UDPMAXBEAM + 1, EXPECT_EQ(-1, _lofar_udp_reader_malformed_header_checks(header)););

		// ((uint8_t) header[CEP_HDR_NTIMESLICE_OFFSET] != UDPNTIMESLICE)
		MODIFY_AND_RESET(header[CEP_HDR_NTIMESLICE_OFFSET], tmpVal, (uint8_t) UDPNTIMESLICE - 1,
		                 EXPECT_EQ(-1, _lofar_udp_reader_malformed_header_checks(header)););

		// (source->padding0 != (uint32_t) 0)
		MODIFY_AND_RESET(source->padding0, tmpVal, (uint8_t) 1, EXPECT_EQ(-1, _lofar_udp_reader_malformed_header_checks(header)););

		// (source->errorBit != (uint32_t) 0)
		MODIFY_AND_RESET(source->errorBit, tmpVal, (uint8_t) 1, EXPECT_EQ(-1, _lofar_udp_reader_malformed_header_checks(header)););

		// (source->bitMode == (uint32_t) 3)
		MODIFY_AND_RESET(source->bitMode, tmpVal, (uint8_t) 3, EXPECT_EQ(-1, _lofar_udp_reader_malformed_header_checks(header)););

		// (source->padding1 > (uint32_t) 1)
		MODIFY_AND_RESET(source->padding1, tmpVal, (uint32_t) 2, EXPECT_EQ(-1, _lofar_udp_reader_malformed_header_checks(header)););

		// (source->padding1 == (uint32_t) 1)
		MODIFY_AND_RESET(source->padding1, tmpVal, (uint32_t) 1, EXPECT_EQ(0, _lofar_udp_reader_malformed_header_checks(header)););

		EXPECT_EQ(0, _lofar_udp_reader_malformed_header_checks(header));
	}

	//int lofar_udp_parse_headers(lofar_udp_obs_meta *meta, const int8_t header[MAX_NUM_PORTS][UDPHDRLEN], const int16_t beamletLimits[2])
	//void _lofar_udp_parse_header_extract_metadata(int port, lofar_udp_obs_meta *meta, const int8_t header[UDPHDRLEN], const int16_t beamletLimits[2])
	{
		SCOPED_TRACE("_lofar_udp_parse_header_buffers");

		lofar_udp_config *config = config_setup();
		config->packetsReadMax = -1;
		lofar_udp_obs_meta *meta = _lofar_udp_configure_obs_meta(config);
		ASSERT_NE(nullptr, meta);
		EXPECT_EQ(-1, meta->packetsReadMax);




		*((int16_t *) &(header[CEP_HDR_STN_ID_OFFSET])) = 333 * 32;
		*((uint8_t *) &(header[CEP_HDR_NBEAM_OFFSET])) = 244;
		source->bitMode = 2;

		int8_t headers[MAX_NUM_PORTS][UDPHDRLEN];
		for (size_t port = 0; port < MAX_NUM_PORTS; port++) {
			memcpy(&(headers[port][0]), &(header[0]), UDPHDRLEN);
		}


		int16_t beamletLimits[2] = {0, 0};
		meta->numPorts = MAX_NUM_PORTS;
		EXPECT_EQ(0, _lofar_udp_parse_header_buffers(meta, headers, beamletLimits));

		*((uint8_t *) &(headers[0][CEP_HDR_NBEAM_OFFSET])) -= 122;
		EXPECT_EQ(0, _lofar_udp_parse_header_buffers(meta, headers, beamletLimits));
		*((uint8_t *) &(headers[0][CEP_HDR_NBEAM_OFFSET])) += 122;

		headers[0][CEP_HDR_RSP_VER_OFFSET] -= 1;
		EXPECT_EQ(-1, _lofar_udp_parse_header_buffers(meta, headers, beamletLimits));
		headers[0][CEP_HDR_RSP_VER_OFFSET] += 1;

		*((int16_t *) &(headers[1][CEP_HDR_STN_ID_OFFSET])) -= 64;
		EXPECT_EQ(0, _lofar_udp_parse_header_buffers(meta, headers, beamletLimits));
		*((int16_t *) &(headers[1][CEP_HDR_STN_ID_OFFSET])) += 64;
		source = (lofar_source_bytes *) &(headers[0][CEP_HDR_SRC_OFFSET]);

		// source->bitMode
		source->bitMode = 1;
		EXPECT_EQ(-1, _lofar_udp_parse_header_buffers(meta, headers, beamletLimits));
		source->bitMode = 0;
		EXPECT_EQ(-1, _lofar_udp_parse_header_buffers(meta, headers, beamletLimits));
		source->bitMode = 2;

		// source->clockBit
		source->clockBit = 1;
		EXPECT_EQ(-1, _lofar_udp_parse_header_buffers(meta, headers, beamletLimits));
		source->clockBit = 0;



		// meta->portRawBeamlets[port] = (int32_t) ((uint8_t) header[CEP_HDR_NBEAM_OFFSET]);
		// Varying beamlet limits will change the fraction of data processed, and possibly number of
		//  ports processed in total
		beamletLimits[0] = 1; beamletLimits[1] = 733;
		EXPECT_EQ(0, _lofar_udp_parse_header_buffers(meta, headers, beamletLimits));
		EXPECT_EQ(MAX_NUM_PORTS * UDPMAXBEAM, meta->totalRawBeamlets);
		FREE_NOT_NULL(meta);
		meta = _lofar_udp_configure_obs_meta(config);
		ASSERT_NE(nullptr, meta);

		beamletLimits[0] = 1; beamletLimits[1] = 246;
		EXPECT_EQ(0, _lofar_udp_parse_header_buffers(meta, headers, beamletLimits));
		EXPECT_EQ(2 * UDPMAXBEAM, meta->totalRawBeamlets);
		EXPECT_EQ(beamletLimits[1] - beamletLimits[0], meta->totalProcBeamlets);
		EXPECT_EQ(beamletLimits[0], meta->baseBeamlets[0]);
		EXPECT_EQ(UDPMAXBEAM, meta->upperBeamlets[0]);

		beamletLimits[0] = 6; beamletLimits[1] = 245;
		EXPECT_EQ(0, _lofar_udp_parse_header_buffers(meta, headers, beamletLimits));
		EXPECT_EQ(2 * UDPMAXBEAM, meta->totalRawBeamlets);
		EXPECT_EQ(beamletLimits[1] - beamletLimits[0], meta->totalProcBeamlets);
		EXPECT_EQ(beamletLimits[0], meta->baseBeamlets[0]);
		EXPECT_EQ(0, meta->baseBeamlets[1]);
		EXPECT_EQ(UDPMAXBEAM, meta->upperBeamlets[0]);
		EXPECT_EQ(1, meta->upperBeamlets[1]);

		beamletLimits[0] = 1; beamletLimits[1] = 244;
		EXPECT_EQ(0, _lofar_udp_parse_header_buffers(meta, headers, beamletLimits));
		EXPECT_EQ(UDPMAXBEAM, meta->totalRawBeamlets);
		EXPECT_EQ(beamletLimits[1] - beamletLimits[0], meta->totalProcBeamlets);
		EXPECT_EQ(beamletLimits[0], meta->baseBeamlets[0]);
		EXPECT_EQ(UDPMAXBEAM, meta->upperBeamlets[0]);

		beamletLimits[0] = 1; beamletLimits[1] = 4;
		EXPECT_EQ(0, _lofar_udp_parse_header_buffers(meta, headers, beamletLimits));
		EXPECT_EQ(UDPMAXBEAM, meta->totalRawBeamlets);
		EXPECT_EQ(beamletLimits[1] - beamletLimits[0], meta->totalProcBeamlets);
		EXPECT_EQ(beamletLimits[0], meta->baseBeamlets[0]);
		EXPECT_EQ(beamletLimits[1], meta->upperBeamlets[0]);

		// meta->stationID = *((int16_t *) &(header[CEP_HDR_STN_ID_OFFSET])) / 32;
		EXPECT_EQ(333, meta->stationID);
		free(meta);
		free(config);
	}

	free(header);
};

TEST(LibReaderTests, PreprocessingReader) {
	//lofar_udp_reader *reader = reader_setup(150);

	//int _lofar_udp_skip_to_packet(lofar_udp_reader *reader)
	//int lofar_udp_get_first_packet_alignment(lofar_udp_reader *reader)

	{
		//int lofar_udp_setup_processing(lofar_udp_obs_meta *meta)
		SCOPED_TRACE("_lofar_udp_setup_processing");
		lofar_udp_obs_meta *meta = _lofar_udp_obs_meta_alloc();
		meta->numPorts = numPorts;

		meta->processingMode = UNSET_MODE;
		EXPECT_EQ(-1, _lofar_udp_setup_processing(meta));

		for (int32_t inputBitMode : bitModes) {
			if (inputBitMode == invalidBitMode) continue;

			meta->inputBitMode = inputBitMode;
			meta->totalProcBeamlets = 61 * numPorts * (16 / inputBitMode);

			for (int32_t port = 0; port < numPorts; port++) {
				meta->portPacketLength[port] = UDPHDRLEN + 61 * (16 / inputBitMode) * UDPNTIMESLICE * UDPNPOL;
			}

			for (calibrate_t calibrateData: calibrationModes) {
				if (calibrateData == invalidCalibration) continue;
				meta->calibrateData = calibrateData;

				for (int32_t processingMode: std::vector<int32_t>{ 0, 1 }) {
					meta->processingMode = (processMode_t) processingMode;
					EXPECT_EQ(0, _lofar_udp_setup_processing(meta));
					EXPECT_EQ(meta->inputBitMode, meta->outputBitMode);
					EXPECT_EQ(meta->numPorts, meta->numOutputs);

					if (calibrateData == APPLY_CALIBRATION) {
						EXPECT_EQ(GENERATE_JONES, meta->calibrateData);
					} else {
						EXPECT_EQ(calibrateData, meta->calibrateData);
					}

					for (int32_t port = 0; port < meta->numOutputs; port++) {
						EXPECT_EQ(meta->portPacketLength[port] + -1 * UDPHDRLEN * processingMode, meta->packetOutputLength[port]);
					}
				}
				meta->calibrateData = calibrateData;

				for (int32_t processingMode: std::vector<int32_t>{ 2, 11, 21, 31 }) {
					meta->processingMode = (processMode_t) processingMode;
					EXPECT_EQ(0, _lofar_udp_setup_processing(meta));
					EXPECT_EQ(UDPNPOL, meta->numOutputs);
					EXPECT_EQ(calibrateData == APPLY_CALIBRATION ? 32 : (meta->inputBitMode == 4 ? 8 : meta->inputBitMode), meta->outputBitMode);

					for (int32_t port = 0; port < meta->numOutputs; port++) {
						EXPECT_EQ(meta->totalProcBeamlets * UDPNPOL * UDPNTIMESLICE * (meta->outputBitMode / 8) / meta->numOutputs, meta->packetOutputLength[port]);
					}
				}

				for (int32_t processingMode: std::vector<int32_t>{ 32, 35 }) {
					meta->processingMode = (processMode_t) processingMode;
					EXPECT_EQ(0, _lofar_udp_setup_processing(meta));
					if (processingMode == 35 || calibrateData == APPLY_CALIBRATION) {
						EXPECT_EQ(32, meta->outputBitMode);
					} else {
						EXPECT_EQ(meta->inputBitMode == 4 ? 8 : meta->inputBitMode, meta->outputBitMode);
					}
					EXPECT_EQ(2, meta->numOutputs);
					for (int32_t port = 0; port < meta->numOutputs; port++) {
						EXPECT_EQ(meta->totalProcBeamlets * UDPNPOL * UDPNTIMESLICE * (meta->outputBitMode  / 8) / meta->numOutputs, meta->packetOutputLength[port]);
					}
				}

				for (int32_t processingMode: std::vector<int32_t>{ 10, 20, 30 }) {
					meta->processingMode = (processMode_t) processingMode;
					EXPECT_EQ(0, _lofar_udp_setup_processing(meta));
					EXPECT_EQ(1, meta->numOutputs);
					EXPECT_EQ(calibrateData == APPLY_CALIBRATION ? 32 : (meta->inputBitMode == 4 ? 8 : meta->inputBitMode), meta->outputBitMode);
					for (int32_t port = 0; port < meta->numOutputs; port++) {
						EXPECT_EQ(meta->totalProcBeamlets * UDPNPOL * UDPNTIMESLICE * (meta->outputBitMode  / 8) / meta->numOutputs, meta->packetOutputLength[port]);
					}
				}

				for (int32_t processingMode : std::vector<int32_t>{ 100, 101, 102, 103, 104,
																	110, 111, 112, 113, 114,
																	120, 121, 122, 123, 124,
																	130, 131, 132, 133, 134,
																	150, 151, 152, 153, 154,
																	160, 161, 162, 163, 164 }) {
					meta->processingMode = (processMode_t) processingMode;
					EXPECT_EQ(0, _lofar_udp_setup_processing(meta));
					if (processingMode < 150) {
						EXPECT_EQ(1, meta->numOutputs);
					} else if (processingMode < 160) {
						EXPECT_EQ(4, meta->numOutputs);
					} else {
						EXPECT_EQ(2, meta->numOutputs);
					}
					EXPECT_EQ(32, meta->outputBitMode);
					for (int32_t port = 0; port < meta->numOutputs; port++) {
						int32_t scaleFactor = 1 << (processingMode % 10);
						EXPECT_EQ(meta->totalProcBeamlets * UDPNTIMESLICE * sizeof(float) / scaleFactor, meta->packetOutputLength[port]);
					}
				}


			}
		}

		FREE_NOT_NULL(meta);


	}

	{
		SCOPED_TRACE("lofar_udp_file_reader_reuse");
		//int lofar_udp_file_reader_reuse(lofar_udp_reader *reader, const long startingPacket, const long packetsReadMax)
		EXPECT_NONFATAL_FAILURE(EXPECT_TRUE(false), "");
	}

};



// Input orders
const std::vector<processMode_t> inputsMatched = {PACKET_FULL_COPY, PACKET_NOHDR_COPY};

// Output sizes
//const std::vector<processMode_t> outputsMatched = {PACKET_FULL_COPY, PACKET_NOHDR_COPY};
const std::vector<processMode_t> output1Arr = {BEAMLET_MAJOR_FULL, BEAMLET_MAJOR_FULL_REV, TIME_MAJOR_FULL,
											   STOKES_I, STOKES_Q, STOKES_U, STOKES_V, STOKES_I_DS2, STOKES_Q_DS2,
											   STOKES_U_DS2, STOKES_V_DS2, STOKES_I_DS4, STOKES_Q_DS4, STOKES_U_DS4,
											   STOKES_V_DS4, STOKES_I_DS8, STOKES_Q_DS8, STOKES_U_DS8, STOKES_V_DS8,
											   STOKES_I_DS16, STOKES_Q_DS16, STOKES_U_DS16, STOKES_V_DS16, STOKES_I_REV,
											   STOKES_Q_REV, STOKES_U_REV, STOKES_V_REV, STOKES_I_DS2_REV, STOKES_Q_DS2_REV,
											   STOKES_U_DS2_REV, STOKES_V_DS2_REV, STOKES_I_DS4_REV, STOKES_Q_DS4_REV,
											   STOKES_U_DS4_REV, STOKES_V_DS4_REV, STOKES_I_DS8_REV, STOKES_Q_DS8_REV,
											   STOKES_U_DS8_REV, STOKES_V_DS8_REV, STOKES_I_DS16_REV, STOKES_Q_DS16_REV,
											   STOKES_U_DS16_REV, STOKES_V_DS16_REV, STOKES_I_TIME,
											   STOKES_Q_TIME, STOKES_U_TIME, STOKES_V_TIME, STOKES_I_DS2_TIME, STOKES_Q_DS2_TIME,
											   STOKES_U_DS2_TIME, STOKES_V_DS2_TIME, STOKES_I_DS4_TIME, STOKES_Q_DS4_TIME,
											   STOKES_U_DS4_TIME, STOKES_V_DS4_TIME, STOKES_I_DS8_TIME, STOKES_Q_DS8_TIME,
											   STOKES_U_DS8_TIME, STOKES_V_DS8_TIME, STOKES_I_DS16_TIME, STOKES_Q_DS16_TIME,
											   STOKES_U_DS16_TIME, STOKES_V_DS16_TIME};
const std::vector<processMode_t> output2Arr = {TIME_MAJOR_ANT_POL, TIME_MAJOR_ANT_POL_FLOAT, STOKES_IV, STOKES_IV_DS2,
											   STOKES_IV_DS4, STOKES_IV_DS8, STOKES_IV_DS16, STOKES_IV_REV,
											   STOKES_IV_DS2_REV, STOKES_IV_DS4_REV, STOKES_IV_DS8_REV,
											   STOKES_IV_DS16_REV, STOKES_IV_TIME, STOKES_IV_DS2_TIME, STOKES_IV_DS4_TIME,
											   STOKES_IV_DS8_TIME, STOKES_IV_DS16_TIME};
const std::vector<processMode_t> output4Arr = {PACKET_SPLIT_POL, BEAMLET_MAJOR_SPLIT_POL, BEAMLET_MAJOR_SPLIT_POL_REV,
											   TIME_MAJOR_SPLIT_POL, STOKES_IQUV, STOKES_IQUV_DS2, STOKES_IQUV_DS4,
											   STOKES_IQUV_DS8, STOKES_IQUV_DS16, STOKES_IQUV_REV, STOKES_IQUV_DS2_REV,
											   STOKES_IQUV_DS4_REV, STOKES_IQUV_DS8_REV, STOKES_IQUV_DS16_REV,
											   STOKES_IQUV_TIME, STOKES_IQUV_DS2_TIME, STOKES_IQUV_DS4_TIME, STOKES_IQUV_DS8_TIME,
											   STOKES_IQUV_DS16_TIME};

// Output bases
//const std::vector<processMode_t> ouputBaseMatched = {PACKET_FULL_COPY, PACKET_NOHDR_COPY};
const std::vector<processMode_t> outputBaseFreqMajor = {PACKET_SPLIT_POL, BEAMLET_MAJOR_FULL, BEAMLET_MAJOR_SPLIT_POL};
const std::vector<processMode_t> outputBaseRevFreqMajor = {BEAMLET_MAJOR_FULL_REV, BEAMLET_MAJOR_SPLIT_POL_REV};
const std::vector<processMode_t> outputBaseTimeMajor = {TIME_MAJOR_FULL, TIME_MAJOR_ANT_POL, TIME_MAJOR_ANT_POL_FLOAT, TIME_MAJOR_SPLIT_POL};
const std::vector<processMode_t> outputBaseStokesDeci = {STOKES_I, STOKES_Q, STOKES_U, STOKES_V, STOKES_IQUV,
														 STOKES_IV, STOKES_I_DS2, STOKES_Q_DS2, STOKES_U_DS2,
														 STOKES_V_DS2, STOKES_IQUV_DS2, STOKES_IV_DS2, STOKES_I_DS4,
														 STOKES_Q_DS4, STOKES_U_DS4, STOKES_V_DS4, STOKES_IQUV_DS4,
														 STOKES_IV_DS4, STOKES_I_DS8, STOKES_Q_DS8, STOKES_U_DS8,
														 STOKES_V_DS8, STOKES_IQUV_DS8, STOKES_IV_DS8, STOKES_I_DS16,
														 STOKES_Q_DS16, STOKES_U_DS16, STOKES_V_DS16, STOKES_IQUV_DS16,
														 STOKES_IV_DS16, STOKES_I_REV, STOKES_Q_REV, STOKES_U_REV,
														 STOKES_V_REV, STOKES_IQUV_REV, STOKES_IV_REV, STOKES_I_DS2_REV,
														 STOKES_Q_DS2_REV, STOKES_U_DS2_REV, STOKES_V_DS2_REV,
														 STOKES_IQUV_DS2_REV, STOKES_IV_DS2_REV, STOKES_I_DS4_REV,
														 STOKES_Q_DS4_REV, STOKES_U_DS4_REV, STOKES_V_DS4_REV,
														 STOKES_IQUV_DS4_REV, STOKES_IV_DS4_REV, STOKES_I_DS8_REV,
														 STOKES_Q_DS8_REV, STOKES_U_DS8_REV, STOKES_V_DS8_REV,
														 STOKES_IQUV_DS8_REV, STOKES_IV_DS8_REV, STOKES_I_DS16_REV,
														 STOKES_Q_DS16_REV, STOKES_U_DS16_REV, STOKES_V_DS16_REV,
														 STOKES_IQUV_DS16_REV, STOKES_IV_DS16_REV,
														 STOKES_I_TIME, STOKES_Q_TIME, STOKES_U_TIME,
														 STOKES_V_TIME, STOKES_IQUV_TIME, STOKES_IV_TIME, STOKES_I_DS2_TIME,
														 STOKES_Q_DS2_TIME, STOKES_U_DS2_TIME, STOKES_V_DS2_TIME,
														 STOKES_IQUV_DS2_TIME, STOKES_IV_DS2_TIME, STOKES_I_DS4_TIME,
														 STOKES_Q_DS4_TIME, STOKES_U_DS4_TIME, STOKES_V_DS4_TIME,
														 STOKES_IQUV_DS4_TIME, STOKES_IV_DS4_TIME, STOKES_I_DS8_TIME,
														 STOKES_Q_DS8_TIME, STOKES_U_DS8_TIME, STOKES_V_DS8_TIME,
														 STOKES_IQUV_DS8_TIME, STOKES_IV_DS8_TIME, STOKES_I_DS16_TIME,
														 STOKES_Q_DS16_TIME, STOKES_U_DS16_TIME, STOKES_V_DS16_TIME,
														 STOKES_IQUV_DS16_TIME, STOKES_IV_DS16_TIME};

// Output offsets
//const std::vector<processMode_t> outputOffsetMatches = {PACKET_FULL_COPY, PACKET_NOHDR_COPY};
const std::vector<processMode_t> outputOffsetTs = {PACKET_SPLIT_POL, TIME_MAJOR_SPLIT_POL};
const std::vector<processMode_t> outputOffstTsBeamPol = {BEAMLET_MAJOR_FULL, BEAMLET_MAJOR_FULL_REV};
const std::vector<processMode_t> outputOffsetTsBeam = {BEAMLET_MAJOR_SPLIT_POL, BEAMLET_MAJOR_SPLIT_POL_REV};
const std::vector<processMode_t> outputOffsetTsNpol = {TIME_MAJOR_FULL};
const std::vector<processMode_t> outputOffsetTsAnt = {TIME_MAJOR_ANT_POL, TIME_MAJOR_ANT_POL_FLOAT};
const std::vector<processMode_t> outputOffsetStokesDeci = {STOKES_I, STOKES_Q, STOKES_U, STOKES_V, STOKES_IQUV,
                                                           STOKES_IV, STOKES_I_DS2, STOKES_Q_DS2, STOKES_U_DS2,
                                                           STOKES_V_DS2, STOKES_IQUV_DS2, STOKES_IV_DS2, STOKES_I_DS4,
                                                           STOKES_Q_DS4, STOKES_U_DS4, STOKES_V_DS4, STOKES_IQUV_DS4,
                                                           STOKES_IV_DS4, STOKES_I_DS8, STOKES_Q_DS8, STOKES_U_DS8,
                                                           STOKES_V_DS8, STOKES_IQUV_DS8, STOKES_IV_DS8, STOKES_I_DS16,
                                                           STOKES_Q_DS16, STOKES_U_DS16, STOKES_V_DS16, STOKES_IQUV_DS16,
                                                           STOKES_IV_DS16, STOKES_I_REV, STOKES_Q_REV, STOKES_U_REV,
                                                           STOKES_V_REV, STOKES_IQUV_REV, STOKES_IV_REV, STOKES_I_DS2_REV,
                                                           STOKES_Q_DS2_REV, STOKES_U_DS2_REV, STOKES_V_DS2_REV,
                                                           STOKES_IQUV_DS2_REV, STOKES_IV_DS2_REV, STOKES_I_DS4_REV,
                                                           STOKES_Q_DS4_REV, STOKES_U_DS4_REV, STOKES_V_DS4_REV,
                                                           STOKES_IQUV_DS4_REV, STOKES_IV_DS4_REV, STOKES_I_DS8_REV,
                                                           STOKES_Q_DS8_REV, STOKES_U_DS8_REV, STOKES_V_DS8_REV,
                                                           STOKES_IQUV_DS8_REV, STOKES_IV_DS8_REV, STOKES_I_DS16_REV,
                                                           STOKES_Q_DS16_REV, STOKES_U_DS16_REV, STOKES_V_DS16_REV,
                                                           STOKES_IQUV_DS16_REV, STOKES_IV_DS16_REV,
                                                           STOKES_I_TIME, STOKES_Q_TIME, STOKES_U_TIME,
                                                           STOKES_V_TIME, STOKES_IQUV_TIME, STOKES_IV_TIME, STOKES_I_DS2_TIME,
                                                           STOKES_Q_DS2_TIME, STOKES_U_DS2_TIME, STOKES_V_DS2_TIME,
                                                           STOKES_IQUV_DS2_TIME, STOKES_IV_DS2_TIME, STOKES_I_DS4_TIME,
                                                           STOKES_Q_DS4_TIME, STOKES_U_DS4_TIME, STOKES_V_DS4_TIME,
                                                           STOKES_IQUV_DS4_TIME, STOKES_IV_DS4_TIME, STOKES_I_DS8_TIME,
                                                           STOKES_Q_DS8_TIME, STOKES_U_DS8_TIME, STOKES_V_DS8_TIME,
                                                           STOKES_IQUV_DS8_TIME, STOKES_IV_DS8_TIME, STOKES_I_DS16_TIME,
                                                           STOKES_Q_DS16_TIME, STOKES_U_DS16_TIME, STOKES_V_DS16_TIME,
                                                           STOKES_IQUV_DS16_TIME, STOKES_IV_DS16_TIME};


#define VECTOR_CONTAINS(vector, findme) \
	std::find(vector.begin(), vector.end(), (findme)) != vector.end()

const int16_t packetLength = UDPHDRLEN + UDPMAXBEAM * 2 * UDPNTIMESLICE;
typedef struct packets {
	union {
		int8_t data[packetLength];
		struct {
			int8_t udpHeader[UDPHDRLEN];
			int8_t beamData[UDPMAXBEAM / 2][UDPNTIMESLICE][UDPNPOL];
		};
	};
} packets;

template <typename O>
static std::tuple<int, int, int, int> processChecker(lofar_udp_reader *reader) {
	if (8 != reader->meta->inputBitMode) {
		return std::tuple<int, int, int, int>(-1, -1, -1, reader->meta->inputBitMode);
	}

	float outputArrStep;
	std::function<int64_t(int64_t)> tsPolStepFunc;
	if (VECTOR_CONTAINS(output1Arr, reader->meta->processingMode)) {
		if (1 != reader->meta->numOutputs) {
			return std::tuple<int, int, int, int>(-1, -1, -2, reader->meta->numOutputs);
		}
		outputArrStep = 0.0f;
		tsPolStepFunc = [](int64_t pol) -> int64_t {
			return pol;
		};
	} else if (VECTOR_CONTAINS(output2Arr, reader->meta->processingMode)) {
		if (2 != reader->meta->numOutputs) {
			return std::tuple<int, int, int, int>(-1, -1, -3, reader->meta->numOutputs);
		}
		outputArrStep = 0.5f;
		tsPolStepFunc = [](int64_t pol) -> int64_t {
			return pol % 2;
		};
	} else if (VECTOR_CONTAINS(output4Arr, reader->meta->processingMode)) {
		if (4 != reader->meta->numOutputs) {
			return std::tuple<int, int, int, int>(-1, -1, -4, reader->meta->numOutputs);
		}
		outputArrStep = 1.0f;
		tsPolStepFunc = [](int64_t pol) -> int64_t {
			return 0 * pol;
		};
	} else if (reader->meta->processingMode != PACKET_NOHDR_COPY) {
		return std::tuple<int, int, int, int>(-1, -1, -5, reader->meta->processingMode);
	}

	float tsOutputStepVal;
	if (VECTOR_CONTAINS(outputOffsetTs, reader->meta->processingMode)) {
		tsOutputStepVal = 1;
	} else if (VECTOR_CONTAINS(outputOffstTsBeamPol, reader->meta->processingMode)) {
		tsOutputStepVal = reader->meta->totalProcBeamlets * UDPNPOL;
	} else if (VECTOR_CONTAINS(outputOffsetTsBeam, reader->meta->processingMode)) {
		tsOutputStepVal = reader->meta->totalProcBeamlets;
	} else if (VECTOR_CONTAINS(outputOffsetTsNpol, reader->meta->processingMode)) {
		tsOutputStepVal = UDPNPOL;
	} else if (VECTOR_CONTAINS(outputOffsetTsAnt, reader->meta->processingMode)) {
		tsOutputStepVal = (UDPNPOL / 2);
	} else if (VECTOR_CONTAINS(outputOffsetStokesDeci, reader->meta->processingMode)) {
		if (reader->meta->processingMode < STOKES_I_TIME) {
			tsOutputStepVal = reader->meta->totalProcBeamlets;
		} else {
			tsOutputStepVal = 1;
		}
	} else if (reader->meta->processingMode != PACKET_NOHDR_COPY) {
		return std::tuple<int, int, int, int>(-1, -1, -6, reader->meta->processingMode);
	}

	std::vector<O*> outputs;
	std::vector<int8_t*> outputIdx;
	for (int16_t i = 0; i < reader->meta->numOutputs; i++) {
		outputs.push_back(static_cast<O*>(calloc(reader->meta->packetOutputLength[i] * reader->meta->packetsPerIteration, sizeof(int8_t))));
		outputIdx.push_back(static_cast<int8_t*>(calloc(reader->meta->packetOutputLength[i] * reader->meta->packetsPerIteration / sizeof(O), sizeof(int8_t))));
	}

	std::vector<int8_t*> inputIdx;
	for (int16_t i = 0; i < reader->meta->numPorts; i++) {
		inputIdx.push_back(static_cast<int8_t*>(calloc((reader->meta->portPacketLength[i] - UDPHDRLEN) * reader->meta->packetsPerIteration, sizeof(int8_t))));
	}


	// Stokes Mode checks
	int32_t baseMode = ((int32_t) reader->meta->processingMode) % 100;
	int8_t deciFac = baseMode % 10;
	baseMode -= deciFac;
	deciFac = (int8_t) (1 << deciFac);

	int8_t stokesIB = 0, stokesQB = 0, stokesUB = 0, stokesVB = 0;
	if (baseMode == 0 || baseMode == 50 || baseMode == 60) {
		stokesIB = 1;
	}
	if (baseMode == 10 || baseMode == 50) {
		stokesQB = 1;
	}
	if (baseMode == 20 || baseMode == 50) {
		stokesUB = 1;
	}
	if (baseMode == 30 || baseMode == 50 || baseMode == 60) {
		stokesVB = 1;
	}


	for (int8_t port = 0; port < reader->meta->numPorts; port++) {
		//const int64_t portPacketLength = reader->meta->portPacketLength[port];
		const int64_t outputPacketLength = reader->meta->packetOutputLength[0];
		packets *workingDataInput = (packets *) reader->meta->inputData[port];

		int64_t scale;
		std::function<int64_t(int64_t,int64_t)> tsOutputBeamFunc;
		if (VECTOR_CONTAINS(outputBaseFreqMajor, reader->meta->processingMode)) {
			scale = (reader->meta->processingMode == PACKET_SPLIT_POL) * UDPNTIMESLICE +
			        (reader->meta->processingMode == BEAMLET_MAJOR_FULL) * UDPNPOL +
			        (reader->meta->processingMode == BEAMLET_MAJOR_SPLIT_POL) * 1;
			tsOutputBeamFunc = [cumulative = reader->meta->portCumulativeBeamlets[port], scale](int64_t base, int64_t beamlet) -> int64_t {
				return base + (beamlet + cumulative) * scale;
			};
		} else if (VECTOR_CONTAINS(outputBaseRevFreqMajor, reader->meta->processingMode)) {
			scale = (reader->meta->processingMode == BEAMLET_MAJOR_FULL_REV) * UDPNPOL +
			        (reader->meta->processingMode == BEAMLET_MAJOR_SPLIT_POL_REV) * 1;
			tsOutputBeamFunc = [cumulative = reader->meta->portCumulativeBeamlets[port], total = reader->meta->totalProcBeamlets, scale](int64_t base, int64_t beamlet) -> int64_t {
				return base + (total - 1 - beamlet - cumulative) * scale;
			};
		} else if (VECTOR_CONTAINS(outputBaseTimeMajor, reader->meta->processingMode)) {
			// return  + packet * UDPNTIMESLICE
			scale = (reader->meta->processingMode == TIME_MAJOR_FULL) * UDPNPOL +
				(reader->meta->processingMode == TIME_MAJOR_ANT_POL || reader->meta->processingMode == TIME_MAJOR_ANT_POL_FLOAT) * (UDPNPOL / 2) +
				(reader->meta->processingMode == TIME_MAJOR_SPLIT_POL) * 1;
			tsOutputBeamFunc = [packets = reader->meta->packetsPerIteration, cumulative = reader->meta->portCumulativeBeamlets[port], scale](int64_t packet, int64_t beamlet) -> int64_t {
				return ((beamlet + cumulative) * (packets * UDPNTIMESLICE) + (packet)) * scale;
			};
		} else if (VECTOR_CONTAINS(outputBaseStokesDeci, reader->meta->processingMode)) {
			scale = 1;
			if (reader->meta->processingMode < STOKES_I_REV) {
				tsOutputBeamFunc = [cumulative = reader->meta->portCumulativeBeamlets[port], scale](int64_t base, int64_t beamlet) -> int64_t {
					return base + (beamlet + cumulative) * scale;
				};
			} else if (reader->meta->processingMode < STOKES_I_TIME) {
				tsOutputBeamFunc = [cumulative = reader->meta->portCumulativeBeamlets[port], total = reader->meta->totalProcBeamlets, scale](int64_t base, int64_t beamlet) -> int64_t {
					return base + (total - 1 - beamlet - cumulative) * scale;
				};
			} else {
				tsOutputBeamFunc = [deciFac, packets = reader->meta->packetsPerIteration, cumulative = reader->meta->portCumulativeBeamlets[port], scale](int64_t packet, int64_t beamlet) -> int64_t {
					return ((beamlet + cumulative) * (packets * UDPNTIMESLICE / deciFac) + (packet)) * scale;
				};
			}
		} else if (reader->meta->processingMode > PACKET_NOHDR_COPY) {
			return std::tuple<int, int, int, int>(-1, -1, -7, reader->meta->processingMode);
		}


		for (int64_t packet = 0; packet < reader->meta->packetsPerIteration; packet++) {
			packets workingData = workingDataInput[packet];
			int64_t inputOffset, outputOffset;

			// Packet copy
			if (VECTOR_CONTAINS(inputsMatched, reader->meta->processingMode)) {
				inputOffset = (reader->meta->processingMode == PACKET_NOHDR_COPY) * UDPHDRLEN;
				outputOffset = packet * reader->meta->packetOutputLength[port];
				int64_t memcmpval;
				if ((memcmpval = memcmp(&(reader->meta->outputData[port][outputOffset]), &(workingData.data[inputOffset]), reader->meta->packetOutputLength[port]))) {
					return std::tuple<int, int, int, int>(port, packet, 1, memcmpval);
				}
				continue;
			}

			int64_t tsOutputBaseVal;
			if (VECTOR_CONTAINS(outputBaseFreqMajor, reader->meta->processingMode)) {
				tsOutputBaseVal = packet * outputPacketLength / sizeof(O);
			} else if (VECTOR_CONTAINS(outputBaseRevFreqMajor, reader->meta->processingMode)) {
				tsOutputBaseVal = packet * outputPacketLength / sizeof(O);
			} else if (VECTOR_CONTAINS(outputBaseTimeMajor, reader->meta->processingMode)) {
				tsOutputBaseVal = packet * UDPNTIMESLICE;
			} else if (VECTOR_CONTAINS(outputBaseStokesDeci, reader->meta->processingMode) ){
				if (reader->meta->processingMode < STOKES_I_TIME) {
					tsOutputBaseVal = packet * outputPacketLength / sizeof(O);
				} else {
					tsOutputBaseVal = packet * UDPNTIMESLICE / deciFac;
				}
			} else {
				return std::tuple<int, int, int, int>(-1, -1, -8, reader->meta->processingMode);
			}


			for (int16_t beamlet = 0; beamlet < reader->meta->upperBeamlets[port]; beamlet++) {
				int64_t tsOutBase = tsOutputBeamFunc(tsOutputBaseVal, beamlet);

				if (reader->meta->processingMode < STOKES_I) {
					for (int16_t ts = 0; ts < UDPNTIMESLICE; ts++) {
						int64_t tsOutOffset = tsOutBase + (int64_t) (ts * tsOutputStepVal);

						for (int8_t pol = 0; pol < UDPNPOL; pol++) {
							int64_t outIdx = tsOutOffset + tsPolStepFunc(pol);
							int8_t outArr = (int8_t) (outputArrStep * pol);
							outputs[outArr][outIdx] = workingData.beamData[beamlet][ts][pol];

							inputIdx[port][packet * 7808 + beamlet * UDPNTIMESLICE * UDPNPOL + ts * UDPNPOL + pol] = 1;
							outputIdx[(int8_t) (outputArrStep * pol)][outIdx] = 1;
						}
						while (0) {};
					}
				} else if constexpr (sizeof(O) == 4) {
					for (int8_t tsD = 0; tsD < UDPNTIMESLICE / deciFac; tsD++) {
						float stokesIV = 0, stokesQV = 0, stokesUV = 0, stokesVV = 0;
						int64_t tsOutOffset = tsOutBase + (int64_t) (tsD * tsOutputStepVal);
						for (int8_t dec = 0; dec < deciFac; dec++) {
							int8_t ts = (int8_t) tsD * deciFac + dec;
							for (int8_t pol = 0; pol < UDPNPOL; pol++)
								inputIdx[port][packet * 7808 + beamlet * UDPNTIMESLICE * UDPNPOL + ts * UDPNPOL + pol] = 1;

							if (stokesIB) stokesIV += stokesI(workingData.beamData[beamlet][ts][0], workingData.beamData[beamlet][ts][1], workingData.beamData[beamlet][ts][2], workingData.beamData[beamlet][ts][3]);
							if (stokesQB) stokesQV += stokesQ(workingData.beamData[beamlet][ts][0], workingData.beamData[beamlet][ts][1], workingData.beamData[beamlet][ts][2], workingData.beamData[beamlet][ts][3]);
							if (stokesUB) stokesUV += stokesU(workingData.beamData[beamlet][ts][0], workingData.beamData[beamlet][ts][1], workingData.beamData[beamlet][ts][2], workingData.beamData[beamlet][ts][3]);
							if (stokesVB) stokesVV += stokesV(workingData.beamData[beamlet][ts][0], workingData.beamData[beamlet][ts][1], workingData.beamData[beamlet][ts][2], workingData.beamData[beamlet][ts][3]);
						}
						int32_t index = 0;
						if (stokesIB) outputs[index++][tsOutOffset] = stokesIV;
						if (stokesQB) outputs[index++][tsOutOffset] = stokesQV;
						if (stokesUB) outputs[index++][tsOutOffset] = stokesUV;
						if (stokesVB) outputs[index++][tsOutOffset] = stokesVV;

						index = 0;
						if (stokesIB) outputIdx[index++][tsOutOffset] = 1;
						if (stokesQB) outputIdx[index++][tsOutOffset] = 1;
						if (stokesUB) outputIdx[index++][tsOutOffset] = 1;
						if (stokesVB) outputIdx[index++][tsOutOffset] = 1;
					}
				} else {
					return std::tuple<int, int, int, int>(-1, -1, -9, -1);
				}
			}
		}
	}

	if (VECTOR_CONTAINS(inputsMatched, reader->meta->processingMode)) {
		return std::tuple<int, int, int, int>(0, 0, 0, 0);
	}

	for (int8_t port = 0; port < reader->meta->numPorts; port++) {
		for (int64_t idx = 0; idx < reader->meta->packetsPerIteration * (reader->meta->portPacketLength[port] - UDPHDRLEN); idx++) {
			if (!inputIdx[port][idx]) {
				printf("Input never read: %d %ld\n", port, idx);
			}
		}
	}
	for (int8_t port = 0; port < reader->meta->numOutputs; port++) {
		for (int64_t idx = 0; idx < reader->meta->packetsPerIteration * reader->meta->packetOutputLength[port] / sizeof(O); idx++) {
			if (!outputIdx[port][idx]) {
				printf("Output never written: %d %ld\n", port, idx);
			}
		}
	}


	for (int8_t outp = 0; outp < reader->meta->numOutputs; outp++) {
		int memcmpval = 0;
		if ((memcmpval = memcmp(reader->meta->outputData[outp], outputs[outp], reader->meta->packetsPerIteration * reader->meta->packetOutputLength[outp]))) {
			// Memcmp may fail as a result of ffast-math, do some manual checks.
			for (int64_t i = 0; i < reader->meta->packetsPerIteration * reader->meta->packetOutputLength[outp] / sizeof(O); i++) {
				if constexpr (sizeof(O) == 1) {
					if (reader->meta->outputData[outp][i] != outputs[outp][i]) {
						//printf("%hhd, %ld: %hhd, %hhd\n", outp, i, reader->meta->outputData[outp][i], outputs[outp][i]);
						return std::tuple<int, int, int, int>(outp, i, 2, memcmpval);
					}
				} else {
					if (((float*) reader->meta->outputData[outp])[i] != outputs[outp][i]) {
						//printf("%hhd, %ld: %f, %f\n", outp, i, ((float *) reader->meta->outputData[outp])[i], outputs[outp][i]);
						return std::tuple<int, int, int, int>(outp, i, 2, memcmpval);
					}
				}
			}
		}
	}

	return std::tuple<int, int, int, int>(0, 0, 0, 0);
}

static std::tuple<int, int, int, int> packetChecker(int64_t packetsPerBlock, int64_t lastPacket, int8_t **inputData, int8_t numInpPorts, int8_t **outputData, int8_t numOutputs) {
	if (numInpPorts != numOutputs) return std::tuple<int, int, int, int>(-1, -1, -1, 0);
	for (int8_t port = 0; port < numInpPorts; port++) {
		int64_t parsedPackets = 0, workingPack = lastPacket - packetsPerBlock;
		packets *inpPort = (packets*) inputData[port];
		packets *outPort = (packets*) outputData[port];

		for (int64_t packet = 0; packet < packetsPerBlock; packet++) {
			int64_t delta = (workingPack + 1) - lofar_udp_time_get_packet_number(inpPort[parsedPackets].data);
			//if (packet == 0 || packet == (packetsPerBlock - 1) || delta) printf("%ld, %ld (%ld)\n", workingPack + 1, lofar_udp_time_get_packet_number(inpPort[parsedPackets].data), delta);
			// If the current packet is the next input packet, ensure the output matches
			if (!delta) {
				if ((delta = memcmp(inpPort[parsedPackets].data, outPort[packet].data, packetLength))) {
					// Check if it matches the manually dropped packet signature
					*((int32_t*) &(inpPort[parsedPackets].data[CEP_HDR_SEQ_OFFSET])) += UDPNTIMESLICE;
					((lofar_source_bytes *) &(inpPort[parsedPackets].data[CEP_HDR_SRC_OFFSET]))->padding1 = 1;
					if ((delta = memcmp(inpPort[parsedPackets].data, outPort[packet].data, packetLength))) {
						return std::tuple<int, int, int, int>(port, packet, 0, delta);
					}
				}
				parsedPackets++;
			// Otherwise, the output should be empty given replayLastPacket disabled
			} else {
				for (int16_t i = UDPHDRLEN; i < packetLength; i++) {
					if (outPort[packet].data[i]) {
						return std::tuple<int, int, int, int>(port, packet, 1, 0);
					}
				}
			}
			workingPack++;
		}
	}

	return std::tuple<int, int, int, int>(0, 0, 0, 0);
}

const std::vector<int32_t> expectedIters = {
	0, // Test case 0: No input
	16, // Test case 1: Perfect
	16, // Test case 2: Perfect
	16, // Test case 3: 1 packet dropped
	14, // Test case 4: 1 iteration dropped
	0,  // Test case 5: Entire port dropped
	0,  // Test case 6: Port misalignment, no common time
	16, // Test case 7: Missing a target packet
	16, // Test case 8: Missing a target packet, and 2 iterations after it
	16, // Test case 9: All iters, lots of lost packets
};
const std::vector<std::vector<int32_t>> expectedDroppedPackets = {
	{0, 0, 0, 0}, // Test case 0: No lost packets (no ops)
	{0, 0, 0, 0}, // Test case 1: No lost packets
	{0, 0, 0, 0}, // Test case 2: No lost packets
	{0, /*1*/ 0 , 0, 0}, // Test case 3: 1 lost packet, but it's at the tail so it's not used
	{0, /*33*/ 0, 0, 0}, // Test case 4: 33 lost packets, but it's at the tail so it's not used
	{0, 0, 0, 0}, // Test case 5: No lost packets (no ops)
	{0, 0, 0, 0}, // Test case 6: No lost packets (no ops)
	{8, 8, 8, 8}, // Test case 7: 8 lost packets on all ports
	{33, 33, 33, 33}, // Test case 8: 33 lost packets on all ports
	{0, 19, 38, 57}, // Test case 9: Variable loss per-port
};
class LibReaderTestsParam : public testing::TestWithParam<std::tuple<processMode_t, calibrate_t, int32_t>> {};
TEST_P(LibReaderTestsParam, ProcessingData) {
	//lofar_udp_reader *reader = reader_setup(150);

	{
		SCOPED_TRACE("The big one");

		processMode_t currMode = std::get<0>(GetParam());
		calibrate_t cal = std::get<1>(GetParam());
		int32_t testNum = std::get<2>(GetParam());
		lofar_udp_config *config = config_setup(1, testNum, 4, INT32_MAX, cal);
		std::cout << config->inputLocations[0] << ", " << currMode << ", " << cal << std::endl;
		config->processingMode = currMode;
		#ifndef NO_TEST_CAL
		// Only do 1 mode with GENERATE_JONES to ensure the path is covered, otherwise everything else is
		//  caused by APPLY_CALIBRATION or NO_CALIBRATION
		//  .... and saved about 4 minutes on the test time.
		if (testNum == 1 && (currMode < 100 || (currMode > 100 && (currMode % 10 < 2)))) {
			if (currMode == PACKET_NOHDR_COPY) {
				config->calibrationDuration = 2.0f * (float) clock200MHzSampleTime * (float) (UDPNTIMESLICE * config->packetsPerIteration);
			} else {
				config->calibrationDuration = 0.1f;
			}
			config->calibrateData = cal;
		} else
		#endif
		{
			config->calibrateData = NO_CALIBRATION;
		}
		lofar_udp_reader *reader = lofar_udp_reader_setup(config);
		if (std::find(flaggedTests.begin(), flaggedTests.end(), testNum) != flaggedTests.end() || currMode == TEST_INVALID_MODE || currMode == UNSET_MODE) {
			EXPECT_EQ(reader, nullptr);
			return;
		} else {
			ASSERT_NE(reader, nullptr);
		}
		//printf("First LastPacket: %ld\n", reader->meta->lastPacket);

		int returnv, iters = 0;
		double timing[2];
		int8_t *buffers[MAX_OUTPUT_DIMS];
		for (int8_t outp = 0; outp < reader->meta->numOutputs; outp++)
			buffers[outp] = (int8_t*) calloc(reader->packetsPerIteration * reader->meta->packetOutputLength[outp], sizeof(int8_t));

		if (currMode != PACKET_FULL_COPY) {
			while ((returnv = lofar_udp_reader_step_timed(reader, timing)) < 1) {
				iters++;
				if (testNum == 1 && cal == NO_CALIBRATION) {
					std::tuple<int, int, int, int> tupleReturn;
					if (reader->meta->outputBitMode == 32) {
						tupleReturn = processChecker<float>(reader);
					} else if (reader->meta->outputBitMode == 8) {
						tupleReturn = processChecker<int8_t>(reader);
					} else {
						ASSERT_TRUE(false);
					}
					EXPECT_EQ(0, std::get<0>(tupleReturn)); // Port
					EXPECT_EQ(0, std::get<1>(tupleReturn)); // Packet
					EXPECT_EQ(0, std::get<2>(tupleReturn)); // Failure mode
					EXPECT_EQ(0, std::get<3>(tupleReturn)); // Extra info
					//ASSERT_EQ(0, std::get<0>(tupleReturn) + std::get<1>(tupleReturn) + std::get<2>(tupleReturn) + std::get<3>(tupleReturn));
				} else {
					EXPECT_NONFATAL_FAILURE(EXPECT_TRUE(false), "");
				}
			}
		} else {
			while ((returnv = lofar_udp_reader_step(reader)) < 1) {
				iters++;
				//printf("Iter %d\n", iters);
				auto tupleReturn = packetChecker(reader->meta->packetsPerIteration, reader->meta->lastPacket, reader->meta->inputData, reader->meta->numPorts,
				                                 reader->meta->outputData, reader->meta->numOutputs);
				EXPECT_EQ(0, std::get<0>(tupleReturn)); // Port
				EXPECT_EQ(0, std::get<1>(tupleReturn)); // Packet
				EXPECT_EQ(0, std::get<2>(tupleReturn)); // Failure mode
				EXPECT_EQ(0, std::get<3>(tupleReturn)); // Extra info
				ASSERT_EQ(0, std::get<0>(tupleReturn) + std::get<1>(tupleReturn) + std::get<2>(tupleReturn) + std::get<3>(tupleReturn));
			}
		}

		if ((reader->meta->calibrateData < APPLY_CALIBRATION || currMode < PACKET_SPLIT_POL) && currMode < STOKES_I && currMode != TIME_MAJOR_ANT_POL_FLOAT) {
			ASSERT_EQ(reader->metadata->nbit, reader->meta->inputBitMode);
		} else {
			ASSERT_EQ(reader->metadata->nbit, reader->meta->outputBitMode * -1);
		}

		//printf("Last returnv, iters: %d, %d\n", returnv, iters);
		ASSERT_EQ(iters, expectedIters[testNum]);
		for (int8_t port = 0; port < reader->meta->numPorts; port++) {
			EXPECT_EQ(reader->meta->portTotalDroppedPackets[port], expectedDroppedPackets[testNum][port]);
		}

		lofar_udp_reader_cleanup(reader);
		lofar_udp_config_cleanup(config);
	}

};

static std::vector<std::tuple<processMode_t, calibrate_t, int32_t>> makeTestMatrix(void) {
	std::vector<std::tuple<processMode_t, calibrate_t, int32_t>> testMatrix;

	for (processMode_t currMode : processingModes) {
		for (calibrate_t cal : std::vector<calibrate_t>{NO_CALIBRATION, GENERATE_JONES, APPLY_CALIBRATION}) {
			if (cal == GENERATE_JONES && currMode > 10) {
				continue;
			}
			for (int32_t testNum = 0; testNum < numTests; testNum++) {
				testMatrix.emplace_back(currMode, cal, testNum);
			}
		}
	}

	return testMatrix;
}

INSTANTIATE_TEST_SUITE_P(CalibrationModes, LibReaderTestsParam, ::testing::ValuesIn(makeTestMatrix()));


TEST(LibReaderTests, ReaderSupportFuncs) {

	{
		SCOPED_TRACE("lofar_udp_reader_calibration");
		//int lofar_udp_reader_calibration(lofar_udp_reader *reader)
		EXPECT_NONFATAL_FAILURE(EXPECT_TRUE(false), "");
	}

	{
		SCOPED_TRACE("lofar_udp_reader_step(_timed)");
		//int lofar_udp_reader_step_timed(lofar_udp_reader *reader, double timing[2])
		//int lofar_udp_reader_step(lofar_udp_reader *reader)
		EXPECT_NONFATAL_FAILURE(EXPECT_TRUE(false), "");
	}

	{
		SCOPED_TRACE("lofar_udp_reader_internal_read_step");
		//int lofar_udp_reader_internal_read_step(lofar_udp_reader *reader)
		EXPECT_NONFATAL_FAILURE(EXPECT_TRUE(false), "");
	}

	{
		SCOPED_TRACE("lofar_udp_shift_remainder_packets");
		//int lofar_udp_shift_remainder_packets(lofar_udp_reader *reader, const long shiftPackets[], const int handlePadding)
		EXPECT_NONFATAL_FAILURE(EXPECT_TRUE(false), "");
	}

};


TEST(LibReaderTests, ProcessingModes) {
	{
		SCOPED_TRACE("4bit_LUT_validation");
		// extern const int8_t bitmodeConversion[256][2];
		for (uint32_t idx = 0; idx < 256; idx++) {
			int8_t lowerNibble = (idx & 0b00000111) - 1 * (idx & 0b00001000);
			int8_t upperNibble = ((idx & 0b01110000) >> 4) - 1 * ((idx & 0b10000000) >> 4);
			EXPECT_EQ(upperNibble, bitmodeConversion[idx][0]);
			EXPECT_EQ(lowerNibble, bitmodeConversion[idx][1]);
		}
	}

	{
		SCOPED_TRACE("data_functions");
		const int numIters = 32;
		for (size_t time = 0; time < numIters; time++) {
			float values[4] = { static_cast<float>(rand()), static_cast<float>(rand()), static_cast<float>(rand()), static_cast<float>(rand()) };
			std::cout << std::to_string(values[0]) << ", " << std::to_string(values[1]) << ", " << std::to_string(values[2]) << ", " << std::to_string(values[3]) << ", " << std::endl;

			for (size_t val = 0; val < 4; val++) {
				if (time == numIters - 2) {
					values[val] = SHRT_MAX;
				} else if (time == numIters - 1) {
					values[val] = SHRT_MIN;
				} else {
					values[val] -= (float) (RAND_MAX / 2);
					values[val] = (float) (SHRT_MIN * (values[val] / ((float) RAND_MAX / 2)));
				}
			}

			//inline float stokesI(float Xr, float Xi, float Yr, float Yi)
			//inline float stokesQ(float Xr, float Xi, float Yr, float Yi)
			//inline float stokesU(float Xr, float Xi, float Yr, float Yi)
			//inline float stokesV(float Xr, float Xi, float Yr, float Yi)
			float stokesIVal = stokesI(values[0], values[1], values[2], values[3]);
			float stokesQVal = stokesQ(values[0], values[1], values[2], values[3]);
			float stokesUVal = stokesU(values[0], values[1], values[2], values[3]);
			float stokesVVal = stokesV(values[0], values[1], values[2], values[3]);
			if (time < numIters - 2) {
				// Fractional checks due to fast-math errors
				EXPECT_LT((values[0] * values[0]
				           + values[1] * values[1]
				           + values[2] * values[2]
				           + values[3] * values[3] - stokesIVal) / stokesIVal, 1e-5);

				EXPECT_LT(((values[0] * values[0]) + (values[1] * values[1])
				           - ((values[2] * values[2]) + (values[3] * values[3])) -
				           stokesQVal) / stokesQVal, 1e-5);

				EXPECT_LT((2 * (values[0] * values[2] - values[1] * (-1 * values[3])) - stokesUVal) / stokesUVal, 1e-5);
				EXPECT_LT((-2 * (values[0] * (-1 * values[3]) + values[1] * values[2]) - stokesVVal) / stokesVVal, 1e-5);
			} else {
				EXPECT_FLOAT_EQ(values[0] * values[0]
					                + values[1] * values[1]
					                + values[2] * values[2]
					                + values[3] * values[3], stokesIVal);
				EXPECT_FLOAT_EQ(((values[0] * values[0]) + (values[1] * values[1]))
				                  - ((values[2] * values[2]) + (values[3] * values[3])), stokesQVal);
				EXPECT_FLOAT_EQ(2 * (values[0] * values[2] - values[1] * (-1 * values[3])), stokesUVal);
				EXPECT_FLOAT_EQ(-2 * (values[0] * (-1 * values[3]) + values[1] * values[2]), stokesVVal);

			}
		}

		{
			SCOPED_TRACE("apply_calibration");
			// template<typename I, typename O> void inline calibrateDataFunc(O *Xr, O *Xi, O *Yr, O *Yi, const float *beamletJones, const int8_t *inputPortData, const long tsInOffset, const int timeStepSize)
			// inline float calibrateSample(float c_1, float c_2, float c_3, float c_4, float c_5, float c_6, float c_7, float c_8)
			EXPECT_NONFATAL_FAILURE(EXPECT_TRUE(false), "");
		}
	}

	{
		SCOPED_TRACE("array_indexing");
		// inline long input_offset_index(long lastInputPacketOffset, int beamlet, int timeStepSize)
		// inline long frequency_major_index(long outputPacketOffset, int beamlet, int baseBeamlet, int cumulativeBeamlets, int offset)
		// inline long reversed_frequency_major_index(long outputPacketOffset, int totalBeamlets, int beamlet, int baseBeamlet, int cumulativeBeamlets, int offset)
		// inline long time_major_index(int beamlet, int baseBeamlet, int cumulativeBeamlets, long packetsPerIteration, long outputTimeIdx)
		EXPECT_NONFATAL_FAILURE(EXPECT_TRUE(false), "");
	}

	{
		SCOPED_TRACE("lofar_udp_cpp_loop_interface");
		// int lofar_udp_cpp_loop_interface(lofar_udp_obs_meta *meta)

		// Required variables:
		// const calibrate_t calibrateData = meta->calibrateData;
		// const int inputBitMode = meta->inputBitMode;
		//const int processingMode = meta->processingMode;
		lofar_udp_obs_meta *meta = _lofar_udp_obs_meta_alloc();
		// Never enter main processing loop
		meta->numPorts = 0;

		const processMode_t fakeMode = TEST_INVALID_MODE;
		std::vector<processMode_t> localProcessingModes = processingModes;
		localProcessingModes.push_back(fakeMode);
		for (int8_t bitmode : bitModes) {
			for (calibrate_t calibration : calibrationModes) {
				for (processMode_t processing : localProcessingModes) {
					meta->inputBitMode = bitmode;
					meta->calibrateData = calibration;
					meta->processingMode = (processMode_t) processing;
					if (bitmode == invalidBitMode || calibration == invalidCalibration || processing == TEST_INVALID_MODE || processing == UNSET_MODE || (processing < 2 && calibration == APPLY_CALIBRATION)) {
						EXPECT_EQ(2, lofar_udp_cpp_loop_interface(meta));
					} else {
						EXPECT_EQ(1, lofar_udp_cpp_loop_interface(meta));
					}
				}
			}
		}

	}
}