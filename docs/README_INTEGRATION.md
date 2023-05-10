Integrating the Library
=======================

**TL;DR**
1. Configure the system with a struct from `lofar_udp_config_alloc()`
2. Pass to `lofar_udp_reader_setup(...)` and error check that the 
  repsonse isn't null
3. Call in a loop with `lofar_udp_reader_step(_timed)(...)`, note that 
   the arrays are pre-populated after reader
  construction 
4. Get data from `reader->meta->outputData[i]`, repeat loop
5. Cleanup allocated data with `lofar_udp_reader_cleanup(reader);`

To expand on this,

1. Include the reader header, this will be your main interface to the library

```C
#include "lofar_udp_reader.h"
```

2. Generate a reader struct. You can configure it using the `lofar_udp_config` struct defined in [**
   lofar_udp_structs.h**](../src/lib/lofar_udp_structs.h). Please see 
   that file for the options available in the current
   version of the CLI, we have modified a few of the basic options in the example below. The values are closely tied to
   the arguments described in [**README_CLI.md**](./README_CLI.md) if you want further explanation of what these options
   control.

    As a manual example,

```C

lofar_udp_config *readerConfig = lofar_udp_config_alloc();

readerConfig->processingMode = STOKES_I; // Choose an output data product

const int8_t numPorts = 4;
readerConfig->numPorts = numPorts; // Process 4 files of data
// char* array of input file names
for (int8_t port = 0; port < numPorts; port++)
    strncpy(readerConfig->inputFiles[port], inputFiles[port], 
                                                        DEF_STR_LEN);
readerConfig->packetsPerIteration = 65536; // Number of packets to read for each operation
readerConfig->replayDroppedPackets = 1; // Copy last packet instead of 0-padding
readerConfig->readerType = ZSTDCOMPRESSED; // See reader_t definitions in lofar_up_general.h.in
readerConfig->beamletLimits = { 0, 0 }; // Process all beamlets

readerConfig->calibrateData = APPLY_CALIBRATION; // Calibrate data with dreamBeam, see alternative options with calibrate_t in lofar_udp_general.h
strncpy(readerConfig->metadata.metadataLocation, myMetaLocation, 
                                                        DEF_STR_LEN);

readerConfig->numPackets = -1; // Process the file entirely


// Generate the reader object -- this is out main interface to the library
lofar_udp_reader *reader =  lofar_udp_reader_setup(config);
```

When you're ready to request data, call `lofar_udp_reader_step` or 
`lofar_udp_reader_step_timed` on your reader (will return more than 
0 on an unrecoverable error). The data will then be available at 
`reader->meta->outputData[i]`, where `i` ranges from `0` to 
`reader->meta->numOutputs - 1`. By default, the data is aligned for 
SIMD operations with registers up to 512 bits long.

You may want to check `reader->meta->packetsPerIteration` to ensure it's
as long as you expect -- this value may shrink if you have out of 
order packets, so reading the entire array will result in the final 
packets being from the previous iteration if this is not accounted 
for. When this occurs, time major operation are re-processed to ensure 
they have the correct boundaries, resulting in the array being 
shorter than the allocated space.

```C
if (lofar_udp_reader_step(reader) > 0) return -1;
int64_t nsamps_processed = reader->meta->packetsPerIteration * UDPNTIMESLICE;

handleData(reader->meta->outputData, numPorts, nsamps_processed);
```

When finished, the clean-up function will free any malloc'd components of the reader and close your input files for you.

```C
lofar_udp_reader_cleanup(reader);
```