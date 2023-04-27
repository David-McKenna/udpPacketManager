#ifndef LOFAR_UDP_STRUCTS_H
#define LOFAR_UDP_STRUCTS_H

#include "lofar_udp_structs_metadata.h"

// Input includes
// Unlock advanced/experimental ZSTD features
#define ZSTD_STATIC_LINKING_ONLY 1

#include <stdio.h>
#include <zstd.h>
#include <hdf5.h>

#ifdef NODADA
// key_t include
#define  _XOPEN_SOURCE
#include <sys/types.h>
#endif // End of NODADA

// PSRDADA include may not be available
#ifndef DADA_INCLUDES
#define DADA_INCLUDES

#include <dada_hdu.h>
#include <ipcio.h>

// Expand DADA header default since we'll likely store the beamctl command in the output header
#define DADA_DEFAULT_HEADER_SIZE 16384

#endif // End of DADA_INCLUDES


// Calibration configuration, mostly superseded, fold into config/reader?
typedef struct lofar_udp_calibration {
	// The current calibration step we are on and the amount that have been generated
	int32_t calibrationStepsGenerated;

	// Estimated duration of observation (used to determine number of steps to calibrate)
	float calibrationDuration;

} lofar_udp_calibration;
extern const lofar_udp_calibration lofar_udp_calibration_default;

typedef struct lofar_udp_io_read_config {
	// Reader configuration, these must be set prior to calling read_setup
	reader_t readerType;
	int64_t readBufSize[MAX_NUM_PORTS];
	int32_t portPacketLength[MAX_NUM_PORTS];
	int8_t numInputs;

	// Reader requires space before the buffer, note for any reallocs
	int32_t preBufferSpace[MAX_NUM_PORTS];

	// Inputs post-formatting
	char inputLocations[MAX_NUM_PORTS][DEF_STR_LEN + 1];
	key_t inputDadaKeys[MAX_NUM_PORTS];
	int32_t basePort;
	int16_t offsetPortCount;
	int16_t stepSizePort;

	// Main reading objects
	FILE *fileRef[MAX_NUM_PORTS];
	ZSTD_DStream *dstream[MAX_NUM_PORTS];
	dada_hdu_t *dadaReader[MAX_NUM_PORTS];

	// ZSTD requirements
	ZSTD_inBuffer readingTracker[MAX_NUM_PORTS];
	ZSTD_outBuffer decompressionTracker[MAX_NUM_PORTS];
	int64_t zstdLastRead[MAX_NUM_PORTS];

	// PSRDADA requirements
	multilog_t *multilog[MAX_NUM_PORTS];
	int64_t dadaPageSize[MAX_NUM_PORTS];

} lofar_udp_io_read_config;
extern const lofar_udp_io_read_config lofar_udp_io_read_config_default;

