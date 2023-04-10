#include "gtest/gtest.h"
#include "lofar_udp_reader.h"
#include "lib_reference_files.hpp"

#include <string>
#include <regex>
#include <valarray>
#include <complex>

#define MODIFY_AND_RESET(target, inter, val, execute) \
	inter = target;                             \
	target = val;                               \
    execute;                                          \
	target = inter;



lofar_udp_config* config_setup(int32_t sampleMeta = 0, int32_t testNumber = 0, int32_t testPorts = 4) {
	lofar_udp_config *config = lofar_udp_config_alloc();
	EXPECT_NE(nullptr, config);
	assert(numPorts <= MAX_NUM_PORTS);
	lofar_udp_calibration *cal = config->calibrationConfiguration;
	EXPECT_NE(nullptr, cal);
	config->calibrationConfiguration = cal;

	for (int32_t port = 0; port < testPorts; port++) {
		std::string workingString = std::regex_replace(inputLocations[testNumber], std::regex("portnum"), std::to_string(port));
		memcpy(config->inputLocations[port], workingString.c_str(), workingString.length());
	}

	config->readerType = std::regex_match(inputLocations[testNumber], std::regex("zst")) ? ZSTDCOMPRESSED : NORMAL;


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

	if (sampleMeta < 0 || sampleMeta > numMeta) {
		std::cerr << "Failed to provide accurate metadata index (" << numMeta << " valid, " << sampleMeta << " given)\n";
		return nullptr;
	}
	if (sampleMeta) {
		memcpy(config->metadata_config.metadataLocation, metadataLocation[sampleMeta].c_str(), metadataLocation[sampleMeta].length());
	}

	return config;
}

lofar_udp_reader* reader_setup(int32_t processingMode) {
	lofar_udp_config *config = config_setup();
	config->processingMode = processingMode;

	return lofar_udp_reader_setup(config);
}


