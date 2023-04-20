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

							/*
							// Non-decimated Stokes
						case 200:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 200, 1>(meta);
						case 210:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 210, 1>(meta);
						case 220:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 220, 1>(meta);
						case 230:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 230, 1>(meta);
						case 250:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 250, 1>(meta);
						case 260:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 260, 1>(meta);



							// Decimated Stokes I
						case 201:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 201, 1>(meta);
						case 202:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 202, 1>(meta);
						case 203:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 203, 1>(meta);
						case 204:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 204, 1>(meta);


							// Decimated Stokes Q
						case 211:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 211, 1>(meta);
						case 212:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 212, 1>(meta);
						case 213:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 213, 1>(meta);
						case 214:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 214, 1>(meta);


							// Decimated Stokes U
						case 221:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 221, 1>(meta);
						case 222:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 222, 1>(meta);
						case 223:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 223, 1>(meta);
						case 224:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 224, 1>(meta);


							// Decimated Stokes V
						case 231:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 231, 1>(meta);
						case 232:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 232, 1>(meta);
						case 233:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 233, 1>(meta);
						case 234:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 234, 1>(meta);

							// Decimated Full Stokes
						case 251:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 251, 1>(meta);
						case 252:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 252, 1>(meta);
						case 253:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 253, 1>(meta);
						case 254:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 254, 1>(meta);


							// Decimated Useful Stokes
						case 261:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 261, 1>(meta);
						case 262:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 262, 1>(meta);
						case 263:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 263, 1>(meta);
						case 264:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 264, 1>(meta);
							*/

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


							/*
							// Non-decimated Stokes
						case 200:
							return lofar_udp_raw_loop<int8_t, float, 200, 1>(meta);
						case 210:
							return lofar_udp_raw_loop<int8_t, float, 210, 1>(meta);
						case 220:
							return lofar_udp_raw_loop<int8_t, float, 220, 1>(meta);
						case 230:
							return lofar_udp_raw_loop<int8_t, float, 230, 1>(meta);
						case 250:
							return lofar_udp_raw_loop<int8_t, float, 250, 1>(meta);
						case 260:
							return lofar_udp_raw_loop<int8_t, float, 260, 1>(meta);



							// Decimated Stokes I
						case 201:
							return lofar_udp_raw_loop<int8_t, float, 201, 1>(meta);
						case 202:
							return lofar_udp_raw_loop<int8_t, float, 202, 1>(meta);
						case 203:
							return lofar_udp_raw_loop<int8_t, float, 203, 1>(meta);
						case 204:
							return lofar_udp_raw_loop<int8_t, float, 204, 1>(meta);


							// Decimated Stokes Q
						case 211:
							return lofar_udp_raw_loop<int8_t, float, 211, 1>(meta);
						case 212:
							return lofar_udp_raw_loop<int8_t, float, 212, 1>(meta);
						case 213:
							return lofar_udp_raw_loop<int8_t, float, 213, 1>(meta);
						case 214:
							return lofar_udp_raw_loop<int8_t, float, 214, 1>(meta);


							// Decimated Stokes U
						case 221:
							return lofar_udp_raw_loop<int8_t, float, 221, 1>(meta);
						case 222:
							return lofar_udp_raw_loop<int8_t, float, 222, 1>(meta);
						case 223:
							return lofar_udp_raw_loop<int8_t, float, 223, 1>(meta);
						case 224:
							return lofar_udp_raw_loop<int8_t, float, 224, 1>(meta);


							// Decimated Stokes V
						case 231:
							return lofar_udp_raw_loop<int8_t, float, 231, 1>(meta);
						case 232:
							return lofar_udp_raw_loop<int8_t, float, 232, 1>(meta);
						case 233:
							return lofar_udp_raw_loop<int8_t, float, 233, 1>(meta);
						case 234:
							return lofar_udp_raw_loop<int8_t, float, 234, 1>(meta);


							// Decimated Full Stokes
						case 251:
							return lofar_udp_raw_loop<int8_t, float, 251, 1>(meta);
						case 252:
							return lofar_udp_raw_loop<int8_t, float, 252, 1>(meta);
						case 253:
							return lofar_udp_raw_loop<int8_t, float, 253, 1>(meta);
						case 254:
							return lofar_udp_raw_loop<int8_t, float, 254, 1>(meta);


							// Decimated Useful Stokes
						case 261:
							return lofar_udp_raw_loop<int8_t, float, 261, 1>(meta);
						case 262:
							return lofar_udp_raw_loop<int8_t, float, 262, 1>(meta);
						case 263:
							return lofar_udp_raw_loop<int8_t, float, 263, 1>(meta);
						case 264:
							return lofar_udp_raw_loop<int8_t, float, 264, 1>(meta);
						*/

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

							/*
							// Non-decimated Stokes
						case 200:
							return lofar_udp_raw_loop<int16_t, float, 200, 1>(meta);
						case 210:
							return lofar_udp_raw_loop<int16_t, float, 210, 1>(meta);
						case 220:
							return lofar_udp_raw_loop<int16_t, float, 220, 1>(meta);
						case 230:
							return lofar_udp_raw_loop<int16_t, float, 230, 1>(meta);
						case 250:
							return lofar_udp_raw_loop<int16_t, float, 250, 1>(meta);
						case 260:
							return lofar_udp_raw_loop<int16_t, float, 260, 1>(meta);



							// Decimated Stokes I
						case 201:
							return lofar_udp_raw_loop<int16_t, float, 201, 1>(meta);
						case 202:
							return lofar_udp_raw_loop<int16_t, float, 202, 1>(meta);
						case 203:
							return lofar_udp_raw_loop<int16_t, float, 203, 1>(meta);
						case 204:
							return lofar_udp_raw_loop<int16_t, float, 204, 1>(meta);


							// Decimated Stokes Q
						case 211:
							return lofar_udp_raw_loop<int16_t, float, 211, 1>(meta);
						case 212:
							return lofar_udp_raw_loop<int16_t, float, 212, 1>(meta);
						case 213:
							return lofar_udp_raw_loop<int16_t, float, 213, 1>(meta);
						case 214:
							return lofar_udp_raw_loop<int16_t, float, 214, 1>(meta);


							// Decimated Stokes U
						case 221:
							return lofar_udp_raw_loop<int16_t, float, 221, 1>(meta);
						case 222:
							return lofar_udp_raw_loop<int16_t, float, 222, 1>(meta);
						case 223:
							return lofar_udp_raw_loop<int16_t, float, 223, 1>(meta);
						case 224:
							return lofar_udp_raw_loop<int16_t, float, 224, 1>(meta);


							// Decimated Stokes V
						case 231:
							return lofar_udp_raw_loop<int16_t, float, 231, 1>(meta);
						case 232:
							return lofar_udp_raw_loop<int16_t, float, 232, 1>(meta);
						case 233:
							return lofar_udp_raw_loop<int16_t, float, 233, 1>(meta);
						case 234:
							return lofar_udp_raw_loop<int16_t, float, 234, 1>(meta);


							// Decimated Full Stokes
						case 251:
							return lofar_udp_raw_loop<int16_t, float, 251, 1>(meta);
						case 252:
							return lofar_udp_raw_loop<int16_t, float, 252, 1>(meta);
						case 253:
							return lofar_udp_raw_loop<int16_t, float, 253, 1>(meta);
						case 254:
							return lofar_udp_raw_loop<int16_t, float, 254, 1>(meta);


							// Decimated Useful Stokes
						case 261:
							return lofar_udp_raw_loop<int16_t, float, 261, 1>(meta);
						case 262:
							return lofar_udp_raw_loop<int16_t, float, 262, 1>(meta);
						case 263:
							return lofar_udp_raw_loop<int16_t, float, 263, 1>(meta);
						case 264:
							return lofar_udp_raw_loop<int16_t, float, 264, 1>(meta);
							*/

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


							/*
							// Non-decimated Stokes
						case 200:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 200, 0>(meta);
						case 210:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 210, 0>(meta);
						case 220:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 220, 0>(meta);
						case 230:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 230, 0>(meta);
						case 250:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 250, 0>(meta);
						case 260:
							return lofar_udp_raw_loop<int8_t, float, 4000 + 260, 0>(meta);


								// Decimated Stokes I
							case 201:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 201, 0>(meta);
							case 202:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 202, 0>(meta);
							case 203:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 203, 0>(meta);
							case 204:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 204, 0>(meta);


								// Decimated Stokes Q
							case 211:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 211, 0>(meta);
							case 212:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 212, 0>(meta);
							case 213:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 213, 0>(meta);
							case 214:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 214, 0>(meta);


								// Decimated Stokes U
							case 221:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 221, 0>(meta);
							case 222:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 222, 0>(meta);
							case 223:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 223, 0>(meta);
							case 224:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 224, 0>(meta);


								// Decimated Stokes V
							case 231:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 231, 0>(meta);
							case 232:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 232, 0>(meta);
							case 233:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 233, 0>(meta);
							case 234:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 234, 0>(meta);

								// Decimated Full Stokes
							case 251:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 251, 0>(meta);
							case 252:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 252, 0>(meta);
							case 253:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 253, 0>(meta);
							case 254:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 254, 0>(meta);


								// Decimated Useful Stokes
							case 261:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 261, 0>(meta);
							case 262:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 262, 0>(meta);
							case 263:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 263, 0>(meta);
							case 264:
								return lofar_udp_raw_loop<int8_t, float, 4000 + 264, 0>(meta);
							*/

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

							/*
								// Non-decimated Stokes
							case 200:
								return lofar_udp_raw_loop<int8_t, float, 200, 0>(meta);
							case 210:
								return lofar_udp_raw_loop<int8_t, float, 210, 0>(meta);
							case 220:
								return lofar_udp_raw_loop<int8_t, float, 220, 0>(meta);
							case 230:
								return lofar_udp_raw_loop<int8_t, float, 230, 0>(meta);
							case 250:
								return lofar_udp_raw_loop<int8_t, float, 250, 0>(meta);
							case 260:
								return lofar_udp_raw_loop<int8_t, float, 260, 0>(meta);



								// Decimated Stokes I
							case 201:
								return lofar_udp_raw_loop<int8_t, float, 201, 0>(meta);
							case 202:
								return lofar_udp_raw_loop<int8_t, float, 202, 0>(meta);
							case 203:
								return lofar_udp_raw_loop<int8_t, float, 203, 0>(meta);
							case 204:
								return lofar_udp_raw_loop<int8_t, float, 204, 0>(meta);


								// Decimated Stokes Q
							case 211:
								return lofar_udp_raw_loop<int8_t, float, 211, 0>(meta);
							case 212:
								return lofar_udp_raw_loop<int8_t, float, 212, 0>(meta);
							case 213:
								return lofar_udp_raw_loop<int8_t, float, 213, 0>(meta);
							case 214:
								return lofar_udp_raw_loop<int8_t, float, 214, 0>(meta);


								// Decimated Stokes U
							case 221:
								return lofar_udp_raw_loop<int8_t, float, 221, 0>(meta);
							case 222:
								return lofar_udp_raw_loop<int8_t, float, 222, 0>(meta);
							case 223:
								return lofar_udp_raw_loop<int8_t, float, 223, 0>(meta);
							case 224:
								return lofar_udp_raw_loop<int8_t, float, 224, 0>(meta);


								// Decimated Stokes V
							case 231:
								return lofar_udp_raw_loop<int8_t, float, 231, 0>(meta);
							case 232:
								return lofar_udp_raw_loop<int8_t, float, 232, 0>(meta);
							case 233:
								return lofar_udp_raw_loop<int8_t, float, 233, 0>(meta);
							case 234:
								return lofar_udp_raw_loop<int8_t, float, 234, 0>(meta);


								// Decimated Full Stokes
							case 251:
								return lofar_udp_raw_loop<int8_t, float, 251, 0>(meta);
							case 252:
								return lofar_udp_raw_loop<int8_t, float, 252, 0>(meta);
							case 253:
								return lofar_udp_raw_loop<int8_t, float, 253, 0>(meta);
							case 254:
								return lofar_udp_raw_loop<int8_t, float, 254, 0>(meta);


								// Decimated Useful Stokes
							case 261:
								return lofar_udp_raw_loop<int8_t, float, 261, 0>(meta);
							case 262:
								return lofar_udp_raw_loop<int8_t, float, 262, 0>(meta);
							case 263:
								return lofar_udp_raw_loop<int8_t, float, 263, 0>(meta);
							case 264:
								return lofar_udp_raw_loop<int8_t, float, 264, 0>(meta);
							*/

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

							/*
								// Non-decimated Stokes
							case 200:
								return lofar_udp_raw_loop<int16_t, float, 200, 0>(meta);
							case 210:
								return lofar_udp_raw_loop<int16_t, float, 210, 0>(meta);
							case 220:
								return lofar_udp_raw_loop<int16_t, float, 220, 0>(meta);
							case 230:
								return lofar_udp_raw_loop<int16_t, float, 230, 0>(meta);
							case 250:
								return lofar_udp_raw_loop<int16_t, float, 250, 0>(meta);
							case 260:
								return lofar_udp_raw_loop<int16_t, float, 260, 0>(meta);



								// Decimated Stokes I
							case 201:
								return lofar_udp_raw_loop<int16_t, float, 201, 0>(meta);
							case 202:
								return lofar_udp_raw_loop<int16_t, float, 202, 0>(meta);
							case 203:
								return lofar_udp_raw_loop<int16_t, float, 203, 0>(meta);
							case 204:
								return lofar_udp_raw_loop<int16_t, float, 204, 0>(meta);


								// Decimated Stokes Q
							case 211:
								return lofar_udp_raw_loop<int16_t, float, 211, 0>(meta);
							case 212:
								return lofar_udp_raw_loop<int16_t, float, 212, 0>(meta);
							case 213:
								return lofar_udp_raw_loop<int16_t, float, 213, 0>(meta);
							case 214:
								return lofar_udp_raw_loop<int16_t, float, 214, 0>(meta);


								// Decimated Stokes U
							case 221:
								return lofar_udp_raw_loop<int16_t, float, 221, 0>(meta);
							case 222:
								return lofar_udp_raw_loop<int16_t, float, 222, 0>(meta);
							case 223:
								return lofar_udp_raw_loop<int16_t, float, 223, 0>(meta);
							case 224:
								return lofar_udp_raw_loop<int16_t, float, 224, 0>(meta);


								// Decimated Stokes V
							case 231:
								return lofar_udp_raw_loop<int16_t, float, 231, 0>(meta);
							case 232:
								return lofar_udp_raw_loop<int16_t, float, 232, 0>(meta);
							case 233:
								return lofar_udp_raw_loop<int16_t, float, 233, 0>(meta);
							case 234:
								return lofar_udp_raw_loop<int16_t, float, 234, 0>(meta);


								// Decimated Full Stokes
							case 251:
								return lofar_udp_raw_loop<int16_t, float, 251, 0>(meta);
							case 252:
								return lofar_udp_raw_loop<int16_t, float, 252, 0>(meta);
							case 253:
								return lofar_udp_raw_loop<int16_t, float, 253, 0>(meta);
							case 254:
								return lofar_udp_raw_loop<int16_t, float, 254, 0>(meta);


								// Decimated Useful Stokes
							case 261:
								return lofar_udp_raw_loop<int16_t, float, 261, 0>(meta);
							case 262:
								return lofar_udp_raw_loop<int16_t, float, 262, 0>(meta);
							case 263:
								return lofar_udp_raw_loop<int16_t, float, 263, 0>(meta);
							case 264:
								return lofar_udp_raw_loop<int16_t, float, 264, 0>(meta);
							*/

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