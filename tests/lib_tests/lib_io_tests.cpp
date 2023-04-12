#include "gtest/gtest.h"
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
				for (reader_t type: std::vector<reader_t>{ NORMAL, FIFO, ZSTDCOMPRESSED, ZSTDCOMPRESSED_INDIRECT, DADA_ACTIVE }) {
					int8_t *testBuffer = (int8_t *) calloc(testContents.length() + 1, sizeof(int8_t));

					for (int i = 0; i < testNumInputs; i++) {
						std::string fileLoc = "./my_file_" + std::to_string(i);
						std::remove(fileLoc.c_str());

					}
					lofar_udp_io_write_config *writeConfig = lofar_udp_io_alloc_write();
					lofar_udp_io_read_config *readConfig = lofar_udp_io_alloc_read();
					writeConfig->readerType = type;
					readConfig->readerType = type;
					writeConfig->numOutputs = testNumInputs;
					readConfig->numInputs = testNumInputs;
					writeConfig->dadaConfig.nbufs = psrdadaBufs;
					writeConfig->dadaConfig.num_readers = 1;
					writeConfig->progressWithExisting = 1;

					std::cout << "Starting tests for " << std::to_string(type) << std::endl;



					std::string fileName = "./my_file_[[idx]]";
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
								ipcio_destroy(&tmp);
								usleep(5000);
							}
							if (!ipcio_connect(&tmp, readConfig->inputDadaKeys[i] + 1)) {
								ipcio_destroy(&tmp);
								usleep(5000);
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

					if (pid == 0 || !(type == DADA_ACTIVE || type == FIFO)) {

						int returnVal = 0;

						returnVal += lofar_udp_io_write_setup(writeConfig, 0);
						// Closing a FIFO disconnects the reader, can't test reallocation with this methodology
						if (type != FIFO) {
							returnVal += lofar_udp_io_write_setup(writeConfig, 0);
						}

						EXPECT_EQ(0, returnVal);

						for (int j = 0; j < ops; j++) {
							for (int i = 0; i < testNumInputs; i++) {
								int64_t bytes = lofar_udp_io_write(writeConfig, (int8_t) i, (int8_t *) testContents.c_str(), testContents.length());
								returnVal += !((int64_t) testContents.length() == bytes);
								EXPECT_EQ(0, returnVal);
							}
						}

						if (type == DADA_ACTIVE) {
							close(syncPipe[1]);
							UNUSED_RETURN(read(syncPipe[0], &syncPipe[1], sizeof(int)));
							close(syncPipe[0]);
						}

						for (int i = 0; i < testNumInputs; i++) {
							lofar_udp_io_write_cleanup(writeConfig, i, 1);
						}

						if (pid == 0) {
							std::cout << "Write exit: " << returnVal  << std::endl;
							exit(returnVal == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
						}

					}

					for (int i = 0; i < testNumInputs; i++) {
						if (type == FIFO) {
							//usleep(5000);
							while (access(readConfig->inputLocations[i], F_OK)) {
								usleep(1000);
							}
						} else if (type == DADA_ACTIVE) {
							close(syncPipe[0]);
							//usleep(5000);
							ipcbuf_t tmpStruct;
							while (ipcbuf_connect(&tmpStruct, readConfig->inputDadaKeys[i] + 1)) {
								usleep(1000);
							}
							ipcbuf_disconnect(&tmpStruct);
							std::cout << std::to_string(readConfig->inputDadaKeys[i] + 1);

						}

						// Be extremely verbose with DADA keys; in testing bits were randomly added to the upper ender of the key.
						if (type == DADA_ACTIVE) std::cout << std::to_string(readConfig->inputDadaKeys[i] + 1) << std::endl;
						EXPECT_EQ(0, lofar_udp_io_read_setup(readConfig, i));
						if (type == DADA_ACTIVE) std::cout << std::to_string(readConfig->inputDadaKeys[i] + 1) << std::endl;

					}

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
						UNUSED_RETURN(write(syncPipe[1], &syncPipe[0], sizeof(int)));
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

					free(testBuffer);
				}
			}
		}

		// int64_t lofar_udp_io_read_temp(const lofar_udp_config *config, int8_t port, int8_t *outbuf, size_t size, int64_t num, int resetSeek)


	}
};


