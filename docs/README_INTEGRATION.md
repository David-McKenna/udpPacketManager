Integrating the Library
=======================
TODO

A lot of this information is outdated, I suggest having a look
at [lofar_cli_guppi_raw.c](../src/CLI/lofar_cli_guppi_raw.c) for an example implementation.

- Configure the system with `lofar_udp_config`
- Pass to `lofar_udp_meta_file_reader_setup_struct(...)` and error check that the repsonse isn't null
- Call in a loop with `lofar_udp_reader_step(_timed)(...)`, note that the arrays are pre-populated after reader
  construction
- Get data from `reader->meta->utputData[i]`, repeat loop
- Cleanup allocated data with `lofar_udp_reader_cleanup(reader);`


1. Include the reader header, this will be your main interface to the library

```
#include "lofar_udp_reader.h"
```

2. Generate a reader struct. You can configure it using the `lofar_udp_config` struct defined in [**
   lofar_udp_reader.h**](../src/lib/lofar_udp_reader.h). Please see that file for the options available in the current
   version of the CLI, we have modified a few of the basic options in the example below. The values are closely tied to
   the arguments described in [**README_CLI.md**](./README_CLI.md) if you want further explanation of what these options
   control.

```

lofar_udp_config readerConfig = lofar_udp_config_default;
lofar_udp_calibration calibrationConfig = lofar_udp_calibration_default;

// char* array of input file names
readerConfig.inputFiles = &inputFiles;

// int
readerConfig.numPorts = 4;
readerConfig.replayDroppedPackets = 1; // Copy last packet instead of 0-padding
readerConfig.verbose = 0;
readerConfig.readerType = 1; // ZSTD compressed files. Raw files: 0, PSRDADA: 2. See reader_t for new inputs
readerConfig.beamletLimits = { 0, 0 }; // Process all beamlets
readerConfig.calibrateData = 1; // Calibrate data with dreamBeam

//long
readerConfig.numPackets = nsamples / UDPNTIMESLICE;

// calibration
calibrationConfig.calibrationFifo = "/tmp/myfifo"; // Location to generate a PIPE for C/Python communication
calibrationConfig.calibrationSubbands = "HBA,12:499"; // Calibrate using a fairly standard set of HBA subbands
calibrationConfig.calibrationDuration = 3600; // Generate 1 hour of Jones matrices at a time
calibrationConfig.calibrationPointing = { 1.0, 0.0 }; // Direction RA = 1.0rad, DEC = 0.0rad
calibrationConfig.calibrationPointingBasis = 'J2000'; // In J2000

// Generate the reader object -- this is out main interface to the library
lofar_udp_reader *reader =  lofar_udp_meta_file_reader_setup_struct(&(config));
```

Either set-up some arbitrary pointers to copy the output data to, or plan to pull from the reader to get your output
data. Be sure your data types match (may change with enabling calibration, different input bitmodes, etc.)

```
char *outputData[numPorts];
for (int i =0; i < ports; i++) {
	ouputData[i] = reader->meta->outputData[i];
}
```

When you're ready to request data, call `lofar_udp_reader_step` or `lofar_udp_reader_step_timed` on your reader (will
return more than 0 on an unrecoverable error). You may want to check `reader->meta->packetsPerIteration` to ensure it's
as long as you expect -- this value may shrink if you have out of order packets, so reading the entire array will result
in the final packets being from the previous iteration.

```
if (lofar_udp_reader_step(reader) > 0) return -1;
int nsamps_processed = reader->meta->packetsPerIteration * 16;

processData(outputData, numPorts, nsamps_processed);
```

When finished, the clean-up function will free any malloc'd components of the reader and close your input files for you.

```
lofar_udp_reader_cleanup(reader);
```

Undocumented: Reuse function (as seen in [lofar_cli_extractor.c](../src/CLI/lofar_cli_extractor.c)): should only be on
the same file, for future time steps. Hitting EOF resizes arrays and a new reader must be generated for a given file
afterwards.