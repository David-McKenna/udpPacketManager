#ifndef __LOFAR_COMPILE_CONSTANTS
#define __LOFAR_COMPILE_CONSTANTS

// I/O array sizes
#define MAX_NUM_PORTS 4
#define MAX_OUTPUT_DIMS 4

// CEP packet reference values
#define UDPHDRLEN 16
#define UDPHDROFF 0
#define UDPCURVER 3
#define UDPMAXBEAM 244
#define UDPNPOL 4
#define UDPNTIMESLICE 16

// Timing values
#define LFREPOCH 1199145600 // 2008-01-01 Unix time, sanity check
#define RSPMAXSEQ 195313
#define CLOCK200MHZ 195312.5
#define CLOCK160MHZ 156250.0

// Execution values
#define OMP_NUM_THREADS OMP_THREADS
#define OMP_NESTED TRUE
#define ZSTD_BUFFERMUL 4096
#define ALIGNMENT_LENGTH 1024



#define CEP_HDR_RSP_VER_OFFSET 0
#define CEP_HDR_SRC_OFFSET 1
#define CEP_HDR_STN_ID_OFFSET 4
#define CEP_HDR_NBEAM_OFFSET 6
#define CEP_HDR_NTIMESLICE_OFFSET 7
#define CEP_HDR_TIME_OFFSET 8
#define CEP_HDR_SEQ_OFFSET 12

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



#ifndef __LOFAR_UDP_VERBOSE_MACRO
#define __LOFAR_UDP_VERBOSE_MACRO

#ifdef ALLOW_VERBOSE
#define VERBOSE(MSG) MSG;
#define VERBOSEP(...) printf("%s: %s", __func__, ##__VA_ARGS__);
#else
#define VERBOSE(MSG) while(0) {};
#define VERBOSEP(...) while(0) {};
#endif

#endif



// Slow stopping macro (enable from makefile)
#ifndef __LOFAR_SLEEP
#define __LOFAR_SLEEP

#ifdef __SLOWDOWN
#define PAUSE sleep(1);
#else
#define PAUSE while(0) {};
#endif

#endif


// Timing macro
#ifndef __LOFAR_UDP_TICKTOCK_MACRO
#define __LOFAR_UDP_TICKTOCK_MACRO
#define CLICK(clock) clock_gettime(CLOCK_MONOTONIC_RAW, &clock);
#define TICKTOCK(tick, tock) ((double) (tock.tv_nsec - tick.tv_nsec) / 1000000000.0) + (tock.tv_sec - tick.tv_sec)
#endif
