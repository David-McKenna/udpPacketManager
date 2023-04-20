#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"
#include "lofar_udp_io.h"
#include <cstdio>
#include <iostream>
#include <filesystem>
// Struct alloc functions

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

					lofar_udp_io_read_config *readConfig = lofar_udp_io_alloc_read();
					readConfig->readerType = type;
					readConfig->numInputs = testNumInputs;

					lofar_udp_io_write_config *writeConfig = lofar_udp_io_alloc_write();
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
						writeConfig->writeBufSize[i] = buffFact * testContents.length();
						readConfig->readBufSize[i] = testContents.length();
					}

					int pid;

					if (type == DADA_ACTIVE || type == FIFO) {
						int ret;
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

						for (int i = 0; i < testNumInputs; i++) {
							lofar_udp_io_write_cleanup(writeConfig, i, 1);
						}


						if (pid == 0) {
							std::cout << "Write exit: " << returnVal  << std::endl;
							free(writeConfig);
							exit(returnVal == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
						}

					}

					// Temp read test
					// int64_t lofar_udp_io_read_temp(const lofar_udp_config *config, int8_t port, int8_t *outbuf, size_t size, int64_t num, int resetSeek)
					lofar_udp_config *config = lofar_udp_config_alloc();
					config->readerType = type;
					for (int i = 0; i < testNumInputs; i++) {
						if (type == DADA_ACTIVE) {
							close(syncPipe[0]);
							//usleep(5000);
							ipcbuf_t tmpStruct;
							while (ipcbuf_connect(&tmpStruct, readConfig->inputDadaKeys[i] + 1)) {
								usleep(1000);
							}
							ipcbuf_disconnect(&tmpStruct);
							std::cout << std::to_string(readConfig->inputDadaKeys[i] + 1) << std::endl;
							config->inputDadaKeys[i] = readConfig->inputDadaKeys[i];
						}

						snprintf(config->inputLocations[i], DEF_STR_LEN, "%s", readConfig->inputLocations[i]);
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
					for (int i = 0; i < testNumInputs; i++) {
						// FIFO cannot be temp, wait for it to be available here
						if (type == FIFO) {
							//usleep(5000);
							while (access(readConfig->inputLocations[i], F_OK)) {
								usleep(1000);
							}
						}

						// Be extremely verbose with DADA keys; in testing bits were randomly added to the upper ender of the key.
						if (type == DADA_ACTIVE) std::cout << std::to_string(readConfig->inputDadaKeys[i] + 1) << std::endl;
						EXPECT_EQ(0, lofar_udp_io_read_setup(readConfig, i));
						if (type == DADA_ACTIVE) std::cout << std::to_string(readConfig->inputDadaKeys[i] + 1) << std::endl;

					}

					// Write/Read comparison
					for (int j = 0; j < ops; j++) {
						for (int8_t i = 0; i < testNumInputs; i++) {
							ARR_INIT(testBuffer, testContents.length(), '\0');
							if (type != ZSTDCOMPRESSED || j != 0) {
								EXPECT_EQ(testContents.length(), lofar_udp_io_read(readConfig, i, testBuffer, testContents.length()));
							} else {
								EXPECT_EQ(-1, lofar_udp_io_read(readConfig, i, testBuffer, testContents.length()));
								readConfig->decompressionTracker[i].dst = testBuffer;
								EXPECT_EQ(testContents.length(), lofar_udp_io_read(readConfig, i, testBuffer, testContents.length()));
							}
							EXPECT_EQ(std::string((char *) testBuffer), testContents);
						}
					}

					if (type == DADA_ACTIVE) {
						write(syncPipe[1], &syncPipe[0], sizeof(int));
						close(syncPipe[1]);
					}

					for (int i = 0; i < testNumInputs; i++) {
						lofar_udp_io_read_cleanup(readConfig, i);
					}

					if (type == DADA_ACTIVE || type == FIFO) {
						wait(&pid);
						EXPECT_EQ(0, pid & 255); // FAILURE ON FORKED WRITER SIDE
					}

					for (int i = 0; i < testNumInputs; i++) {
						std::remove(readConfig->inputLocations[i]);
					}

					free(writeConfig);
					free(readConfig);
					free(testBuffer);
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


TEST(LibIoTests, ConfigSetupHelper) {
	//int lofar_udp_io_read_setup_helper(lofar_udp_io_read_config *input, const lofar_udp_config *config, const lofar_udp_obs_meta *meta,
	//                                   int port);
	//int lofar_udp_io_write_setup_helper(lofar_udp_io_write_config *config, const lofar_udp_obs_meta *meta, int iter);
	EXPECT_NONFATAL_FAILURE(EXPECT_TRUE(false), "");
};

TEST(LibIoTests, OptargParser) {
	//int lofar_udp_io_read_parse_optarg(lofar_udp_config *config, const char optarg[]);
	//int lofar_udp_io_write_parse_optarg(lofar_udp_io_write_config *config, const char optarg[]);
	EXPECT_NONFATAL_FAILURE(EXPECT_TRUE(false), "");
};

/*

// optarg Parse functions



// Operate functions
// long lofar_udp_io_read_HDF5(lofar_udp_io_read_config *input, int port, char *targetArray, long nchars);

long lofar_udp_io_write_HDF5(lofar_udp_io_write_config *config, const int outp, const int8_t *src, const long nchars);
long lofar_udp_io_write_metadata_HDF5(lofar_udp_io_write_config *config, lofar_udp_metadata *metadata);

*/