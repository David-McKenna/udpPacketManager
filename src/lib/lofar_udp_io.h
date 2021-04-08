#ifndef LOFAR_UDP_IO
#define LOFAR_UDP_IO

#include "lofar_udp_structs.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>


int lofar_udp_io_read_parse_optarg(lofar_udp_config *config, const char optarg[]);
int lofar_udp_io_write_parse_optarg(lofar_udp_io_write_config *config, const char optarg[]);

// Setup functions
int lofar_udp_io_read_setup(lofar_udp_reader_input *input, const lofar_udp_config *config, const lofar_udp_meta *meta,
							int port);
int lofar_udp_io_read_setup_FILE(lofar_udp_reader_input *input, const lofar_udp_config *config, int port);
int
lofar_udp_io_read_setup_ZSTD(lofar_udp_reader_input *input, const lofar_udp_config *config, const lofar_udp_meta *meta,
							 int port);
int
lofar_udp_io_read_setup_DADA(lofar_udp_reader_input *input, const lofar_udp_config *config, const lofar_udp_meta *meta,
							 int port);
int lofar_udp_io_write_setup(lofar_udp_io_write_config *config, const lofar_udp_meta *meta, int iter);
int lofar_udp_io_write_setup_FILE(lofar_udp_io_write_config *config, const lofar_udp_meta *meta, int outp, int iter);
int lofar_udp_io_write_setup_ZSTD(lofar_udp_io_write_config *config, const lofar_udp_meta *meta, int outp, int iter);
int lofar_udp_io_write_setup_DADA(lofar_udp_io_write_config *config, const lofar_udp_meta *meta, int outp);

// Read functions
long lofar_udp_io_read(lofar_udp_reader_input *input, int port, char *targetArray, long nchars);
long lofar_udp_io_read_FILE(lofar_udp_reader_input *input, int port, char *targetArray, long nchars);
long lofar_udp_io_read_ZSTD(lofar_udp_reader_input *input, int port, long nchars);
long lofar_udp_io_read_DADA(lofar_udp_reader_input *input, int port, char *targetArray, long nchars);
long lofar_udp_io_write(lofar_udp_io_write_config *config, int outp, char *src, const long nchars);
long lofar_udp_io_write_FILE(lofar_udp_io_write_config *config, int outp, char *src, const long nchars);
long lofar_udp_io_write_ZSTD(lofar_udp_io_write_config *config, int outp, char *src, const long nchars);
long lofar_udp_io_write_DADA(lofar_udp_io_write_config *config, int outp, char *src, const long nchars);
int lofar_udp_io_read_cleanup(lofar_udp_reader_input *input, int port);
int lofar_udp_io_read_cleanup_FILE(lofar_udp_reader_input *input, int port);
int lofar_udp_io_read_cleanup_ZSTD(lofar_udp_reader_input *input, int port);
int lofar_udp_io_read_cleanup_DADA(lofar_udp_reader_input *input, int port);
int lofar_udp_io_write_cleanup(lofar_udp_io_write_config *config, int outp, int fullClean);
int lofar_udp_io_write_cleanup_FILE(lofar_udp_io_write_config *config, int outp, int fullClean);
int lofar_udp_io_write_cleanup_ZSTD(lofar_udp_io_write_config *config, int outp, int fullClean);
int lofar_udp_io_write_cleanup_DADA(lofar_udp_io_write_config *config, int outp, int fullClean);


// Temporary read functions
int
lofar_udp_io_fread_temp(const lofar_udp_config *config, int port, void *outbuf, size_t size, int num, int resetSeek);
int lofar_udp_io_fread_temp_FILE(void *outbuf, size_t size, int num, const char inputFile[], int resetSeek);
int lofar_udp_io_fread_temp_ZSTD(void *outbuf, size_t size, int num, const char inputFile[], int resetSeek);
int lofar_udp_io_fread_temp_DADA(void *outbuf, size_t size, int num, int dadaKey, int resetSeek);

// Metadata functions
long FILE_file_size(FILE *fileptr);
long fd_file_size(int fd);

#endif