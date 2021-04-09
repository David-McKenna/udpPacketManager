#ifndef LOFAR_UDP_STRUCTS_H
#define LOFAR_UDP_STRUCTS_H

#include "lofar_udp_general.h"

// Input includes
// Unlock advanced ZSTD features
#define ZSTD_STATIC_LINKING_ONLY

#include <zstd.h>
#include <stdio.h>

// PSRDADA include may not be available
#ifndef DADA_INCLUDES
#define DADA_INCLUDES

#ifndef NODADA

#include <dada_hdu.h>
#include <ipcio.h>


#define DADA_DEFAULT_HEADER_SIZE 8192
#else
typedef struct dada_hdu_t dada_hdu_t;
typedef struct multilog_t multilog_t;
typedef struct icpio_t ipcio_t;
#endif

#endif

typedef struct lofar_udp_calibration {
	// The current calibration step we are on and the amount that have been generated
	int calibrationStepsGenerated;

	// Location to generate the FIFO pipe to communicate with dreamBeam
	char calibrationFifo[DEF_STR_LEN];

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
extern const lofar_udp_calibration lofar_udp_calibration_default;

typedef struct lofar_udp_reader_input {
	reader_t readerType;
	// Raw file pointers
	FILE *fileRef[MAX_NUM_PORTS];

	// ZSTD requirements
	ZSTD_DStream *dstream[MAX_NUM_PORTS];
	ZSTD_inBuffer readingTracker[MAX_NUM_PORTS];
	ZSTD_outBuffer decompressionTracker[MAX_NUM_PORTS];

	// PSRDADA keys, buffers
	int dadaKey[MAX_NUM_PORTS];
	multilog_t *multilog[MAX_NUM_PORTS];
	dada_hdu_t *dadaReader[MAX_NUM_PORTS];
	long dadaPageSize[MAX_NUM_PORTS];

} lofar_udp_reader_input;
extern const lofar_udp_reader_input lofar_udp_reader_input_default;

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


	// Track the output metadata
	int numOutputs;
	int outputBitMode;
	int packetOutputLength[MAX_OUTPUT_DIMS];


	// Track the number of ports to process and the packet loss on each
	int numPorts;
	long portLastDroppedPackets[MAX_NUM_PORTS];
	long portTotalDroppedPackets[MAX_NUM_PORTS];

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
extern const lofar_udp_meta lofar_udp_meta_default;

// File data + decompression struct
typedef struct lofar_udp_reader {
	// Data sources struct
	lofar_udp_reader_input *input;

	// Metadata / input data array struct
	lofar_udp_meta *meta;

	// Calibration configuration struct
	lofar_udp_calibration *calibration;

	// Number of OpenMP Threads to use
	int ompThreads;

	// Cache the constant length for the arrays malloc'd by the reader, will be used to reset meta
	long packetsPerIteration;

} lofar_udp_reader;
extern const lofar_udp_reader lofar_udp_reader_default;

// Configuration struct
typedef struct lofar_udp_config {
	// Points to input files, compressed or uncompressed
	char inputLocations[MAX_NUM_PORTS][DEF_STR_LEN];

	// Number of ports of raw data being provided in inputFiles
	int numPorts;

	// Configure whether to path with 0's (0) or replay last packet (1) when we
	// encounter a dropped/missed packet
	int replayDroppedPackets;

	// Processing mode, see documentation
	int processingMode;

	// Enable verbose mode when compile with -DALLOW_VERBOSE
	int verbose;

	// Number of packets to process per iteration
	long packetsPerIteration;

	// Index of the starting packet
	long startingPacket;

	// Index of the last packet to process
	long packetsReadMax;

	// Define the input type (see reader_t enums)
	int readerType;

	// Lower / Upper limits of beamlets to process
	int beamletLimits[2];

	// Enable / disable dreamBeam polarmetric corrections
	int calibrateData;

	// Configure calibration parameters
	lofar_udp_calibration *calibrationConfiguration;

	// Number of OMP threads to use while processing
	int ompThreads;

	// Input PSRDADA ringbuffer keys
	int dadaKeys[MAX_NUM_PORTS];

} lofar_udp_config;
extern const lofar_udp_config lofar_udp_config_default;

// Output wrapper struct
typedef struct lofar_udp_io_write_config {
	reader_t readerType;

	int appendExisting;

	char outputFormat[DEF_STR_LEN];
	char outputLocations[MAX_OUTPUT_DIMS][DEF_STR_LEN];
	int baseVal;
	int offsetVal;

	FILE *outputFiles[MAX_OUTPUT_DIMS];
	// ZSTD requirements
	ZSTD_CStream *cstream[MAX_OUTPUT_DIMS];
	ZSTD_CCtx_params *cparams;
	ZSTD_outBuffer outputBuffer[MAX_OUTPUT_DIMS];

	// PSRDADA keys, buffers
	int outputDadaKeys[MAX_OUTPUT_DIMS];
	ipcio_t *dadaWriter[MAX_OUTPUT_DIMS][2];

} lofar_udp_io_write_config;
extern const lofar_udp_io_write_config lofar_udp_io_write_config_default;

#endif
