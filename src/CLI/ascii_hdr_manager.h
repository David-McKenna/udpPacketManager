#include <getopt.h>
#include <unistd.h>


// Grab a couple of defines from the main header
#include "lofar_udp_io.h"

#ifndef ASCII_HDR_H
#define ASCII_HDR_H

// Shorthand for the number of arguments we support
#define HEADER_ARGS 33

// Define a struct of variables for a fake ASCII header
//
// We likely don't need half as many keys as provided here, see
// https://github.com/UCBerkeleySETI/rawspec/blob/master/rawspec_rawutils.c#L155
//
// char values are 68(+\0) in length as each entry is 80 chars, each key is up to 8
// chars long, and parsing requires '= ' between the key and value. Two bytes
// are left as a buffer.
//
struct ascii_hdr_s {
	char src_name[69];
	char ra_str[69];
	char dec_str[69];

	double obsfreq;
	double obsbw;
	double chan_bw;
	int obsnchan;
	int npol;
	int nbits;
	double tbin;

	char fd_poln[69];
	char trk_mode[69];
	char obs_mode[69];
	char cal_mode[69];
	double scanlen;

	char projid[69];
	char observer[69];
	char telescop[69];
	char frontend[69];
	char backend[69];
	char datahost[69];
	int dataport;
	int overlap;

	long blocsize;
	char daqpulse[69];
	int stt_imjd;
	int stt_smjd;
	long pktidx;
	char pktfmt[69];
	double stt_offs;
	int pktsize;
	double dropblk;
	double droptot;

};

// Define the default valued struct
typedef struct ascii_hdr_s ascii_hdr;
extern ascii_hdr ascii_hdr_default;

// Prototypes
void writeHdr(FILE *fileRef, ascii_hdr *header);
void writeStr(FILE *fileRef, char key[], char val[]);
void writeInt(FILE *fileRef, char key[], int val);
void writeLong(FILE *fileRef, char key[], long val);
void writeDouble(FILE *fileRef, char key[], double val);
int parseHdrFile(char inputFile[], ascii_hdr *header);
#endif