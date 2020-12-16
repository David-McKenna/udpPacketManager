// Standard required includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zstd.h>
#include <omp.h>
#include <limits.h>
#include <time.h>
#include <malloc.h>
#include <errno.h>
// Calibration requirements
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include "lofar_udp_general.h"

#ifndef __LOFAR_UDP_READER_STRUCTS
#define __LOFAR_UDP_READER_STRUCTS

typedef struct lofar_udp_calibration {
	// The current calibration step we are on and the amount that have been generated
	int calibrationStepsGenerated;

	// Location to generate the FIFO pipe to communicate with dreamBeam
	char *calibrationFifo;

	// Calibration strategy to use with dreamBeam (see documentation)
	// Maximum length: both antenna set (2x 5), all 3-digit subbands (4 ea), in 4-bit mode (976 subbands)
	// ~= 10 + 3904 = 3914
	char calibrationSubbands[4096];

	// Estimated duration of observation (used to determine number of steps to calibrate)
	float calibrationDuration;

	// Station pointing for calibration, Ra/Dec-like, coordinate system
	float calibrationPointing[2];
	char calibrationPointingBasis[128];

} lofar_udp_calibration;
extern lofar_udp_calibration lofar_udp_calibration_default;


// Metadata struct
typedef struct lofar_udp_meta {
	// Input/Output data storage
	char *inputData[MAX_NUM_PORTS];
	char *outputData[MAX_OUTPUT_DIMS];
	long inputDataOffset[MAX_NUM_PORTS]; // Account for data shifts

	// Checks for data quality (reset on steps)
	int inputDataReady;
	int outputDataReady;


	// Track the packets, logging the beamlet counts and their metadata
	int portRawBeamlets[MAX_NUM_PORTS];
	int portRawCumulativeBeamlets[MAX_NUM_PORTS];
	int totalRawBeamlets;

	// Tracking beamlets with respect to the processing strategy
	int baseBeamlets[MAX_NUM_PORTS];
	int upperBeamlets[MAX_NUM_PORTS];
	int portCumulativeBeamlets[MAX_NUM_PORTS];
	int totalProcBeamlets;

	// Input characteristics
	int inputBitMode;
	int portPacketLength[MAX_NUM_PORTS];
	int clockBit;

	// Calibration data
	int calibrateData;
	int calibrationStep;
	float **jonesMatrices;


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

	// Overall runtime information
	long packetsPerIteration;
	long packetsRead;
	long packetsReadMax;
	long leadingPacket;
	long lastPacket;

	// Other metadata
	int stationID;

	// Configuration: verbosity of processing
	#ifdef ALLOW_VERBOSE
	int VERBOSE;
	#endif

} lofar_udp_meta;
extern lofar_udp_meta lofar_udp_meta_default;


// File data + decompression struct
typedef struct lofar_udp_reader {
	FILE *fileRef[MAX_NUM_PORTS];

	int compressedReader;

	// Setup ZSTD requirements
	ZSTD_DStream *dstream[MAX_NUM_PORTS];
	ZSTD_inBuffer readingTracker[MAX_NUM_PORTS];
	ZSTD_outBuffer decompressionTracker[MAX_NUM_PORTS];

	// ZSTD Raw data input buffer
	char *inBuffer[MAX_NUM_PORTS];

	// Cache the constant length for the arrays malloc'd by the reader, will be used to reset meta
	long packetsPerIteration;

	// Metadata / data struct
	lofar_udp_meta *meta;

		// Calibration configuration struct
	lofar_udp_calibration *calibration;

} lofar_udp_reader;
extern lofar_udp_reader lofar_udp_reader_default;

// Confugration struct
typedef struct lofar_udp_config {
	// Points to input files, compressed or uncompressed
	FILE **inputFiles;

	// Number of ports of raw data being provided in inputFIles
	int numPorts;

	// Configure whether to path with 0's (0) or replay last packet (1) when we
	// encounter a dropped/missed packet
	int replayDroppedPackets;

	// Porcessing mode, see documentation
	int processingMode;

	// Enable verbose mode when ccompile with -DALLOW_VERBOSE
	int verbose;

	// Number of packets to process per iteration
	long packetsPerIteration;

	// Index of the starting packet
	long startingPacket;

	// Index of the last packet to process
	long packetsReadMax;

	// Whether or not inputFiles are compressed files or raw packet captures
	int compressedReader;

	// Lower / Upper limits of beamlets to process
	int beamletLimits[2];

	// Enable / disable dreamBeam polarmetric corrections
	int calibrateData;

	// Configure calibration parameters
	lofar_udp_calibration *calibrationConfiguration;

} lofar_udp_config;
extern lofar_udp_config lofar_udp_config_default;
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
lofar_udp_reader* lofar_udp_meta_file_reader_setup_struct(lofar_udp_config *config);
lofar_udp_reader* lofar_udp_file_reader_setup(FILE **inputFiles, lofar_udp_meta *meta, const int compressedReader, lofar_udp_calibration *calibration);
int lofar_udp_file_reader_reuse(lofar_udp_reader *reader, const long startingPacket, const long packetsReadMax);

// Initialisation helpers
int lofar_udp_parse_headers(lofar_udp_meta *meta, char header[MAX_NUM_PORTS][UDPHDRLEN], const int beamletLimits[2]);
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
int lofar_udp_reader_cleanup(lofar_udp_reader *reader);
int lofar_udp_reader_cleanup_f(lofar_udp_reader *reader, const int closeFiles);



// Maybe move this to misc?
int fread_temp_ZSTD(void *outbuf, const size_t size, int num, FILE* inputFile, const int resetSeek);


#ifdef __cplusplus
}
#endif
#endif