#ifndef LOFAR_UDP_METADATA_H
#define LOFAR_UDP_METADATA_H


#include "lofar_udp_general.h"
#include "lofar_udp_structs.h"
#include "lofar_udp_misc.h"
#include "lofar_udp_io.h"

#include <errno.h>
#include <getopt.h>
#include <string.h>

// gethostname(2)
#include <unistd.h>

// PSRDADA header interface
#include <ascii_header.h>

// Shorthand for the number of arguments we support + 1 for endline
#define ASCII_HDR_MEMBS 35


// Allow C++ imports too
#ifdef __cplusplus
extern "C" {
#endif

// Main public functons
int lofar_udp_metadata_setup();
int lofar_udp_metadata_update();
int lofar_udp_metadata_write();

// Internal representations
int lofar_udp_metadata_setup_GUPPI(lofar_udp_metadata *metadata);
int lofar_udp_metadata_setup_DADA(lofar_udp_metadata *metadata);
int lofar_udp_metadata_setup_SIGPROC(lofar_udp_metadata *metadata);

int lofar_udp_metadata_update_GUPPI(lofar_udp_reader *reader, lofar_udp_metadata *metadata, int newBlock);
int lofar_udp_metadata_update_DADA(lofar_udp_metadata *metadata);
int lofar_udp_metadata_update_SIGPROC(lofar_udp_metadata *metadata);

int lofar_udp_metadata_write_GUPPI(char *headerBuffer, size_t headerLength, ascii_hdr *headerStruct);
int lofar_udp_metadata_write_DADA(char *headerBuffer, size_t headerLength, lofar_udp_metadata *headerStruct);
int lofar_udp_metadata_write_SIGPROC(char *headerBuffer, size_t headerLength, sigproc_hdr *headerStruct);


// Internal functions
int lofar_udp_metadata_parse_input_file(lofar_udp_metadata *metadata, const char inputFile[]);
int lofar_udp_metadata_parse_reader(lofar_udp_metadata *metadata, lofar_udp_reader *reader);
int lofar_udp_metadata_parse_subbands(lofar_udp_metadata *metadata, const char *inputLine, int *results);
int lofar_udp_metadata_parse_pointing(lofar_udp_metadata *metadata, const char inputStr[], int digi);
int lofar_udp_metadata_parse_rcumode(lofar_udp_metadata *metadata, char *inputStr, int *beamctlData);
int lofar_udp_metadata_parse_csv(const char *inputStr, int *values, int *limits);
int lofar_udp_metadata_count_csv(const char *inputStr);
int lofar_udp_metadata_get_tsv(const char *inputStr, const char *keyword, char *result);
int lofar_udp_metadata_parse_beamctl(lofar_udp_metadata *metadata, const char *inputLine, int *rcuMode);
int lofar_udp_metadata_get_clockmode(int input);
int lofar_udp_metadata_get_rcumode(int input);
metadata_t lofar_udp_metadata_string_to_meta(const char input[]);
#ifdef __cplusplus
}
#endif

#endif // End of LOFAR_UDP_METADATA_H