/*
// Setup wrapper functions
int lofar_udp_io_read_setup(lofar_udp_io_read_config *input, int port);
int lofar_udp_io_write_setup(lofar_udp_io_write_config *config, int iter);
int lofar_udp_io_read_setup_helper(lofar_udp_io_read_config *input, const lofar_udp_config *config, const lofar_udp_obs_meta *meta,
                                   int port);
int lofar_udp_io_write_setup_helper(lofar_udp_io_write_config *config, const lofar_udp_obs_meta *meta, int iter);

// Operate wrapper functions
long lofar_udp_io_read(lofar_udp_io_read_config *input, int port, int8_t *targetArray, const long nchars);
long lofar_udp_io_write(lofar_udp_io_write_config *config, int outp, const int8_t *src, const long nchars);
long lofar_udp_io_write_metadata(lofar_udp_io_write_config *outConfig, int outp, lofar_udp_metadata *metadata, char *headerBuffer, size_t headerLength);
int
lofar_udp_io_read_temp(const lofar_udp_config *config, int port, void *outbuf, size_t size, int num, int resetSeek);

// Cleanup wrapper functions
int lofar_udp_io_read_cleanup(lofar_udp_io_read_config *input, int port);
int lofar_udp_io_write_cleanup(lofar_udp_io_write_config *config, int outp, int fullClean);

// optarg Parse functions
int lofar_udp_io_read_parse_optarg(lofar_udp_config *config, const char optarg[]);
int lofar_udp_io_write_parse_optarg(lofar_udp_io_write_config *config, const char optarg[]);



// Internal wrapped functions
// Setup functions
int lofar_udp_io_read_setup_FILE(lofar_udp_io_read_config *input, const char *inputLocation, int port);
int
lofar_udp_io_read_setup_ZSTD(lofar_udp_io_read_config *input, const char *inputLocation, const int port);
int
lofar_udp_io_read_setup_DADA(lofar_udp_io_read_config *input, const int dadaKey, const int port);
// int lofar_udp_io_read_setup_HDF5(lofar_udp_io_read_config *input, const char *inputLocation, const int port);

int lofar_udp_io_write_setup_FILE(lofar_udp_io_write_config *config, int outp, int iter);
int lofar_udp_io_write_setup_ZSTD(lofar_udp_io_write_config *config, int outp, int iter);
int lofar_udp_io_write_setup_DADA(lofar_udp_io_write_config *config, int outp);
int lofar_udp_io_write_setup_HDF5(lofar_udp_io_write_config *config, __attribute__((unused)) int outp, int iter);


// Operate functions
long lofar_udp_io_read_FILE(lofar_udp_io_read_config *input, int port, char *targetArray, long nchars);
long lofar_udp_io_read_ZSTD(lofar_udp_io_read_config *input, int port, char *targetArray, long nchars);
long lofar_udp_io_read_DADA(lofar_udp_io_read_config *input, int port, char *targetArray, long nchars);
// long lofar_udp_io_read_HDF5(lofar_udp_io_read_config *input, int port, char *targetArray, long nchars);

long lofar_udp_io_write_FILE(lofar_udp_io_write_config *config, const int outp, const int8_t *src, const long nchars);
long lofar_udp_io_write_ZSTD(lofar_udp_io_write_config *config, const int outp, const int8_t *src, const long nchars);
long lofar_udp_io_write_DADA(ipcio_t *ringbuffer, const int outp, const int8_t *src, const long nchars);
long lofar_udp_io_write_HDF5(lofar_udp_io_write_config *config, const int outp, const int8_t *src, const long nchars);
long lofar_udp_io_write_metadata_HDF5(lofar_udp_io_write_config *config, lofar_udp_metadata *metadata);

int lofar_udp_io_read_temp_FILE(void *outbuf, size_t size, int num, const char inputFile[], int resetSeek);
int lofar_udp_io_read_temp_ZSTD(void *outbuf, size_t size, int num, const char inputFile[], int resetSeek);
int lofar_udp_io_read_temp_DADA(void *outbuf, size_t size, int num, int dadaKey, int resetSeek);
// int lofar_udp_io_read_temp_HDF5(void *outbuf, size_t size, int num, const char inputFile[], int resetSeek);

// Cleanup functions
int lofar_udp_io_read_cleanup_FILE(lofar_udp_io_read_config *input, int port);
int lofar_udp_io_read_cleanup_ZSTD(lofar_udp_io_read_config *input, int port);
int lofar_udp_io_read_cleanup_DADA(lofar_udp_io_read_config *input, int port);
__attribute__((unused)) int lofar_udp_io_read_cleanup_HDF5(lofar_udp_io_read_config *input, int port);

int lofar_udp_io_write_cleanup_FILE(lofar_udp_io_write_config *config, int outp);
int lofar_udp_io_write_cleanup_ZSTD(lofar_udp_io_write_config *config, int outp, int fullClean);
int lofar_udp_io_write_cleanup_DADA(lofar_udp_io_write_config *config, int outp, int fullClean);
int lofar_udp_io_write_cleanup_HDF5(lofar_udp_io_write_config *config, int outp, int fullClean);
*/