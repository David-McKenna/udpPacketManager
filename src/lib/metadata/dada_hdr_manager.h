#ifndef METADATA_STRCUTS_H
#define METADATA_STRCUTS_H

#include "lofar_udp_general.h"
#include "lofar_udp_structs.h"
#include "lofar_udp_misc.h"
#include "lofar_cli_meta.h"

#include "error.h"
// PSRDADA header interface
#include <ascii_header.h>

// gethostname(2)
#include <unistd.h>

int lofar_udp_dada_parse_header(lofar_udp_metadata *hdr, char *header);
int lofar_udp_generate_generic_hdr(char *header, lofar_udp_metadata *hdr);
int lofar_udp_generate_upm_header(char *header, lofar_udp_metadata *hdr, lofar_udp_reader *reader);

int wrap_ascii_header_set_char(char header[], const char key[], const char value[]);
int wrap_ascii_header_set_int(char header[], const char key[], const int value);
int wrap_ascii_header_set_long(char header[], const char key[], const long value);
int wrap_ascii_header_set_double(char header[], const char key[], const double value);
int wrap_ascii_header_get_char(char header[], const char key[], const char *value);
int wrap_ascii_header_get_int(char header[], const char key[], const int *value);
int wrap_ascii_header_get_long(char header[], const char key[], const long *value);
int wrap_ascii_header_get_double(char header[], const char key[], const double *value);

int isEmpty(const char *string);
int intNotSet(int input);
int longNotSet(long input);
int doubleNotSet(double input);

#endif // End of METADATA_STRUCTS_H