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
			// memcpy does not modify items elementwise, keep at 0
			return lofar_udp_raw_loop<signed char, signed char, 0>(meta);
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
			// memcpy does not modify items elementwise, keep at 1
			return lofar_udp_raw_loop<signed char, signed char, 1>(meta);
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
			return lofar_udp_raw_loop<signed char, signed char, 4002>(meta);
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
			return lofar_udp_raw_loop<signed char, signed char, 4010>(meta);
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
			return lofar_udp_raw_loop<signed char, signed char, 4011>(meta);
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
			return lofar_udp_raw_loop<signed char, signed char, 4020>(meta);
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
			return lofar_udp_raw_loop<signed char, signed char, 4021>(meta);
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
			return lofar_udp_raw_loop<signed char, signed char, 4030>(meta);
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
			return lofar_udp_raw_loop<signed char, signed char, 4031>(meta);
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
			return lofar_udp_raw_loop<signed char, signed char, 4032>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4100>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4110>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4120>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4130>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4101>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4111>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4121>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4131>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4102>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4112>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4122>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4132>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4103>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4113>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4123>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4133>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4104>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4114>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4124>(meta);
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
			return lofar_udp_raw_loop<signed char, float, 4134>(meta);
		case 8:
			return lofar_udp_raw_loop<signed char, float, 134>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 134>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}


