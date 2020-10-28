// Standard required includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zstd.h>
#include <omp.h>
#include <limits.h>
#include <time.h>
#include <malloc.h>

#ifndef __LOFAR_COMPILE_CONSTANTS
#define __LOFAR_COMPILE_CONSTANTS

// I/O array sizes
#define MAX_NUM_PORTS 4
#define MAX_OUTPUT_DIMS 4

// CEP packet reference values
#define UDPHDRLEN 16
#define UDPCURVER 3
#define UDPMAXBEAM 244
#define UDPNPOL 4
#define UDPNTIMESLICE 16

// Timing values
#define LFREPOCH 1199145600 // 2008-01-01 Unix time
#define RSPMAXSEQ 195313
#define CLOCK200MHZ 195312.5
#define CLOCK160MHZ 156250.0

// Execution values
#define OMP_NUM_THREADS OMP_THREADS
#define OMP_NESTED TRUE
#define ZSTD_BUFFERMUL 4096
#define ALIGNMENT_LENGTH 1024
#endif



// Slow stpping macro (enable from makefile)
#ifndef __LOFAR_SLEEP
#define __LOFAR_SLEEP

#ifdef __SLOWDOWN
#include <unistd.h>
#define PAUSE sleep(1);
#else
#define PAUSE while(0) {};
#endif

#endif



// Add extra verbosity at compile time
#ifndef __LOFAR_UDP_VERBOSE_MACRO
#define __LOFAR_UDP_VERBOSE_MACRO

#ifdef ALLOW_VERBOSE
#define VERBOSE(MSG) MSG; PAUSE;
#else
#define VERBOSE(MSG) while(0) {};
#endif

#endif



// Timing macro
#ifndef __LOFAR_UDP_TICKTOCK_MACRO
#define __LOFAR_UDP_TICKTOCK_MACRO
#define CLICK(clock) clock_gettime(CLOCK_MONOTONIC_RAW, &clock);
#define TICKTOCK(tick, tock) ((double) (tock.tv_nsec - tick.tv_nsec) / 1000000000.0) + (tock.tv_sec - tick.tv_sec)
#endif


// Raw data stored as chars, offer methods to read as other value types as required
#ifndef __LOFAR_UNION_TYPES
#define __LOFAR_UNION_TYPES
typedef union char_unsigned_int {
    unsigned int ui;
    char c[4];
} char_unsigned_int;

typedef union char_short {
    short s;
    char c[2];
} char_short;
#endif



// Decode the 'two source' bytes in the CEP header
#ifndef __LOFAR_SOURCE_BYTE_STRUCT
#define __LOFAR_SOURCE_BYTE_STRUCT
// For unpacking the header source bytes
typedef struct __attribute__((__packed__)) lofar_source_bytes {
	unsigned int rsp : 5;
	unsigned int padding0 : 1;
	unsigned int errorBit : 1;
	unsigned int clockBit : 1;
	unsigned int bitMode  : 2;
	unsigned int padding1 : 6;

} lofar_source_bytes;
#endif



// Metadata struct
#ifndef __LOFAR_UDP_META_STRUCT
#define __LOFAR_UDP_META_STRUCT
typedef struct lofar_udp_meta lofar_udp_meta;
typedef int (*lofar_udp_processing_function )(lofar_udp_meta*);

