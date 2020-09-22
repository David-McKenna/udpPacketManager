Integrating the Library
=======================
TODO


Include the reader header
```
#include "lofar_udp_reader.h"
```


You may need to manually include some C functions for some compilers (CUDA needed this in my case). For a basic implementation, all you need is
```
extern "C"  {
  int lofar_udp_reader_step(lofar_udp_reader *reader);
  lofar_udp_reader* lofar_udp_meta_file_reader_setup(FILE **inputFiles, const int numPorts, const int replayDroppedPackets, const int processingMode, const int verbose, const long packetsPerIteration, const long startingPacket, const long packetsReadMax, const int compressedReader);
}
```

Generate your reader
```
lofar_udp_reader *reader;
const long packetGulp = nsamp / 16;


// inputFiles: char* array, ports: dim of inputFiles, replayDroppedPackets (int): 0 -> zeros 1-> re-use last packet, processingMode (int): see README_CLI.md \
// packetGulp: number of packets (divide by 16 for number of samples), startingPacket (long): startingPacket ID or -1, maxPackets (long), \
// compressedInput (int): 1 if inputFile are zstd comrpessed

reader = lofar_udp_meta_file_reader_setup(inputFiles, ports, replayDroppedPackets, processingMode, verbose, packetGulp, (long) -1, LONG_MAX, compressedInput);
```

Either setup some arbitrary pointers or plan to pull from the reader to get your output data
```
char *outputData[ports];
for (int i =0; i < ports; i++)
	ouputData[i] = reader->meta->outputData[i];
```

When you're ready to request data, call `lofar_udp_reader_step` or `lofar_udp_reader_step_timed` on your reader (will return more than 0 on an unrecoverable error). You may want to check `reader->meta->packetsPerIteration` to ensure it's as long as you expect -- this value may shrink if you have out of order packets, so reading the entire array will result in the final packets being from the previous iteration.
```
if (lofar_udp_reader_step(reader) > 0) return 0;
int nsamps_processed = reader->meta->packetsPerIteration * 16;

processData(outputData, nsamps_processed);
```

When finished, the cleanup function will free any malloc'd components of the reader and close your input files for you.
```
lofar_udp_reader_cleanup(reader);
```

Reuse: should only be on the same file, for future time steps. Hitting EOF resizes arrays and a new reader must be generated for a given file afterwards.