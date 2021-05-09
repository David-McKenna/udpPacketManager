#ifndef LOFAR_UDP_STRUCTS_H
#define LOFAR_UDP_STRUCTS_H

#include "lofar_udp_structs_metadata.h"

// Input includes
// Unlock advanced ZSTD features
#define ZSTD_STATIC_LINKING_ONLY 1

#include <zstd.h>
#include <stdio.h>

// PSRDADA include may not be available
#ifndef DADA_INCLUDES
#define DADA_INCLUDES

#include <dada_hdu.h>
#include <ipcio.h>

// Expand DADA header default since we'll likely store the beamctl command in the output header
#define DADA_DEFAULT_HEADER_SIZE 16384

#endif // End of DADA_INCLUDES

typedef struct lofar_udp_calibration {
	// The current calibration step we are on and the amount that have been generated
	int calibrationStepsGenerated;

	// Location to generate the FIFO pipe to communicate with dreamBeam
	char calibrationFifo[DEF_STR_LEN + 1];

	// Calibration strategy to use with dreamBeam (see documentation)
	// Maximum length: both antenna set (2x 5), all 3-digit subbands (4 ea), in 4-bit mode (976 subbands)
	// ~= 10 + 3904 = 3914
	char calibrationSubbands[2 * DEF_STR_LEN + 1];

	// Estimated duration of observation (used to determine number of steps to calibrate)
	float calibrationDuration;

	// Station pointing for calibration, Ra/Dec-like, coordinate system
	float calibrationPointing[2];
	char calibrationPointingBasis[128];

} lofar_udp_calibration;
extern const lofar_udp_calibration lofar_udp_calibration_default;

typedef struct lofar_udp_io_read_config {
	// Reader configuration, these must be set prior to calling read_setup
	reader_t readerType;
	long readBufSize[MAX_NUM_PORTS];
	int portPacketLength[MAX_NUM_PORTS];
	int numInputs;

	// Inputs post-formatting
	char inputLocations[MAX_NUM_PORTS][DEF_STR_LEN + 1];
	int dadaKeys[MAX_NUM_PORTS];
	int basePort;
	int offsetPortCount;
	int stepSizePort;

	// Main reading objects
	FILE *fileRef[MAX_NUM_PORTS];
	ZSTD_DStream *dstream[MAX_NUM_PORTS];
	dada_hdu_t *dadaReader[MAX_NUM_PORTS];

	// ZSTD requirements
	ZSTD_inBuffer readingTracker[MAX_NUM_PORTS];
	ZSTD_outBuffer decompressionTracker[MAX_NUM_PORTS];

	// PSRDADA requirements
	multilog_t *multilog[MAX_NUM_PORTS];
	long dadaPageSize[MAX_NUM_PORTS];

} lofar_udp_io_read_config;
extern const lofar_udp_io_read_config lofar_udp_io_read_config_default;

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

} lofar_udp_input_meta;
extern const lofar_udp_input_meta lofar_udp_input_meta_default;

// File data + decompression struct
typedef struct lofar_udp_reader {
	// Data sources struct
	lofar_udp_io_read_config *input;

	// Input data metadata
	lofar_udp_input_meta *meta;

	// Observation metadata
	lofar_udp_metadata *metadata;

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

	// Define the input type (see reader_t enums)
	reader_t readerType;

	// Points to input files, compressed or uncompressed
	char inputLocations[MAX_NUM_PORTS][DEF_STR_LEN + 1];

	// Input PSRDADA ringbuffer keys
	int dadaKeys[MAX_NUM_PORTS];

	// Define the output metadata type (see metadata_t enums)
	metadata_t metadataType;
	// Points to a file containing metadata that we can parse
	char metadataLocation[DEF_STR_LEN + 1];

	// Number of valid ports of raw data being provided in inputLocations / dadaKeys
	//
	// basePort - the base port number, i.e. 0 or 16130
	// offsetPortCount - the number of ports away from the base number, [0-3]
	// stepSizePort - the number to add to basePort for each port (any non-zero number)
	// numPorts - the number of ports to process, [1, 4 - offsetPortCount]
	// basePort must ALWAYS be the absolute base value if you want to parse metadata, i.e.
	//  if you want to parse ust ports 2 and 3, set the baseVal to 0, offsetPortCount to 2
	//  and numPorts to 2. Otherwise we can't parse the beamctl command correctly.
	//
	// These can all be set by using the io_read_parse_optarg function
	int basePort;
	int offsetPortCount;
	int stepSizePort;
	int numPorts;

	// Processing mode, see the documentation
	int processingMode;

	// Number of packets to process per iteration
	long packetsPerIteration;

	// Packet number of the starting packet
	long startingPacket;

	// Packet number / offset from base of the last packet to process
	long packetsReadMax;

	// Configure whether to path with 0's (0) or replay last packet (1) when we
	// encounter a dropped/missed packet
	int replayDroppedPackets;

	// Lower / Upper limits of beamlets to process [0, numPorts * (beamlets per bitmode) + 1]
	// This input is exclusive on the upper limit, not inclusive like LOFAR inputs
	// E.g., to access beamlets 3 - 9, specify { 3, 10 }
	int beamletLimits[2];

	// Enable / disable dreamBeam polarmetric corrections
	int calibrateData;

	// Configure calibration parameters
	// May be overwritten if metadata is provided.
	lofar_udp_calibration *calibrationConfiguration;

	// Number of OpenMP threads to use while processing data
	// Minimum advised is 2 times the number of ports being processed, so typically 8.
	int ompThreads;

	// Enable verbose mode when the library is compiled with -DALLOW_VERBOSE
	int verbose;

} lofar_udp_config;
extern const lofar_udp_config lofar_udp_config_default;

// Output wrapper struct
typedef struct lofar_udp_io_write_config {
	// Writer configuration, these must be set prior to calling write_setup
	reader_t readerType;
	lofar_udp_metadata *metadata;
	long writeBufSize[MAX_OUTPUT_DIMS];
	int appendExisting;
	int numOutputs;

	// Outputs pre- and post-formatting
	char outputFormat[DEF_STR_LEN + 1];
	char outputLocations[MAX_OUTPUT_DIMS][DEF_STR_LEN + 1];
	int outputDadaKeys[MAX_OUTPUT_DIMS];
	int baseVal;
	int stepSize;
	long firstPacket;

	// Main writer objects
	FILE *outputFiles[MAX_OUTPUT_DIMS];
	struct {
		ZSTD_CStream *cstream;
		ZSTD_outBuffer compressionBuffer;
	} zstdWriter[MAX_OUTPUT_DIMS];
	struct {
		ipcio_t *ringbuffer;
		ipcio_t *header;
		multilog_t *multilog;
	} dadaWriter[MAX_OUTPUT_DIMS];


	struct {
		int compressionLevel;

	} zstdConfig;
	struct {
		uint64_t nbufs;
		uint64_t header_size;
		unsigned int num_readers;
		char syslog;
		char programName[64];
		float cleanup_timeout;
	} dadaConfig;


	// ZSTD requirements
	ZSTD_CCtx_params *cparams;

	// PSRDADA requirements
	int enableMultilog;


} lofar_udp_io_write_config;
extern const lofar_udp_io_write_config lofar_udp_io_write_config_default;

#endif // End of LOFAR_UDP_STRUCTS_H
