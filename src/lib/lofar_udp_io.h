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

// optarg Parse functions
int lofar_udp_io_read_parse_optarg(lofar_udp_config *config, const char optarg[]);
int lofar_udp_io_write_parse_optarg(lofar_udp_io_write_config *config, const char optarg[]);


#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-avoid-const-params-in-decls"
// Setup wrapper functions
int _lofar_udp_io_read_setup_internal_lib_helper(lofar_udp_io_read_config *const input, const lofar_udp_config *config, const lofar_udp_obs_meta *meta,
                                                 int8_t port);
int lofar_udp_io_write_setup_helper(lofar_udp_io_write_config *config, const lofar_udp_obs_meta *meta, int32_t iter);

// Operate wrapper functions
int64_t lofar_udp_io_read(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars);
int64_t lofar_udp_io_read_temp(const lofar_udp_config *config, int8_t port, int8_t *outbuf, int64_t size, int64_t num, int8_t resetSeek);
int64_t lofar_udp_io_write(lofar_udp_io_write_config *const config, int8_t outp, const int8_t *src, int64_t nchars);
int64_t lofar_udp_io_write_metadata(lofar_udp_io_write_config *const outConfig, int8_t outp, const lofar_udp_metadata *metadata, const int8_t *headerBuffer, int64_t headerLength);

// More generic setup functions
int lofar_udp_io_read_setup(lofar_udp_io_read_config *input, int8_t port);
int lofar_udp_io_write_setup(lofar_udp_io_write_config *config, int32_t iter);

// Cleanup wrapper functions
void lofar_udp_io_read_cleanup(lofar_udp_io_read_config *input, int8_t port);
void lofar_udp_io_write_cleanup(lofar_udp_io_write_config *config, int8_t outp, int8_t fullClean);



// Internal wrapped functions
// Setup functions
int _lofar_udp_io_read_setup_FILE(lofar_udp_io_read_config *const input, const char *inputLocation, int8_t port);
int
_lofar_udp_io_read_setup_ZSTD(lofar_udp_io_read_config *const input, const char *inputLocation, int8_t port);
int
_lofar_udp_io_read_setup_DADA(lofar_udp_io_read_config *const input, int32_t dadaKey, int8_t port);
__attribute__((unused)) int _lofar_udp_io_read_setup_HDF5(lofar_udp_io_read_config *const input, const char *inputLocation, int8_t port);

int _lofar_udp_io_write_setup_FILE(lofar_udp_io_write_config *const config, int8_t outp, int32_t iter);
int _lofar_udp_io_write_setup_ZSTD(lofar_udp_io_write_config *const config, int8_t outp, int32_t iter);
int _lofar_udp_io_write_setup_DADA(lofar_udp_io_write_config *const config, int8_t outp);
int _lofar_udp_io_write_setup_HDF5(lofar_udp_io_write_config *const config, int8_t outp, int32_t iter);


// Operate functions
int64_t _lofar_udp_io_read_FILE(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars);
int64_t _lofar_udp_io_read_ZSTD(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars);
int64_t _lofar_udp_io_read_DADA(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars);
__attribute__((unused)) int64_t _lofar_udp_io_read_HDF5(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars);

// ZSTD fixup
int64_t _lofar_udp_io_read_ZSTD_fix_buffer_size(int64_t bufferSize, int8_t deltaOnly);

int64_t _lofar_udp_io_write_FILE(lofar_udp_io_write_config *const config, int8_t outp, const int8_t *src, int64_t nchars);
int64_t _lofar_udp_io_write_FIFO(lofar_udp_io_write_config *const config, int8_t outp, const int8_t *src, int64_t nchars);
int64_t _lofar_udp_io_write_ZSTD(lofar_udp_io_write_config *const config, int8_t outp, const int8_t *src, int64_t nchars);
int64_t _lofar_udp_io_write_DADA(ipcio_t *const ringbuffer, const int8_t *src, int64_t nchars, int8_t ipcbuf);
int64_t _lofar_udp_io_write_HDF5(lofar_udp_io_write_config *const config, int8_t outp, const int8_t *src, int64_t nchars);
int64_t _lofar_udp_io_write_metadata_HDF5(lofar_udp_io_write_config *const config, const lofar_udp_metadata *metadata);

int64_t _lofar_udp_io_read_temp_FILE(void *outbuf, int64_t size, int64_t num, const char inputFile[], int8_t resetSeek);
int64_t _lofar_udp_io_read_temp_ZSTD(void *outbuf, int64_t size, int64_t num, const char inputFile[], int8_t resetSeek);
int64_t _lofar_udp_io_read_temp_DADA(void *outbuf, int64_t size, int64_t num, key_t dadaKey, int8_t resetSeek);
int64_t _lofar_udp_io_read_temp_HDF5(void *outbuf, int64_t size, int8_t num, const char inputFile[], int resetSeek);

// Cleanup functions
void _lofar_udp_io_read_cleanup_FILE(lofar_udp_io_read_config *const input, int8_t port);
void _lofar_udp_io_read_cleanup_ZSTD(lofar_udp_io_read_config *const input, int8_t port);
void _lofar_udp_io_read_cleanup_DADA(lofar_udp_io_read_config *const input, int8_t port);
__attribute__((unused)) void _lofar_udp_io_read_cleanup_HDF5(lofar_udp_io_read_config *const input, int8_t port);

void _lofar_udp_io_write_cleanup_FILE(lofar_udp_io_write_config *const config, int8_t outp);
void _lofar_udp_io_write_cleanup_ZSTD(lofar_udp_io_write_config *const config, int8_t outp, int8_t fullClean);
void _lofar_udp_io_write_cleanup_DADA(lofar_udp_io_write_config *const config, int8_t outp, int8_t fullClean);
void _lofar_udp_io_write_cleanup_HDF5(lofar_udp_io_write_config *config, int8_t outp, int8_t fullClean);
#pragma clang diagnostic pop

// Other functions
void _swapCharPtr(char **a, char **b);
long _FILE_file_size(FILE *fileptr);
long _fd_file_size(int fd);

#ifdef __cplusplus
}
#endif


#endif