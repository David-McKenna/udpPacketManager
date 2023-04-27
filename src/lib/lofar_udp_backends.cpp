#include "lofar_udp_backends.hpp"


/**
 * @brief      A bridge between the C++ and C components of the codebase, I really wish I could generate this with metaprogramming.
 *
 * @param      meta  The lofar_udp_obs_meta struct
 *
 * @return     0 (Success) / -1 (Packet loss occurred) / 1 (ERROR: Unknown target or illegal configuration)
 */
int32_t lofar_udp_cpp_loop_interface(lofar_udp_obs_meta *meta) {
	VERBOSE(if (meta->VERBOSE) {
		printf("Entered C++ call for %d (%d, %d)\n", meta->processingMode, meta->calibrateData,
		       meta->inputBitMode);
	});

	const calibrate_t calibrateData = meta->calibrateData;
	if (calibrateData > NO_CALIBRATION) {
		// Defaults to -1 -> bump to 0 for first iteration, increase with subsequent iterations
		meta->calibrationStep += 1;
	}

	const int8_t inputBitMode = meta->inputBitMode;
	const processMode_t processingMode = meta->processingMode;
	// Interfaces to calibrateData cases

	// This is a string of 3 nested statements:
	// 		Are we calibrating data?
	// 		What is out input bit mode?
	// 		What is our processing mode?
	// The target template is then used to process the data.
	// See docs/newProcessingMode.md for more details
	switch (calibrateData) {
		case APPLY_CALIBRATION:
			// Bit-mode dependant inputs
			switch (inputBitMode) {
				case 4:
					switch (processingMode) {
						case PACKET_SPLIT_POL:
							return lofar_udp_raw_loop<int8_t, float, 4000 + PACKET_SPLIT_POL, 1>(meta);


							// Beamlet-major modes
						case BEAMLET_MAJOR_FULL:
							return lofar_udp_raw_loop<int8_t, float, 4000 + BEAMLET_MAJOR_FULL, 1>(meta);
						case BEAMLET_MAJOR_SPLIT_POL:
							return lofar_udp_raw_loop<int8_t, float, 4000 + BEAMLET_MAJOR_SPLIT_POL, 1>(meta);



							// Reversed Beamlet-major modes
						case BEAMLET_MAJOR_FULL_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + BEAMLET_MAJOR_FULL_REV, 1>(meta);
						case BEAMLET_MAJOR_SPLIT_POL_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + BEAMLET_MAJOR_SPLIT_POL_REV, 1>(meta);



							// Time-major modes
						case TIME_MAJOR_FULL:
							return lofar_udp_raw_loop<int8_t, float, 4000 + TIME_MAJOR_FULL, 1>(meta);
						case TIME_MAJOR_SPLIT_POL:
							return lofar_udp_raw_loop<int8_t, float, 4000 + TIME_MAJOR_SPLIT_POL, 1>(meta);
						case TIME_MAJOR_ANT_POL:
							return lofar_udp_raw_loop<int8_t, float, 4000 + TIME_MAJOR_ANT_POL, 1>(meta);
						case TIME_MAJOR_ANT_POL_FLOAT:
							return lofar_udp_raw_loop<int8_t, float, 4000 + TIME_MAJOR_ANT_POL_FLOAT, 1>(meta);



							// Non-decimated Stokes
						case STOKES_I:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I, 1>(meta);
						case STOKES_Q:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q, 1>(meta);
						case STOKES_U:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U, 1>(meta);
						case STOKES_V:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V, 1>(meta);
						case STOKES_IQUV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV, 1>(meta);
						case STOKES_IV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV, 1>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS2, 1>(meta);
						case STOKES_I_DS4:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS4, 1>(meta);
						case STOKES_I_DS8:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS8, 1>(meta);
						case STOKES_I_DS16:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS16, 1>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS2, 1>(meta);
						case STOKES_Q_DS4:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS4, 1>(meta);
						case STOKES_Q_DS8:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS8, 1>(meta);
						case STOKES_Q_DS16:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS16, 1>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS2, 1>(meta);
						case STOKES_U_DS4:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS4, 1>(meta);
						case STOKES_U_DS8:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS8, 1>(meta);
						case STOKES_U_DS16:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS16, 1>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS2, 1>(meta);
						case STOKES_V_DS4:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS4, 1>(meta);
						case STOKES_V_DS8:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS8, 1>(meta);
						case STOKES_V_DS16:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS16, 1>(meta);

							// Decimated Full Stokes
						case STOKES_IQUV_DS2:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS2, 1>(meta);
						case STOKES_IQUV_DS4:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS4, 1>(meta);
						case STOKES_IQUV_DS8:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS8, 1>(meta);
						case STOKES_IQUV_DS16:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS16, 1>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS2, 1>(meta);
						case STOKES_IV_DS4:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS4, 1>(meta);
						case STOKES_IV_DS8:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS8, 1>(meta);
						case STOKES_IV_DS16:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS16, 1>(meta);

							// Non-decimated Stokes
						case STOKES_I_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_REV, 1>(meta);
						case STOKES_Q_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_REV, 1>(meta);
						case STOKES_U_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_REV, 1>(meta);
						case STOKES_V_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_REV, 1>(meta);
						case STOKES_IQUV_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_REV, 1>(meta);
						case STOKES_IV_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_REV, 1>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS2_REV, 1>(meta);
						case STOKES_I_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS4_REV, 1>(meta);
						case STOKES_I_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS8_REV, 1>(meta);
						case STOKES_I_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS16_REV, 1>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS2_REV, 1>(meta);
						case STOKES_Q_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS4_REV, 1>(meta);
						case STOKES_Q_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS8_REV, 1>(meta);
						case STOKES_Q_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS16_REV, 1>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS2_REV, 1>(meta);
						case STOKES_U_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS4_REV, 1>(meta);
						case STOKES_U_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS8_REV, 1>(meta);
						case STOKES_U_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS16_REV, 1>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS2_REV, 1>(meta);
						case STOKES_V_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS4_REV, 1>(meta);
						case STOKES_V_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS8_REV, 1>(meta);
						case STOKES_V_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS16_REV, 1>(meta);

							// Decimated Full Stokes
						case STOKES_IQUV_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS2_REV, 1>(meta);
						case STOKES_IQUV_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS4_REV, 1>(meta);
						case STOKES_IQUV_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS8_REV, 1>(meta);
						case STOKES_IQUV_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS16_REV, 1>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS2_REV, 1>(meta);
						case STOKES_IV_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS4_REV, 1>(meta);
						case STOKES_IV_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS8_REV, 1>(meta);
						case STOKES_IV_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS16_REV, 1>(meta);

							// Non-decimated Stokes
						case STOKES_I_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_TIME, 1>(meta);
						case STOKES_Q_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_TIME, 1>(meta);
						case STOKES_U_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_TIME, 1>(meta);
						case STOKES_V_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_TIME, 1>(meta);
						case STOKES_IQUV_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_TIME, 1>(meta);
						case STOKES_IV_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_TIME, 1>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS2_TIME, 1>(meta);
						case STOKES_I_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS4_TIME, 1>(meta);
						case STOKES_I_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS8_TIME, 1>(meta);
						case STOKES_I_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS16_TIME, 1>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS2_TIME, 1>(meta);
						case STOKES_Q_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS4_TIME, 1>(meta);
						case STOKES_Q_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS8_TIME, 1>(meta);
						case STOKES_Q_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS16_TIME, 1>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS2_TIME, 1>(meta);
						case STOKES_U_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS4_TIME, 1>(meta);
						case STOKES_U_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS8_TIME, 1>(meta);
						case STOKES_U_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS16_TIME, 1>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS2_TIME, 1>(meta);
						case STOKES_V_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS4_TIME, 1>(meta);
						case STOKES_V_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS8_TIME, 1>(meta);
						case STOKES_V_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS16_TIME, 1>(meta);

							// Decimated Full Stokes
						case STOKES_IQUV_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS2_TIME, 1>(meta);
						case STOKES_IQUV_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS4_TIME, 1>(meta);
						case STOKES_IQUV_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS8_TIME, 1>(meta);
						case STOKES_IQUV_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS16_TIME, 1>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS2_TIME, 1>(meta);
						case STOKES_IV_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS4_TIME, 1>(meta);
						case STOKES_IV_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS8_TIME, 1>(meta);
						case STOKES_IV_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS16_TIME, 1>(meta);

						default:
							fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode,
							        inputBitMode, calibrateData);
							return 2;
					}


				case 8:
					switch (processingMode) {
						case PACKET_SPLIT_POL:
							return lofar_udp_raw_loop<int8_t, float, PACKET_SPLIT_POL, 1>(meta);


							// Beamlet-major modes
						case BEAMLET_MAJOR_FULL:
							return lofar_udp_raw_loop<int8_t, float, BEAMLET_MAJOR_FULL, 1>(meta);
						case BEAMLET_MAJOR_SPLIT_POL:
							return lofar_udp_raw_loop<int8_t, float, BEAMLET_MAJOR_SPLIT_POL, 1>(meta);



							// Reversed Beamlet-major modes
						case BEAMLET_MAJOR_FULL_REV:
							return lofar_udp_raw_loop<int8_t, float, BEAMLET_MAJOR_FULL_REV, 1>(meta);
						case BEAMLET_MAJOR_SPLIT_POL_REV:
							return lofar_udp_raw_loop<int8_t, float, BEAMLET_MAJOR_SPLIT_POL_REV, 1>(meta);



							// Time-major modes
						case TIME_MAJOR_FULL:
							return lofar_udp_raw_loop<int8_t, float, TIME_MAJOR_FULL, 1>(meta);
						case TIME_MAJOR_SPLIT_POL:
							return lofar_udp_raw_loop<int8_t, float, TIME_MAJOR_SPLIT_POL, 1>(meta);
						case TIME_MAJOR_ANT_POL:
							return lofar_udp_raw_loop<int8_t, float, TIME_MAJOR_ANT_POL, 1>(meta);
						case TIME_MAJOR_ANT_POL_FLOAT:
							return lofar_udp_raw_loop<int8_t, float, TIME_MAJOR_ANT_POL_FLOAT, 1>(meta);



							// Non-decimated Stokes
						case STOKES_I:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I, 1>(meta);
						case STOKES_Q:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q, 1>(meta);
						case STOKES_U:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U, 1>(meta);
						case STOKES_V:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V, 1>(meta);
						case STOKES_IQUV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV, 1>(meta);
						case STOKES_IV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV, 1>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS2, 1>(meta);
						case STOKES_I_DS4:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS4, 1>(meta);
						case STOKES_I_DS8:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS8, 1>(meta);
						case STOKES_I_DS16:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS16, 1>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS2, 1>(meta);
						case STOKES_Q_DS4:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS4, 1>(meta);
						case STOKES_Q_DS8:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS8, 1>(meta);
						case STOKES_Q_DS16:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS16, 1>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS2, 1>(meta);
						case STOKES_U_DS4:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS4, 1>(meta);
						case STOKES_U_DS8:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS8, 1>(meta);
						case STOKES_U_DS16:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS16, 1>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS2, 1>(meta);
						case STOKES_V_DS4:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS4, 1>(meta);
						case STOKES_V_DS8:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS8, 1>(meta);
						case STOKES_V_DS16:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS16, 1>(meta);


							// Decimated Full Stokes
						case STOKES_IQUV_DS2:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS2, 1>(meta);
						case STOKES_IQUV_DS4:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS4, 1>(meta);
						case STOKES_IQUV_DS8:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS8, 1>(meta);
						case STOKES_IQUV_DS16:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS16, 1>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS2, 1>(meta);
						case STOKES_IV_DS4:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS4, 1>(meta);
						case STOKES_IV_DS8:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS8, 1>(meta);
						case STOKES_IV_DS16:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS16, 1>(meta);


							// Non-decimated Stokes
						case STOKES_I_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_REV, 1>(meta);
						case STOKES_Q_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_REV, 1>(meta);
						case STOKES_U_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_REV, 1>(meta);
						case STOKES_V_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_REV, 1>(meta);
						case STOKES_IQUV_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_REV, 1>(meta);
						case STOKES_IV_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_REV, 1>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS2_REV, 1>(meta);
						case STOKES_I_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS4_REV, 1>(meta);
						case STOKES_I_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS8_REV, 1>(meta);
						case STOKES_I_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS16_REV, 1>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS2_REV, 1>(meta);
						case STOKES_Q_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS4_REV, 1>(meta);
						case STOKES_Q_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS8_REV, 1>(meta);
						case STOKES_Q_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS16_REV, 1>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS2_REV, 1>(meta);
						case STOKES_U_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS4_REV, 1>(meta);
						case STOKES_U_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS8_REV, 1>(meta);
						case STOKES_U_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS16_REV, 1>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS2_REV, 1>(meta);
						case STOKES_V_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS4_REV, 1>(meta);
						case STOKES_V_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS8_REV, 1>(meta);
						case STOKES_V_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS16_REV, 1>(meta);


							// Decimated Full Stokes
						case STOKES_IQUV_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS2_REV, 1>(meta);
						case STOKES_IQUV_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS4_REV, 1>(meta);
						case STOKES_IQUV_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS8_REV, 1>(meta);
						case STOKES_IQUV_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS16_REV, 1>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS2_REV, 1>(meta);
						case STOKES_IV_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS4_REV, 1>(meta);
						case STOKES_IV_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS8_REV, 1>(meta);
						case STOKES_IV_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS16_REV, 1>(meta);


							// Non-decimated Stokes
						case STOKES_I_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_TIME, 1>(meta);
						case STOKES_Q_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_TIME, 1>(meta);
						case STOKES_U_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_TIME, 1>(meta);
						case STOKES_V_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_TIME, 1>(meta);
						case STOKES_IQUV_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_TIME, 1>(meta);
						case STOKES_IV_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_TIME, 1>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS2_TIME, 1>(meta);
						case STOKES_I_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS4_TIME, 1>(meta);
						case STOKES_I_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS8_TIME, 1>(meta);
						case STOKES_I_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS16_TIME, 1>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS2_TIME, 1>(meta);
						case STOKES_Q_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS4_TIME, 1>(meta);
						case STOKES_Q_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS8_TIME, 1>(meta);
						case STOKES_Q_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS16_TIME, 1>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS2_TIME, 1>(meta);
						case STOKES_U_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS4_TIME, 1>(meta);
						case STOKES_U_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS8_TIME, 1>(meta);
						case STOKES_U_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS16_TIME, 1>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS2_TIME, 1>(meta);
						case STOKES_V_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS4_TIME, 1>(meta);
						case STOKES_V_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS8_TIME, 1>(meta);
						case STOKES_V_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS16_TIME, 1>(meta);


							// Decimated Full Stokes
						case STOKES_IQUV_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS2_TIME, 1>(meta);
						case STOKES_IQUV_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS4_TIME, 1>(meta);
						case STOKES_IQUV_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS8_TIME, 1>(meta);
						case STOKES_IQUV_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS16_TIME, 1>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS2_TIME, 1>(meta);
						case STOKES_IV_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS4_TIME, 1>(meta);
						case STOKES_IV_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS8_TIME, 1>(meta);
						case STOKES_IV_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS16_TIME, 1>(meta);


						default:
							fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode,
							        inputBitMode, calibrateData);
							return 2;
					}


				case 16:
					switch (processingMode) {
						case PACKET_SPLIT_POL:
							return lofar_udp_raw_loop<int16_t, float, PACKET_SPLIT_POL, 1>(meta);


							// Beamlet-major modes
						case BEAMLET_MAJOR_FULL:
							return lofar_udp_raw_loop<int16_t, float, BEAMLET_MAJOR_FULL, 1>(meta);
						case BEAMLET_MAJOR_SPLIT_POL:
							return lofar_udp_raw_loop<int16_t, float, BEAMLET_MAJOR_SPLIT_POL, 1>(meta);



							// Reversed Beamlet-major modes
						case BEAMLET_MAJOR_FULL_REV:
							return lofar_udp_raw_loop<int16_t, float, BEAMLET_MAJOR_FULL_REV, 1>(meta);
						case BEAMLET_MAJOR_SPLIT_POL_REV:
							return lofar_udp_raw_loop<int16_t, float, BEAMLET_MAJOR_SPLIT_POL_REV, 1>(meta);



							// Time-major modes
						case TIME_MAJOR_FULL:
							return lofar_udp_raw_loop<int16_t, float, TIME_MAJOR_FULL, 1>(meta);
						case TIME_MAJOR_SPLIT_POL:
							return lofar_udp_raw_loop<int16_t, float, TIME_MAJOR_SPLIT_POL, 1>(meta);
						case TIME_MAJOR_ANT_POL:
							return lofar_udp_raw_loop<int16_t, float, TIME_MAJOR_ANT_POL, 1>(meta);
						case TIME_MAJOR_ANT_POL_FLOAT:
							return lofar_udp_raw_loop<int16_t, float, TIME_MAJOR_ANT_POL_FLOAT, 1>(meta);



							// Non-decimated Stokes
						case STOKES_I:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I, 1>(meta);
						case STOKES_Q:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q, 1>(meta);
						case STOKES_U:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U, 1>(meta);
						case STOKES_V:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V, 1>(meta);
						case STOKES_IQUV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV, 1>(meta);
						case STOKES_IV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV, 1>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS2, 1>(meta);
						case STOKES_I_DS4:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS4, 1>(meta);
						case STOKES_I_DS8:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS8, 1>(meta);
						case STOKES_I_DS16:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS16, 1>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS2, 1>(meta);
						case STOKES_Q_DS4:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS4, 1>(meta);
						case STOKES_Q_DS8:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS8, 1>(meta);
						case STOKES_Q_DS16:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS16, 1>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS2, 1>(meta);
						case STOKES_U_DS4:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS4, 1>(meta);
						case STOKES_U_DS8:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS8, 1>(meta);
						case STOKES_U_DS16:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS16, 1>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS2, 1>(meta);
						case STOKES_V_DS4:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS4, 1>(meta);
						case STOKES_V_DS8:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS8, 1>(meta);
						case STOKES_V_DS16:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS16, 1>(meta);


							// Decimated Full Stokes
						case STOKES_IQUV_DS2:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS2, 1>(meta);
						case STOKES_IQUV_DS4:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS4, 1>(meta);
						case STOKES_IQUV_DS8:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS8, 1>(meta);
						case STOKES_IQUV_DS16:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS16, 1>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS2, 1>(meta);
						case STOKES_IV_DS4:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS4, 1>(meta);
						case STOKES_IV_DS8:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS8, 1>(meta);
						case STOKES_IV_DS16:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS16, 1>(meta);

							// Non-decimated Stokes
						case STOKES_I_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_REV, 1>(meta);
						case STOKES_Q_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_REV, 1>(meta);
						case STOKES_U_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_REV, 1>(meta);
						case STOKES_V_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_REV, 1>(meta);
						case STOKES_IQUV_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_REV, 1>(meta);
						case STOKES_IV_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_REV, 1>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS2_REV, 1>(meta);
						case STOKES_I_DS4_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS4_REV, 1>(meta);
						case STOKES_I_DS8_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS8_REV, 1>(meta);
						case STOKES_I_DS16_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS16_REV, 1>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS2_REV, 1>(meta);
						case STOKES_Q_DS4_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS4_REV, 1>(meta);
						case STOKES_Q_DS8_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS8_REV, 1>(meta);
						case STOKES_Q_DS16_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS16_REV, 1>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS2_REV, 1>(meta);
						case STOKES_U_DS4_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS4_REV, 1>(meta);
						case STOKES_U_DS8_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS8_REV, 1>(meta);
						case STOKES_U_DS16_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS16_REV, 1>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS2_REV, 1>(meta);
						case STOKES_V_DS4_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS4_REV, 1>(meta);
						case STOKES_V_DS8_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS8_REV, 1>(meta);
						case STOKES_V_DS16_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS16_REV, 1>(meta);


							// Decimated Full Stokes
						case STOKES_IQUV_DS2_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS2_REV, 1>(meta);
						case STOKES_IQUV_DS4_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS4_REV, 1>(meta);
						case STOKES_IQUV_DS8_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS8_REV, 1>(meta);
						case STOKES_IQUV_DS16_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS16_REV, 1>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS2_REV, 1>(meta);
						case STOKES_IV_DS4_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS4_REV, 1>(meta);
						case STOKES_IV_DS8_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS8_REV, 1>(meta);
						case STOKES_IV_DS16_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS16_REV, 1>(meta);

							// Non-decimated Stokes
						case STOKES_I_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_TIME, 1>(meta);
						case STOKES_Q_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_TIME, 1>(meta);
						case STOKES_U_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_TIME, 1>(meta);
						case STOKES_V_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_TIME, 1>(meta);
						case STOKES_IQUV_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_TIME, 1>(meta);
						case STOKES_IV_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_TIME, 1>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS2_TIME, 1>(meta);
						case STOKES_I_DS4_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS4_TIME, 1>(meta);
						case STOKES_I_DS8_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS8_TIME, 1>(meta);
						case STOKES_I_DS16_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS16_TIME, 1>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS2_TIME, 1>(meta);
						case STOKES_Q_DS4_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS4_TIME, 1>(meta);
						case STOKES_Q_DS8_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS8_TIME, 1>(meta);
						case STOKES_Q_DS16_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS16_TIME, 1>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS2_TIME, 1>(meta);
						case STOKES_U_DS4_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS4_TIME, 1>(meta);
						case STOKES_U_DS8_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS8_TIME, 1>(meta);
						case STOKES_U_DS16_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS16_TIME, 1>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS2_TIME, 1>(meta);
						case STOKES_V_DS4_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS4_TIME, 1>(meta);
						case STOKES_V_DS8_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS8_TIME, 1>(meta);
						case STOKES_V_DS16_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS16_TIME, 1>(meta);


							// Decimated Full Stokes
						case STOKES_IQUV_DS2_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS2_TIME, 1>(meta);
						case STOKES_IQUV_DS4_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS4_TIME, 1>(meta);
						case STOKES_IQUV_DS8_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS8_TIME, 1>(meta);
						case STOKES_IQUV_DS16_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS16_TIME, 1>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS2_TIME, 1>(meta);
						case STOKES_IV_DS4_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS4_TIME, 1>(meta);
						case STOKES_IV_DS8_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS8_TIME, 1>(meta);
						case STOKES_IV_DS16_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS16_TIME, 1>(meta);

						default:
							fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode,
							        inputBitMode, calibrateData);
							return 2;
					}

				default:
					fprintf(stderr, "Unexpected bitmode %d (%d, %d). Exiting.\n", inputBitMode, processingMode,
					        calibrateData);
					return 2;
			}


			// Interfaces to raw data interfaces (no calibration applied in our code)
		case NO_CALIBRATION:
		case GENERATE_JONES:
			// Bitmode dependant inputs
			switch (inputBitMode) {
				case 4:
					switch (processingMode) {
						case PACKET_FULL_COPY:
							return lofar_udp_raw_loop<int8_t, char, 4000 + PACKET_FULL_COPY, 0>(meta);
						case PACKET_NOHDR_COPY:
							return lofar_udp_raw_loop<int8_t, int8_t, 4000 + PACKET_NOHDR_COPY, 0>(meta);
						case PACKET_SPLIT_POL:
							return lofar_udp_raw_loop<int8_t, int8_t, 4000 + PACKET_SPLIT_POL, 0>(meta);


							// Beamlet-major modes
						case BEAMLET_MAJOR_FULL:
							return lofar_udp_raw_loop<int8_t, int8_t, 4000 + BEAMLET_MAJOR_FULL, 0>(meta);
						case BEAMLET_MAJOR_SPLIT_POL:
							return lofar_udp_raw_loop<int8_t, int8_t, 4000 + BEAMLET_MAJOR_SPLIT_POL, 0>(meta);



							// Reversed Beamlet-major modes
						case BEAMLET_MAJOR_FULL_REV:
							return lofar_udp_raw_loop<int8_t, int8_t, 4000 + BEAMLET_MAJOR_FULL_REV, 0>(meta);
						case BEAMLET_MAJOR_SPLIT_POL_REV:
							return lofar_udp_raw_loop<int8_t, int8_t, 4000 + BEAMLET_MAJOR_SPLIT_POL_REV, 0>(meta);



							// Time-major modes
						case TIME_MAJOR_FULL:
							return lofar_udp_raw_loop<int8_t, int8_t, 4000 + TIME_MAJOR_FULL, 0>(meta);
						case TIME_MAJOR_SPLIT_POL:
							return lofar_udp_raw_loop<int8_t, int8_t, 4000 + TIME_MAJOR_SPLIT_POL, 0>(meta);
						case TIME_MAJOR_ANT_POL:
							return lofar_udp_raw_loop<int8_t, int8_t, 4000 + TIME_MAJOR_ANT_POL, 0>(meta);
						case TIME_MAJOR_ANT_POL_FLOAT:
							return lofar_udp_raw_loop<int8_t, float, 4000 + TIME_MAJOR_ANT_POL_FLOAT, 0>(meta);



							// Non-decimated Stokes
						case STOKES_I:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I, 0>(meta);
						case STOKES_Q:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q, 0>(meta);
						case STOKES_U:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U, 0>(meta);
						case STOKES_V:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V, 0>(meta);
						case STOKES_IQUV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV, 0>(meta);
						case STOKES_IV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV, 0>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS2, 0>(meta);
						case STOKES_I_DS4:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS4, 0>(meta);
						case STOKES_I_DS8:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS8, 0>(meta);
						case STOKES_I_DS16:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS16, 0>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS2, 0>(meta);
						case STOKES_Q_DS4:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS4, 0>(meta);
						case STOKES_Q_DS8:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS8, 0>(meta);
						case STOKES_Q_DS16:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS16, 0>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS2, 0>(meta);
						case STOKES_U_DS4:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS4, 0>(meta);
						case STOKES_U_DS8:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS8, 0>(meta);
						case STOKES_U_DS16:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS16, 0>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS2, 0>(meta);
						case STOKES_V_DS4:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS4, 0>(meta);
						case STOKES_V_DS8:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS8, 0>(meta);
						case STOKES_V_DS16:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS16, 0>(meta);

							// Decimated Full Stokes
						case STOKES_IQUV_DS2:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS2, 0>(meta);
						case STOKES_IQUV_DS4:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS4, 0>(meta);
						case STOKES_IQUV_DS8:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS8, 0>(meta);
						case STOKES_IQUV_DS16:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS16, 0>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS2, 0>(meta);
						case STOKES_IV_DS4:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS4, 0>(meta);
						case STOKES_IV_DS8:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS8, 0>(meta);
						case STOKES_IV_DS16:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS16, 0>(meta);


							// Non-decimated Stokes
						case STOKES_I_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_REV, 0>(meta);
						case STOKES_Q_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_REV, 0>(meta);
						case STOKES_U_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_REV, 0>(meta);
						case STOKES_V_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_REV, 0>(meta);
						case STOKES_IQUV_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_REV, 0>(meta);
						case STOKES_IV_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_REV, 0>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS2_REV, 0>(meta);
						case STOKES_I_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS4_REV, 0>(meta);
						case STOKES_I_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS8_REV, 0>(meta);
						case STOKES_I_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS16_REV, 0>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS2_REV, 0>(meta);
						case STOKES_Q_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS4_REV, 0>(meta);
						case STOKES_Q_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS8_REV, 0>(meta);
						case STOKES_Q_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS16_REV, 0>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS2_REV, 0>(meta);
						case STOKES_U_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS4_REV, 0>(meta);
						case STOKES_U_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS8_REV, 0>(meta);
						case STOKES_U_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS16_REV, 0>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS2_REV, 0>(meta);
						case STOKES_V_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS4_REV, 0>(meta);
						case STOKES_V_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS8_REV, 0>(meta);
						case STOKES_V_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS16_REV, 0>(meta);

							// Decimated Full Stokes
						case STOKES_IQUV_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS2_REV, 0>(meta);
						case STOKES_IQUV_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS4_REV, 0>(meta);
						case STOKES_IQUV_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS8_REV, 0>(meta);
						case STOKES_IQUV_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS16_REV, 0>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS2_REV, 0>(meta);
						case STOKES_IV_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS4_REV, 0>(meta);
						case STOKES_IV_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS8_REV, 0>(meta);
						case STOKES_IV_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS16_REV, 0>(meta);



							// Non-decimated Stokes
						case STOKES_I_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_TIME, 0>(meta);
						case STOKES_Q_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_TIME, 0>(meta);
						case STOKES_U_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_TIME, 0>(meta);
						case STOKES_V_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_TIME, 0>(meta);
						case STOKES_IQUV_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_TIME, 0>(meta);
						case STOKES_IV_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_TIME, 0>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS2_TIME, 0>(meta);
						case STOKES_I_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS4_TIME, 0>(meta);
						case STOKES_I_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS8_TIME, 0>(meta);
						case STOKES_I_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_I_DS16_TIME, 0>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS2_TIME, 0>(meta);
						case STOKES_Q_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS4_TIME, 0>(meta);
						case STOKES_Q_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS8_TIME, 0>(meta);
						case STOKES_Q_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_Q_DS16_TIME, 0>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS2_TIME, 0>(meta);
						case STOKES_U_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS4_TIME, 0>(meta);
						case STOKES_U_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS8_TIME, 0>(meta);
						case STOKES_U_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_U_DS16_TIME, 0>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS2_TIME, 0>(meta);
						case STOKES_V_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS4_TIME, 0>(meta);
						case STOKES_V_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS8_TIME, 0>(meta);
						case STOKES_V_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_V_DS16_TIME, 0>(meta);

							// Decimated Full Stokes
						case STOKES_IQUV_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS2_TIME, 0>(meta);
						case STOKES_IQUV_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS4_TIME, 0>(meta);
						case STOKES_IQUV_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS8_TIME, 0>(meta);
						case STOKES_IQUV_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IQUV_DS16_TIME, 0>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS2_TIME, 0>(meta);
						case STOKES_IV_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS4_TIME, 0>(meta);
						case STOKES_IV_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS8_TIME, 0>(meta);
						case STOKES_IV_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, 4000 + STOKES_IV_DS16_TIME, 0>(meta);

						default:
							fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode,
							        inputBitMode, calibrateData);
							return 2;
					}


				case 8:
					switch (processingMode) {
						case PACKET_FULL_COPY:
							return lofar_udp_raw_loop<int8_t, int8_t, PACKET_FULL_COPY, 0>(meta);
						case PACKET_NOHDR_COPY:
							return lofar_udp_raw_loop<int8_t, int8_t, PACKET_NOHDR_COPY, 0>(meta);
						case PACKET_SPLIT_POL:
							return lofar_udp_raw_loop<int8_t, int8_t, PACKET_SPLIT_POL, 0>(meta);

							// Beamlet-major modes
						case BEAMLET_MAJOR_FULL:
							return lofar_udp_raw_loop<int8_t, int8_t, BEAMLET_MAJOR_FULL, 0>(meta);
						case BEAMLET_MAJOR_SPLIT_POL:
							return lofar_udp_raw_loop<int8_t, int8_t, BEAMLET_MAJOR_SPLIT_POL, 0>(meta);



							// Reversed Beamlet-major modes
						case BEAMLET_MAJOR_FULL_REV:
							return lofar_udp_raw_loop<int8_t, int8_t, BEAMLET_MAJOR_FULL_REV, 0>(meta);
						case BEAMLET_MAJOR_SPLIT_POL_REV:
							return lofar_udp_raw_loop<int8_t, int8_t, BEAMLET_MAJOR_SPLIT_POL_REV, 0>(meta);



							// Time-major modes
						case TIME_MAJOR_FULL:
							return lofar_udp_raw_loop<int8_t, int8_t, TIME_MAJOR_FULL, 0>(meta);
						case TIME_MAJOR_SPLIT_POL:
							return lofar_udp_raw_loop<int8_t, int8_t, TIME_MAJOR_SPLIT_POL, 0>(meta);
						case TIME_MAJOR_ANT_POL:
							return lofar_udp_raw_loop<int8_t, int8_t, TIME_MAJOR_ANT_POL, 0>(meta);
						case TIME_MAJOR_ANT_POL_FLOAT:
							return lofar_udp_raw_loop<int8_t, float, TIME_MAJOR_ANT_POL_FLOAT, 0>(meta);



							// Non-decimated Stokes
						case STOKES_I:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I, 0>(meta);
						case STOKES_Q:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q, 0>(meta);
						case STOKES_U:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U, 0>(meta);
						case STOKES_V:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V, 0>(meta);
						case STOKES_IQUV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV, 0>(meta);
						case STOKES_IV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV, 0>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS2, 0>(meta);
						case STOKES_I_DS4:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS4, 0>(meta);
						case STOKES_I_DS8:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS8, 0>(meta);
						case STOKES_I_DS16:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS16, 0>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS2, 0>(meta);
						case STOKES_Q_DS4:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS4, 0>(meta);
						case STOKES_Q_DS8:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS8, 0>(meta);
						case STOKES_Q_DS16:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS16, 0>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS2, 0>(meta);
						case STOKES_U_DS4:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS4, 0>(meta);
						case STOKES_U_DS8:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS8, 0>(meta);
						case STOKES_U_DS16:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS16, 0>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS2, 0>(meta);
						case STOKES_V_DS4:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS4, 0>(meta);
						case STOKES_V_DS8:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS8, 0>(meta);
						case STOKES_V_DS16:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS16, 0>(meta);


							// Decimated Full Stokes
						case STOKES_IQUV_DS2:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS2, 0>(meta);
						case STOKES_IQUV_DS4:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS4, 0>(meta);
						case STOKES_IQUV_DS8:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS8, 0>(meta);
						case STOKES_IQUV_DS16:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS16, 0>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS2, 0>(meta);
						case STOKES_IV_DS4:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS4, 0>(meta);
						case STOKES_IV_DS8:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS8, 0>(meta);
						case STOKES_IV_DS16:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS16, 0>(meta);

							// Non-decimated Stokes
						case STOKES_I_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_REV, 0>(meta);
						case STOKES_Q_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_REV, 0>(meta);
						case STOKES_U_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_REV, 0>(meta);
						case STOKES_V_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_REV, 0>(meta);
						case STOKES_IQUV_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_REV, 0>(meta);
						case STOKES_IV_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_REV, 0>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS2_REV, 0>(meta);
						case STOKES_I_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS4_REV, 0>(meta);
						case STOKES_I_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS8_REV, 0>(meta);
						case STOKES_I_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS16_REV, 0>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS2_REV, 0>(meta);
						case STOKES_Q_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS4_REV, 0>(meta);
						case STOKES_Q_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS8_REV, 0>(meta);
						case STOKES_Q_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS16_REV, 0>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS2_REV, 0>(meta);
						case STOKES_U_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS4_REV, 0>(meta);
						case STOKES_U_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS8_REV, 0>(meta);
						case STOKES_U_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS16_REV, 0>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS2_REV, 0>(meta);
						case STOKES_V_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS4_REV, 0>(meta);
						case STOKES_V_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS8_REV, 0>(meta);
						case STOKES_V_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS16_REV, 0>(meta);


							// Decimated Full Stokes
						case STOKES_IQUV_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS2_REV, 0>(meta);
						case STOKES_IQUV_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS4_REV, 0>(meta);
						case STOKES_IQUV_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS8_REV, 0>(meta);
						case STOKES_IQUV_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS16_REV, 0>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS2_REV, 0>(meta);
						case STOKES_IV_DS4_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS4_REV, 0>(meta);
						case STOKES_IV_DS8_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS8_REV, 0>(meta);
						case STOKES_IV_DS16_REV:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS16_REV, 0>(meta);

							// Non-decimated Stokes
						case STOKES_I_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_TIME, 0>(meta);
						case STOKES_Q_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_TIME, 0>(meta);
						case STOKES_U_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_TIME, 0>(meta);
						case STOKES_V_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_TIME, 0>(meta);
						case STOKES_IQUV_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_TIME, 0>(meta);
						case STOKES_IV_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_TIME, 0>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS2_TIME, 0>(meta);
						case STOKES_I_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS4_TIME, 0>(meta);
						case STOKES_I_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS8_TIME, 0>(meta);
						case STOKES_I_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_I_DS16_TIME, 0>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS2_TIME, 0>(meta);
						case STOKES_Q_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS4_TIME, 0>(meta);
						case STOKES_Q_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS8_TIME, 0>(meta);
						case STOKES_Q_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_Q_DS16_TIME, 0>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS2_TIME, 0>(meta);
						case STOKES_U_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS4_TIME, 0>(meta);
						case STOKES_U_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS8_TIME, 0>(meta);
						case STOKES_U_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_U_DS16_TIME, 0>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS2_TIME, 0>(meta);
						case STOKES_V_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS4_TIME, 0>(meta);
						case STOKES_V_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS8_TIME, 0>(meta);
						case STOKES_V_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_V_DS16_TIME, 0>(meta);


							// Decimated Full Stokes
						case STOKES_IQUV_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS2_TIME, 0>(meta);
						case STOKES_IQUV_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS4_TIME, 0>(meta);
						case STOKES_IQUV_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS8_TIME, 0>(meta);
						case STOKES_IQUV_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IQUV_DS16_TIME, 0>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS2_TIME, 0>(meta);
						case STOKES_IV_DS4_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS4_TIME, 0>(meta);
						case STOKES_IV_DS8_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS8_TIME, 0>(meta);
						case STOKES_IV_DS16_TIME:
							return lofar_udp_raw_loop<int8_t, float, STOKES_IV_DS16_TIME, 0>(meta);

						default:
							fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode,
							        inputBitMode, calibrateData);
							return 2;
					}


				case 16:
					switch (processingMode) {
						case PACKET_FULL_COPY:
							return lofar_udp_raw_loop<int16_t, int16_t, PACKET_FULL_COPY, 0>(meta);
						case PACKET_NOHDR_COPY:
							return lofar_udp_raw_loop<int16_t, int16_t, PACKET_NOHDR_COPY, 0>(meta);
						case PACKET_SPLIT_POL:
							return lofar_udp_raw_loop<int16_t, int16_t, PACKET_SPLIT_POL, 0>(meta);

							// Beamlet-major modes
						case BEAMLET_MAJOR_FULL:
							return lofar_udp_raw_loop<int16_t, int16_t, BEAMLET_MAJOR_FULL, 0>(meta);
						case BEAMLET_MAJOR_SPLIT_POL:
							return lofar_udp_raw_loop<int16_t, int16_t, BEAMLET_MAJOR_SPLIT_POL, 0>(meta);



							// Reversed Beamlet-major modes
						case BEAMLET_MAJOR_FULL_REV:
							return lofar_udp_raw_loop<int16_t, int16_t, BEAMLET_MAJOR_FULL_REV, 0>(meta);
						case BEAMLET_MAJOR_SPLIT_POL_REV:
							return lofar_udp_raw_loop<int16_t, int16_t, BEAMLET_MAJOR_SPLIT_POL_REV, 0>(meta);



							// Time-major modes
						case TIME_MAJOR_FULL:
							return lofar_udp_raw_loop<int16_t, int16_t, TIME_MAJOR_FULL, 0>(meta);
						case TIME_MAJOR_SPLIT_POL:
							return lofar_udp_raw_loop<int16_t, int16_t, TIME_MAJOR_SPLIT_POL, 0>(meta);
						case TIME_MAJOR_ANT_POL:
							return lofar_udp_raw_loop<int16_t, int16_t, TIME_MAJOR_ANT_POL, 0>(meta);
						case TIME_MAJOR_ANT_POL_FLOAT:
							return lofar_udp_raw_loop<int16_t, float, TIME_MAJOR_ANT_POL_FLOAT, 0>(meta);



							// Non-decimated Stokes
						case STOKES_I:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I, 0>(meta);
						case STOKES_Q:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q, 0>(meta);
						case STOKES_U:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U, 0>(meta);
						case STOKES_V:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V, 0>(meta);
						case STOKES_IQUV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV, 0>(meta);
						case STOKES_IV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV, 0>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS2, 0>(meta);
						case STOKES_I_DS4:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS4, 0>(meta);
						case STOKES_I_DS8:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS8, 0>(meta);
						case STOKES_I_DS16:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS16, 0>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS2, 0>(meta);
						case STOKES_Q_DS4:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS4, 0>(meta);
						case STOKES_Q_DS8:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS8, 0>(meta);
						case STOKES_Q_DS16:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS16, 0>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS2, 0>(meta);
						case STOKES_U_DS4:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS4, 0>(meta);
						case STOKES_U_DS8:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS8, 0>(meta);
						case STOKES_U_DS16:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS16, 0>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS2, 0>(meta);
						case STOKES_V_DS4:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS4, 0>(meta);
						case STOKES_V_DS8:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS8, 0>(meta);
						case STOKES_V_DS16:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS16, 0>(meta);


							// Decimated Full Stokes
						case STOKES_IQUV_DS2:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS2, 0>(meta);
						case STOKES_IQUV_DS4:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS4, 0>(meta);
						case STOKES_IQUV_DS8:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS8, 0>(meta);
						case STOKES_IQUV_DS16:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS16, 0>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS2, 0>(meta);
						case STOKES_IV_DS4:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS4, 0>(meta);
						case STOKES_IV_DS8:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS8, 0>(meta);
						case STOKES_IV_DS16:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS16, 0>(meta);


							// Non-decimated Stokes
						case STOKES_I_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_REV, 0>(meta);
						case STOKES_Q_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_REV, 0>(meta);
						case STOKES_U_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_REV, 0>(meta);
						case STOKES_V_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_REV, 0>(meta);
						case STOKES_IQUV_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_REV, 0>(meta);
						case STOKES_IV_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_REV, 0>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS2_REV, 0>(meta);
						case STOKES_I_DS4_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS4_REV, 0>(meta);
						case STOKES_I_DS8_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS8_REV, 0>(meta);
						case STOKES_I_DS16_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS16_REV, 0>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS2_REV, 0>(meta);
						case STOKES_Q_DS4_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS4_REV, 0>(meta);
						case STOKES_Q_DS8_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS8_REV, 0>(meta);
						case STOKES_Q_DS16_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS16_REV, 0>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS2_REV, 0>(meta);
						case STOKES_U_DS4_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS4_REV, 0>(meta);
						case STOKES_U_DS8_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS8_REV, 0>(meta);
						case STOKES_U_DS16_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS16_REV, 0>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS2_REV, 0>(meta);
						case STOKES_V_DS4_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS4_REV, 0>(meta);
						case STOKES_V_DS8_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS8_REV, 0>(meta);
						case STOKES_V_DS16_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS16_REV, 0>(meta);


							// Decimated Full Stokes
						case STOKES_IQUV_DS2_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS2_REV, 0>(meta);
						case STOKES_IQUV_DS4_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS4_REV, 0>(meta);
						case STOKES_IQUV_DS8_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS8_REV, 0>(meta);
						case STOKES_IQUV_DS16_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS16_REV, 0>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS2_REV, 0>(meta);
						case STOKES_IV_DS4_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS4_REV, 0>(meta);
						case STOKES_IV_DS8_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS8_REV, 0>(meta);
						case STOKES_IV_DS16_REV:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS16_REV, 0>(meta);

							// Non-decimated Stokes
						case STOKES_I_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_TIME, 0>(meta);
						case STOKES_Q_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_TIME, 0>(meta);
						case STOKES_U_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_TIME, 0>(meta);
						case STOKES_V_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_TIME, 0>(meta);
						case STOKES_IQUV_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_TIME, 0>(meta);
						case STOKES_IV_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_TIME, 0>(meta);



							// Decimated Stokes I
						case STOKES_I_DS2_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS2_TIME, 0>(meta);
						case STOKES_I_DS4_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS4_TIME, 0>(meta);
						case STOKES_I_DS8_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS8_TIME, 0>(meta);
						case STOKES_I_DS16_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_I_DS16_TIME, 0>(meta);


							// Decimated Stokes Q
						case STOKES_Q_DS2_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS2_TIME, 0>(meta);
						case STOKES_Q_DS4_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS4_TIME, 0>(meta);
						case STOKES_Q_DS8_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS8_TIME, 0>(meta);
						case STOKES_Q_DS16_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_Q_DS16_TIME, 0>(meta);


							// Decimated Stokes U
						case STOKES_U_DS2_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS2_TIME, 0>(meta);
						case STOKES_U_DS4_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS4_TIME, 0>(meta);
						case STOKES_U_DS8_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS8_TIME, 0>(meta);
						case STOKES_U_DS16_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_U_DS16_TIME, 0>(meta);


							// Decimated Stokes V
						case STOKES_V_DS2_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS2_TIME, 0>(meta);
						case STOKES_V_DS4_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS4_TIME, 0>(meta);
						case STOKES_V_DS8_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS8_TIME, 0>(meta);
						case STOKES_V_DS16_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_V_DS16_TIME, 0>(meta);


							// Decimated Full Stokes
						case STOKES_IQUV_DS2_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS2_TIME, 0>(meta);
						case STOKES_IQUV_DS4_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS4_TIME, 0>(meta);
						case STOKES_IQUV_DS8_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS8_TIME, 0>(meta);
						case STOKES_IQUV_DS16_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IQUV_DS16_TIME, 0>(meta);


							// Decimated Useful Stokes
						case STOKES_IV_DS2_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS2_TIME, 0>(meta);
						case STOKES_IV_DS4_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS4_TIME, 0>(meta);
						case STOKES_IV_DS8_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS8_TIME, 0>(meta);
						case STOKES_IV_DS16_TIME:
							return lofar_udp_raw_loop<int16_t, float, STOKES_IV_DS16_TIME, 0>(meta);

						default:
							fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode,
							        inputBitMode, calibrateData);
							return 2;
					}

				default:
					fprintf(stderr, "Unexpected bitmode %d (%d, %d). Exiting.\n", inputBitMode, processingMode,
					        calibrateData);
					return 2;
			}

		default:
			fprintf(stderr, "Unexpected calibration mode (%d). Exiting.\n", calibrateData);
			return 2;
	}
}


// LUT for 4-bit data, faster than re-calculating upper/lower nibble for every sample.
//@formatter:off
const int8_t bitmodeConversion[256][2] = {
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
//@formatter:on