#include "lofar_udp_structs.h"

// Merge in the metadata structs
#include "lofar_udp_structs_metadata.c" // NOLINT(bugprone-suspicious-include)

// Reader struct default
const lofar_udp_io_read_config lofar_udp_io_read_config_default = {
	.readerType = NO_ACTION,
	.readBufSize = { -1 }, // NEEDS FULL RUNTIME INITIALISATION
	.portPacketLength = { -1 }, // NEEDS FULL RUNTIME INITIALISATION
	.numInputs = 0,

	// Reader requires space before the buffer, note for any reallocs
	.preBufferSpace = { 0 }, // NEEDS FULL RUNTIME INITIALISATION
							 // Set to 0 to assume no use as default

	// Inputs pre- and post-formatting
	.inputLocations = { "" }, // NEEDS FULL RUNTIME INITIALISATION
	.inputDadaKeys = { -1 }, // NEEDS FULL RUNTIME INITIALISATION
	.basePort = 0,
	.offsetPortCount = 0,
	.stepSizePort = 1,

	.dstream = { NULL }, // NEEDS FULL RUNTIME INITIALISATION
	.dadaReader = { NULL }, // NEEDS FULL RUNTIME INITIALISATION

	// Associated objects
	.readingTracker = { { NULL, 0, 0 } }, // NEEDS FULL RUNTIME INITIALISATION
	.decompressionTracker = { { NULL, 0, 0 } }, // NEEDS FULL RUNTIME INITIALISATION
	.zstdLastRead = { 0 }, // NEEDS FULL RUNTIME INITIALISATION
	.multilog = { NULL }, // NEEDS FULL RUNTIME INITIALISATION
	.dadaPageSize = { -1 } // NEEDS FULL RUNTIME INITIALISATION
};

// Writer struct default
const lofar_udp_io_write_config lofar_udp_io_write_config_default = {
	// Control options
	.readerType = NO_ACTION,
	.writeBufSize = { -1 }, // NEEDS FULL RUNTIME INITIALISATION
	.progressWithExisting = 0,
	.numOutputs = 0,

	// Outputs pre- and post-formatting
	.outputFormat = "",
	.outputLocations = { "" },
	.outputDadaKeys = { -1, }, // NEEDS FULL RUNTIME INITIALISATION
	.baseVal = 0,
	.stepSize = 1,
	.firstPacket = 0,

	// Main writing objects
	.outputFiles = { NULL, },
	.zstdWriter = { { NULL, { NULL, 0, 0 } } },
	.dadaWriter = { { NULL, NULL } },
	.hdf5Writer = { 0,
	               .hdf5DSetWriter = {{ 0, { 0, 0 }}}
	},



	// zstd configuration
	.zstdConfig = {
		.compressionLevel = 3,
		.numThreads = 2,
		.enableSeek = -1, // Not implemented
	},

	// PSRDADA configuration
	.dadaConfig = {
		.nbufs = 1,
		.header_size = DADA_DEFAULT_HEADER_SIZE,
		.num_readers = 1,
		.syslog = 1,
		.programName = "udpPacketManager",
		.cleanup_timeout = 30.0f
	},

	// Misc options
	.cparams = NULL, // ZSTD configuration
	.externalChannelisation = 1,

};

// Calibration default
// While in here we default to enabling calibration, the overall default is read from
// the lofar_udp_config and overwrites the value in here.
const lofar_udp_calibration lofar_udp_calibration_default = {
	.calibrationStepsGenerated = 0,
	.calibrationDuration = 3600.0f,
};

// Configuration default
const lofar_udp_config lofar_udp_config_default = {
	.readerType = NO_ACTION,
	.inputLocations = { "" },
	.inputDadaKeys = { -1 }, // NEEDS FULL RUNTIME INITIALISATION
	.metadata_config = {
		// Initialised by alloc func
	},
	.numPorts = 4,
	.replayDroppedPackets = 0,
	.processingMode = UNSET_MODE,
	.verbose = 0,
	.packetsPerIteration = 65536,
	.startingPacket = -1,
	.packetsReadMax = LONG_MAX,
	.beamletLimits = { 0, 0 },
	.calibrateData = NO_CALIBRATION,
	.calibrationDuration = 3600.0f,
	.ompThreads = OMP_THREADS,


	.basePort = 0,
	.offsetPortCount = 0,
	.stepSizePort = 1
};

// Reader / meta with NULL-initialised values to help the cleanup function
const lofar_udp_reader lofar_udp_reader_default = {
	.input = NULL,
	.meta = NULL,
	.metadata = NULL,
	.calibration = NULL,
	.ompThreads = OMP_THREADS,
	.packetsPerIteration = -1
};


