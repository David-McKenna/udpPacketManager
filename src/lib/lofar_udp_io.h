#ifndef LOFAR_UDP_IO
#define LOFAR_UDP_IO

#include "lofar_udp_structs.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

// BitShuffle header for HDF5 filter
#include "bshuf_h5filter.h"


// Allow C++ imports too
#ifdef __cplusplus
extern "C" {
#endif
// Struct alloc functions
lofar_udp_io_read_config* lofar_udp_io_alloc_read();
lofar_udp_io_write_config* lofar_udp_io_alloc_write();

// Setup wrapper functions
int lofar_udp_io_read_setup(lofar_udp_io_read_config *input, int port);
int lofar_udp_io_write_setup(lofar_udp_io_write_config *config, int iter);
int lofar_udp_io_read_setup_helper(lofar_udp_io_read_config *input, const lofar_udp_config *config, const lofar_udp_input_meta *meta,
                                   int port);
int lofar_udp_io_write_setup_helper(lofar_udp_io_write_config *config, const lofar_udp_input_meta *meta, int iter);

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


// Other functions
void swapCharPtr(char **a, char **b);
long FILE_file_size(FILE *fileptr);
long fd_file_size(int fd);

#ifdef __cplusplus
}
#endif


#endif