#include "lofar_udp_structs.h"

// Merge in the metadata structs
#include "lofar_udp_structs_metadata.c"

// Reader struct default
const lofar_udp_io_read_config lofar_udp_io_read_config_default = {
	.readerType = NO_ACTION,
	.readBufSize = { -1 }, // NEEDS FULL RUNTIME INITIALISATION
	.portPacketLength = { -1 }, // NEEDS FULL RUNTIME INITIALISATION
	.numInputs = 0,

	// Inputs pre- and post-formatting
	.inputLocations = { "" },
	.dadaKeys = { -1 }, // NEEDS FULL RUNTIME INITIALISATION
	.basePort = 0,
	.offsetPortCount = 0,
	.stepSizePort = 1,

	// Main reading objects
	.fileRef = { NULL },
	.dstream = { NULL },
	.dadaReader = { NULL },

	// Associated objects
	.readingTracker = { { NULL } },
	.decompressionTracker = { { NULL } },
	.multilog = { NULL },
	.dadaPageSize = { -1 } // NEEDS FULL RUNTIME INITIALISATION
};

// Writer struct default
const lofar_udp_io_write_config lofar_udp_io_write_config_default = {
	// Control options
	.readerType = NO_ACTION,
	.metadata = NULL,
	.fallbackMetadata = NULL,
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
	.zstdWriter = { { NULL } },
	.dadaWriter = { { NULL } },
	.hdf5Writer = { 0, },
	.hdf5DSetWriter = { { 0, { 0, 0 }}},



	// zstd configuration
	.zstdConfig = {
		.compressionLevel = 3,
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
	.enableMultilog = 1,
	.externalChannelisation = 1,

};

// Calibration default
// While in here we default to enabling calibration, the overall default is read from
// the lofar_udp_config and overwrites the value in here.
const lofar_udp_calibration lofar_udp_calibration_default = {
	.calibrationStepsGenerated = 0,
	.calibrationFifo = "/tmp/udp_calibation_pipe",
	.calibrationDuration = 3600.0f,
};

// Configuration default
const lofar_udp_config lofar_udp_config_default = {
	.inputLocations = {  },
	.metadata_config = {
		// Initialised by alloc func
	},
	.numPorts = 4,
	.replayDroppedPackets = 0,
	.processingMode = 0,
	.verbose = 0,
	.packetsPerIteration = 65536,
	.startingPacket = -1,
	.packetsReadMax = -1,
	.readerType = NO_ACTION,
	.beamletLimits = { 0, 0 },
	.calibrateData = NO_CALIBRATION,
	.calibrationConfiguration = NULL,
	.ompThreads = OMP_THREADS,
	.dadaKeys = { -1 }, // NEEDS FULL RUNTIME INITIALISATION

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


lofar_udp_obs_meta *lofar_udp_obs_meta_alloc() {
	DEFAULT_STRUCT_ALLOC(lofar_udp_obs_meta, meta, lofar_udp_obs_meta_default, ;, NULL);

	return meta;
}

/**
 * @brief Generate a fully allocated reader struct
 *
 * @return ptr: success, NULL: failure
 */
lofar_udp_reader* lofar_udp_reader_alloc(lofar_udp_obs_meta *meta) {
	CHECK_ALLOC_NOCLEAN(meta, NULL);
	DEFAULT_STRUCT_ALLOC(lofar_udp_reader, reader, lofar_udp_reader_default, ;, NULL);
	reader->meta = meta;

	lofar_udp_io_read_config *input = lofar_udp_io_alloc_read();
	CHECK_ALLOC(input, NULL, free(reader););
	reader->input = input;

	return reader;
}

lofar_udp_calibration* lofar_udp_calibration_alloc() {
	DEFAULT_STRUCT_ALLOC(lofar_udp_calibration, calibration, lofar_udp_calibration_default, ;, NULL);

	return calibration;
}

lofar_udp_config* lofar_udp_config_alloc() {
	DEFAULT_STRUCT_ALLOC(lofar_udp_config, config, lofar_udp_config_default, ;, NULL);
	STRUCT_COPY_INIT(metadata_config, &(config->metadata_config), metadata_config_default, free(config);, NULL);

	config->calibrationConfiguration = lofar_udp_calibration_alloc();
	CHECK_ALLOC(config->calibrationConfiguration, NULL, free(config););


	return config;
}

lofar_udp_io_read_config* lofar_udp_io_alloc_read() {
	DEFAULT_STRUCT_ALLOC(lofar_udp_io_read_config, input, lofar_udp_io_read_config_default, ;, NULL);

	ARR_INIT(input->readBufSize, MAX_NUM_PORTS, -1);
	ARR_INIT(input->portPacketLength, MAX_NUM_PORTS, -1);
	ARR_INIT(input->dadaKeys, MAX_NUM_PORTS, -1);
	ARR_INIT(input->dadaPageSize, MAX_NUM_PORTS, -1);

	return input;
}

lofar_udp_io_write_config* lofar_udp_io_alloc_write() {
	DEFAULT_STRUCT_ALLOC(lofar_udp_io_write_config, output, lofar_udp_io_write_config_default, ;, NULL);

	ARR_INIT(output->writeBufSize, MAX_OUTPUT_DIMS, -1);
	ARR_INIT(output->outputDadaKeys, MAX_OUTPUT_DIMS, -1);

	return output;
}


void lofar_udp_config_cleanup(lofar_udp_config *config) {
	if (config != NULL) FREE_NOT_NULL(config->calibrationConfiguration);
	FREE_NOT_NULL(config);
}