// meta with NULL-initialised values to help the cleanup function
const lofar_udp_obs_meta lofar_udp_obs_meta_default = {
	.inputData = { NULL },
	.outputData = { NULL },
	.packetsRead = 0,
	.processingMode = UNSET_MODE,
	.dataOrder = UNKNOWN,
	.inputDataReady = 0,
	.outputDataReady = 0,
	.jonesMatrices = NULL,
	.calibrationStep = 0
};

const metadata_config metadata_config_default = {
	.metadataType = NO_META,
	.metadataLocation = "",
	.externalChannelisation = 0,
	.externalDownsampling = 0
};


lofar_udp_obs_meta *_lofar_udp_obs_meta_alloc() {
	DEFAULT_STRUCT_ALLOC(lofar_udp_obs_meta, meta, lofar_udp_obs_meta_default, ;, NULL);

	ARR_INIT(meta->inputData, MAX_NUM_PORTS, NULL);
	ARR_INIT(meta->outputData, MAX_NUM_PORTS, NULL);
	ARR_INIT(meta->inputDataOffset, MAX_NUM_PORTS, 0);

	ARR_INIT(meta->portRawBeamlets, MAX_NUM_PORTS, -1);
	ARR_INIT(meta->portRawCumulativeBeamlets, MAX_NUM_PORTS, -1);

	ARR_INIT(meta->baseBeamlets, MAX_NUM_PORTS, 0);
	ARR_INIT(meta->upperBeamlets, MAX_NUM_PORTS, -1);
	ARR_INIT(meta->portCumulativeBeamlets, MAX_NUM_PORTS, -1);
	ARR_INIT(meta->portPacketLength, MAX_NUM_PORTS, -1);
	ARR_INIT(meta->packetOutputLength, MAX_NUM_PORTS, -1);
	ARR_INIT(meta->portLastDroppedPackets, MAX_NUM_PORTS, 0);
	ARR_INIT(meta->portTotalDroppedPackets, MAX_NUM_PORTS, 0);

	return meta;
}

/**
 * @brief Generate a fully allocated reader struct
 *
 * @return ptr: success, NULL: failure
 */
lofar_udp_reader* _lofar_udp_reader_alloc(lofar_udp_obs_meta *meta) {
	CHECK_ALLOC_NOCLEAN(meta, NULL);
	DEFAULT_STRUCT_ALLOC(lofar_udp_reader, reader, lofar_udp_reader_default, ;, NULL);
	reader->meta = meta;

	lofar_udp_io_read_config *input = lofar_udp_io_read_alloc();
	CHECK_ALLOC(input, NULL, free(reader););
	reader->input = input;

	return reader;
}

lofar_udp_calibration* _lofar_udp_calibration_alloc() {
	DEFAULT_STRUCT_ALLOC(lofar_udp_calibration, calibration, lofar_udp_calibration_default, ;, NULL);

	return calibration;
}

lofar_udp_config* lofar_udp_config_alloc() {
	DEFAULT_STRUCT_ALLOC(lofar_udp_config, config, lofar_udp_config_default, ;, NULL);
	STRUCT_COPY_INIT(metadata_config, &(config->metadata_config), metadata_config_default);

	STR_INIT(config->inputLocations, MAX_NUM_PORTS);
	ARR_INIT(config->inputDadaKeys, MAX_NUM_PORTS, -1);

	return config;
}

lofar_udp_io_read_config* lofar_udp_io_read_alloc() {
	DEFAULT_STRUCT_ALLOC(lofar_udp_io_read_config, input, lofar_udp_io_read_config_default, ;, NULL);

	ARR_INIT(input->readBufSize, MAX_NUM_PORTS, -1);
	ARR_INIT(input->portPacketLength, MAX_NUM_PORTS, -1);
	ARR_INIT(input->preBufferSpace, MAX_NUM_PORTS, 0); // Init as 0 default-value, only update on use
	STR_INIT(input->inputLocations, MAX_NUM_PORTS);
	ARR_INIT(input->inputDadaKeys, MAX_NUM_PORTS, -1);
	ARR_INIT(input->fileRef, MAX_NUM_PORTS, NULL);
	ARR_INIT(input->dstream, MAX_NUM_PORTS, NULL);
	ARR_INIT(input->dadaReader, MAX_NUM_PORTS, NULL);
	ARR_INIT(input->multilog, MAX_NUM_PORTS, NULL);
	ARR_INIT(input->zstdLastRead, MAX_NUM_PORTS, 0);
	ARR_INIT(input->dadaPageSize, MAX_NUM_PORTS, -1);

	for (int8_t port = 0; port < MAX_NUM_PORTS; port++) {
		input->readingTracker[port].src = NULL;
		input->readingTracker[port].size = 0;
		input->readingTracker[port].pos = 0;
		input->decompressionTracker[port].dst = NULL;
		input->decompressionTracker[port].size = 0;
		input->decompressionTracker[port].pos = 0;
	}

	return input;
}

