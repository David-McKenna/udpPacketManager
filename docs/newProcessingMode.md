Adding New Processing Modes
---------

1. Add a named enum for the processing mode as a `processMode_t` in
   `[lofar_udp_general.h]](../src/lib/lofar_udp_general.h.in)`, and to 
   the list of testables modes in
   `[lib_reference_files.hpp](../tests/lib_tests/lib_reference_files.hpp)`
   in order to makeit pass through the standard battery of tests.

2. Create the CPP/C bridge `if--else` statements in
   `[lofar_udp_backends.cpp](../src/lib/lofar_udp_backends.cpp)`, 
   the main function is called `int32_t 
   lofar_udp_cpp_loop_interface(lofar_udp_obs_meta *meta)`. You will 
   need to pick both a processing mode int enum (any value greater 
   than 0 and not in use by other modes) and an output data format. 
   -- You will need to add the statement 6 times in total: with / 
   without calibration (of disable calibration as an option) and for 
   the 3 input bit modes, 4, 8 and 16. -- Calibration takes a 1 when 
   enabled, 0 when disabled. -- Input type is always signed char for 
   4-bit and 8-bit inputs, 16-bit takes signed short as the input. 
   -- For copy methods, the output datatype should be the same as 
   the input. Though you can change it, eg to convert to float by 
   using float as the output datatype. Be sure to account for this 
   later on when calculating output sizes. -- The 4-bit processing 
   enum is (almost) always 4000 larger than the default enum, to 
   signal to the processing loop that the data packet needs to have 
   the bits upacked before proceeding. If you have a processing mode 
   that just performs a type independant data move, e.g. 
   memcpy an entire packet, this change is not needed.

  Here's an example of what mode 30, the time-majour single output 
  looks like in the function.

```
switch (calibrateData) {
	case APPLY_CALIBRATION:
		// Bit-mode dependant inputs
		switch (inputBitMode) {
			case 4:
				switch (processingMode) {
						// Time-major modes
					case TIME_MAJOR_FULL:
						return lofar_udp_raw_loop<int8_t, float, 4000 + TIME_MAJOR_FULL, 1>(meta);
				}
			case 8:
				switch (processingMode) {
						// Time-major modes
					case TIME_MAJOR_FULL:
						return lofar_udp_raw_loop<int8_t, float, TIME_MAJOR_FULL, 1>(meta);
				}
			case 16:
				switch (processingMode) {
						// Time-major modes
					case TIME_MAJOR_FULL:
						return lofar_udp_raw_loop<int16_t, float, TIME_MAJOR_FULL, 1>(meta);
				}
		}

		// Interfaces to raw data interfaces (no calibration applied in our code)
	case NO_CALIBRATION:
	case GENERATE_JONES:
		// Bitmode dependant inputs
		switch (inputBitMode) {
			case 4:
				switch (processingMode) {
						// Time-major modes
					case TIME_MAJOR_FULL:
						return lofar_udp_raw_loop<int8_t, int8_t, 4000 + TIME_MAJOR_FULL, 0>(meta);
				}
			case 8:
				switch (processingMode) {
						// Time-major modes
					case TIME_MAJOR_FULL:
						return lofar_udp_raw_loop<int8_t, int8_t, TIME_MAJOR_FULL, 0>(meta);
				}
			case 16:
				switch (processingMode) {
						// Time-major modes
					case TIME_MAJOR_FULL:
						return lofar_udp_raw_loop<int16_t, int16_t, TIME_MAJOR_FULL, 0>(meta);
				}
		}
}
```

   If you miss a case, the default cases should raise an error when the 
   tests are run, allowing you to indentify which case is missing 
   and add it.

3. Create the task kernel in `[lofar_udp_backends.hpp](../src/lib/lofar_udp_backends.hpp)`, following the 
   format below. Have a look at the existing kernels and you'll 
   likely be able to find an input/output idx calculation that suits 
   what you are doing. Here's an annotated description of mode 30, the time-major single-output processing mode.

