Architecture
============

This document is a high-level overview of the udpPacketManager library's internal workings, call tree and some of the unexpected design choices in parts of the library.

The repo consists of four main components:
- [The library itself](#the-main-library)
- A mini [library of metadata writers](#metadata-writers)
- An [external Python executable](#python---dreambeam-wrapper) for interfacing with [dreamBeam](https://github.com/2baOrNot2ba/dreamBeam) to generate Jones matrices for calibration
- [A set of reference CLIs](#the-clis), which can also be used to process data

# The Main Library
Interfacing with the library from a user perspective should only require three function calls, to setup a library for processing, to processed the input and then finally to cleanup any memory allocated for processing. 

These are the functions
- lofar_udp_reader_setup()
- lofar_udp_reader_step() or lofar_udp_reader_step_timed()
- lofar_udp_reader_cleanup()

All of these functions can be found in the [lofar_udp_reader.c](../src/lib/lofar_udp_reader.c) file.



To save on compute time, while the library does have a verbose output, it is only enabled in debug builds. It can be forcibly enabled in all builds by appending `-DVERBOSE` to your `CFLAGS` enviroment variable.

### lofar_udp_reader_setup()
The *lofar_udp_reader_setup* function takes in a configuration struct defined in [lofar_udp_structs.c](../src/lib/lofar_udp_structs.h), opens the input datastreams, reads in data and populates as much metadata as it can from the input packet stream. It returns the pointer to a *lofar_udp_reader* struct which contains all of the informaiton and alocated buffers required to read in new data and re-order it into a set output.

Calling this function will read in a standard gulp of data from the set number of ports, find the lowest common packet after a given starting point that is present on all ports of data and align them. This function does not re-order the data into an output, this will only occur after the first *lofar_udp_reader_step*-like function is called.


### lofar_udp_reader_step()
This function performs a combined call to read function in [`lofar_udp_io.c`](../src/lib/lofar_udp_io.c) to get new input data and then passes that input data to the C++ function in [lofar_udp_backends.cpp](../src/lib/lofar_udp_backends.cpp), which handles re-ordering the data to the desired output.

In the case that there is large packet loss, a significant number of out-of-order packets or the input returns an end of stream marker, this function will return a non-zero value.

### lofar_udp_reader_cleanup()
This function calls `free` on all allocated memory, closes input data streams and generals cleans up after the library.


### Implementation details
The input read buffers are padded in two cases:
- They are always extended by 2 packets to hold both a reference packet from a previous iteration, and a blank packet for the case that packets are not replayed when packets are dropped
- If the input is from a zstd compressed file, the decompression system expects a given output size, so we need to extend the input buffer size to align with the expected size (only needed in cases where the packets per iteration isn't a multiple of 2)

# Metadata Writers



# Python - dreamBeam Wrapper

# The CLIs
The CLIs act as reference on how to use the library, as well as an interface to process an observation from raw CEP packets to (un)calibrated voltages or Stokes outputs.