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



#ifndef __LOFAR_UDP_VERBOSE_MACRO
#define __LOFAR_UDP_VERBOSE_MACRO

#ifdef ALLOW_VERBOSE
#include <stdio.h>
#define VERBOSE(MSG) MSG;
#else
#define VERBOSE(MSG) while(0) {};
#endif

#endif

#ifndef __LOFAR_SAMPLING_CONSTS
#define __LOFAR_SAMPLING_CONSTS
extern const float clock200MHzSteps;
extern const float clock160MHzSteps;
extern const float clockStepsDelta;

// Time steps per second in each clock mode
extern const double clock200MHzSample;
extern const double clock160MHzSample;
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



#ifndef __LOFAR_UDP_MISC_H
#define __LOFAR_UDP_MISC_H
#ifdef __cplusplus
extern "C" {
#endif
//long lowestBufferMultiple(int zstdBuffSize, long targetBufferSize);
//long maximumBuffer(int zstdBuffSize, int udpPacketSize, long maximumBufferSize);

long beamformed_packno(unsigned int timestamp, unsigned int sequence, unsigned int clock200MHz);
long lofar_get_packet_number(char *inputData);
unsigned int lofar_get_next_packet_sequence(char *inputData);
double lofar_get_packet_time(char *inputData);
double lofar_get_packet_time_mjd(char *inputData);

#ifdef __cplusplus
}
#endif
#endif