TEST(LibReaderTests, SetupReader) {
	lofar_udp_config *config = config_setup();
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
		lofar_udp_calibration *cal = config->calibrationConfiguration;
		config->calibrationConfiguration = nullptr;
		EXPECT_EQ(-1, _lofar_udp_reader_config_check(config));
		config->calibrationConfiguration = cal;

		// (config->calibrateData > NO_CALIBRATION && config->calibrationConfiguration != NULL) &&  (strcmp(config->calibrationConfiguration->calibrationFifo, "") == 0)
		config->calibrationConfiguration->calibrationFifo[0] = '\0';
		EXPECT_EQ(-1, _lofar_udp_reader_config_check(config));
		config->calibrateData = NO_CALIBRATION;

		// (config->processingMode < 0)
		MODIFY_AND_RESET(config->processingMode, tmpVal, -1, EXPECT_EQ(-1, _lofar_udp_reader_config_check(config)););

		// (config->startingPacket > 0 && config->startingPacket < LFREPOCH)
		MODIFY_AND_RESET(config->startingPacket, tmpVal, 1, EXPECT_EQ(-1, _lofar_udp_reader_config_check(config)););

		// (config->packetsReadMax < 1)
		MODIFY_AND_RESET(config->packetsReadMax, tmpVal, 0, EXPECT_EQ(-1, _lofar_udp_reader_config_check(config)););

		// (config->packetsReadMax < config->packetsPerIteration) -> config->packetsReadMax = config->packetsPerIteration
		MODIFY_AND_RESET(config->packetsReadMax, tmpVal, 1, _lofar_udp_reader_config_check(config); EXPECT_EQ(config->packetsPerIteration, config->packetsReadMax););

		// ((2 * config->ompThreads) < config->numPorts) && (config->ompThreads < 1) -> config->ompThreads = OMP_THREADS
		MODIFY_AND_RESET(config->ompThreads, tmpVal, -1, \
            _lofar_udp_reader_config_check(config); \
            EXPECT_EQ(OMP_THREADS, config->ompThreads);
		);
		MODIFY_AND_RESET(config->ompThreads, tmpVal, 1, EXPECT_EQ(0, _lofar_udp_reader_config_check(config)););

		// (config->replayDroppedPackets != 0 && config->replayDroppedPackets != 1)
		MODIFY_AND_RESET(config->replayDroppedPackets, tmpVal, 2, EXPECT_EQ(-1, _lofar_udp_reader_config_check(config)););
		EXPECT_EQ(0, _lofar_udp_reader_config_check(config));


		//lofar_udp_reader *lofar_udp_reader_setup(lofar_udp_config *config)
		MODIFY_AND_RESET(config->replayDroppedPackets, tmpVal, 2, EXPECT_EQ(nullptr, lofar_udp_reader_setup(config)););
		//MODIFY_AND_RESET(config->packetsReadMax, tmpVal, LONG_MAX, EXPECT_NE(nullptr, lofar_udp_reader_setup(config)););
	}

	free(config->calibrationConfiguration);
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
		MODIFY_AND_RESET(header[CEP_HDR_NBEAM_OFFSET], tmpVal, (uint8_t) UDPMAXBEAM + 1, EXPECT_EQ(-1, _lofar_udp_reader_malformed_header_checks(header)););

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
	//void lofar_udp_parse_extract_header_metadata(int port, lofar_udp_obs_meta *meta, const int8_t header[UDPHDRLEN], const int16_t beamletLimits[2])
	{
		SCOPED_TRACE("_lofar_udp_parse_headers");

		lofar_udp_obs_meta *meta = lofar_udp_obs_meta_alloc();
		lofar_udp_config *config = config_setup();
		config->packetsReadMax = -1;
		_lofar_udp_configure_obs_meta(config, meta);
		EXPECT_EQ(LONG_MAX, meta->packetsReadMax);




		*((int16_t *) &(header[CEP_HDR_STN_ID_OFFSET])) = 333 * 32;
		*((uint8_t *) &(header[CEP_HDR_NBEAM_OFFSET])) = 244;
		source->bitMode = 2;

		int8_t headers[MAX_NUM_PORTS][UDPHDRLEN];
		for (size_t port = 0; port < MAX_NUM_PORTS; port++) {
			memcpy(&(headers[port][0]), &(header[0]), UDPHDRLEN);
		}


		int16_t beamletLimits[2] = {0, 0};
		meta->numPorts = MAX_NUM_PORTS;
		EXPECT_EQ(0, _lofar_udp_parse_headers(meta, headers, beamletLimits));

		*((uint8_t *) &(headers[0][CEP_HDR_NBEAM_OFFSET])) -= 122;
		EXPECT_EQ(0, _lofar_udp_parse_headers(meta, headers, beamletLimits));
		*((uint8_t *) &(headers[0][CEP_HDR_NBEAM_OFFSET])) += 122;

		headers[0][CEP_HDR_RSP_VER_OFFSET] -= 1;
		EXPECT_EQ(-1, _lofar_udp_parse_headers(meta, headers, beamletLimits));
		headers[0][CEP_HDR_RSP_VER_OFFSET] += 1;

		*((int16_t *) &(headers[1][CEP_HDR_STN_ID_OFFSET])) -= 64;
		EXPECT_EQ(0, _lofar_udp_parse_headers(meta, headers, beamletLimits));
		*((int16_t *) &(headers[1][CEP_HDR_STN_ID_OFFSET])) += 64;
		source = (lofar_source_bytes *) &(headers[0][CEP_HDR_SRC_OFFSET]);

		// source->bitMode
		source->bitMode = 1;
		EXPECT_EQ(-1, _lofar_udp_parse_headers(meta, headers, beamletLimits));
		source->bitMode = 0;
		EXPECT_EQ(-1, _lofar_udp_parse_headers(meta, headers, beamletLimits));
		source->bitMode = 2;

		// meta->portRawBeamlets[port] = (int32_t) ((uint8_t) header[CEP_HDR_NBEAM_OFFSET]);
		// Varying beamlet limits will change the fraction of data processed, and possibly number of
		//  ports processed in total
		beamletLimits[0] = 1; beamletLimits[1] = 733;
		EXPECT_EQ(0, _lofar_udp_parse_headers(meta, headers, beamletLimits));
		EXPECT_EQ(MAX_NUM_PORTS * UDPMAXBEAM, meta->totalRawBeamlets);
		_lofar_udp_configure_obs_meta(config, meta);

		beamletLimits[0] = 1; beamletLimits[1] = 246;
		EXPECT_EQ(0, _lofar_udp_parse_headers(meta, headers, beamletLimits));
		EXPECT_EQ(2 * UDPMAXBEAM, meta->totalRawBeamlets);
		EXPECT_EQ(beamletLimits[1] - beamletLimits[0], meta->totalProcBeamlets);
		EXPECT_EQ(beamletLimits[0], meta->baseBeamlets[0]);
		EXPECT_EQ(UDPMAXBEAM, meta->upperBeamlets[0]);

		beamletLimits[0] = 6; beamletLimits[1] = 245;
		EXPECT_EQ(0, _lofar_udp_parse_headers(meta, headers, beamletLimits));
		EXPECT_EQ(2 * UDPMAXBEAM, meta->totalRawBeamlets);
		EXPECT_EQ(beamletLimits[1] - beamletLimits[0], meta->totalProcBeamlets);
		EXPECT_EQ(beamletLimits[0], meta->baseBeamlets[0]);
		EXPECT_EQ(0, meta->baseBeamlets[1]);
		EXPECT_EQ(UDPMAXBEAM, meta->upperBeamlets[0]);
		EXPECT_EQ(1, meta->upperBeamlets[1]);

		beamletLimits[0] = 1; beamletLimits[1] = 244;
		EXPECT_EQ(0, _lofar_udp_parse_headers(meta, headers, beamletLimits));
		EXPECT_EQ(UDPMAXBEAM, meta->totalRawBeamlets);
		EXPECT_EQ(beamletLimits[1] - beamletLimits[0], meta->totalProcBeamlets);
		EXPECT_EQ(beamletLimits[0], meta->baseBeamlets[0]);
		EXPECT_EQ(UDPMAXBEAM, meta->upperBeamlets[0]);

		beamletLimits[0] = 1; beamletLimits[1] = 4;
		EXPECT_EQ(0, _lofar_udp_parse_headers(meta, headers, beamletLimits));
		EXPECT_EQ(UDPMAXBEAM, meta->totalRawBeamlets);
		EXPECT_EQ(beamletLimits[1] - beamletLimits[0], meta->totalProcBeamlets);
		EXPECT_EQ(beamletLimits[0], meta->baseBeamlets[0]);
		EXPECT_EQ(beamletLimits[1], meta->upperBeamlets[0]);

		// meta->stationID = *((int16_t *) &(header[CEP_HDR_STN_ID_OFFSET])) / 32;
		EXPECT_EQ(333, meta->stationID);
		free(meta);
		free(config);
	}

};

