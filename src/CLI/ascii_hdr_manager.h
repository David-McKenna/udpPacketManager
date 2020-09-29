#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>


// Add extra verbosity at compile time
#ifndef __LOFAR_UDP_VERBOSE_MACRO
#define __LOFAR_UDP_VERBOSE_MACRO

#ifdef ALLOW_VERBOSE
#define VERBOSE(MSG) MSG; PAUSE;
#else
#define VERBOSE(MSG) while(0) {};
#endif

#endif

// Slow stopping macro (enable from makefile)
#ifndef __LOFAR_SLEEP
#define __LOFAR_SLEEP

#ifdef __SLOWDOWN
#include <unistd.h>
#define PAUSE sleep(1);
#else
#define PAUSE while(0) {};
#endif

#endif

#ifndef __ASCII_HDR_H
#define __ASCII_HDR_H

// Shorthand for the number of arguments we support
#define HEADER_ARGS 33

// Define a struct of variables for a fake ASCII header
//
// We likely don't need half as many keys as provided here, see
// https://github.com/UCBerkeleySETI/rawspec/blob/master/rawspec_rawutils.c#L155
//
// char values are 68 in length as each entry is 80 chars, each key is up to 8
// chars long, and parsing requires '= ' between the key and value. Two bytes
// are left as a buffer.
//
struct ascii_hdr_s {
	char src_name[68];
	char ra_str[68];
	char dec_str[68];

	double obsfreq;
	double obsbw;
	double chan_bw;
	int obsnchan;
	int npol;
	int nbits;
	double tbin;

	char fd_poln[68];
	char trk_mode[68];
	char obs_mode[68];
	char cal_mode[68];
	double scanlen;

	char projid[68];
	char observer[68];
	char telescop[68];
	char frontend[68];
	char backend[68];
	char datahost[68];
	int dataport;
	int overlap;

	long blocsize;
	char daqpulse[68];
	int stt_imjd;
	int stt_smjd;
	long pktidx;
	char pktfmt[68];
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