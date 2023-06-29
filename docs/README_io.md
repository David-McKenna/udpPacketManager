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
- \(inputLocations OR inputDadaKeys\)\[0:numInputs\]
- numInputs

There is a function for parsing the first and second variables directly from an input `optarg` flag or char array into the
configuration struct, `lofar_udp_io_read_parse_optarg()`. This expects an input string following the conventions that are detailed in the [the
CLI readme](README_CLI.md).

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

Repeated calls to `lofar_udp_io_read`, alongside a target buffer and requested read length will produce as much data as possible from the
source. By default, it will return the amount of data read, but may produce a negative or 0 value when no more data is available.

Write lengths must always be less than the maximum length set during the struct configuration.

For the `ZSTANDARD` reader, the input buffer must always match the buffer provided to the initialisation function, or error may occur (this
does not apply to the `ZSTANDARD_INDIRECT` mode).

## Cleanup

A single call to `lofar_udp_io_read_cleanup()` with your
configuration struct will close any references to the input files
and cleanup any allocated memory from the struct, before freeing the
struct itself.

## Example Usage

```C
int32_t myFunc(void) {
	// `goto` use not recommended, used to keep example clean
	const char inputFormat[] = "myInputFile_[[port]].zst,0,2";
	const int32_t numInputs = 2;
	int32_t returnVal = 0;

	lofar_udp_io_read_config *reader = lofar_udp_io_read_alloc();
	reader->numInputs = numInputs;

	// Sets readerType, inputLocations, and associated base/offset/step values
	if (lofar_udp_io_read_parse_optarg(reader, inputFormat) < 0) {
		goto cleanup;
	}

	const int64_t numBytesPerRead = 2 << 20;
	int8_t *readerBuffers[numInputs] = { NULL };
	for (int32_t i = 0; i < numInputs; i++) {
		readerBuffers[i] = (int8_t*) calloc(numBytesPerRead, sizeof(int8_t));
		if (readerBuffers[i] == NULL) {
			goto cleanup;
		}

		if (lofar_udp_io_read_setup_helper(reader, readerBuffers, numBytesPerRead, i) < 0) {
			goto cleanup;
		}
	}


	// Operate on the data while requesting it
	int64_t lastBytesRead[numInputs] = { 0 };
	while (returnVal == 0) {
		for (int32_t i = 0; i < numInputs; i++) {
			if ((lastBytesRead[i] = lofar_udp_io_read(reader, i, readerBuffers[i], numBytesPerRead)) < 0) {
				returnVal = -1;
			}
		}
		// Do something with data
	}

	returnVal = 0;


	if (0) {
		cleanup:
		returnVal = 1;
	}
	lofar_udp_io_read_cleanup(reader);
	for (int32_t i = 0; i < numInputs; i++) {
		FREE_NOT_NULL(readerBuffers[i]);
	}
	return returnVal;
}
```

# `lofar_udp_io_write` Interface

## Setup
A pre-initialised struct can be allocated on the heap by calling the
`lofar_udp_io_write_config_alloc()` function. It may return `NULL` if
the allocation fails.

The struct should then be configured, with at least the following
members set:
- readerType
- outputFormat
- progressWithExisting
- Any additional configuration required for the specific writer (`zstdConfig`, `dadaConfig`)

The first two of these can be parsed from a format string (see [the CLI readme](README_CLI.md) for formatting options) using the
`lofar_udp_io_write_parse_optarg()` function.

Additionally, we recommend preparing:
- The maximum number of elements expected to be written per iteration
- The number of outputs
- A starting iteration (for output `[[iter]]` substitution)
- A starting reference number (for output `[[pack]]` substitution)

Which can then be passed into the `lofar_udp_io_write_setup_helper()` function to help with initialisation and sanity checking. If you want
to initialise the maximum write size and still call this function, the outputLength array will be ignored if it's first value is `LONG_MIN`.

## Operations

Repeated calls to `lofar_udp_io_write`, alongside an input buffer and requested write length will output as much data as possible from the
source. By default, it will return the amount of data written, though it may return a 0 or negative value on an error.

Write lengths must always be less than the maximum length set during the struct configuration.

## Cleanup

A single call to `lofar_udp_io_write_cleanup()` with your
configuration struct will begin one of two given cleanup processes. The `fullClean` variable option allows for the struct to be reset for
future iterations when it is set to 0, or will clear all open references, allocations and free itself when it is set to any non-zero value.

## Example Usage

```C
int32_t myFunc(void) {
	// `goto` use not recommended, used to keep example clean
	const char outputFormat[] = "myOutputFile_[[port]].zst,0,2";
	const int8_t numOutputs = 2;
	int64_t numBytesPerWrite[numOutputs];
	int32_t returnVal = 0;

	lofar_udp_io_write_config *writer = lofar_udp_io_write_alloc();
	writer->numOutputs = numOutputs;
	writer->progressWithExisting = 0; // Don't proceed if the output already exists
	writer->zstdConfig.compressionLevel = 3;
	writer->zstdConfig.numThreads = 2;

	// Sets readerType, inputLocations, and associated base/offset/step values
	if (lofar_udp_io_write_parse_optarg(writer, outputFormat) < 0) {
		goto cleanup;
	}

	for (int8_t i = 0; i < numOutputs; i++) {
		numBytesPerWrite[i] = 2 << 20;
	}

	if (lofar_udp_io_write_setup_helper(writer, numBytesPerWrite, numOutputs, 0, 0) < 0) {
		goto cleanup;
	}

	// Operate on the data while requesting it
	while (returnVal == 0) {
		// Request some data to write
		int8_t **workingData = give_me_data(numOutputs, numBytesPerWrite);
		for (int8_t i = 0; i < numOutputs; i++) {
			if ((lofar_udp_io_write(writer, i, workingData[i], numBytesPerWrite[i])) < 0) {
				returnVal = -1;
			}
		}
	}

	returnVal = 0;


	if (0) {
		cleanup:
		returnVal = 1;
	}
	lofar_udp_io_write_cleanup(writer, 1);
	return returnVal;
}
```