// Metadata struct
typedef struct lofar_udp_obs_meta {
	// Input/Output data storage
	int8_t *inputData[MAX_NUM_PORTS];
	int8_t *outputData[MAX_OUTPUT_DIMS];
	int64_t inputDataOffset[MAX_NUM_PORTS]; // Account for data shifts

	// Checks for data quality (reset on steps)
	int8_t inputDataReady;
	int8_t outputDataReady;


	// Track the packets, logging the beamlet counts and their metadata
	int16_t portRawBeamlets[MAX_NUM_PORTS];
	int16_t portRawCumulativeBeamlets[MAX_NUM_PORTS];
	int16_t totalRawBeamlets;

	// Tracking beamlets with respect to the processing strategy
	int16_t baseBeamlets[MAX_NUM_PORTS];
	int16_t upperBeamlets[MAX_NUM_PORTS];
	int16_t portCumulativeBeamlets[MAX_NUM_PORTS];
	int16_t totalProcBeamlets;

	// Input characteristics
	int8_t inputBitMode;
	int16_t portPacketLength[MAX_NUM_PORTS];
	int8_t clockBit;

	// Calibration data
	calibrate_t calibrateData;
	int32_t calibrationStep;
	float **jonesMatrices;


	// Track the output metadata
	int8_t numOutputs;
	int8_t outputBitMode;
	int32_t packetOutputLength[MAX_OUTPUT_DIMS];


	// Track the number of ports to process and the packet loss on each
	int8_t numPorts;
	int64_t portLastDroppedPackets[MAX_NUM_PORTS];
	int64_t portTotalDroppedPackets[MAX_NUM_PORTS];

	// Configuration: replay last packet or copy a 0 packed file, set the processing mode, and it's related processing function
	int8_t replayDroppedPackets;
	processMode_t processingMode;
	dataOrder_t dataOrder;

	// Overall runtime information
	int64_t packetsPerIteration;
	int64_t packetsRead;
	int64_t packetsReadMax;
	int64_t leadingPacket;
	int64_t lastPacket;

	// Other metadata
	int32_t stationID;

	// Configuration: verbosity of processing
#ifdef ALLOW_VERBOSE
	int32_t VERBOSE;
#endif

} lofar_udp_obs_meta;
extern const lofar_udp_obs_meta lofar_udp_obs_meta_default;

// Main operations struct
typedef struct lofar_udp_reader {
	// Data sources struct
	lofar_udp_io_read_config *input;

	// Input data metadata
	lofar_udp_obs_meta *meta;

	// Observation metadata
	lofar_udp_metadata *metadata;

	// Calibration configuration struct
	lofar_udp_calibration *calibration;

	// Number of OpenMP Threads to use
	int32_t ompThreads;

	// Cache the constant length for the arrays malloc'd by the reader, will be used to reset meta
	int64_t packetsPerIteration;

} lofar_udp_reader;
extern const lofar_udp_reader lofar_udp_reader_default;

typedef struct metadata_config {
	// Define the output metadata type (see metadata_t enums)
	metadata_t metadataType;

	// Points to a file containing metadata that we can parse
	char metadataLocation[DEF_STR_LEN + 1];

	// External modifiers to final output data
	int32_t externalChannelisation;
	int32_t externalDownsampling;
} metadata_config;
extern const metadata_config metadata_config_default;

// Configuration struct
typedef struct lofar_udp_config {

	// Define the input type (see reader_t enums)
	reader_t readerType;

	// Points to input files, compressed or uncompressed
	char inputLocations[MAX_NUM_PORTS][DEF_STR_LEN + 1];

	// Input PSRDADA ringbuffer keys
	key_t inputDadaKeys[MAX_NUM_PORTS];

	struct metadata_config metadata_config;

	// Number of valid ports of raw data being provided in inputLocations / dadaKeys
	//
	// basePort - the base port number, i.e. 0 or 16130
	// offsetPortCount - the number of ports away from the base number, [0-3]
	// stepSizePort - the number to add to basePort for each port (any non-zero number)
	// numPorts - the number of ports to process, [1, 4 - offsetPortCount]
	// basePort must ALWAYS be the absolute base value if you want to parse metadata, i.e.
	//  if you want to parse just ports 2 and 3, set the baseVal to 0, offsetPortCount to 2
	//  and numPorts to 2. Otherwise, we can't parse the beamctl command correctly.
	//
	// These can all be set by using the io_read_parse_optarg function
	int32_t basePort;
	int16_t offsetPortCount;
	int16_t stepSizePort;
	int8_t numPorts;

	// Processing mode, see the documentation
	int32_t processingMode;

	// Number of packets to process per iteration
	int64_t packetsPerIteration;

	// Packet number of the starting packet
	int64_t startingPacket;

	// Packet number / offset from base of the last packet to process
	int64_t packetsReadMax;

	// Configure whether to path with 0's (0) or replay last packet (1) when we
	// encounter a dropped/missed packet
	int8_t replayDroppedPackets;

	// Lower / Upper limits of beamlets to process [0, numPorts * (beamlets per bitmode) + 1]
	// This input is exclusive on the upper limit, not inclusive like LOFAR inputs
	// E.g., to access beamlets 3 - 9, specify { 3, 10 }
	int16_t beamletLimits[2];

	// Enable / disable dreamBeam polarmetric corrections
	calibrate_t calibrateData;
	float calibrationDuration;

	// Number of OpenMP threads to use while processing data
	// Minimum advised is 2 times the number of ports being processed, so typically 8.
	int32_t ompThreads;

	// Enable verbose mode when the library is compiled with -DALLOW_VERBOSE
	int32_t verbose;

} lofar_udp_config;
extern const lofar_udp_config lofar_udp_config_default;