int lofar_udp_raw_udp_full_stokes(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_full_stokes\n"));
	switch(meta->inputBitMode) {
		case 4:
			return lofar_udp_raw_loop<signed char, float, 4150>(meta);
		case 8:
			return lofar_udp_raw_loop<signed char, float, 150>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 150>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_full_stokes_sum2(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_full_stokes_sum2\n"));
	switch(meta->inputBitMode) {
		case 4:
			return lofar_udp_raw_loop<signed char, float, 4151>(meta);
		case 8:
			return lofar_udp_raw_loop<signed char, float, 151>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 151>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_full_stokes_sum4(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_full_stokes_sum4\n"));
	switch(meta->inputBitMode) {
		case 4:
			return lofar_udp_raw_loop<signed char, float, 4152>(meta);
		case 8:
			return lofar_udp_raw_loop<signed char, float, 152>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 152>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_full_stokes_sum8(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_full_stokes_sum8\n"));
	switch(meta->inputBitMode) {
		case 4:
			return lofar_udp_raw_loop<signed char, float, 4153>(meta);
		case 8:
			return lofar_udp_raw_loop<signed char, float, 153>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 153>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_full_stokes_sum16(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_full_stokes_sum16\n"));
	switch(meta->inputBitMode) {
		case 4:
			return lofar_udp_raw_loop<signed char, float, 4154>(meta);
		case 8:
			return lofar_udp_raw_loop<signed char, float, 154>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 154>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_useful_stokes(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_useful_stokes\n"));
	switch(meta->inputBitMode) {
		case 4:
			return lofar_udp_raw_loop<signed char, float, 4160>(meta);
		case 8:
			return lofar_udp_raw_loop<signed char, float, 160>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 160>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_useful_stokes_sum2(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_useful_stokes_sum2\n"));
	switch(meta->inputBitMode) {
		case 4:
			return lofar_udp_raw_loop<signed char, float, 4161>(meta);
		case 8:
			return lofar_udp_raw_loop<signed char, float, 161>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 161>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_useful_stokes_sum4(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_useful_stokes_sum4\n"));
	switch(meta->inputBitMode) {
		case 4:
			return lofar_udp_raw_loop<signed char, float, 4162>(meta);
		case 8:
			return lofar_udp_raw_loop<signed char, float, 162>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 162>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_useful_stokes_sum8(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_useful_stokes_sum8\n"));
	switch(meta->inputBitMode) {
		case 4:
			return lofar_udp_raw_loop<signed char, float, 4163>(meta);
		case 8:
			return lofar_udp_raw_loop<signed char, float, 163>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 163>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}

int lofar_udp_raw_udp_useful_stokes_sum16(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for lofar_udp_raw_udp_useful_stokes_sum16\n"));
	switch(meta->inputBitMode) {
		case 4:
			return lofar_udp_raw_loop<signed char, float, 4164>(meta);
		case 8:
			return lofar_udp_raw_loop<signed char, float, 164>(meta);
		case 16:
			return lofar_udp_raw_loop<signed short, float, 164>(meta);

		default:
			fprintf(stderr, "Unexpected bitmode %d. Exiting.\n", meta->inputBitMode);
			return 1;
	}
}


/* LUT for 4-bit data */
const char bitmodeConversion[256][2] = {
		{ 0 , 0 }, { 0 , 1 }, { 0 , 2 }, { 0 , 3 }, { 0 , 4 }, 
		{ 0 , 5 }, { 0 , 6 }, { 0 , 7 }, { 0 , -8 }, { 0 , -7 }, 
		{ 0 , -6 }, { 0 , -5 }, { 0 , -4 }, { 0 , -3 }, { 0 , -2 }, 
		{ 0 , -1 }, { 1 , 0 }, { 1 , 1 }, { 1 , 2 }, { 1 , 3 }, 
		{ 1 , 4 }, { 1 , 5 }, { 1 , 6 }, { 1 , 7 }, { 1 , -8 }, 
		{ 1 , -7 }, { 1 , -6 }, { 1 , -5 }, { 1 , -4 }, { 1 , -3 }, 
		{ 1 , -2 }, { 1 , -1 }, { 2 , 0 }, { 2 , 1 }, { 2 , 2 }, 
		{ 2 , 3 }, { 2 , 4 }, { 2 , 5 }, { 2 , 6 }, { 2 , 7 }, 
		{ 2 , -8 }, { 2 , -7 }, { 2 , -6 }, { 2 , -5 }, { 2 , -4 }, 
		{ 2 , -3 }, { 2 , -2 }, { 2 , -1 }, { 3 , 0 }, { 3 , 1 }, 
		{ 3 , 2 }, { 3 , 3 }, { 3 , 4 }, { 3 , 5 }, { 3 , 6 }, 
		{ 3 , 7 }, { 3 , -8 }, { 3 , -7 }, { 3 , -6 }, { 3 , -5 }, 
		{ 3 , -4 }, { 3 , -3 }, { 3 , -2 }, { 3 , -1 }, { 4 , 0 }, 
		{ 4 , 1 }, { 4 , 2 }, { 4 , 3 }, { 4 , 4 }, { 4 , 5 }, 
		{ 4 , 6 }, { 4 , 7 }, { 4 , -8 }, { 4 , -7 }, { 4 , -6 }, 
		{ 4 , -5 }, { 4 , -4 }, { 4 , -3 }, { 4 , -2 }, { 4 , -1 }, 
		{ 5 , 0 }, { 5 , 1 }, { 5 , 2 }, { 5 , 3 }, { 5 , 4 }, 
		{ 5 , 5 }, { 5 , 6 }, { 5 , 7 }, { 5 , -8 }, { 5 , -7 }, 
		{ 5 , -6 }, { 5 , -5 }, { 5 , -4 }, { 5 , -3 }, { 5 , -2 }, 
		{ 5 , -1 }, { 6 , 0 }, { 6 , 1 }, { 6 , 2 }, { 6 , 3 }, 
		{ 6 , 4 }, { 6 , 5 }, { 6 , 6 }, { 6 , 7 }, { 6 , -8 }, 
		{ 6 , -7 }, { 6 , -6 }, { 6 , -5 }, { 6 , -4 }, { 6 , -3 }, 
		{ 6 , -2 }, { 6 , -1 }, { 7 , 0 }, { 7 , 1 }, { 7 , 2 }, 
		{ 7 , 3 }, { 7 , 4 }, { 7 , 5 }, { 7 , 6 }, { 7 , 7 }, 
		{ 7 , -8 }, { 7 , -7 }, { 7 , -6 }, { 7 , -5 }, { 7 , -4 }, 
		{ 7 , -3 }, { 7 , -2 }, { 7 , -1 }, { -8 , 0 }, { -8 , 1 }, 
		{ -8 , 2 }, { -8 , 3 }, { -8 , 4 }, { -8 , 5 }, { -8 , 6 }, 
		{ -8 , 7 }, { -8 , -8 }, { -8 , -7 }, { -8 , -6 }, { -8 , -5 }, 
		{ -8 , -4 }, { -8 , -3 }, { -8 , -2 }, { -8 , -1 }, { -7 , 0 }, 
		{ -7 , 1 }, { -7 , 2 }, { -7 , 3 }, { -7 , 4 }, { -7 , 5 }, 
		{ -7 , 6 }, { -7 , 7 }, { -7 , -8 }, { -7 , -7 }, { -7 , -6 }, 
		{ -7 , -5 }, { -7 , -4 }, { -7 , -3 }, { -7 , -2 }, { -7 , -1 }, 
		{ -6 , 0 }, { -6 , 1 }, { -6 , 2 }, { -6 , 3 }, { -6 , 4 }, 
		{ -6 , 5 }, { -6 , 6 }, { -6 , 7 }, { -6 , -8 }, { -6 , -7 }, 
		{ -6 , -6 }, { -6 , -5 }, { -6 , -4 }, { -6 , -3 }, { -6 , -2 }, 
		{ -6 , -1 }, { -5 , 0 }, { -5 , 1 }, { -5 , 2 }, { -5 , 3 }, 
		{ -5 , 4 }, { -5 , 5 }, { -5 , 6 }, { -5 , 7 }, { -5 , -8 }, 
		{ -5 , -7 }, { -5 , -6 }, { -5 , -5 }, { -5 , -4 }, { -5 , -3 }, 
		{ -5 , -2 }, { -5 , -1 }, { -4 , 0 }, { -4 , 1 }, { -4 , 2 }, 
		{ -4 , 3 }, { -4 , 4 }, { -4 , 5 }, { -4 , 6 }, { -4 , 7 }, 
		{ -4 , -8 }, { -4 , -7 }, { -4 , -6 }, { -4 , -5 }, { -4 , -4 }, 
		{ -4 , -3 }, { -4 , -2 }, { -4 , -1 }, { -3 , 0 }, { -3 , 1 }, 
		{ -3 , 2 }, { -3 , 3 }, { -3 , 4 }, { -3 , 5 }, { -3 , 6 }, 
		{ -3 , 7 }, { -3 , -8 }, { -3 , -7 }, { -3 , -6 }, { -3 , -5 }, 
		{ -3 , -4 }, { -3 , -3 }, { -3 , -2 }, { -3 , -1 }, { -2 , 0 }, 
		{ -2 , 1 }, { -2 , 2 }, { -2 , 3 }, { -2 , 4 }, { -2 , 5 }, 
		{ -2 , 6 }, { -2 , 7 }, { -2 , -8 }, { -2 , -7 }, { -2 , -6 }, 
		{ -2 , -5 }, { -2 , -4 }, { -2 , -3 }, { -2 , -2 }, { -2 , -1 }, 
		{ -1 , 0 }, { -1 , 1 }, { -1 , 2 }, { -1 , 3 }, { -1 , 4 }, 
		{ -1 , 5 }, { -1 , 6 }, { -1 , 7 }, { -1 , -8 }, { -1 , -7 },
		{ -1 , -6 }, { -1 , -5 }, { -1 , -4 }, { -1 , -3 }, { -1 , -2 }, 
		{ -1 , -1 }

};