lofar_udp_io_write_config* lofar_udp_io_write_alloc() {
	DEFAULT_STRUCT_ALLOC(lofar_udp_io_write_config, output, lofar_udp_io_write_config_default, ;, NULL);

	ARR_INIT(output->writeBufSize, MAX_OUTPUT_DIMS, -1);
	STR_INIT(output->outputLocations, MAX_OUTPUT_DIMS);
	ARR_INIT(output->outputDadaKeys, MAX_OUTPUT_DIMS, -1);
	ARR_INIT(output->outputFiles, MAX_OUTPUT_DIMS, NULL);

	for (int8_t outp = 0; outp < MAX_OUTPUT_DIMS; outp++) {
		output->zstdWriter[outp].cstream = NULL;
		output->zstdWriter[outp].compressionBuffer.dst = NULL;
		output->zstdWriter[outp].compressionBuffer.size = 0;
		output->zstdWriter[outp].compressionBuffer.pos = 0;

		output->dadaWriter[outp].hdu = NULL;
		output->dadaWriter[outp].multilog = NULL;

		output->hdf5Writer.hdf5DSetWriter[outp].dset = 0;
		output->hdf5Writer.hdf5DSetWriter[outp].dims[0] = -1;
		output->hdf5Writer.hdf5DSetWriter[outp].dims[1] = -1;
	}

	return output;
}

void lofar_udp_config_cleanup(lofar_udp_config *config) {
	FREE_NOT_NULL(config);
}

/**
 * @brief 			Check input header data for malformed variables
 *
 * @param header[in] 	16-byte CEP Header
 *
 * @return 0: success, other: failure
 */
int32_t _lofar_udp_malformed_header_checks(const int8_t header[16]) {
	lofar_source_bytes *source = (lofar_source_bytes *) &(header[CEP_HDR_SRC_OFFSET]);

	VERBOSE(
		printf("RSP: %d\n", source->rsp);
        printf("Padding0: %d\n", source->padding0);
        printf("ERRORBIT: %d\n", source->errorBit);
        printf("CLOCK: %d\n", source->clockBit);
        printf("bitMode: %d\n", source->bitMode);
        printf("padding1: %d\n\n", source->padding1);
	);

	if ((uint8_t) header[CEP_HDR_RSP_VER_OFFSET] < UDPCURVER) {
		fprintf(stderr, "Input header appears malformed (RSP Version less than 3, %d), exiting.\n", (int) header[CEP_HDR_RSP_VER_OFFSET]);
		return -1;
	}

	if (*((int32_t *) &(header[CEP_HDR_TIME_OFFSET])) < LFREPOCH) {
		fprintf(stderr, "Input header appears malformed (data timestamp before 2008, %u), exiting.\n", *((uint32_t *) &(header[CEP_HDR_TIME_OFFSET])));
		return -1;
	}

	if (*((int32_t *) &(header[CEP_HDR_SEQ_OFFSET])) > RSPMAXSEQ) {
		fprintf(stderr,
		        "Input header appears malformed (sequence higher than 200MHz clock maximum, %d), exiting.\n", *((uint32_t *) &(header[CEP_HDR_SEQ_OFFSET])));
		return -1;
	}

	if ((uint8_t) header[CEP_HDR_NBEAM_OFFSET] > UDPMAXBEAM) {
		fprintf(stderr,
		        "Input header appears malformed (more than UDPMAXBEAM beamlets on a port, %d), exiting.\n",
		        header[CEP_HDR_NBEAM_OFFSET]);
		return -1;
	}

	if ((uint8_t) header[CEP_HDR_NTIMESLICE_OFFSET] != UDPNTIMESLICE) {
		fprintf(stderr,
		        "Input header appears malformed (time slices are %d, not UDPNTIMESLICE), exiting.\n",
		        header[CEP_HDR_NTIMESLICE_OFFSET]);
		return -1;
	}

	if (source->padding0 != (uint32_t) 0) {
		fprintf(stderr, "Input header appears malformed (padding bit (0) is set), exiting.\n");
		return -1;
	} else if (source->errorBit != (uint32_t) 0) {
		fprintf(stderr, "Input header appears malformed (error bit is set), exiting.\n");
		return -1;
	} else if (source->bitMode == (uint32_t) 3) {
		fprintf(stderr, "Input header appears malformed (BM of 3 doesn't exist), exiting.\n");
		return -1;
	} else if (source->padding1 > (uint32_t) 1) {
		fprintf(stderr, "Input header appears malformed (padding bits (1) are set), exiting.\n");
		return -1;
	} else if (source->padding1 == (uint32_t) 1) {
		fprintf(stderr,
		        "Input header appears malformed (our replay packet warning bit is set), continuing with caution...\n");
	}

	return 0;

}

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