#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"
#include "lofar_udp_io.h"
#include <cstdio>
#include <iostream>

TEST(LibIoTests, SetupUseCleanup) {

	{
		SCOPED_TRACE("WriteAndReadBack");
		const int testNumInputs = 3;
		const int dadaBase = 16130;
		const int psrdadaBufs = 32;
		std::string testContents = "This is a test string that will be written and then read back, sometimes it might even be done successfully?";
		int syncPipe[2];
		for (float buffFact : std::vector<float>{0.5, 3.0}) {
			for (int32_t ops: std::vector<int32_t>{ 1, 4, 2 * psrdadaBufs }) {
				for (reader_t type: std::vector<reader_t>{ NORMAL, FIFO, ZSTDCOMPRESSED, ZSTDCOMPRESSED_INDIRECT, DADA_ACTIVE, HDF5 }) {
					if (type == HDF5) {
						//EXPECT_NONFATAL_FAILURE(EXPECT_TRUE(false);, "HDF% does not have a reader component to allow for verification.");
						continue;
					}
					int8_t *testBuffer = (int8_t *) calloc(testContents.length() + 1, sizeof(int8_t));

					std::string fileName = "./my_file_[[idx]]";

					for (int i = 0; i < testNumInputs; i++) {
						std::string fileLoc = fileName.substr(0, 10) + std::to_string(i);
						std::remove(fileLoc.c_str());

					}

					lofar_udp_io_read_config *readConfig = lofar_udp_io_read_alloc();
					readConfig->readerType = type;

					lofar_udp_io_write_config *writeConfig = lofar_udp_io_write_alloc();
					writeConfig->readerType = type;
					writeConfig->numOutputs = testNumInputs;
					writeConfig->dadaConfig.nbufs = psrdadaBufs;
					writeConfig->dadaConfig.num_readers = 1;
					writeConfig->progressWithExisting = 1;

					std::cout << "Starting tests for " << std::to_string(type) << std::endl;



					strncpy(writeConfig->outputFormat, fileName.c_str(), DEF_STR_LEN);

					fileName = "./my_file_";
					for (int i = 0; i < testNumInputs; i++) {
						std::string combined = fileName + std::to_string(i);
						strncpy(readConfig->inputLocations[i], combined.c_str(), DEF_STR_LEN);
						readConfig->inputDadaKeys[i] = dadaBase + 10 * i;
						writeConfig->outputDadaKeys[i] = dadaBase + 10 * i;
						ipcio_t tmp;
						if (type == DADA_ACTIVE) {
							if (!ipcio_connect(&tmp, readConfig->inputDadaKeys[i])) {
								FREE_NOT_NULL(tmp.buf.shm_addr);
								ipcio_destroy(&tmp);
							}
							if (!ipcio_connect(&tmp, readConfig->inputDadaKeys[i] + 1)) {
								FREE_NOT_NULL(tmp.buf.shm_addr);
								ipcio_destroy(&tmp);
							}
						}
						writeConfig->writeBufSize[i] = (int64_t) (buffFact * (float) testContents.length());
						readConfig->readBufSize[i] = testContents.length();
					}

					int pid = 0;

					if (type == DADA_ACTIVE || type == FIFO) {
						int ret = 0;
						if (type == DADA_ACTIVE) {
							ret = pipe(syncPipe);
						}
						assert(ret != -1);
						pid = fork();
						if (pid < 0) {
							std::cerr << "ERROR: Failed to fork, " << errno << ", " << strerror(errno) << std::endl;
							exit(EXIT_FAILURE);
						}
					} else {
						pid = 1;
					}

					if (pid != 0) {
						if (type == DADA_ACTIVE || type == FIFO) {
							//free(writeConfig);
						}
					} else {
						//free(readConfig);
					}

					// Write test
					if (pid == 0 || !(type == DADA_ACTIVE || type == FIFO)) {

						int returnVal = 0;

						returnVal += lofar_udp_io_write_setup(writeConfig, 0);
						// Closing a FIFO disconnects the reader, can't test reallocation with this methodology
						if (type != FIFO) {
							returnVal += lofar_udp_io_write_setup(writeConfig, 0);
						}

						EXPECT_EQ(0, returnVal);

						// Do an extra iteration for DADA to prevent the final buffer from freezing the reader
						for (int j = 0; j < (ops + (type == DADA_ACTIVE)); j++) {
							for (int i = 0; i < testNumInputs; i++) {
								int64_t bytes = lofar_udp_io_write(writeConfig, (int8_t) i, (int8_t *) testContents.c_str(), testContents.length());
								returnVal += !((int64_t) testContents.length() == bytes);
								EXPECT_EQ(0, returnVal);
							}
						}

						if (type == DADA_ACTIVE) {
							close(syncPipe[1]);
							read(syncPipe[0], &syncPipe[1], sizeof(int));
							close(syncPipe[0]);
						}

						lofar_udp_io_write_cleanup(writeConfig, 1);
						writeConfig = NULL;


						if (pid == 0) {
							std::cout << "Write exit: " << returnVal  << std::endl;
							exit(returnVal == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
						}

					}

					// Temp read test
					// int64_t lofar_udp_io_read_temp(const lofar_udp_config *config, int8_t port, int8_t *outbuf, size_t size, int64_t num, int resetSeek)
					lofar_udp_config *config = lofar_udp_config_alloc();
					config->readerType = type;
					for (int8_t i = 0; i < testNumInputs; i++) {
						if (type == DADA_ACTIVE) {
							close(syncPipe[0]);
							//usleep(5000);
							ipcbuf_t tmpStruct;
							while (ipcbuf_connect(&tmpStruct, readConfig->inputDadaKeys[i] + 1)) {
								usleep(1000);
							}
							ipcbuf_disconnect(&tmpStruct);
							free(tmpStruct.shm_addr);
							std::cout << std::to_string(readConfig->inputDadaKeys[i] + 1) << std::endl;
							config->inputDadaKeys[i] = readConfig->inputDadaKeys[i];
						}

						snprintf(config->inputLocations[i], DEF_STR_LEN + 1, "%s", readConfig->inputLocations[i]);
						int8_t *buffer = (int8_t*) calloc(testContents.length() + 1, sizeof(int8_t));

						if (type == FIFO) {
							// We cannot temp-read a FIFO
							EXPECT_EQ(-1, lofar_udp_io_read_temp(config, i, buffer, sizeof(int8_t), testContents.length(), 1));
						} else if (type == DADA_ACTIVE) {
							// We can only read the current buffer of a DADA ringbuffer
							int64_t limitedRead = buffFact < 1 ? writeConfig->writeBufSize[i] - 1 : testContents.length();
							EXPECT_EQ(buffFact < 1 ? limitedRead : testContents.length(), lofar_udp_io_read_temp(config, i, buffer, sizeof(int8_t), testContents.length(), 1));
							EXPECT_EQ(std::string((char*) buffer), testContents.substr(0, limitedRead));
						} else {
							EXPECT_EQ(testContents.length(), lofar_udp_io_read_temp(config, i, buffer, sizeof(int8_t), testContents.length(), 1));
						}


						free(buffer);
					}

					std::cout << "Temp reads complete" << std::endl;
					lofar_udp_config_cleanup(config);

					// Read test
					int8_t **ptrBuffers = (int8_t**) calloc(testNumInputs, sizeof(int8_t*));
					for (int8_t i = 0; i < testNumInputs; i++) {
						// FIFO cannot be temp, wait for it to be available here
						if (type == FIFO) {
							//usleep(5000);
							while (access(readConfig->inputLocations[i], F_OK)) {
								usleep(1000);
							}
						}

						// Be extremely verbose with DADA keys; in testing bits were randomly added to the upper ender of the key.
						if (type == DADA_ACTIVE) std::cout << std::to_string(readConfig->inputDadaKeys[i] + 1) << std::endl;
						ptrBuffers[i] = (int8_t*) calloc(testContents.length() + 1, sizeof(int8_t));
						ASSERT_EQ(0, lofar_udp_io_read_setup_helper(readConfig, ptrBuffers, (testContents.length() + 1) * sizeof(int8_t), i));
						if (type == DADA_ACTIVE) std::cout << std::to_string(readConfig->inputDadaKeys[i] + 1) << std::endl;

					}

					// Write/Read comparison
					for (int j = 0; j < ops; j++) {
						for (int8_t i = 0; i < testNumInputs; i++) {
							ARR_INIT(testBuffer, (int64_t) testContents.length(), '\0');
							if (type != ZSTDCOMPRESSED || j != 0) {
								EXPECT_EQ(testContents.length(), lofar_udp_io_read(readConfig, i, ptrBuffers[i], testContents.length()));
							} else {
								printf("%p - %p = %ld\n", testBuffer, ptrBuffers[i], testBuffer - ptrBuffers[i]);
								EXPECT_EQ(-1, lofar_udp_io_read(readConfig, i, testBuffer, testContents.length()));
								EXPECT_EQ(testContents.length(), lofar_udp_io_read(readConfig, i, ptrBuffers[i], testContents.length()));
							}

							// substr limit for readers that read in blocks (ZSTD)
							EXPECT_EQ(testContents, std::string((char *) ptrBuffers[i], testContents.length()));
							ARR_INIT(ptrBuffers[i], (int64_t) testContents.length(), '\0');

						}
					}

					if (type == DADA_ACTIVE) {
						write(syncPipe[1], &syncPipe[0], sizeof(int));
						close(syncPipe[1]);
					}

					for (int i = 0; i < testNumInputs; i++) {
						FREE_NOT_NULL(ptrBuffers[i]);
					}
					lofar_udp_io_read_cleanup(readConfig);
					FREE_NOT_NULL(ptrBuffers);

					if (type == DADA_ACTIVE || type == FIFO) {
						wait(&pid);
						EXPECT_EQ(0, pid & 255); // FAILURE ON FORKED WRITER SIDE
					}

					for (int i = 0; i < testNumInputs; i++) {
						std::remove(readConfig->inputLocations[i]);
					}

					free(testBuffer);
					FREE_NOT_NULL(writeConfig);
				}
			}
		}

	}
};


#define ZSTDBUFLEN 131072
TEST(LibIoTests, ZSTDBufferExpansion) {
	//_lofar_udp_io_read_ZSTD_fix_buffer_size(size, deltaOnly)

	{
		SCOPED_TRACE("NormalCall");
		EXPECT_EQ(ZSTDBUFLEN, _lofar_udp_io_read_ZSTD_fix_buffer_size(0, 0));
		EXPECT_EQ(ZSTDBUFLEN * 2, _lofar_udp_io_read_ZSTD_fix_buffer_size(ZSTDBUFLEN + 1, 0));
		EXPECT_EQ(ZSTDBUFLEN, _lofar_udp_io_read_ZSTD_fix_buffer_size(130000, 0));

		EXPECT_EQ(ZSTDBUFLEN, _lofar_udp_io_read_ZSTD_fix_buffer_size(0, 1));
		EXPECT_EQ(ZSTDBUFLEN - 1, _lofar_udp_io_read_ZSTD_fix_buffer_size(1, 1));
		EXPECT_EQ(ZSTDBUFLEN - 1, _lofar_udp_io_read_ZSTD_fix_buffer_size(ZSTDBUFLEN + 1, 1));
		EXPECT_EQ(ZSTDBUFLEN - 1, _lofar_udp_io_read_ZSTD_fix_buffer_size(ZSTDBUFLEN + 1, 1));
		EXPECT_EQ(1, _lofar_udp_io_read_ZSTD_fix_buffer_size(ZSTDBUFLEN - 1, 1));
	}
};
#undef ZSTDBUFLEN


TEST(LibIoTests, ConfigReadSetupHelper) {
	//int lofar_udp_io_read_setup_helper(lofar_udp_io_read_config *input, const lofar_udp_config *config, const lofar_udp_obs_meta *meta,
	//                                   int port);
	//int lofar_udp_io_write_setup_helper(lofar_udp_io_write_config *config, const lofar_udp_obs_meta *meta, int iter);
	lofar_udp_io_read_config *input = lofar_udp_io_read_alloc();
	EXPECT_EQ(-1, lofar_udp_io_read_setup(nullptr, 0));
	EXPECT_EQ(-2, lofar_udp_io_read_setup(input, -1));
	input->readerType = HDF5;
	EXPECT_EQ(-1, lofar_udp_io_read_setup(input, 0));
	lofar_udp_io_read_cleanup(input);
	lofar_udp_io_read_cleanup(nullptr);

	//int32_t _lofar_udp_io_read_setup_internal_lib_helper(lofar_udp_io_read_config *const input, const lofar_udp_config *config, lofar_udp_obs_meta *meta,
	//                                                     int8_t port);
	input = lofar_udp_io_read_alloc();
	lofar_udp_config *config = lofar_udp_config_alloc();
	lofar_udp_obs_meta *meta = _lofar_udp_obs_meta_alloc();
	EXPECT_EQ(-1, _lofar_udp_io_read_setup_internal_lib_helper(nullptr, nullptr, nullptr, 0));
	EXPECT_EQ(-1, _lofar_udp_io_read_setup_internal_lib_helper(input, config, meta, -1));
	EXPECT_EQ(-1, _lofar_udp_io_read_setup_internal_lib_helper(input, config, meta, 0));
	input->readerType = NO_ACTION;
	config->readerType = DADA_ACTIVE;
	EXPECT_EQ(-1, _lofar_udp_io_read_setup_internal_lib_helper(input, config, meta, 0));
	input->readerType = ZSTDCOMPRESSED;
	config->readerType = DADA_ACTIVE;
	EXPECT_EQ(-1, _lofar_udp_io_read_setup_internal_lib_helper(input, config, meta, 0));

	lofar_udp_config_cleanup(config);
	free(meta);

	//lofar_udp_io_read_setup_helper(lofar_udp_io_read_config *input, int8_t **outputArr, int64_t maxReadSize, int8_t port)
	input = lofar_udp_io_read_alloc();
	int8_t **outputArr = (int8_t**) calloc(1, sizeof(int8_t *));
	EXPECT_EQ(-1, lofar_udp_io_read_setup_helper(nullptr, nullptr, -1, -1));
	EXPECT_EQ(-1, lofar_udp_io_read_setup_helper(nullptr, nullptr, -1, 0));
	input->numInputs = 0;
	EXPECT_EQ(-1, lofar_udp_io_read_setup_helper(input, outputArr, -1, 1));
	EXPECT_EQ(-1, lofar_udp_io_read_setup_helper(input, outputArr, -1, 0));
	outputArr[0] = (int8_t*) calloc(1, sizeof(int8_t));
	input->readerType = NO_ACTION;
	EXPECT_EQ(-1, lofar_udp_io_read_setup_helper(input, outputArr, -1, 0));
	input->readerType = ZSTDCOMPRESSED;
	EXPECT_EQ(-1, lofar_udp_io_read_setup_helper(input, outputArr, -1, 0));
	free(outputArr[0]);
	free(outputArr);
}

TEST(LibIoTests, ConfigWriteSetupHelper) {
	lofar_udp_io_write_config *output = lofar_udp_io_write_alloc();
	EXPECT_EQ(-1, lofar_udp_io_write_setup(nullptr, 0));
	EXPECT_EQ(-2, lofar_udp_io_write_setup(output, -1));
	output->numOutputs = 0;
	EXPECT_EQ(-3, lofar_udp_io_write_setup(output, 0));
	output->numOutputs = MAX_OUTPUT_DIMS + 1;
	EXPECT_EQ(-3, lofar_udp_io_write_setup(output, 0));
	output->numOutputs = 4;
	output->readerType = DADA_ACTIVE;
	EXPECT_EQ(-4, lofar_udp_io_write_setup(output, 1));


	EXPECT_EQ(-1, _lofar_udp_io_write_internal_lib_setup_helper(nullptr, nullptr, -1));
	lofar_udp_obs_meta *meta = _lofar_udp_obs_meta_alloc();
	lofar_udp_reader *rdr = _lofar_udp_reader_alloc(meta);
	rdr->packetsPerIteration = -1;
	EXPECT_EQ(-2, _lofar_udp_io_write_internal_lib_setup_helper(output, rdr, -1));
	rdr->packetsPerIteration = 2;
	output->numOutputs = 4;
	EXPECT_EQ(-3, _lofar_udp_io_write_internal_lib_setup_helper(output, rdr, -1));
	for (int8_t outp = 0; outp < 4; outp++) rdr->meta->packetOutputLength[outp] = 64;
	EXPECT_EQ(-2, _lofar_udp_io_write_internal_lib_setup_helper(output, rdr, -1));
	for (int8_t outp = 0; outp < 4; outp++) EXPECT_EQ(rdr->packetsPerIteration * rdr->meta->packetOutputLength[outp], output->writeBufSize[outp]);
	lofar_udp_io_write_cleanup(output, 0);
	lofar_udp_io_write_cleanup(nullptr, 0);

	int64_t outputLength[4] = { LONG_MIN, 64, 64, 64 };
	//lofar_udp_io_write_setup_helper(output, outputLength, 4, int32_t iter, int64_t firstPacket)
	EXPECT_EQ(-1, lofar_udp_io_write_setup_helper(nullptr, outputLength, 4, -1, -1));
	EXPECT_EQ(-2, lofar_udp_io_write_setup_helper(output, outputLength, -1, -1, -1));
	EXPECT_EQ(-2, lofar_udp_io_write_setup_helper(output, outputLength, 4, -1, -1));
	outputLength[0] = -1;
	EXPECT_EQ(-3, lofar_udp_io_write_setup_helper(output, outputLength, 4, -1, -1));
	outputLength[0] = 32;
	EXPECT_EQ(-2, lofar_udp_io_write_setup_helper(output, outputLength, 4, -1, -1));
	for (int8_t outp = 0; outp < 4; outp++) EXPECT_EQ(outputLength[outp], output->writeBufSize[outp]);

	free(output);
}

TEST(LibIoTests, ReadSanityChecks) {
	//lofar_udp_io_read(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars)
	lofar_udp_io_read_config *input = lofar_udp_io_read_alloc();
	EXPECT_EQ(-1, lofar_udp_io_read(input, 0, nullptr, -1));
	EXPECT_EQ(0, lofar_udp_io_read(input, 0, nullptr, 0));
	input->readBufSize[0] = 2;
	EXPECT_EQ(-1, lofar_udp_io_read(input, 0, nullptr, 3));
	EXPECT_EQ(-1, lofar_udp_io_read(input, 0, nullptr, 2));
	EXPECT_EQ(-1, lofar_udp_io_read(input, -1, nullptr, 2));
	EXPECT_EQ(-1, lofar_udp_io_read(input, MAX_NUM_PORTS + 1, nullptr, 2));

	free(input);
};

TEST(LibIoTests, ReadTempSanityChecks) {
	// int64_t
	//lofar_udp_io_read_temp(const lofar_udp_config *config, int8_t port, int8_t *outbuf, int64_t size, int64_t num,
	//                       int8_t resetSeek)
	int8_t tmp[4];
	lofar_udp_config *config = lofar_udp_config_alloc();
	EXPECT_EQ(-1, lofar_udp_io_read_temp(nullptr, -1, nullptr, -1, -1, 0));
	EXPECT_EQ(-2, lofar_udp_io_read_temp(config, -1, tmp, -1, -1, 0));
	EXPECT_EQ(-2, lofar_udp_io_read_temp(config, MAX_NUM_PORTS + 1, tmp, -1, -1, 0));
	EXPECT_EQ(0, lofar_udp_io_read_temp(config, 0, tmp, -1, 0, 0));
	EXPECT_EQ(-3, lofar_udp_io_read_temp(config, 0, tmp, -1, -1, 0));

	free(config);
};

TEST(LibIoTests, WriteSanityChecks) {
	// lofar_udp_io_write(lofar_udp_io_write_config *const config, int8_t outp, const int8_t *src, int64_t nchars)
	lofar_udp_io_write_config *output = lofar_udp_io_write_alloc();
	EXPECT_EQ(-1, lofar_udp_io_write(output, 0, nullptr, -1));
	EXPECT_EQ(0, lofar_udp_io_write(output, 0, nullptr, 0));
	EXPECT_EQ(-1, lofar_udp_io_write(output, 0, nullptr, 1));
	EXPECT_EQ(-1, lofar_udp_io_write(output, 0, nullptr, 1));
	EXPECT_EQ(-1, lofar_udp_io_write(output, -1, nullptr, 1));
	EXPECT_EQ(-1, lofar_udp_io_write(output, 5, nullptr, 1));
	EXPECT_EQ(-1, lofar_udp_io_write(output, 0, nullptr, 2));

	free(output);
};

TEST(LibIoTests, WriteMetaanityChecks) {
	// int64_t lofar_udp_io_write_metadata(lofar_udp_io_write_config *const outConfig, int8_t outp, const lofar_udp_metadata *metadata, const int8_t *headerBuffer, int64_t headerLength) {
	lofar_udp_io_write_config *output = lofar_udp_io_write_alloc();
	lofar_udp_metadata *metadata = lofar_udp_metadata_alloc();
	EXPECT_EQ(-1, lofar_udp_io_write_metadata(nullptr, 0, nullptr, nullptr, -1));
	EXPECT_EQ(-1, lofar_udp_io_write_metadata(nullptr, 0, nullptr, nullptr, -1));
	EXPECT_EQ(-2, lofar_udp_io_write_metadata(output, -1, metadata, nullptr, -1));
	EXPECT_EQ(-2, lofar_udp_io_write_metadata(output, MAX_OUTPUT_DIMS + 1, metadata, nullptr, -1));
	output->readerType = HDF5;
	metadata->type = SIGPROC;
	EXPECT_EQ(-3, lofar_udp_io_write_metadata(output, 1, metadata, nullptr, -1));
	output->readerType = ZSTDCOMPRESSED;
	metadata->type = HDF5_META;
	EXPECT_EQ(-3, lofar_udp_io_write_metadata(output, 1, metadata, nullptr, -1));
	output->readerType = ZSTDCOMPRESSED;
	metadata->type = SIGPROC;
	EXPECT_EQ(-4, lofar_udp_io_write_metadata(output, 1, metadata, nullptr, 1));
	EXPECT_EQ(-4, lofar_udp_io_write_metadata(output, 1, metadata, (int8_t*) 1, -1));

	free(output);
	free(metadata);

}


TEST(LibIoTests, OptargGeneralParser) {
	// reader_t lofar_udp_io_parse_type_optarg(const char optargc[], char *fileFormat, int32_t *baseVal, int16_t *stepSize, int8_t *offsetVal)

	// lofar_udp_io_parse_type_optarg
	// (optargc == NULL || fileFormat == NULL || baseVal == NULL || stepSize == NULL || offsetVal == NULL)
	// if (strstr(optargc, "%") != NULL)
	// if (optargc[4] == ':') // FILE, ZSTD, FIFO, DADA, HDF5
	// ".zst", ".hdf5", ".h5"
	// NO_ACTION for nothing above
	int32_t baseVal;
	int16_t stepVal;
	int8_t offsetVal;
	char fileFormat[DEF_STR_LEN];
	EXPECT_EQ(-1, lofar_udp_io_parse_type_optarg(nullptr, nullptr, nullptr, nullptr, nullptr));
	EXPECT_EQ(-1, lofar_udp_io_parse_type_optarg("", fileFormat, &baseVal, &stepVal, &offsetVal));

	const std::vector<std::tuple<reader_t, std::string, std::string>> testCases = {
		{NORMAL, "Hell:%o,1,1,1", "Hell:%o"},
		{NORMAL, "FILE:file,1,1,1", "file"},
		{FIFO, "FIFO:File,1,1,1", "File"},
		{ZSTDCOMPRESSED, "ZSTD:CFile,1,1,1", "CFile"},
		{ZSTDCOMPRESSED, "AFile.zst,1,1,1", "AFile.zst"},
		{DADA_ACTIVE, "DADA:1000,1,1,1", "1000"},
		{HDF5, "HDF5:File,22", "File"},
		{HDF5, "AFile.h5,22", "AFile.h5"},
		{HDF5, "AFile.hdf5,22", "AFile.hdf5"},

	};

	for (std::tuple<reader_t, std::string, std::string> caseVal : testCases) {
		EXPECT_EQ(std::get<0>(caseVal), lofar_udp_io_parse_type_optarg(std::get<1>(caseVal).c_str(), fileFormat, &baseVal, &stepVal, &offsetVal));
		EXPECT_EQ(std::get<2>(caseVal), std::string(fileFormat));
		EXPECT_EQ(std::get<0>(caseVal) == HDF5 ? 22 : 1, baseVal);
		EXPECT_EQ(std::get<0>(caseVal) == HDF5 ? -1 : 1, stepVal);
		EXPECT_EQ(std::get<0>(caseVal) == HDF5 ? -1 : 1, offsetVal);
		baseVal = -1; stepVal = -1; offsetVal = -1;
	}

	//int32_t lofar_udp_io_parse_format(char *dest, const char format[], int32_t port, int32_t iter, int32_t idx, int64_t pack)
	EXPECT_EQ(-1, lofar_udp_io_parse_format(nullptr, nullptr, 0, 0, 0, 0));
	const std::vector<std::string> parseTests = {
		"[[pack]]_[[port]]_[[iter]]__[[idx]]_[[fake]]",
		"64_1_0021__3_[[fake]]",
		"64_1_[[iter]]__3_[[fake]]"
	};
	int32_t port = 1;
	int32_t iter = 21;
	int32_t idx = 3;
	int64_t packet = 64;
	EXPECT_EQ(-1, lofar_udp_io_parse_format(fileFormat, parseTests[0].c_str(), -1, iter, idx, packet));
	EXPECT_EQ(-1, lofar_udp_io_parse_format(fileFormat, parseTests[0].c_str(), port, -1, idx, packet));
	EXPECT_EQ(-1, lofar_udp_io_parse_format(fileFormat, parseTests[0].c_str(), port, iter, -1, packet));

	EXPECT_EQ(-1, lofar_udp_io_parse_format(fileFormat, parseTests[0].c_str(), port, iter, idx, packet));
	EXPECT_EQ(parseTests[1], std::string(fileFormat));
};


const std::vector<std::string> testOptargStrings = {
	"FILE:ThisisAFile[[port]],100,10,2",
	"DADA:1000,10,2",
	"DADA:1000,1,1",
	"DADA:-1,10,2",
	"DADA:NotANum,10,2",
};

TEST(LibIoTests, OptargReaderParser) {
	// _lofar_udp_io_read_internal_lib_parse_optarg(lofar_udp_config *config, const char optargc[])
	lofar_udp_config *config = lofar_udp_config_alloc();
	EXPECT_EQ(-1, _lofar_udp_io_read_internal_lib_parse_optarg(nullptr, nullptr));
	config->numPorts = 2;

	EXPECT_EQ(0, _lofar_udp_io_read_internal_lib_parse_optarg(config, testOptargStrings[0].c_str()));
	EXPECT_EQ(std::string("ThisisAFile120"), std::string(config->inputLocations[0]));
	EXPECT_EQ(100, config->basePort);
	EXPECT_EQ(10, config->stepSizePort);
	EXPECT_EQ(2, config->offsetPortCount);
	EXPECT_EQ(0, _lofar_udp_io_read_internal_lib_parse_optarg(config, testOptargStrings[1].c_str()));
	EXPECT_EQ(1020, config->inputDadaKeys[0]);
	EXPECT_EQ(10, config->stepSizePort);
	EXPECT_EQ(2, config->offsetPortCount);

	EXPECT_EQ(0, _lofar_udp_io_read_internal_lib_parse_optarg(config, testOptargStrings[2].c_str()));
	EXPECT_EQ(2, config->stepSizePort);
	EXPECT_EQ(-5, _lofar_udp_io_read_internal_lib_parse_optarg(config, testOptargStrings[3].c_str()));
	EXPECT_EQ(-4, _lofar_udp_io_read_internal_lib_parse_optarg(config, testOptargStrings[4].c_str()));

	free(config);
};

TEST(LibIoTests, OptArgWriterParser) {
	// lofar_udp_io_write_parse_optarg(lofar_udp_io_write_config *config, const char optargc[])
	lofar_udp_io_write_config *output = lofar_udp_io_write_alloc();

	EXPECT_EQ(-1, lofar_udp_io_write_parse_optarg(nullptr, nullptr));


	output->numOutputs = 2;

	EXPECT_EQ(0, lofar_udp_io_write_parse_optarg(output, testOptargStrings[0].c_str()));
	EXPECT_EQ(std::string("ThisisAFile[[port]]"), std::string(output->outputFormat));
	EXPECT_EQ(100, output->baseVal);
	EXPECT_EQ(10, output->stepSize);
	EXPECT_EQ(0, lofar_udp_io_write_parse_optarg(output, testOptargStrings[1].c_str()));
	EXPECT_EQ(10, output->stepSize);

	EXPECT_EQ(0, lofar_udp_io_write_parse_optarg(output, testOptargStrings[2].c_str()));
	EXPECT_EQ(2, output->stepSize);
	EXPECT_EQ(-4, lofar_udp_io_write_parse_optarg(output, testOptargStrings[3].c_str()));
	EXPECT_EQ(-3, lofar_udp_io_write_parse_optarg(output, testOptargStrings[4].c_str()));

	free(output);
};