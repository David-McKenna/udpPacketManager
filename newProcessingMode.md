New Modes
---------

1. Define prototype CPP/C bridge function at the top of `lofar_udp_backends.hpp`

```
int lofar_udp_raw_udp_my_new_kernel(lofar_udp_meta *meta);
```

2. Create the CPP/C bridge in `lofar_udp_backends.cpp`. You will need to pick both a processing mode int enum (any value greater than 0 and not in use by other modes) and an output data format. For copy methods, the output datatype should be the same as the input. Though you can change it, eg to convert to float by using float as the output datatype. 

```
int lofar_udp_raw_udp_my_new_kernel(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_my_new_kernel\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, OUTPUT_DTYPE, KERNEL_ENUM_VAL>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, OUTPUT_DTYPE, KERNEL_ENUM_VAL>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

```

3. Create the task kernel in `lofar_udp_backends.hpp`, following the format of

```
template<typename I, typename O>
void inline udp_myNewKernel(params) {
	...

	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
		tsInOffset = <inIdx>;
		tsOutOffset = <outIdx>;

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			// Copy the value from the input data (char / short) to an output array (arb. format)
			outputData[outputFileIdx][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
			...

			tsInOffset += stepsPerOutputFile * timeStepSize;
			tsOutOffset += nextOffset;
		}
	}
}
```

4. Include the kernel in the switch statement of `int lofar_udp_raw_loop(lofar_udp_meta *meta)`

```
case KERNEL_ENUM_VAL:
	#ifdef __INTEL_COMPILER
	#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
	#endif
	udp_myNewKernel<I, O>(iLoop, lastInputPacketOffset, timeStepSize....);
	break;

```

5. In `lofar_udp_reader.c`, go to `int lofar_udp_setup_processing(lofar_udp_meta *meta)` and add the case for your enum and CPP/C bridge.

```
case KERNEL_ENUM_VAL:
	meta->processFunc = &lofar_udp_raw_udp_my_new_kernel;
	break;
```

6. Staying in `int lofar_udp_setup_processing(lofar_udp_meta *meta)`, run the maths on the input / output data sizes and add your case to the switch statement. If adding a completely new calculation, be sue to add a `break;` statement afterwards, as the compiler warning is disabled for this switch statement. In the case of a re-rodering operation, you will just need to define the number of output arrays.

7. Add documentation to `README_CLI.md` and `lofar_cli_meta.c`.