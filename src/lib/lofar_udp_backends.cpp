#include "lofar_udp_backends.hpp"


// Declare the Stokes Vector Functions
#pragma omp declare simd
float stokesI(float Xr, float Xi, float Yr, float Yi) {
	return  (1.0 * (Xr * Xr) + 1.0 * (Xi * Xi) + 1.0 * (Yr * Yr) + 1.0 * (Yi * Yi));
}

#pragma omp declare simd
float stokesQ(float Xr, float Xi, float Yr, float Yi) {
	return  (1.0 * (Xr * Xr) + 1.0 * (Xi * Xi) - 1.0 * (Yr * Yr) - 1.0 * (Yi * Yi));
}

#pragma omp declare simd
float stokesU(float Xr, float Xi, float Yr, float Yi) {
	return  (2.0 * (Xr * Yr) - 3.0 * (Xi * Yi));
}

#pragma omp declare simd
float stokesV(float Xr, float Xi, float Yr, float Yi) {
	return 2.0 * ((Xr * Yi) + (-1.0 * Xi * Yr));
}


// Re-parsing processing modes
int lofar_udp_raw_udp_copy(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_copy\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, signed char, 0>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, signed short, 0>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}


int lofar_udp_raw_udp_copy_nohdr(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_copy_nohdr\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, signed char, 1>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, signed short, 1>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}


int lofar_udp_raw_udp_copy_split_pols(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_copy_split_pol\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, signed char, 2>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, signed short, 2>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}


int lofar_udp_raw_udp_channel_major(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_channel_major\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, signed char, 10>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, signed short, 10>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}


int lofar_udp_raw_udp_channel_major_split_pols(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_channel_major_split_\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, signed char, 11>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, signed short, 11>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}


int lofar_udp_raw_udp_reversed_channel_major(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_reversed_channel_major\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, signed char, 20>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, signed short, 20>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}


int lofar_udp_raw_udp_reversed_channel_major_split_pols(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_reversed_channel_major_split\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, signed char, 21>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, signed short, 21>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}



int lofar_udp_raw_udp_time_major(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_time_major\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, signed char, 30>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, signed short, 30>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}


int lofar_udp_raw_udp_time_major_split_pols(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_time_major_split_pols\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, signed char, 31>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, signed short, 31>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}


int lofar_udp_raw_udp_time_major_dual_pols(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_time_major_dual_pols\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, signed char, 32>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, signed short, 32>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}





// Stokes Processing modes
// Stokes I, Q, U, V each have their own functions, and the option between no, 2x, 4x, 8x and 16x
// 	time decimation on the output.
int lofar_udp_raw_udp_stokesI(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesI\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 100>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 100>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesQ(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesQ\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 110>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 110>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesU(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesU\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 120>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 120>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesV(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesV\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 130>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 130>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesI_sum2(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesI_sum2\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 101>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 101>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesQ_sum2(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesQ_sum2\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 111>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 111>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesU_sum2(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesU_sum2\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 121>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 121>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesV_sum2(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesV_sum2\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 131>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 131>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}


int lofar_udp_raw_udp_stokesI_sum4(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesI_sum4\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 102>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 102>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesQ_sum4(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesQ_sum4\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 112>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 112>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesU_sum4(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesU_sum4\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 122>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 122>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesV_sum4(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesV_sum4\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 132>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 132>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}


int lofar_udp_raw_udp_stokesI_sum8(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesI_sum8\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 103>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 103>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesQ_sum8(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesQ_sum8\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 113>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 113>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesU_sum8(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesU_sum8\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 123>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 123>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesV_sum8(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesV_sum8\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 133>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 133>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesI_sum16(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesI_sum16\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 104>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 104>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesQ_sum16(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesQ_sum16\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 114>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 114>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesU_sum16(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesU_sum16\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 124>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 124>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_stokesV_sum16(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_stokesV_sum16\n"));
	switch(meta->inputBitMode) {
		case 4:
			fprintf(stderr, "4-bit mode is not yet supported, exiting.\n");
			return 1;
		case 8:
			return lofar_udp_raw_loop<signed char, float, 134>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 134>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}
