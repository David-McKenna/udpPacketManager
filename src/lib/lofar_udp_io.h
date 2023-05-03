#ifndef LOFAR_UDP_IO
#define LOFAR_UDP_IO

#include "lofar_udp_structs.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>

// BitShuffle header for HDF5 filter
#include "bshuf_h5filter.h"


// Allow C++ imports too
#ifdef __cplusplus
extern "C" {
#endif

// optarg/file name Parse functions
reader_t lofar_udp_io_parse_type_optarg(const char optargc[], char *fileFormat, int32_t *baseVal, int16_t *stepSize, int8_t *offsetVal);
int32_t _lofar_udp_io_read_internal_lib_parse_optarg(lofar_udp_config *config, const char optargc[]);
int32_t lofar_udp_io_write_parse_optarg(lofar_udp_io_write_config *config, const char optargc[]);
int32_t lofar_udp_io_parse_format(char *dest, const char format[], int32_t port, int32_t iter, int32_t idx, int64_t pack);

// Setup wrapper functions
int32_t lofar_udp_io_read_setup_helper(lofar_udp_io_read_config *input, int8_t **outputArr, int64_t maxReadSize, int8_t port);
int32_t lofar_udp_io_write_setup_helper(lofar_udp_io_write_config *config, int64_t outputLength[], int8_t numOutputs, int32_t iter, int64_t firstPacket);
// Operate wrapper functions
int64_t lofar_udp_io_read(lofar_udp_io_read_config *const input, int8_t port, int8_t *targetArray, int64_t nchars);
int64_t lofar_udp_io_read_temp(const lofar_udp_config *config, int8_t port, int8_t *outbuf, int64_t size, int64_t num, int8_t resetSeek);
int64_t lofar_udp_io_write(lofar_udp_io_write_config *const config, int8_t outp, const int8_t *src, int64_t nchars);
int64_t lofar_udp_io_write_metadata(lofar_udp_io_write_config *const outConfig, int8_t outp, const lofar_udp_metadata *metadata, const int8_t *headerBuffer, int64_t headerLength);

// More generic setup functions
int32_t lofar_udp_io_read_setup(lofar_udp_io_read_config *input, int8_t port);
int32_t lofar_udp_io_write_setup(lofar_udp_io_write_config *config, int32_t iter);

// Cleanup wrapper functions
void lofar_udp_io_read_cleanup(lofar_udp_io_read_config *input, int8_t port);
void lofar_udp_io_write_cleanup(lofar_udp_io_write_config *config, int8_t outp, int8_t fullClean);



// Internal wrapped functions
// Setup functions
int32_t _lofar_udp_io_read_setup_internal_lib_helper(lofar_udp_io_read_config *const input, const lofar_udp_config *config, lofar_udp_obs_meta *meta,
                                                 int8_t port);
int32_t _lofar_udp_io_write_internal_lib_setup_helper(lofar_udp_io_write_config *config, lofar_udp_reader *reader, int32_t iter);

int32_t _lofar_udp_io_read_setup_FILE(lofar_udp_io_read_config *const input, const char *inputLocation, int8_t port);
int32_t
_lofar_udp_io_read_setup_ZSTD(lofar_udp_io_read_config *const input, const char *inputLocation, int8_t port);
int32_t
_lofar_udp_io_read_setup_DADA(lofar_udp_io_read_config *const input, key_t dadaKey, int8_t port);
__attribute__((unused)) int32_t _lofar_udp_io_read_setup_HDF5(lofar_udp_io_read_config *const input, const char *inputLocation, int8_t port);

int32_t _lofar_udp_io_write_setup_FILE(lofar_udp_io_write_config *const config, int8_t outp, int32_t iter);
int32_t _lofar_udp_io_write_setup_ZSTD(lofar_udp_io_write_config *const config, int8_t outp, int32_t iter);
int32_t _lofar_udp_io_write_setup_DADA(lofar_udp_io_write_config *const config, int8_t outp);
int32_t _lofar_udp_io_write_setup_HDF5(lofar_udp_io_write_config *const config, int8_t outp, int32_t iter);


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
int64_t _lofar_udp_io_read_temp_HDF5(void *outbuf, int64_t size, int8_t num, const char inputFile[], int8_t resetSeek);

// Cleanup functions
void _lofar_udp_io_read_cleanup_FILE(lofar_udp_io_read_config *const input, int8_t port);
void _lofar_udp_io_read_cleanup_ZSTD(lofar_udp_io_read_config *const input, int8_t port);
void _lofar_udp_io_read_cleanup_DADA(lofar_udp_io_read_config *const input, int8_t port);
__attribute__((unused)) void _lofar_udp_io_read_cleanup_HDF5(lofar_udp_io_read_config *const input, int8_t port);

void _lofar_udp_io_write_cleanup_FILE(lofar_udp_io_write_config *const config, int8_t outp);
void _lofar_udp_io_write_cleanup_ZSTD(lofar_udp_io_write_config *const config, int8_t outp, int8_t fullClean);
void _lofar_udp_io_write_cleanup_DADA(lofar_udp_io_write_config *const config, int8_t outp, int8_t fullClean);
void _lofar_udp_io_write_cleanup_HDF5(lofar_udp_io_write_config *const config, int8_t outp, int8_t fullClean);

// Other functions
void _swapCharPtr(char **a, char **b);
int64_t _FILE_file_size(FILE *fileptr);
int64_t _fd_file_size(int32_t fd);

#ifdef __cplusplus
}
#endif


#endif

/**
 * Copyright (C) 2023 David McKenna
 * This file is part of udpPacketManager <https://github.com/David-McKenna/udpPacketManager>.
 *
 * udpPacketManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * udpPacketManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with udpPacketManager.  If not, see <http://www.gnu.org/licenses/>.
 **/
