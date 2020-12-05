#include "lofar_udp_backends.hpp"


// Declare the function to calculate a calibrated sample
// Calculates one output component of:
// [(x_1, y_1i), (x_2, y_2i)] . [(s_1, s_2i)] = J . (X,Y).T
// [(x_3, y_3i), (x_4, y_4i)]   [(s_3, s_4i)]
// 
// ===
// [(x_1 * s_1 - y_1 * s_2 + x_2 * s_3 - y_2 * s_4), i (x_1 * s_2 + y_1 * s_1 + x_2 * s_4 + y_2 * s_3)] = (X_r, X_i)
// [(x_3 * s_1 - y_3 * s_2 + x_4 * s_3 - y_4 * s_4), i (x_3 * s_2 + y_3 * s_1 + x_4 * s_4 + y_4 * s_3)] = (Y_r, Y_i)
#pragma omp declare simd
float calibrateSample(float c_1, float c_2, float c_3, float c_4, float c_5, float c_6, float c_7, float c_8) {
	return (c_1 * c_2) + (c_3 * c_4) + (c_5 * c_6) + (c_7 * c_8);
}

// Declare the Stokes Vector Functions
#pragma omp declare simd
float stokesI(float Xr, float Xi, float Yr, float Yi) {
	return  (Xr * Xr) + (Xi * Xi) + (Yr * Yr) + (Yi * Yi);
}

#pragma omp declare simd
float stokesQ(float Xr, float Xi, float Yr, float Yi) {
	return  ((Xr * Xr) + (Xi * Xi) - (Yr * Yr) - (Yi * Yi));
}

#pragma omp declare simd
float stokesU(float Xr, float Xi, float Yr, float Yi) {
	return  (2.0 * (Xr * Yr) - 3.0 * (Xi * Yi));
}

#pragma omp declare simd
float stokesV(float Xr, float Xi, float Yr, float Yi) {
	return 2.0 * ((Xr * Yi) - (Xi * Yr));
}