TEST(LibReaderTests, PreprocessingReader) {
	lofar_udp_reader *reader = reader_setup(150);

	//int lofar_udp_skip_to_packet(lofar_udp_reader *reader)

	//int lofar_udp_get_first_packet_alignment(lofar_udp_reader *reader)

	{
		//int lofar_udp_setup_processing(lofar_udp_obs_meta *meta)
		SCOPED_TRACE("_lofar_udp_setup_processing");
		lofar_udp_obs_meta *meta = lofar_udp_obs_meta_alloc();
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


	}
	//int lofar_udp_file_reader_reuse(lofar_udp_reader *reader, const long startingPacket, const long packetsReadMax)

};


TEST(LibReaderTests, ProcessingData) {
	lofar_udp_reader *reader = reader_setup(150);

	//int lofar_udp_reader_calibration(lofar_udp_reader *reader)

	//int lofar_udp_reader_step_timed(lofar_udp_reader *reader, double timing[2])
	//int lofar_udp_reader_step(lofar_udp_reader *reader)
	//int lofar_udp_reader_internal_read_step(lofar_udp_reader *reader)

	//int lofar_udp_shift_remainder_packets(lofar_udp_reader *reader, const long shiftPackets[], const int handlePadding)

	//void lofar_udp_reader_cleanup(lofar_udp_reader *reader)

	lofar_udp_reader_cleanup(reader);
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

			for (size_t val = 0; val < 4; val++) {
				if (time == numIters - 2) {
					values[val] = SHRT_MAX;
				} else if (time == numIters - 1) {
					values[val] = SHRT_MIN;
				} else {
					values[val] -= RAND_MAX / 2;
					values[val] = (int16_t) (SHRT_MIN * (values[val] / (RAND_MAX / 2)));
				}
			}

			//inline float stokesI(float Xr, float Xi, float Yr, float Yi)
			//inline float stokesQ(float Xr, float Xi, float Yr, float Yi)
			//inline float stokesU(float Xr, float Xi, float Yr, float Yi)
			//inline float stokesV(float Xr, float Xi, float Yr, float Yi)
			EXPECT_FLOAT_EQ(values[0] * values[0]
			          + values[1] * values[1]
			          + values[2] * values[2]
			          + values[3] * values[3], stokesI(values[0], values[1], values[2], values[3]));

			EXPECT_FLOAT_EQ(values[0] * values[0] + values[1] * values[1]
			          - (values[2] * values[2] + values[3] * values[3]),
			          stokesQ(values[0], values[1], values[2], values[3]));

			EXPECT_FLOAT_EQ(2 * (values[0] * values[2] - values[1] * (-1 * values[3])), stokesU(values[0], values[1], values[2], values[3]));
			EXPECT_FLOAT_EQ(-2 * (values[0] * (-1 * values[3]) + values[1] * values[2]), stokesV(values[0], values[1], values[2], values[3]));
		}

		// template<typename I, typename O> void inline calibrateDataFunc(O *Xr, O *Xi, O *Yr, O *Yi, const float *beamletJones, const int8_t *inputPortData, const long tsInOffset, const int timeStepSize)
		// inline float calibrateSample(float c_1, float c_2, float c_3, float c_4, float c_5, float c_6, float c_7, float c_8)
	}

	{
		SCOPED_TRACE("array_indexing");
		// inline long input_offset_index(long lastInputPacketOffset, int beamlet, int timeStepSize)
		// inline long frequency_major_index(long outputPacketOffset, int beamlet, int baseBeamlet, int cumulativeBeamlets, int offset)
		// inline long reversed_frequency_major_index(long outputPacketOffset, int totalBeamlets, int beamlet, int baseBeamlet, int cumulativeBeamlets, int offset)
		// inline long time_major_index(int beamlet, int baseBeamlet, int cumulativeBeamlets, long packetsPerIteration, long outputTimeIdx)
	}

	{
		SCOPED_TRACE("lofar_udp_cpp_loop_interface");
		// int lofar_udp_cpp_loop_interface(lofar_udp_obs_meta *meta)

		// Required variables:
		// const calibrate_t calibrateData = meta->calibrateData;
		// const int inputBitMode = meta->inputBitMode;
		//const int processingMode = meta->processingMode;
		lofar_udp_obs_meta *meta = lofar_udp_obs_meta_alloc();
		// Never enter main processing loop
		meta->numPorts = 0;

		const int32_t fakeMode = TEST_INVALID_MODE;
		std::vector<int32_t> localProcessingModes = processingModes;
		localProcessingModes.push_back(fakeMode);
		for (int32_t bitmode : bitModes) {
			for (calibrate_t calibration : calibrationModes) {
				for (int32_t processing : localProcessingModes) {
					meta->inputBitMode = bitmode;
					meta->calibrateData = calibration;
					meta->processingMode = (processMode_t) processing;
					if (bitmode == invalidBitMode || calibration == invalidCalibration || processing == TEST_INVALID_MODE || (processing < 2 && calibration == APPLY_CALIBRATION)) {
						EXPECT_EQ(2, lofar_udp_cpp_loop_interface(meta));
					} else {
						EXPECT_EQ(1, lofar_udp_cpp_loop_interface(meta));
					}
				}
			}
		}

	}
}