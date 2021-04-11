#include <getopt.h>
#include <unistd.h>


// Grab a couple of defines from the main header
#include "lofar_udp_io.h"


#ifndef ASCII_HDR_H
#define ASCII_HDR_H

// Shorthand for the number of arguments we support + 1 for endline
#define HDR_MEMBS 35

// Define a struct of variables for a fake ASCII header
//
// We likely don't need half as many keys as provided here, see
// https://github.com/UCBerkeleySETI/rawspec/blob/master/rawspec_rawutils.c#L155
//
// char values are 68(+\0) in length as each entry is 80 chars, each key is up to 8
// chars long, and parsing requires '= ' between the key and value. Two bytes
// are left as a buffer.
//
#define ASCII_HDR_STR_LEN 69
// Naming conventions all over the place to match the actual header key names
// @formatter:off
typedef struct ascii_hdr {
	char src_name[ASCII_HDR_STR_LEN];
	char ra_str[ASCII_HDR_STR_LEN];
	char dec_str[ASCII_HDR_STR_LEN];

	double obsfreq;
	double obsbw;
	double chan_bw;
	int obsnchan;
	int npol;
	int nbits;
	double tbin;

	char fd_poln[ASCII_HDR_STR_LEN];
	char trk_mode[ASCII_HDR_STR_LEN];
	char obs_mode[ASCII_HDR_STR_LEN];
	char cal_mode[ASCII_HDR_STR_LEN];
	double scanlen;

	char projid[ASCII_HDR_STR_LEN];
	char observer[ASCII_HDR_STR_LEN];
	char telescop[ASCII_HDR_STR_LEN];
	char frontend[ASCII_HDR_STR_LEN];
	char backend[ASCII_HDR_STR_LEN];
	char datahost[ASCII_HDR_STR_LEN];
	int dataport;
	int overlap;

	long blocsize;
	char daqpulse[ASCII_HDR_STR_LEN];
	int stt_imjd;
	int stt_smjd;
	long pktidx;
	char pktfmt[ASCII_HDR_STR_LEN];
	double stt_offs;
	int pktsize;
	double dropblk;
	double droptot;

} ascii_hdr;
// @formatter:on
extern const ascii_hdr ascii_hdr_default;

// Prototypes
int lofar_udp_metadata_write_ASCII(char *headerBuffer, size_t headerLength, ascii_hdr *headerStruct);
int parseHdrFile(char inputFile[], ascii_hdr *header);

// Private functions
/*
int writeStr(char *headerBuffer, char key[], char val[]);
int writeInt(char *headerBuffer, char key[], int val);
int writeLong(char *headerBuffer, char key[], long val);
int writeDouble(char *headerBuffer, char key[], double val);
 */
#endif