```
// Types and per-mode optimisations should be used to initialise a template for optimal performance
template<typename I, typename O, const int8_t calibrateData, ...>
static inline void udp_myNewKernel(...) {
	// Define the expected base offset for the given packet, these examples do not account for downsampling.
	// 
	// 1. Frequency major data, or 1:1 packets input to packet output offset.
	//	 outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	//
	// 2. Time-major offset
	//	 outputTimeIdx = iLoop * UDPNTIMESLICE / sizeof(O);
	const int64_t outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	
	// Optionally: define intermediate storage for calibrated voltages, this mode does not use them.
	//O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE]; 
	
	// Define a pointer to the Jones matrix for calibration
	const float *beamletJones;

   // Iterate across the pre-configured beamlet lower/upper limits for the given port
	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
	    // Calculate the input frequency offset for the current beamlet (no changes for new modes)
		const int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		
		// Calculate the output offset for the first sample, we will iterate from there
		//
		// 1. Frequency Major base offset, reversed_ prefix will reverse the frequency order
		//     tsOutOffsetBase = frequency_major_index(outputPacketOffset, beamlet, baseBeamlet, cumulativeBeamlets, UDPNPOL);
		//     tsOutOffsetBase = reversed_frequency_major_index(outputPacketOffset, beamlet, baseBeamlet, cumulativeBeamlets, UDPNPOL);
		//
		// 2. Time Major base offset, takes a downsample factor as a template parameter
		//     tsOutOffsetBase = time_major_index<DOWNSAMPLE_FAC>(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputTimeIdx);
		const int64_t tsOutOffsetBase = frequency_major_index(outputPacketOffset, beamlet, baseBeamlet, cumulativeBeamlets, UDPNPOL);

        // Convert the input array to it's true byte type from int8_t as an array called castPtr, offset by tsInOffsetBase
        // Initialises the beamletJones matrix for the current beamlet
		TYPE_AND_CAL_SETUP(I, O);

		// Macro to define an optimisation pragma based on the current compiler
		LOOP_OPTIMISATION
		
		// Iterate across all time samples
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
		    // The input offset is standard, and does not change between processing modes.
		    // We don't need to use tsInputOffsetBase here as it was used when initialising castPtr in TYPE_AND_CAL_SETUP
			const int64_t tsInOffset = ts * UDPNPOL;
			
			// The output offset differs greatly depending on the processing mode, we will provide samples but your values 
			//    will be highly dependant on your choice of processing scheme.
			//
			// 1. Frequeny Major iterations, 
			//    - Frequency Major split across polarisation
			//       tsOutOffset = tsOutOffsetBase + ts * totalBeamlets
			//    - Frequency Major to a single output
			//       tsOutOffset = tsOutOffsetBase + ts * (totalBeamlets * UDPNPOL)
			//
			// 2. Time Major iterations,
			//    - Time major split across polarisation
			//       tsOutOffset = tsOutOffsetBase + ts
			//    - Time Major to a single output
			//       tsOutOffset = tsOutOffsetBase + ts * UDPNPOL
			//
			// etc.
			
			const int64_t tsOutOffset = tsOutOffsetBase + ts * (totalBeamlets * UDPNPOL);

            // If calibration is enabled, we can either write directly to the output array, or to the intermediate buffers,
            //    calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, castPtr, tsInOffset);
            // which can then be used to calculate the output data instead of indexing into castPtr.
			if constexpr (calibrateData) {
			    // Move the output calibrated voltages directly into the output array
				calibrateDataFunc<I, O>(&(outputData[0][tsOutOffset]),
				                        &(outputData[0][tsOutOffset + 1]),
				                        &(outputData[0][tsOutOffset + 2]),
				                        &(outputData[0][tsOutOffset + 3]),
				                        beamletJones, castPtr, tsInOffset);

            // Otherwise, do a direct data copy following some known pattern
			} else {
				outputData[0][tsOutOffset] = castPtr[tsInOffset]; // Xr
				outputData[0][tsOutOffset + 1] = castPtr[tsInOffset + 1]; // Xi
				outputData[0][tsOutOffset + 2] = castPtr[tsInOffset + 2]; // Yr
				outputData[0][tsOutOffset + 3] = castPtr[tsInOffset + 3]; // Yi
			}
		}
	}
}
```

4. Include the kernel in the switch statement of `int32_t 
   lofar_udp_raw_loop(lofar_udp_obs_meta *meta)` in `
   [lofar_udp_backends.hpp](../src/lib/lofar_udp_backends.hpp)`

```
else if constexpr (trueState == KERNEL_ENUM_VAL) {
	udp_myNewKernel<I, O, calibrateData>(iLoop, lastInputPacketOffset, timeStepSize....);
}
...

```

5. Go to `[lofar_udp_reader.c](../src/lib/lofar_udp_reader.c)` and find 
   the `int32_t _lofar_udp_setup_processing(lofar_udp_obs_meta *meta)` function. You will
   need to add your mode to two switch statements here. One is a simple fall-through to check that the mode is defned.
   For the second, you'll need to determine the input / output data sizes and add your processing mode to the second
   switch statement. If adding a completely new calculation, be sure to add a `break;` statement afterward, as the
   compiler warning is disabled for this switch statement. In the 
   case of a re-ordering operation, you will just need to
   define the number of output arrays.

6. Go to `[lofar_udp_metadata.c](../src/lib/lofar_udp_metadata.c)`. 
   You will need to add your processing mode to 6 switches in the 
   function `int32_t _lofar_udp_metadata_processing_mode_metadata(lofar_udp_metadata 
   *const metadata)` in order to correctly generate metadata for 
   your output, you will need to know/create:
     - Frequency ordering
     - String representation of data
     - Data dimensions & polarisations
     - Output array selection
     - Output data type (voltage/detected)
     - Output bit size

7. Add documentation to `[README_CLI.md](./README_CLI.md)` and 
   `[lofar_cli_meta.c](../src/CLI/lofar_cli_meta.c)`. to describe 
   your processing mode.

8. Update the tests in `[lib_reader_tests.cpp](..
   /tests/lib_tests/lib_reader_tests.cpp)` to have an independant 
   way of validating the output is as expected.

9. Update the Metadata tests in `[lib_metadata_tests.cpp](..
   /tests/lib_tests/lib_metadata_tests.cpp)` to account for the 
   specifics of your processing mode.