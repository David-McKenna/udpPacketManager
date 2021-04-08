Adding New Processing Modes
---------

1. Create the CPP/C bridge `if--else` statements in `lofar_udp_backends.cpp`, the main function is
   called `int lofar_udp_cpp_loop_interface(lofar_udp_meta *meta)`. You will need to pick both a processing mode int
   enum (any value greater than 0 and not in use by other modes) and an output data format. -- You will need to add the
   statement 6 times in total: with / without calibration (of disable calibration as an option) and for the 3 input bit
   modes, 4, 8 and 16. -- Calibration takes a 1 when enabled, 0 when disabled. -- Input type is always signed char for
   4-bit and 8-bit inputs, 16-bit takes signed short as the input. -- For copy methods, the output datatype should be
   the same as the input. Though you can change it, eg to convert to float by using float as the output datatype. Be
   sure to account for this later on when calculating output sizes. -- The 4-bit processing enum is (almost) always 4000
   larger than the default enum, to signal to the processing loop that the data packet needs to have the bits upacked
   before proceeding. If you have a processing mode that just performs a data move, eg memcpy, this change is not
   needed.

Here's an example of what mode 30 looks like in the function.

```
int lofar_udp_cpp_loop_interface(lofar_udp_meta *meta) {
	if (calibrateData == 1) {
		// Bitmode dependant inputs
		if (inputBitMode == 4) {

			// Time-major modes
			} else if (processingMode == 30) {
				return lofar_udp_raw_loop<signed char, signed char, 4030, 1>(meta);
			} else {
				...
			}
			


		} else if (inputBitMode == 8) {

			// Time-major modes
			} else if (processingMode == 30) {
				return lofar_udp_raw_loop<signed char, signed char, 30, 1>(meta);
			} else {
				...
			}

		} else if (inputBitMode == 16) {

			...

			// Time-major modes
			} else if (processingMode == 30) {
				return lofar_udp_raw_loop<signed char, signed short, 30, 1>(meta);
			} else {
				...
			}

		} else {
			...
		}

	// Interfaces to raw data interfaces (no calibration)
	} else {
		// Bitmode dependant inputs
		if (inputBitMode == 4) {

			// Time-major modes
			} else if (processingMode == 30) {
				return lofar_udp_raw_loop<signed char, signed char, 4030, 0>(meta);
			} else {
				...
			}
			


		} else if (inputBitMode == 8) {

			// Time-major modes
			} else if (processingMode == 30) {
				return lofar_udp_raw_loop<signed char, signed char, 30, 0>(meta);
			} else {
				...
			}

		} else if (inputBitMode == 16) {

			...

			// Time-major modes
			} else if (processingMode == 30) {
				return lofar_udp_raw_loop<signed char, signed short, 30, 0>(meta);
			} else {
				...
			}

		} else {
			...
		}
	}
}

```

3. Create the task kernel in `lofar_udp_backends.hpp`, following the format below. Have a look at the existing kernels
   and you'll likely be able to find an input/output idx calculation that suits what you are doing.

```
template<typename I, typename O>
void inline udp_myNewKernel(params) {
	// Define the expected base offset for the given packet
	// 
	// 1. 1:1 packets input to packet output offset. You probably want this one.
	//	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	//
	// 2. Time-major offset
	//	long outputTimeIdx = iLoop * UDPNTIMESLICE / sizeof(O);

	// Initialise other local variables
	long tsInOffset, tsOutOffset;

	// Initialise variabled related to calibration, silence compiler warnings related to it for the non-calibrated calls.
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop

	...


	// Some compiler pragmas that were determined to give optimal performance through A/B testing GCC-9 and ICC-2021.2-beta
	// Though the GCC-9 implementation is fast, this balloons the output archives to 80+MB, and icnreases compile time similarly.
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		// Get the input data offset
		tsInOffset = input_offset_index(long lastInputPacketOffset, int beamlet, int timeStepSize);

		// Determine an output data offset, this heavily depends on the output ordering you're trying to achieve
		// Here's some samples
		// 1. Channel Major Data
		//	Take the input data, order it such that we have sequential frequency samples beside eachother.
		//	tsOutOffset = outputPacketOffset + beamlet - baseBeamlet + cumulativeBeamlets;

		// 2. Time Major Data
		//	Take the input data, order it such that we have sequentual time samples beside eachother, with each frequency channel separated by N time samples
		//	tsOutOffset = 4 * (((beamlet - baseBeamlet + cumulativeBeamlets) * packetsPerIteration * UDPNTIMESLICE ) + outputTimeIdx);
		// 3. Reversed-Order Channel Major Data
		//	Perform #1, but reverse the channel order (for negative frequency ordering, needed by many filterbank/dedispersing programs)
		//	tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - beamlet + baseBeamlet - cumulativeBeamlets);

		// The last two modes are now available via inline functions
		// 		inline long frequency_major_index(long outputPacketOffset, int totalBeamlets, int beamlet, int baseBeamlet, int cumulativeBeamlets);
		// 		inline long time_major_index(int beamlet, int baseBeamlet, int cumulativeBeamlets, long packetsPerIteration, long outputTimeIdx);


		// If performing calibration, extract the Jones matrix for this frequency
		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(cumulativeBeamlets + beamlet - baseBeamlet) * JONESMATSIZE]);
		}

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {

			// Setup two control flows, either calibrate the data, or read the data straight to the output indices.
			if constexpr (calibrateData) {
				calibrateData<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr; // Xr
				...
			} else {
				outputData[outputFileIdx][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
				...
			}

			// Update the input and output offsets for the next time iteration
			// You probably want to use this input time offset to skip to the next time sample for the given beam
			tsInOffset += stepsPerOutputFile * timeStepSize;

			// Your output time offset can be extremely variable, but often is just some constant.
			tsOutOffset += nextOffset;
		}
	}
}
```

4. Include the kernel in the switch statement of `int lofar_udp_raw_loop(lofar_udp_meta *meta)`

```
else if (trueState == KERNEL_ENUM_VAL) {
	udp_myNewKernel<I, O, calibrateData>(iLoop, lastInputPacketOffset, timeStepSize....);
}
...

```

5. Go to `lofar_udp_reader.c` and find the `int lofar_udp_setup_processing(lofar_udp_meta *meta)` function. You will
   need to add your mode to two switch statements here. One is a simple fall-through to check that the mode is defned.
   For the second, you'll need to determine the input / output data sizes and add your processing mode to the second
   switch statement. If adding a completely new calculation, be sure to add a `break;` statement afterwards, as the
   compiler warning is disabled for this switch statement. In the case of a re-rodering operation, you will just need to
   define the number of output arrays.

6. Add documentation to `README_CLI.md` and `lofar_cli_meta.c`.

7. Generate hashes for the output mode by adding it to the
   makefiles' `./tests/obj-generated-$(LIB_VER).$(LIB_VER_MINOR)` target, either in the compressed or uncompressed
   loop. `make test-make-hashes` will generate an output and add a hash to tests/hashVariables.txt file in the git repo.
   Ensure no other mode hashes change.