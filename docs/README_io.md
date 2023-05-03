# Overview

The `lofar_udp_io_*` function offer an interface to one of several 
I/O sources, from writing to disk as normal files or Zstandard 
compressed files, or to in-memory writes through named FIFO pipes 
and PSRDADA ringbuffers. 

The standard way to access the interfaces are through the use of a 
`_alloc()` and then a `_setup()` function to configure a struct for 
I/O operations, followed by several calls to a read/write 
operation function, and then a `_cleanup()` function before exiting 
the program. The sections below give a brief overview of the 
expected configuration of the structs before calling the setup 
functions, and the expected behaviour while performing some I/O 
operations.

# `lofar_udp_io_read` Interface

## Setup 
A pre-initialised struct can be allocated on the heap by calling the 
`lofar_udp_io_read_config_alloc()` function. It may return `NULL` if 
the allocation fails.

The struct should then be configured, with at least the following 
members set:
- readerType
- numInputs
- \(inputLocations OR inputDadaKeys\)\[0,numInputs\]

Unlike the write interface, there current isn't a function for 
parsing these variable directly from an input `optarg` into the 
configuration struct, but th equivilent functionality as can be seen 
for the writer struct can be replicated through the use of the 
`lofar_udp_io_parse_type_optarg` function to get the readerType, and 
`lofar_udp_io_parse_format` function to convert a file pattern 
format (see [the CLI readme](README_CLI.md)) into file names.

Additionally, we recommend preparing:
- The maximum number of elements expected to be read per iteration
- A buffer than can hold this number of elements

Both of those variables can then be used alongside the 
`lofar_udp_io_read_setup_helper` function in order to simplify the 
setup process, accounting for any requirements of some processing 
methods (such as the page sizes for Zstandard compressed buffers).

If you want to directly use the `lofar_udp_io_read_setup` function, 
the size variable should be set into the `readBufSize[0,
numInputs]`, `decompressionTracker[0,numInputs].size`, while the 
buffer is used to initialise the `decompressionTracker[port].src` if 
the readerType is `ZSTDCOMPRESSED`.

## Operations

## Cleanup

A single call to `lofar_udp_io_read_cleanup()` with your 
configuration struct will close any references to the input files 
and cleanup any allocated memory from the struct, before freeing the 
struct itself.

## Example Usage

```C

```

# `lofar_udp_io_write` Interface