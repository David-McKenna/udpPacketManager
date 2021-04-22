#include <getopt.h>
#include <unistd.h>
#include <string.h>

// Grab a couple of defines from the main headers
#include "lofar_udp_general.h"
#include "lofar_udp_structs.h"
#include "lofar_udp_io.h"


#ifndef ASCII_HDR_H
#define ASCII_HDR_H

// Shorthand for the number of arguments we support + 1 for endline
#define HDR_MEMBS 35

extern const ascii_hdr ascii_hdr_default;

// Prototypes
int lofar_udp_metadata_setup_GUPPI(lofar_udp_metadata *metadata);
int lofar_udp_metadata_write_GUPPI(char *headerBuffer, size_t headerLength, ascii_hdr *headerStruct);
int parseHdrFile(char inputFile[], ascii_hdr *header);

// Private functions
/*
int writeStr(char *headerBuffer, char key[], char val[]);
int writeInt(char *headerBuffer, char key[], int val);
int writeLong(char *headerBuffer, char key[], long val);
int writeDouble(char *headerBuffer, char key[], double val);
 */
#endif