// Output wrapper struct
typedef struct lofar_udp_io_write_config {
	// Writer configuration, these must be set prior to calling write_setup
	reader_t readerType;
	lofar_udp_metadata *metadata;
	int64_t writeBufSize[MAX_OUTPUT_DIMS];
	int8_t progressWithExisting;
	int8_t numOutputs;

	// Outputs pre- and post-formatting
	char outputFormat[DEF_STR_LEN + 1];
	char outputLocations[MAX_OUTPUT_DIMS][DEF_STR_LEN + 1];
	key_t outputDadaKeys[MAX_OUTPUT_DIMS];
	int32_t baseVal;
	int16_t stepSize;
	int64_t firstPacket;

	// Main writer objects
	FILE *outputFiles[MAX_OUTPUT_DIMS];
	struct {
		ZSTD_CStream *cstream;
		ZSTD_outBuffer compressionBuffer;
	} zstdWriter[MAX_OUTPUT_DIMS];
	struct {
		dada_hdu_t *hdu;
		multilog_t *multilog;
	} dadaWriter[MAX_OUTPUT_DIMS];
	struct {
		int8_t initialised;
		int8_t metadataInitialised;
		hid_t file;
		hid_t dtype;
		size_t elementSize;
		struct {
			hid_t dset;
			hsize_t dims[2];
		} hdf5DSetWriter[MAX_OUTPUT_DIMS];
	} hdf5Writer;


	struct {
		int32_t compressionLevel;
		int32_t numThreads;
		int8_t enableSeek; // Not implemented
	} zstdConfig;
	struct {
		uint64_t nbufs;
		uint64_t header_size;
		uint32_t num_readers;
		char syslog;
		char programName[64];
		float cleanup_timeout;
	} dadaConfig;


	// ZSTD requirements
	ZSTD_CCtx_params *cparams;

	// Channel count modified
	int32_t externalChannelisation;


} lofar_udp_io_write_config;
extern const lofar_udp_io_write_config lofar_udp_io_write_config_default;



#ifdef __cplusplus
extern "C" {
#endif

// Helper functions to allocate and initialised structs
// External
lofar_udp_config *lofar_udp_config_alloc();
void lofar_udp_config_cleanup(lofar_udp_config *config);
lofar_udp_io_read_config *lofar_udp_io_read_alloc();
lofar_udp_io_write_config *lofar_udp_io_write_alloc();

// Internal
lofar_udp_calibration *_lofar_udp_calibration_alloc();
lofar_udp_obs_meta *_lofar_udp_obs_meta_alloc();
lofar_udp_reader *_lofar_udp_reader_alloc(lofar_udp_obs_meta *meta); // Reminder that reader is always built AFTER meta parsing



#ifdef __cplusplus
}
#endif

#endif // End of LOFAR_UDP_STRUCTS_H


/**
 * Copyright (C) 2023 David McKenna
 * This file is part of udpPacketManager <https://github.com/David-McKenna/udpPacketManager>.
 *
 * udpPacketManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * udpPacketManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with udpPacketManager.  If not, see <http://www.gnu.org/licenses/>.
 **/