typedef struct lofar_udp_meta {
	// Input/Output data storage
	char *inputData[MAX_NUM_PORTS];
	char *outputData[MAX_OUTPUT_DIMS];
	long inputDataOffset[MAX_NUM_PORTS]; // Account for data shifts

	// Checks for data quality (reset on steps)
	int inputDataReady;
	int outputDataReady;


	// Track the packets, logging the beamlet counts and their metadata
	int portBeamlets[MAX_NUM_PORTS];
	int portCumulativeBeamlets[MAX_NUM_PORTS];
	int totalBeamlets;

	int inputBitMode;
	int portPacketLength[MAX_NUM_PORTS];


	// Track the output metatdata
	int numOutputs;
	int outputBitMode;
	int packetOutputLength[MAX_OUTPUT_DIMS];


	// Track the number of ports to process and the packet loss on each
	int numPorts;
	int portLastDroppedPackets[MAX_NUM_PORTS];
	int portTotalDroppedPackets[MAX_NUM_PORTS];

	// Configuration: replay last packet or copy a 0 packed file, set the processing mode and it's related processing function
	int replayDroppedPackets;
	int processingMode;
	lofar_udp_processing_function processFunc;

	// Overall runtime information
	long packetsPerIteration;
	long packetsRead;
	long packetsReadMax;
	long leadingPacket;
	long lastPacket;

	// Configuration: verbosity of processing
	#ifdef ALLOW_VERBOSE
	int VERBOSE;
	#endif

} lofar_udp_meta;
#endif



// File data + decompression struct
#ifndef __LOFAR_UDP_READER_STRUCTS
#define __LOFAR_UDP_READER_STRUCTS
typedef struct lofar_udp_reader {
	FILE* fileRef[MAX_NUM_PORTS];

	int compressedReader;

	// Setup ZSTD requirements
	ZSTD_DStream* dstream[MAX_NUM_PORTS];
	ZSTD_inBuffer readingTracker[MAX_NUM_PORTS];
	ZSTD_outBuffer decompressionTracker[MAX_NUM_PORTS];

	// ZSTD Raw data input buffer
	char* inBuffer[MAX_NUM_PORTS];

	// Cache the constant length for the arrays malloc'd by the reader, will be used to reset meta
	long packetsPerIteration;

	// Metadata / data struct
	lofar_udp_meta* meta;

} lofar_udp_reader;
#endif




// Function Prototypes
#ifndef __LOFAR_UDP_READER_H
#define __LOFAR_UDP_READER_H

// Allow C++ imports too
#ifdef __cplusplus
extern "C" {
#endif 

// Reader/meta struct initialisation
lofar_udp_reader* lofar_udp_meta_file_reader_setup(FILE **inputFiles, const int numPorts, const int replayDroppedPackets, const int processingMode, const int verbose, const long packetsPerIteration, const long startingPacket, const long packetsReadMax, const int compressedReader);
lofar_udp_reader* lofar_udp_file_reader_setup(FILE **inputFiles, lofar_udp_meta *meta, const int compressedReader);
int lofar_udp_file_reader_reuse(lofar_udp_reader *reader, const long startingPacket, const long packetsReadMax);

// Initialisation helpers
int lofar_udp_parse_headers(lofar_udp_meta *meta, const char header[MAX_NUM_PORTS][UDPHDRLEN]);
int lofar_udp_setup_processing(lofar_udp_meta *meta);
int lofar_udp_get_first_packet_alignment(lofar_udp_reader *reader);
int lofar_udp_get_first_packet_alignment_meta(lofar_udp_meta *meta);

// Raw input data haandlers
int lofar_udp_reader_step(lofar_udp_reader *reader);
int lofar_udp_reader_step_timed(lofar_udp_reader *reader, double timing[2]);
int lofar_udp_reader_read_step(lofar_udp_reader *reader);
int lofar_udp_shift_remainder_packets(lofar_udp_reader *reader, const int shiftPackets[], const int handlePadding);
long lofar_udp_reader_nchars(lofar_udp_reader *reader, const int port, char *targetArray, const long nchars, const long knownOffset);
//int lofar_udp_realign_data(lofar_udp_reader *reader);


// Reader struct cleanup
int lofar_udp_reader_cleanup(const lofar_udp_reader *reader);




// Maybe move this to misc?
int fread_temp_ZSTD(void *outbuf, const size_t size, int num, FILE* inputFile, const int resetSeek);


#ifdef __cplusplus
}
#endif
#endif