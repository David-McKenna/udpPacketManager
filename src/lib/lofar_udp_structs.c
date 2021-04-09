#include "lofar_udp_structs.h"


const lofar_udp_reader_input lofar_udp_reader_input_default = {
		.fileRef = { NULL },
		.dstream = { NULL },
		.dadaKey = { -1 },
		.dadaReader = { NULL },
		.multilog = { NULL }
};

// Calibration default
// While in here we default to enbling calibration, the overall default is read from
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
		.inputLocations = { "" },
		.numPorts = 4,
		.replayDroppedPackets = 0,
		.processingMode = 0,
		.verbose = 0,
		.packetsPerIteration = 65536,
		.startingPacket = -1,
		.packetsReadMax = -1,
		.readerType = 0,
		.beamletLimits = { 0, 0 },
		.calibrateData = 0,
		.calibrationConfiguration = NULL,
		.ompThreads = OMP_THREADS,
		.dadaKeys = { -1 }
};

// Reader / meta with NULL-initialised values to help the cleanup function
const lofar_udp_reader lofar_udp_reader_default = {
		.input = NULL,
		.ompThreads = OMP_THREADS,
		.meta = NULL
};


// meta with NULL-initialised values to help the cleanup function
const lofar_udp_meta lofar_udp_meta_default = {
		.inputData = { NULL },
		.outputData = { NULL },
		.packetsRead = 0,
		.inputDataReady = 0,
		.outputDataReady = 0,
		.jonesMatrices = NULL,
		.calibrationStep = 0
};

// Writer struct default
const lofar_udp_io_write_config lofar_udp_io_write_config_default = {
		.readerType = NORMAL,
		.appendExisting = 0,
		.outputFormat = "",
		.outputLocations = { "" },
		.baseVal = 0,
		.offsetVal = 1,

		.outputFiles = { NULL },
		.outputDadaKeys = { -1 },

		.cstream = { NULL },
		.dadaWriter = {{ NULL }}
};
