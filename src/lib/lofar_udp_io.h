


#include "lofar_udp_reader.h"
int fread_temp_FILE(void *outbuf, const size_t size, int num, FILE* inputFile, const int resetSeek);
int fread_temp_ZSTD(void *outbuf, const size_t size, int num, FILE* inputFile, const int resetSeek);
#ifndef NODADA
int fread_temp_dada(void *outbuf, const size_t size, int num, int dadaKey, const int resetSeek);
#endif
long fd_file_size(int fd);