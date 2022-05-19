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

};

// Calibration default
// While in here we default to enabling calibration, the overall default is read from
// the lofar_udp_config and overwrites the value in here.
const lofar_udp_calibration lofar_udp_calibration_default = {
	.calibrationStepsGenerated = 0,
	.calibrationFifo = "/tmp/udp_calibation_pipe",
	.calibrationSubbands = "HBA,12:499",
	.calibrationDuration = 3600.0f,
	.calibrationPointing = { 0.0f, 0.7853982f },
	.calibrationPointingBasis = "AZELGO"
};

// Configuration default
const lofar_udp_config lofar_udp_config_default = {
	.inputLocations = {  },
	.metadata_config = {
		.metadataType = NO_META,
		.metadataLocation = "",
		.externalChannelisation = -1,
		.externalDownsampling = -1
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
	.ompThreads = OMP_THREADS,
	.meta = NULL
};


// meta with NULL-initialised values to help the cleanup function
const lofar_udp_input_meta lofar_udp_input_meta_default = {
	.inputData = { NULL },
	.outputData = { NULL },
	.packetsRead = 0,
	.inputDataReady = 0,
	.outputDataReady = 0,
	.jonesMatrices = NULL,
	.calibrationStep = 0
};