// C interface for the C++ loop and kernels
int lofar_udp_cpp_loop_interface(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) printf("Entered C++ call for %d (%d, %d)\n", meta->processingMode, meta->calibrateData, meta->bitMode));

	const int calibrateData = meta->calibrateData;
	const int inputBitMode = meta->inputBitMode;
	const int processingMode = meta->processingMode;
	// Interfaces to calibrateData cases
	if (calibrateData == 1) {
		// Bit-mode dependant inputs
		if (inputBitMode == 4) {
			if (processingMode == 2) {
				return lofar_udp_raw_loop<signed char, float, 4002, 1>(meta);
			

			// Beamlet-major modes
			} else if (processingMode == 10) {
				return lofar_udp_raw_loop<signed char, float, 4010, 1>(meta);
			} else if (processingMode == 11) {
				return lofar_udp_raw_loop<signed char, float, 4011, 1>(meta);
			


			// Reversed Beamlet-major modes
			} else if (processingMode == 20) {
				return lofar_udp_raw_loop<signed char, float, 4020, 1>(meta);
			} else if (processingMode == 21) {
				return lofar_udp_raw_loop<signed char, float, 4021, 1>(meta);
			


			// Time-major modes
			} else if (processingMode == 30) {
				return lofar_udp_raw_loop<signed char, float, 4030, 1>(meta);
			} else if (processingMode == 31) {
				return lofar_udp_raw_loop<signed char, float, 4031, 1>(meta);
			} else if (processingMode == 32) {
				return lofar_udp_raw_loop<signed char, float, 4032, 1>(meta);
			


			// Non-decimated Stokes
			} else if (processingMode == 100) {
				return lofar_udp_raw_loop<signed char, float, 4100, 1>(meta);
			} else if (processingMode == 110) {
				return lofar_udp_raw_loop<signed char, float, 4110, 1>(meta);
			} else if (processingMode == 120) {
				return lofar_udp_raw_loop<signed char, float, 4120, 1>(meta);
			} else if (processingMode == 130) {
				return lofar_udp_raw_loop<signed char, float, 4130, 1>(meta);
			} else if (processingMode == 150) {
				return lofar_udp_raw_loop<signed char, float, 4150, 1>(meta);
			} else if (processingMode == 160) {
				return lofar_udp_raw_loop<signed char, float, 4160, 1>(meta);



			// Decimated Stokes I
			} else if (processingMode == 101) {
				return lofar_udp_raw_loop<signed char, float, 4101, 1>(meta);
			} else if (processingMode == 102) {
				return lofar_udp_raw_loop<signed char, float, 4102, 1>(meta);
			} else if (processingMode == 103) {
				return lofar_udp_raw_loop<signed char, float, 4103, 1>(meta);
			} else if (processingMode == 104) {
				return lofar_udp_raw_loop<signed char, float, 4104, 1>(meta);


			// Deciates Stokes Q
			} else if (processingMode == 111) {
				return lofar_udp_raw_loop<signed char, float, 4111, 1>(meta);
			} else if (processingMode == 112) {
				return lofar_udp_raw_loop<signed char, float, 4112, 1>(meta);
			} else if (processingMode == 113) {
				return lofar_udp_raw_loop<signed char, float, 4113, 1>(meta);
			} else if (processingMode == 114) {
				return lofar_udp_raw_loop<signed char, float, 4114, 1>(meta);


			// Decimated Stokes U
			} else if (processingMode == 121) {
				return lofar_udp_raw_loop<signed char, float, 4121, 1>(meta);
			} else if (processingMode == 122) {
				return lofar_udp_raw_loop<signed char, float, 4122, 1>(meta);
			} else if (processingMode == 123) {
				return lofar_udp_raw_loop<signed char, float, 4123, 1>(meta);
			} else if (processingMode == 124) {
				return lofar_udp_raw_loop<signed char, float, 4124, 1>(meta);


			// Decimated Stokes V
			} else if (processingMode == 131) {
				return lofar_udp_raw_loop<signed char, float, 4131, 1>(meta);
			} else if (processingMode == 132) {
				return lofar_udp_raw_loop<signed char, float, 4132, 1>(meta);
			} else if (processingMode == 133) {
				return lofar_udp_raw_loop<signed char, float, 4133, 1>(meta);
			} else if (processingMode == 134) {
				return lofar_udp_raw_loop<signed char, float, 4134, 1>(meta);

			// Decimated Full Stokes
			} else if (processingMode == 151) {
				return lofar_udp_raw_loop<signed char, float, 4151, 1>(meta);
			} else if (processingMode == 152) {
				return lofar_udp_raw_loop<signed char, float, 4152, 1>(meta);
			} else if (processingMode == 153) {
				return lofar_udp_raw_loop<signed char, float, 4153, 1>(meta);
			} else if (processingMode == 154) {
				return lofar_udp_raw_loop<signed char, float, 4154, 1>(meta);
			

			// Decimated Useful Stokes
			} else if (processingMode == 161) {
				return lofar_udp_raw_loop<signed char, float, 4161, 1>(meta);
			} else if (processingMode == 162) {
				return lofar_udp_raw_loop<signed char, float, 4162, 1>(meta);
			} else if (processingMode == 163) {
				return lofar_udp_raw_loop<signed char, float, 4163, 1>(meta);
			} else if (processingMode == 164) {
				return lofar_udp_raw_loop<signed char, float, 4164, 1>(meta);

			} else {
				fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode, inputBitMode, calibrateData);
				return 1;
			}
			




		} else if (inputBitMode == 8) {
			if (processingMode == 2) {
				return lofar_udp_raw_loop<signed char, float, 2, 1>(meta);
			

			// Beamlet-major modes
			} else if (processingMode == 10) {
				return lofar_udp_raw_loop<signed char, float, 10, 1>(meta);
			} else if (processingMode == 11) {
				return lofar_udp_raw_loop<signed char, float, 11, 1>(meta);
			


			// Reversed Beamlet-major modes
			} else if (processingMode == 20) {
				return lofar_udp_raw_loop<signed char, float, 20, 1>(meta);
			} else if (processingMode == 21) {
				return lofar_udp_raw_loop<signed char, float, 21, 1>(meta);
			


			// Time-major modes
			} else if (processingMode == 30) {
				return lofar_udp_raw_loop<signed char, float, 30, 1>(meta);
			} else if (processingMode == 31) {
				return lofar_udp_raw_loop<signed char, float, 31, 1>(meta);
			} else if (processingMode == 32) {
				return lofar_udp_raw_loop<signed char, float, 32, 1>(meta);
			


			// Non-decimated Stokes
			} else if (processingMode == 100) {
				return lofar_udp_raw_loop<signed char, float, 100, 1>(meta);
			} else if (processingMode == 110) {
				return lofar_udp_raw_loop<signed char, float, 110, 1>(meta);
			} else if (processingMode == 120) {
				return lofar_udp_raw_loop<signed char, float, 120, 1>(meta);
			} else if (processingMode == 130) {
				return lofar_udp_raw_loop<signed char, float, 130, 1>(meta);
			} else if (processingMode == 150) {
				return lofar_udp_raw_loop<signed char, float, 150, 1>(meta);
			} else if (processingMode == 160) {
				return lofar_udp_raw_loop<signed char, float, 160, 1>(meta);



			// Decimated Stokes I
			} else if (processingMode == 101) {
				return lofar_udp_raw_loop<signed char, float, 101, 1>(meta);
			} else if (processingMode == 102) {
				return lofar_udp_raw_loop<signed char, float, 102, 1>(meta);
			} else if (processingMode == 103) {
				return lofar_udp_raw_loop<signed char, float, 103, 1>(meta);
			} else if (processingMode == 104) {
				return lofar_udp_raw_loop<signed char, float, 104, 1>(meta);


			// Deciates Stokes Q
			} else if (processingMode == 111) {
				return lofar_udp_raw_loop<signed char, float, 111, 1>(meta);
			} else if (processingMode == 112) {
				return lofar_udp_raw_loop<signed char, float, 112, 1>(meta);
			} else if (processingMode == 113) {
				return lofar_udp_raw_loop<signed char, float, 113, 1>(meta);
			} else if (processingMode == 114) {
				return lofar_udp_raw_loop<signed char, float, 114, 1>(meta);


			// Decimated Stokes U
			} else if (processingMode == 121) {
				return lofar_udp_raw_loop<signed char, float, 121, 1>(meta);
			} else if (processingMode == 122) {
				return lofar_udp_raw_loop<signed char, float, 122, 1>(meta);
			} else if (processingMode == 123) {
				return lofar_udp_raw_loop<signed char, float, 123, 1>(meta);
			} else if (processingMode == 124) {
				return lofar_udp_raw_loop<signed char, float, 124, 1>(meta);


			// Decimated Stokes V
			} else if (processingMode == 131) {
				return lofar_udp_raw_loop<signed char, float, 131, 1>(meta);
			} else if (processingMode == 132) {
				return lofar_udp_raw_loop<signed char, float, 132, 1>(meta);
			} else if (processingMode == 133) {
				return lofar_udp_raw_loop<signed char, float, 133, 1>(meta);
			} else if (processingMode == 134) {
				return lofar_udp_raw_loop<signed char, float, 134, 1>(meta);


			// Decimated Full Stokes
			} else if (processingMode == 151) {
				return lofar_udp_raw_loop<signed char, float, 151, 1>(meta);
			} else if (processingMode == 152) {
				return lofar_udp_raw_loop<signed char, float, 152, 1>(meta);
			} else if (processingMode == 153) {
				return lofar_udp_raw_loop<signed char, float, 153, 1>(meta);
			} else if (processingMode == 154) {
				return lofar_udp_raw_loop<signed char, float, 154, 1>(meta);
			

			// Decimated Useful Stokes
			} else if (processingMode == 161) {
				return lofar_udp_raw_loop<signed char, float, 161, 1>(meta);
			} else if (processingMode == 162) {
				return lofar_udp_raw_loop<signed char, float, 162, 1>(meta);
			} else if (processingMode == 163) {
				return lofar_udp_raw_loop<signed char, float, 163, 1>(meta);
			} else if (processingMode == 164) {
				return lofar_udp_raw_loop<signed char, float, 164, 1>(meta);

			} else {
				fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode, inputBitMode, calibrateData);
				return 1;
			}





		} else if (inputBitMode == 16) {
			if (processingMode == 2) {
				return lofar_udp_raw_loop<signed char, float, 2, 1>(meta);
			

			// Beamlet-major modes
			} else if (processingMode == 10) {
				return lofar_udp_raw_loop<signed char, float, 10, 1>(meta);
			} else if (processingMode == 11) {
				return lofar_udp_raw_loop<signed char, float, 11, 1>(meta);
			


			// Reversed Beamlet-major modes
			} else if (processingMode == 20) {
				return lofar_udp_raw_loop<signed char, float, 20, 1>(meta);
			} else if (processingMode == 21) {
				return lofar_udp_raw_loop<signed char, float, 21, 1>(meta);
			


			// Time-major modes
			} else if (processingMode == 30) {
				return lofar_udp_raw_loop<signed char, float, 30, 1>(meta);
			} else if (processingMode == 31) {
				return lofar_udp_raw_loop<signed char, float, 31, 1>(meta);
			} else if (processingMode == 32) {
				return lofar_udp_raw_loop<signed char, float, 32, 1>(meta);
			


			// Non-decimated Stokes
			} else if (processingMode == 100) {
				return lofar_udp_raw_loop<signed short, float, 100, 1>(meta);
			} else if (processingMode == 110) {
				return lofar_udp_raw_loop<signed short, float, 110, 1>(meta);
			} else if (processingMode == 120) {
				return lofar_udp_raw_loop<signed short, float, 120, 1>(meta);
			} else if (processingMode == 130) {
				return lofar_udp_raw_loop<signed short, float, 130, 1>(meta);
			} else if (processingMode == 150) {
				return lofar_udp_raw_loop<signed short, float, 150, 1>(meta);
			} else if (processingMode == 160) {
				return lofar_udp_raw_loop<signed short, float, 160, 1>(meta);



			// Decimated Stokes I
			} else if (processingMode == 101) {
				return lofar_udp_raw_loop<signed short, float, 101, 1>(meta);
			} else if (processingMode == 102) {
				return lofar_udp_raw_loop<signed short, float, 102, 1>(meta);
			} else if (processingMode == 103) {
				return lofar_udp_raw_loop<signed short, float, 103, 1>(meta);
			} else if (processingMode == 104) {
				return lofar_udp_raw_loop<signed short, float, 104, 1>(meta);


			// Deciates Stokes Q
			} else if (processingMode == 111) {
				return lofar_udp_raw_loop<signed short, float, 111, 1>(meta);
			} else if (processingMode == 112) {
				return lofar_udp_raw_loop<signed short, float, 112, 1>(meta);
			} else if (processingMode == 113) {
				return lofar_udp_raw_loop<signed short, float, 113, 1>(meta);
			} else if (processingMode == 114) {
				return lofar_udp_raw_loop<signed short, float, 114, 1>(meta);


			// Decimated Stokes U
			} else if (processingMode == 121) {
				return lofar_udp_raw_loop<signed short, float, 121, 1>(meta);
			} else if (processingMode == 122) {
				return lofar_udp_raw_loop<signed short, float, 122, 1>(meta);
			} else if (processingMode == 123) {
				return lofar_udp_raw_loop<signed short, float, 123, 1>(meta);
			} else if (processingMode == 124) {
				return lofar_udp_raw_loop<signed short, float, 124, 1>(meta);


			// Decimated Stokes V
			} else if (processingMode == 131) {
				return lofar_udp_raw_loop<signed short, float, 131, 1>(meta);
			} else if (processingMode == 132) {
				return lofar_udp_raw_loop<signed short, float, 132, 1>(meta);
			} else if (processingMode == 133) {
				return lofar_udp_raw_loop<signed short, float, 133, 1>(meta);
			} else if (processingMode == 134) {
				return lofar_udp_raw_loop<signed short, float, 134, 1>(meta);


			// Decimated Full Stokes
			} else if (processingMode == 151) {
				return lofar_udp_raw_loop<signed short, float, 151, 1>(meta);
			} else if (processingMode == 152) {
				return lofar_udp_raw_loop<signed short, float, 152, 1>(meta);
			} else if (processingMode == 153) {
				return lofar_udp_raw_loop<signed short, float, 153, 1>(meta);
			} else if (processingMode == 154) {
				return lofar_udp_raw_loop<signed short, float, 154, 1>(meta);
			

			// Decimated Useful Stokes
			} else if (processingMode == 161) {
				return lofar_udp_raw_loop<signed short, float, 161, 1>(meta);
			} else if (processingMode == 162) {
				return lofar_udp_raw_loop<signed short, float, 162, 1>(meta);
			} else if (processingMode == 163) {
				return lofar_udp_raw_loop<signed short, float, 163, 1>(meta);
			} else if (processingMode == 164) {
				return lofar_udp_raw_loop<signed short, float, 164, 1>(meta);

			} else {
				fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode, inputBitMode, calibrateData);
				return 1;
			}





		} else {
			fprintf(stderr, "Unexpected bitmode %d (%d, %d). Exiting.\n", inputBitMode, processingMode, calibrateData);
			return 1;
		}


	// Interfaces to raw data interfaces (no calibration)
	} else {
		// Bitmode dependant inputs
		if (inputBitMode == 4) {
			if (processingMode == 0) {
				return lofar_udp_raw_loop<signed char, signed char, 0, 0>(meta);
			} else if (processingMode == 1) {
				return lofar_udp_raw_loop<signed char, signed char, 1, 0>(meta);
			} else if (processingMode == 2) {
				return lofar_udp_raw_loop<signed char, signed char, 4002, 0>(meta);
			

			// Beamlet-major modes
			} else if (processingMode == 10) {
				return lofar_udp_raw_loop<signed char, signed char, 4010, 0>(meta);
			} else if (processingMode == 11) {
				return lofar_udp_raw_loop<signed char, signed char, 4011, 0>(meta);
			


			// Reversed Beamlet-major modes
			} else if (processingMode == 20) {
				return lofar_udp_raw_loop<signed char, signed char, 4020, 0>(meta);
			} else if (processingMode == 21) {
				return lofar_udp_raw_loop<signed char, signed char, 4021, 0>(meta);
			


			// Time-major modes
			} else if (processingMode == 30) {
				return lofar_udp_raw_loop<signed char, signed char, 4030, 0>(meta);
			} else if (processingMode == 31) {
				return lofar_udp_raw_loop<signed char, signed char, 4031, 0>(meta);
			} else if (processingMode == 32) {
				return lofar_udp_raw_loop<signed char, signed char, 4032, 0>(meta);
			


			// Non-decimated Stokes
			} else if (processingMode == 100) {
				return lofar_udp_raw_loop<signed char, float, 4100, 0>(meta);
			} else if (processingMode == 110) {
				return lofar_udp_raw_loop<signed char, float, 4110, 0>(meta);
			} else if (processingMode == 120) {
				return lofar_udp_raw_loop<signed char, float, 4120, 0>(meta);
			} else if (processingMode == 130) {
				return lofar_udp_raw_loop<signed char, float, 4130, 0>(meta);
			} else if (processingMode == 150) {
				return lofar_udp_raw_loop<signed char, float, 4150, 0>(meta);
			} else if (processingMode == 160) {
				return lofar_udp_raw_loop<signed char, float, 4160, 0>(meta);



			// Decimated Stokes I
			} else if (processingMode == 101) {
				return lofar_udp_raw_loop<signed char, float, 4101, 0>(meta);
			} else if (processingMode == 102) {
				return lofar_udp_raw_loop<signed char, float, 4102, 0>(meta);
			} else if (processingMode == 103) {
				return lofar_udp_raw_loop<signed char, float, 4103, 0>(meta);
			} else if (processingMode == 104) {
				return lofar_udp_raw_loop<signed char, float, 4104, 0>(meta);


			// Deciates Stokes Q
			} else if (processingMode == 111) {
				return lofar_udp_raw_loop<signed char, float, 4111, 0>(meta);
			} else if (processingMode == 112) {
				return lofar_udp_raw_loop<signed char, float, 4112, 0>(meta);
			} else if (processingMode == 113) {
				return lofar_udp_raw_loop<signed char, float, 4113, 0>(meta);
			} else if (processingMode == 114) {
				return lofar_udp_raw_loop<signed char, float, 4114, 0>(meta);


			// Decimated Stokes U
			} else if (processingMode == 121) {
				return lofar_udp_raw_loop<signed char, float, 4121, 0>(meta);
			} else if (processingMode == 122) {
				return lofar_udp_raw_loop<signed char, float, 4122, 0>(meta);
			} else if (processingMode == 123) {
				return lofar_udp_raw_loop<signed char, float, 4123, 0>(meta);
			} else if (processingMode == 124) {
				return lofar_udp_raw_loop<signed char, float, 4124, 0>(meta);


			// Decimated Stokes V
			} else if (processingMode == 131) {
				return lofar_udp_raw_loop<signed char, float, 4131, 0>(meta);
			} else if (processingMode == 132) {
				return lofar_udp_raw_loop<signed char, float, 4132, 0>(meta);
			} else if (processingMode == 133) {
				return lofar_udp_raw_loop<signed char, float, 4133, 0>(meta);
			} else if (processingMode == 134) {
				return lofar_udp_raw_loop<signed char, float, 4134, 0>(meta);

			// Decimated Full Stokes
			} else if (processingMode == 151) {
				return lofar_udp_raw_loop<signed char, float, 4151, 0>(meta);
			} else if (processingMode == 152) {
				return lofar_udp_raw_loop<signed char, float, 4152, 0>(meta);
			} else if (processingMode == 153) {
				return lofar_udp_raw_loop<signed char, float, 4153, 0>(meta);
			} else if (processingMode == 154) {
				return lofar_udp_raw_loop<signed char, float, 4154, 0>(meta);
			

			// Decimated Useful Stokes
			} else if (processingMode == 161) {
				return lofar_udp_raw_loop<signed char, float, 4161, 0>(meta);
			} else if (processingMode == 162) {
				return lofar_udp_raw_loop<signed char, float, 4162, 0>(meta);
			} else if (processingMode == 163) {
				return lofar_udp_raw_loop<signed char, float, 4163, 0>(meta);
			} else if (processingMode == 164) {
				return lofar_udp_raw_loop<signed char, float, 4164, 0>(meta);

			} else {
				fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode, inputBitMode, calibrateData);
				return 1;
			}
			




		} else if (inputBitMode == 8) {
			if (processingMode == 0) {
				return lofar_udp_raw_loop<signed char, signed char, 0, 0>(meta);
			} else if (processingMode == 1) {
				return lofar_udp_raw_loop<signed char, signed char, 1, 0>(meta);
			} else if (processingMode == 2) {
				return lofar_udp_raw_loop<signed char, signed char, 4002, 0>(meta);

			// Beamlet-major modes
			} else if (processingMode == 10) {
				return lofar_udp_raw_loop<signed char, signed char, 10, 0>(meta);
			} else if (processingMode == 11) {
				return lofar_udp_raw_loop<signed char, signed char, 11, 0>(meta);
			


			// Reversed Beamlet-major modes
			} else if (processingMode == 20) {
				return lofar_udp_raw_loop<signed char, signed char, 20, 0>(meta);
			} else if (processingMode == 21) {
				return lofar_udp_raw_loop<signed char, signed char, 21, 0>(meta);
			


			// Time-major modes
			} else if (processingMode == 30) {
				return lofar_udp_raw_loop<signed char, signed char, 30, 0>(meta);
			} else if (processingMode == 31) {
				return lofar_udp_raw_loop<signed char, signed char, 31, 0>(meta);
			} else if (processingMode == 32) {
				return lofar_udp_raw_loop<signed char, signed char, 32, 0>(meta);
			


			// Non-decimated Stokes
			} else if (processingMode == 100) {
				return lofar_udp_raw_loop<signed char, float, 100, 0>(meta);
			} else if (processingMode == 110) {
				return lofar_udp_raw_loop<signed char, float, 110, 0>(meta);
			} else if (processingMode == 120) {
				return lofar_udp_raw_loop<signed char, float, 120, 0>(meta);
			} else if (processingMode == 130) {
				return lofar_udp_raw_loop<signed char, float, 130, 0>(meta);
			} else if (processingMode == 150) {
				return lofar_udp_raw_loop<signed char, float, 150, 0>(meta);
			} else if (processingMode == 160) {
				return lofar_udp_raw_loop<signed char, float, 160, 0>(meta);



			// Decimated Stokes I
			} else if (processingMode == 101) {
				return lofar_udp_raw_loop<signed char, float, 101, 0>(meta);
			} else if (processingMode == 102) {
				return lofar_udp_raw_loop<signed char, float, 102, 0>(meta);
			} else if (processingMode == 103) {
				return lofar_udp_raw_loop<signed char, float, 103, 0>(meta);
			} else if (processingMode == 104) {
				return lofar_udp_raw_loop<signed char, float, 104, 0>(meta);


			// Deciates Stokes Q
			} else if (processingMode == 111) {
				return lofar_udp_raw_loop<signed char, float, 111, 0>(meta);
			} else if (processingMode == 112) {
				return lofar_udp_raw_loop<signed char, float, 112, 0>(meta);
			} else if (processingMode == 113) {
				return lofar_udp_raw_loop<signed char, float, 113, 0>(meta);
			} else if (processingMode == 114) {
				return lofar_udp_raw_loop<signed char, float, 114, 0>(meta);


			// Decimated Stokes U
			} else if (processingMode == 121) {
				return lofar_udp_raw_loop<signed char, float, 121, 0>(meta);
			} else if (processingMode == 122) {
				return lofar_udp_raw_loop<signed char, float, 122, 0>(meta);
			} else if (processingMode == 123) {
				return lofar_udp_raw_loop<signed char, float, 123, 0>(meta);
			} else if (processingMode == 124) {
				return lofar_udp_raw_loop<signed char, float, 124, 0>(meta);


			// Decimated Stokes V
			} else if (processingMode == 131) {
				return lofar_udp_raw_loop<signed char, float, 131, 0>(meta);
			} else if (processingMode == 132) {
				return lofar_udp_raw_loop<signed char, float, 132, 0>(meta);
			} else if (processingMode == 133) {
				return lofar_udp_raw_loop<signed char, float, 133, 0>(meta);
			} else if (processingMode == 134) {
				return lofar_udp_raw_loop<signed char, float, 134, 0>(meta);


			// Decimated Full Stokes
			} else if (processingMode == 151) {
				return lofar_udp_raw_loop<signed char, float, 151, 0>(meta);
			} else if (processingMode == 152) {
				return lofar_udp_raw_loop<signed char, float, 152, 0>(meta);
			} else if (processingMode == 153) {
				return lofar_udp_raw_loop<signed char, float, 153, 0>(meta);
			} else if (processingMode == 154) {
				return lofar_udp_raw_loop<signed char, float, 154, 0>(meta);
			

			// Decimated Useful Stokes
			} else if (processingMode == 161) {
				return lofar_udp_raw_loop<signed char, float, 161, 0>(meta);
			} else if (processingMode == 162) {
				return lofar_udp_raw_loop<signed char, float, 162, 0>(meta);
			} else if (processingMode == 163) {
				return lofar_udp_raw_loop<signed char, float, 163, 0>(meta);
			} else if (processingMode == 164) {
				return lofar_udp_raw_loop<signed char, float, 164, 0>(meta);

			} else {
				fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode, inputBitMode, calibrateData);
				return 1;
			}





		} else if (inputBitMode == 16) {
			if (processingMode == 0) {
				return lofar_udp_raw_loop<signed char, signed short, 0, 0>(meta);
			} else if (processingMode == 1) {
				return lofar_udp_raw_loop<signed char, signed short, 1, 0>(meta);
			} else if (processingMode == 2) {
				return lofar_udp_raw_loop<signed char, signed short, 4002, 0>(meta);

			// Beamlet-major modes
			} else if (processingMode == 10) {
				return lofar_udp_raw_loop<signed char, signed short, 10, 0>(meta);
			} else if (processingMode == 11) {
				return lofar_udp_raw_loop<signed char, signed short, 11, 0>(meta);
			


			// Reversed Beamlet-major modes
			} else if (processingMode == 20) {
				return lofar_udp_raw_loop<signed char, signed short, 20, 0>(meta);
			} else if (processingMode == 21) {
				return lofar_udp_raw_loop<signed char, signed short, 21, 0>(meta);
			


			// Time-major modes
			} else if (processingMode == 30) {
				return lofar_udp_raw_loop<signed char, signed short, 30, 0>(meta);
			} else if (processingMode == 31) {
				return lofar_udp_raw_loop<signed char, signed short, 31, 0>(meta);
			} else if (processingMode == 32) {
				return lofar_udp_raw_loop<signed char, signed short, 32, 0>(meta);
			


			// Non-decimated Stokes
			} else if (processingMode == 100) {
				return lofar_udp_raw_loop<signed short, float, 100, 0>(meta);
			} else if (processingMode == 110) {
				return lofar_udp_raw_loop<signed short, float, 110, 0>(meta);
			} else if (processingMode == 120) {
				return lofar_udp_raw_loop<signed short, float, 120, 0>(meta);
			} else if (processingMode == 130) {
				return lofar_udp_raw_loop<signed short, float, 130, 0>(meta);
			} else if (processingMode == 150) {
				return lofar_udp_raw_loop<signed short, float, 150, 0>(meta);
			} else if (processingMode == 160) {
				return lofar_udp_raw_loop<signed short, float, 160, 0>(meta);



			// Decimated Stokes I
			} else if (processingMode == 101) {
				return lofar_udp_raw_loop<signed short, float, 101, 0>(meta);
			} else if (processingMode == 102) {
				return lofar_udp_raw_loop<signed short, float, 102, 0>(meta);
			} else if (processingMode == 103) {
				return lofar_udp_raw_loop<signed short, float, 103, 0>(meta);
			} else if (processingMode == 104) {
				return lofar_udp_raw_loop<signed short, float, 104, 0>(meta);


			// Deciates Stokes Q
			} else if (processingMode == 111) {
				return lofar_udp_raw_loop<signed short, float, 111, 0>(meta);
			} else if (processingMode == 112) {
				return lofar_udp_raw_loop<signed short, float, 112, 0>(meta);
			} else if (processingMode == 113) {
				return lofar_udp_raw_loop<signed short, float, 113, 0>(meta);
			} else if (processingMode == 114) {
				return lofar_udp_raw_loop<signed short, float, 114, 0>(meta);


			// Decimated Stokes U
			} else if (processingMode == 121) {
				return lofar_udp_raw_loop<signed short, float, 121, 0>(meta);
			} else if (processingMode == 122) {
				return lofar_udp_raw_loop<signed short, float, 122, 0>(meta);
			} else if (processingMode == 123) {
				return lofar_udp_raw_loop<signed short, float, 123, 0>(meta);
			} else if (processingMode == 124) {
				return lofar_udp_raw_loop<signed short, float, 124, 0>(meta);


			// Decimated Stokes V
			} else if (processingMode == 131) {
				return lofar_udp_raw_loop<signed short, float, 131, 0>(meta);
			} else if (processingMode == 132) {
				return lofar_udp_raw_loop<signed short, float, 132, 0>(meta);
			} else if (processingMode == 133) {
				return lofar_udp_raw_loop<signed short, float, 133, 0>(meta);
			} else if (processingMode == 134) {
				return lofar_udp_raw_loop<signed short, float, 134, 0>(meta);


			// Decimated Full Stokes
			} else if (processingMode == 151) {
				return lofar_udp_raw_loop<signed short, float, 151, 0>(meta);
			} else if (processingMode == 152) {
				return lofar_udp_raw_loop<signed short, float, 152, 0>(meta);
			} else if (processingMode == 153) {
				return lofar_udp_raw_loop<signed short, float, 153, 0>(meta);
			} else if (processingMode == 154) {
				return lofar_udp_raw_loop<signed short, float, 154, 0>(meta);
			

			// Decimated Useful Stokes
			} else if (processingMode == 161) {
				return lofar_udp_raw_loop<signed short, float, 161, 0>(meta);
			} else if (processingMode == 162) {
				return lofar_udp_raw_loop<signed short, float, 162, 0>(meta);
			} else if (processingMode == 163) {
				return lofar_udp_raw_loop<signed short, float, 163, 0>(meta);
			} else if (processingMode == 164) {
				return lofar_udp_raw_loop<signed short, float, 164, 0>(meta);

			} else {
				fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode, inputBitMode, calibrateData);
				return 1;
			}

		} else {
			fprintf(stderr, "Unexpected bitmode %d (%d, %d). Exiting.\n", inputBitMode, processingMode, calibrateData);
			return 1;
		}
	}
}
// LUT for 4-bit data, faster than re-calculating upper/lower nibble for every sample.
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
