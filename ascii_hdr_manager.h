#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>

#ifndef __ASCII_HDR_H
#define __ASCII_HDR_H

#define HEADER_ARGS 24
// Likely don't need hald as many as provided here https://github.com/UCBerkeleySETI/rawspec/blob/master/rawspec_rawutils.c#L155
struct ascii_hdr_s {
	char src_name[68];
	char ra_str[68];
	char dec_str[68];

	// VV Validate, LOFAR mode 5 12-499
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

};

typedef struct ascii_hdr_s ascii_hdr;
extern ascii_hdr ascii_hdr_default;


void writeHdr(FILE *fileRef, ascii_hdr *header);
void writeStr(FILE *fileRef, char key[], char val[]);
void writeInt(FILE *fileRef, char key[], int val);
void writeLong(FILE *fileRef, char key[], long val);
void writeDouble(FILE *fileRef, char key[], double val);
int parseHdrFile(char inputFile[], ascii_hdr *header);
#endif