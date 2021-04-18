#include "lofar_udp_backends.hpp"


/**
 * @brief      A bridge between the C++ and C components of the codebase
 *
 * @param      meta  The lofar_udp_meta struct
 *
 * @return     0 (Success) / -1 (Packet loss occurred) / 1 (ERROR: Unknown target or illegal configuration)
 */
int lofar_udp_cpp_loop_interface(lofar_udp_meta *meta) {
	VERBOSE(if (meta->VERBOSE) {
		printf("Entered C++ call for %d (%d, %d)\n", meta->processingMode, meta->calibrateData,
			   meta->inputBitMode);
	});

	int calibrateData = meta->calibrateData;

	const int inputBitMode = meta->inputBitMode;
	const int processingMode = meta->processingMode;
	// Interfaces to calibrateData cases

	// This is a string of 3 nested statements:
	// 		Are we calibrating data?
	// 		What is out input bit mode?
	// 		What is our processing modes?
	// The target template is then used to process the data.
	// See docs/newProcessingMode.md for more details
	switch (calibrateData) {
		case 1:
			// Bit-mode dependant inputs
			switch (inputBitMode) {
				case 4:
					switch (processingMode) {
						case 2:
							return lofar_udp_raw_loop<signed char, float, 4002, 1>(meta);


							// Beamlet-major modes
						case 10:
							return lofar_udp_raw_loop<signed char, float, 4010, 1>(meta);
						case 11:
							return lofar_udp_raw_loop<signed char, float, 4011, 1>(meta);



							// Reversed Beamlet-major modes
						case 20:
							return lofar_udp_raw_loop<signed char, float, 4020, 1>(meta);
						case 21:
							return lofar_udp_raw_loop<signed char, float, 4021, 1>(meta);



							// Time-major modes
						case 30:
							return lofar_udp_raw_loop<signed char, float, 4030, 1>(meta);
						case 31:
							return lofar_udp_raw_loop<signed char, float, 4031, 1>(meta);
						case 32:
							return lofar_udp_raw_loop<signed char, float, 4032, 1>(meta);
						case 35:
							return lofar_udp_raw_loop<signed char, float, 4035, 1>(meta);



							// Non-decimated Stokes
						case 100:
							return lofar_udp_raw_loop<signed char, float, 4100, 1>(meta);
						case 110:
							return lofar_udp_raw_loop<signed char, float, 4110, 1>(meta);
						case 120:
							return lofar_udp_raw_loop<signed char, float, 4120, 1>(meta);
						case 130:
							return lofar_udp_raw_loop<signed char, float, 4130, 1>(meta);
						case 150:
							return lofar_udp_raw_loop<signed char, float, 4150, 1>(meta);
						case 160:
							return lofar_udp_raw_loop<signed char, float, 4160, 1>(meta);



							// Decimated Stokes I
						case 101:
							return lofar_udp_raw_loop<signed char, float, 4101, 1>(meta);
						case 102:
							return lofar_udp_raw_loop<signed char, float, 4102, 1>(meta);
						case 103:
							return lofar_udp_raw_loop<signed char, float, 4103, 1>(meta);
						case 104:
							return lofar_udp_raw_loop<signed char, float, 4104, 1>(meta);


							// Deciates Stokes Q
						case 111:
							return lofar_udp_raw_loop<signed char, float, 4111, 1>(meta);
						case 112:
							return lofar_udp_raw_loop<signed char, float, 4112, 1>(meta);
						case 113:
							return lofar_udp_raw_loop<signed char, float, 4113, 1>(meta);
						case 114:
							return lofar_udp_raw_loop<signed char, float, 4114, 1>(meta);


							// Decimated Stokes U
						case 121:
							return lofar_udp_raw_loop<signed char, float, 4121, 1>(meta);
						case 122:
							return lofar_udp_raw_loop<signed char, float, 4122, 1>(meta);
						case 123:
							return lofar_udp_raw_loop<signed char, float, 4123, 1>(meta);
						case 124:
							return lofar_udp_raw_loop<signed char, float, 4124, 1>(meta);


							// Decimated Stokes V
						case 131:
							return lofar_udp_raw_loop<signed char, float, 4131, 1>(meta);
						case 132:
							return lofar_udp_raw_loop<signed char, float, 4132, 1>(meta);
						case 133:
							return lofar_udp_raw_loop<signed char, float, 4133, 1>(meta);
						case 134:
							return lofar_udp_raw_loop<signed char, float, 4134, 1>(meta);

							// Decimated Full Stokes
						case 151:
							return lofar_udp_raw_loop<signed char, float, 4151, 1>(meta);
						case 152:
							return lofar_udp_raw_loop<signed char, float, 4152, 1>(meta);
						case 153:
							return lofar_udp_raw_loop<signed char, float, 4153, 1>(meta);
						case 154:
							return lofar_udp_raw_loop<signed char, float, 4154, 1>(meta);


							// Decimated Useful Stokes
						case 161:
							return lofar_udp_raw_loop<signed char, float, 4161, 1>(meta);
						case 162:
							return lofar_udp_raw_loop<signed char, float, 4162, 1>(meta);
						case 163:
							return lofar_udp_raw_loop<signed char, float, 4163, 1>(meta);
						case 164:
							return lofar_udp_raw_loop<signed char, float, 4164, 1>(meta);


						// Non-decimated Stokes
						case 200:
							return lofar_udp_raw_loop<signed char, float, 4200, 1>(meta);
						case 210:
							return lofar_udp_raw_loop<signed char, float, 4210, 1>(meta);
						case 220:
							return lofar_udp_raw_loop<signed char, float, 4220, 1>(meta);
						case 230:
							return lofar_udp_raw_loop<signed char, float, 4230, 1>(meta);
						case 250:
							return lofar_udp_raw_loop<signed char, float, 4250, 1>(meta);
						case 260:
							return lofar_udp_raw_loop<signed char, float, 4260, 1>(meta);



							// Decimated Stokes I
						case 201:
							return lofar_udp_raw_loop<signed char, float, 4201, 1>(meta);
						case 202:
							return lofar_udp_raw_loop<signed char, float, 4202, 1>(meta);
						case 203:
							return lofar_udp_raw_loop<signed char, float, 4203, 1>(meta);
						case 204:
							return lofar_udp_raw_loop<signed char, float, 4204, 1>(meta);


							// Deciates Stokes Q
						case 211:
							return lofar_udp_raw_loop<signed char, float, 4211, 1>(meta);
						case 212:
							return lofar_udp_raw_loop<signed char, float, 4212, 1>(meta);
						case 213:
							return lofar_udp_raw_loop<signed char, float, 4213, 1>(meta);
						case 214:
							return lofar_udp_raw_loop<signed char, float, 4214, 1>(meta);


							// Decimated Stokes U
						case 221:
							return lofar_udp_raw_loop<signed char, float, 4221, 1>(meta);
						case 222:
							return lofar_udp_raw_loop<signed char, float, 4222, 1>(meta);
						case 223:
							return lofar_udp_raw_loop<signed char, float, 4223, 1>(meta);
						case 224:
							return lofar_udp_raw_loop<signed char, float, 4224, 1>(meta);


							// Decimated Stokes V
						case 231:
							return lofar_udp_raw_loop<signed char, float, 4231, 1>(meta);
						case 232:
							return lofar_udp_raw_loop<signed char, float, 4232, 1>(meta);
						case 233:
							return lofar_udp_raw_loop<signed char, float, 4233, 1>(meta);
						case 234:
							return lofar_udp_raw_loop<signed char, float, 4234, 1>(meta);

							// Decimated Full Stokes
						case 251:
							return lofar_udp_raw_loop<signed char, float, 4251, 1>(meta);
						case 252:
							return lofar_udp_raw_loop<signed char, float, 4252, 1>(meta);
						case 253:
							return lofar_udp_raw_loop<signed char, float, 4253, 1>(meta);
						case 254:
							return lofar_udp_raw_loop<signed char, float, 4254, 1>(meta);


							// Decimated Useful Stokes
						case 261:
							return lofar_udp_raw_loop<signed char, float, 4261, 1>(meta);
						case 262:
							return lofar_udp_raw_loop<signed char, float, 4262, 1>(meta);
						case 263:
							return lofar_udp_raw_loop<signed char, float, 4263, 1>(meta);
						case 264:
							return lofar_udp_raw_loop<signed char, float, 4264, 1>(meta);

						default:
							fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode,
									inputBitMode, calibrateData);
							return 1;
					}


				case 8:
					switch (processingMode) {
						case 2:
							return lofar_udp_raw_loop<signed char, float, 2, 1>(meta);


							// Beamlet-major modes
						case 10:
							return lofar_udp_raw_loop<signed char, float, 10, 1>(meta);
						case 11:
							return lofar_udp_raw_loop<signed char, float, 11, 1>(meta);



							// Reversed Beamlet-major modes
						case 20:
							return lofar_udp_raw_loop<signed char, float, 20, 1>(meta);
						case 21:
							return lofar_udp_raw_loop<signed char, float, 21, 1>(meta);



							// Time-major modes
						case 30:
							return lofar_udp_raw_loop<signed char, float, 30, 1>(meta);
						case 31:
							return lofar_udp_raw_loop<signed char, float, 31, 1>(meta);
						case 32:
							return lofar_udp_raw_loop<signed char, float, 32, 1>(meta);
						case 35:
							return lofar_udp_raw_loop<signed char, float, 35, 1>(meta);



							// Non-decimated Stokes
						case 100:
							return lofar_udp_raw_loop<signed char, float, 100, 1>(meta);
						case 110:
							return lofar_udp_raw_loop<signed char, float, 110, 1>(meta);
						case 120:
							return lofar_udp_raw_loop<signed char, float, 120, 1>(meta);
						case 130:
							return lofar_udp_raw_loop<signed char, float, 130, 1>(meta);
						case 150:
							return lofar_udp_raw_loop<signed char, float, 150, 1>(meta);
						case 160:
							return lofar_udp_raw_loop<signed char, float, 160, 1>(meta);



							// Decimated Stokes I
						case 101:
							return lofar_udp_raw_loop<signed char, float, 101, 1>(meta);
						case 102:
							return lofar_udp_raw_loop<signed char, float, 102, 1>(meta);
						case 103:
							return lofar_udp_raw_loop<signed char, float, 103, 1>(meta);
						case 104:
							return lofar_udp_raw_loop<signed char, float, 104, 1>(meta);


							// Deciates Stokes Q
						case 111:
							return lofar_udp_raw_loop<signed char, float, 111, 1>(meta);
						case 112:
							return lofar_udp_raw_loop<signed char, float, 112, 1>(meta);
						case 113:
							return lofar_udp_raw_loop<signed char, float, 113, 1>(meta);
						case 114:
							return lofar_udp_raw_loop<signed char, float, 114, 1>(meta);


							// Decimated Stokes U
						case 121:
							return lofar_udp_raw_loop<signed char, float, 121, 1>(meta);
						case 122:
							return lofar_udp_raw_loop<signed char, float, 122, 1>(meta);
						case 123:
							return lofar_udp_raw_loop<signed char, float, 123, 1>(meta);
						case 124:
							return lofar_udp_raw_loop<signed char, float, 124, 1>(meta);


							// Decimated Stokes V
						case 131:
							return lofar_udp_raw_loop<signed char, float, 131, 1>(meta);
						case 132:
							return lofar_udp_raw_loop<signed char, float, 132, 1>(meta);
						case 133:
							return lofar_udp_raw_loop<signed char, float, 133, 1>(meta);
						case 134:
							return lofar_udp_raw_loop<signed char, float, 134, 1>(meta);


							// Decimated Full Stokes
						case 151:
							return lofar_udp_raw_loop<signed char, float, 151, 1>(meta);
						case 152:
							return lofar_udp_raw_loop<signed char, float, 152, 1>(meta);
						case 153:
							return lofar_udp_raw_loop<signed char, float, 153, 1>(meta);
						case 154:
							return lofar_udp_raw_loop<signed char, float, 154, 1>(meta);


							// Decimated Useful Stokes
						case 161:
							return lofar_udp_raw_loop<signed char, float, 161, 1>(meta);
						case 162:
							return lofar_udp_raw_loop<signed char, float, 162, 1>(meta);
						case 163:
							return lofar_udp_raw_loop<signed char, float, 163, 1>(meta);
						case 164:
							return lofar_udp_raw_loop<signed char, float, 164, 1>(meta);


							// Non-decimated Stokes
						case 200:
							return lofar_udp_raw_loop<signed char, float, 200, 1>(meta);
						case 210:
							return lofar_udp_raw_loop<signed char, float, 210, 1>(meta);
						case 220:
							return lofar_udp_raw_loop<signed char, float, 220, 1>(meta);
						case 230:
							return lofar_udp_raw_loop<signed char, float, 230, 1>(meta);
						case 250:
							return lofar_udp_raw_loop<signed char, float, 250, 1>(meta);
						case 260:
							return lofar_udp_raw_loop<signed char, float, 260, 1>(meta);



							// Decimated Stokes I
						case 201:
							return lofar_udp_raw_loop<signed char, float, 201, 1>(meta);
						case 202:
							return lofar_udp_raw_loop<signed char, float, 202, 1>(meta);
						case 203:
							return lofar_udp_raw_loop<signed char, float, 203, 1>(meta);
						case 204:
							return lofar_udp_raw_loop<signed char, float, 204, 1>(meta);


							// Deciates Stokes Q
						case 211:
							return lofar_udp_raw_loop<signed char, float, 211, 1>(meta);
						case 212:
							return lofar_udp_raw_loop<signed char, float, 212, 1>(meta);
						case 213:
							return lofar_udp_raw_loop<signed char, float, 213, 1>(meta);
						case 214:
							return lofar_udp_raw_loop<signed char, float, 214, 1>(meta);


							// Decimated Stokes U
						case 221:
							return lofar_udp_raw_loop<signed char, float, 221, 1>(meta);
						case 222:
							return lofar_udp_raw_loop<signed char, float, 222, 1>(meta);
						case 223:
							return lofar_udp_raw_loop<signed char, float, 223, 1>(meta);
						case 224:
							return lofar_udp_raw_loop<signed char, float, 224, 1>(meta);


							// Decimated Stokes V
						case 231:
							return lofar_udp_raw_loop<signed char, float, 231, 1>(meta);
						case 232:
							return lofar_udp_raw_loop<signed char, float, 232, 1>(meta);
						case 233:
							return lofar_udp_raw_loop<signed char, float, 233, 1>(meta);
						case 234:
							return lofar_udp_raw_loop<signed char, float, 234, 1>(meta);


							// Decimated Full Stokes
						case 251:
							return lofar_udp_raw_loop<signed char, float, 251, 1>(meta);
						case 252:
							return lofar_udp_raw_loop<signed char, float, 252, 1>(meta);
						case 253:
							return lofar_udp_raw_loop<signed char, float, 253, 1>(meta);
						case 254:
							return lofar_udp_raw_loop<signed char, float, 254, 1>(meta);


							// Decimated Useful Stokes
						case 261:
							return lofar_udp_raw_loop<signed char, float, 261, 1>(meta);
						case 262:
							return lofar_udp_raw_loop<signed char, float, 262, 1>(meta);
						case 263:
							return lofar_udp_raw_loop<signed char, float, 263, 1>(meta);
						case 264:
							return lofar_udp_raw_loop<signed char, float, 264, 1>(meta);

						default:
							fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode,
									inputBitMode, calibrateData);
							return 1;
					}


				case 16:
					switch (processingMode) {
						case 2:
							return lofar_udp_raw_loop<signed char, float, 2, 1>(meta);


							// Beamlet-major modes
						case 10:
							return lofar_udp_raw_loop<signed char, float, 10, 1>(meta);
						case 11:
							return lofar_udp_raw_loop<signed char, float, 11, 1>(meta);



							// Reversed Beamlet-major modes
						case 20:
							return lofar_udp_raw_loop<signed char, float, 20, 1>(meta);
						case 21:
							return lofar_udp_raw_loop<signed char, float, 21, 1>(meta);



							// Time-major modes
						case 30:
							return lofar_udp_raw_loop<signed char, float, 30, 1>(meta);
						case 31:
							return lofar_udp_raw_loop<signed char, float, 31, 1>(meta);
						case 32:
							return lofar_udp_raw_loop<signed char, float, 32, 1>(meta);
						case 35:
							return lofar_udp_raw_loop<signed char, float, 35, 1>(meta);



							// Non-decimated Stokes
						case 100:
							return lofar_udp_raw_loop<signed short, float, 100, 1>(meta);
						case 110:
							return lofar_udp_raw_loop<signed short, float, 110, 1>(meta);
						case 120:
							return lofar_udp_raw_loop<signed short, float, 120, 1>(meta);
						case 130:
							return lofar_udp_raw_loop<signed short, float, 130, 1>(meta);
						case 150:
							return lofar_udp_raw_loop<signed short, float, 150, 1>(meta);
						case 160:
							return lofar_udp_raw_loop<signed short, float, 160, 1>(meta);



							// Decimated Stokes I
						case 101:
							return lofar_udp_raw_loop<signed short, float, 101, 1>(meta);
						case 102:
							return lofar_udp_raw_loop<signed short, float, 102, 1>(meta);
						case 103:
							return lofar_udp_raw_loop<signed short, float, 103, 1>(meta);
						case 104:
							return lofar_udp_raw_loop<signed short, float, 104, 1>(meta);


							// Deciates Stokes Q
						case 111:
							return lofar_udp_raw_loop<signed short, float, 111, 1>(meta);
						case 112:
							return lofar_udp_raw_loop<signed short, float, 112, 1>(meta);
						case 113:
							return lofar_udp_raw_loop<signed short, float, 113, 1>(meta);
						case 114:
							return lofar_udp_raw_loop<signed short, float, 114, 1>(meta);


							// Decimated Stokes U
						case 121:
							return lofar_udp_raw_loop<signed short, float, 121, 1>(meta);
						case 122:
							return lofar_udp_raw_loop<signed short, float, 122, 1>(meta);
						case 123:
							return lofar_udp_raw_loop<signed short, float, 123, 1>(meta);
						case 124:
							return lofar_udp_raw_loop<signed short, float, 124, 1>(meta);


							// Decimated Stokes V
						case 131:
							return lofar_udp_raw_loop<signed short, float, 131, 1>(meta);
						case 132:
							return lofar_udp_raw_loop<signed short, float, 132, 1>(meta);
						case 133:
							return lofar_udp_raw_loop<signed short, float, 133, 1>(meta);
						case 134:
							return lofar_udp_raw_loop<signed short, float, 134, 1>(meta);


							// Decimated Full Stokes
						case 151:
							return lofar_udp_raw_loop<signed short, float, 151, 1>(meta);
						case 152:
							return lofar_udp_raw_loop<signed short, float, 152, 1>(meta);
						case 153:
							return lofar_udp_raw_loop<signed short, float, 153, 1>(meta);
						case 154:
							return lofar_udp_raw_loop<signed short, float, 154, 1>(meta);


							// Decimated Useful Stokes
						case 161:
							return lofar_udp_raw_loop<signed short, float, 161, 1>(meta);
						case 162:
							return lofar_udp_raw_loop<signed short, float, 162, 1>(meta);
						case 163:
							return lofar_udp_raw_loop<signed short, float, 163, 1>(meta);
						case 164:
							return lofar_udp_raw_loop<signed short, float, 164, 1>(meta);

								// Non-decimated Stokes
						case 200:
							return lofar_udp_raw_loop<signed short, float, 200, 1>(meta);
						case 210:
							return lofar_udp_raw_loop<signed short, float, 210, 1>(meta);
						case 220:
							return lofar_udp_raw_loop<signed short, float, 220, 1>(meta);
						case 230:
							return lofar_udp_raw_loop<signed short, float, 230, 1>(meta);
						case 250:
							return lofar_udp_raw_loop<signed short, float, 250, 1>(meta);
						case 260:
							return lofar_udp_raw_loop<signed short, float, 260, 1>(meta);



							// Decimated Stokes I
						case 201:
							return lofar_udp_raw_loop<signed short, float, 201, 1>(meta);
						case 202:
							return lofar_udp_raw_loop<signed short, float, 202, 1>(meta);
						case 203:
							return lofar_udp_raw_loop<signed short, float, 203, 1>(meta);
						case 204:
							return lofar_udp_raw_loop<signed short, float, 204, 1>(meta);


							// Deciates Stokes Q
						case 211:
							return lofar_udp_raw_loop<signed short, float, 211, 1>(meta);
						case 212:
							return lofar_udp_raw_loop<signed short, float, 212, 1>(meta);
						case 213:
							return lofar_udp_raw_loop<signed short, float, 213, 1>(meta);
						case 214:
							return lofar_udp_raw_loop<signed short, float, 214, 1>(meta);


							// Decimated Stokes U
						case 221:
							return lofar_udp_raw_loop<signed short, float, 221, 1>(meta);
						case 222:
							return lofar_udp_raw_loop<signed short, float, 222, 1>(meta);
						case 223:
							return lofar_udp_raw_loop<signed short, float, 223, 1>(meta);
						case 224:
							return lofar_udp_raw_loop<signed short, float, 224, 1>(meta);


							// Decimated Stokes V
						case 231:
							return lofar_udp_raw_loop<signed short, float, 231, 1>(meta);
						case 232:
							return lofar_udp_raw_loop<signed short, float, 232, 1>(meta);
						case 233:
							return lofar_udp_raw_loop<signed short, float, 233, 1>(meta);
						case 234:
							return lofar_udp_raw_loop<signed short, float, 234, 1>(meta);


							// Decimated Full Stokes
						case 251:
							return lofar_udp_raw_loop<signed short, float, 251, 1>(meta);
						case 252:
							return lofar_udp_raw_loop<signed short, float, 252, 1>(meta);
						case 253:
							return lofar_udp_raw_loop<signed short, float, 253, 1>(meta);
						case 254:
							return lofar_udp_raw_loop<signed short, float, 254, 1>(meta);


							// Decimated Useful Stokes
						case 261:
							return lofar_udp_raw_loop<signed short, float, 261, 1>(meta);
						case 262:
							return lofar_udp_raw_loop<signed short, float, 262, 1>(meta);
						case 263:
							return lofar_udp_raw_loop<signed short, float, 263, 1>(meta);
						case 264:
							return lofar_udp_raw_loop<signed short, float, 264, 1>(meta);

						default:
							fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode,
									inputBitMode, calibrateData);
							return 1;
					}

				default:
					fprintf(stderr, "Unexpected bitmode %d (%d, %d). Exiting.\n", inputBitMode, processingMode,
							calibrateData);
					return 1;
			}


			// Interfaces to raw data interfaces (no calibration)
		case 0:
			// Bitmode dependant inputs
			switch (inputBitMode) {
				case 4:
					switch (processingMode) {
						case 0:
							return lofar_udp_raw_loop<signed char, signed char, 0, 0>(meta);
						case 1:
							return lofar_udp_raw_loop<signed char, signed char, 1, 0>(meta);
						case 2:
							return lofar_udp_raw_loop<signed char, signed char, 4002, 0>(meta);


							// Beamlet-major modes
						case 10:
							return lofar_udp_raw_loop<signed char, signed char, 4010, 0>(meta);
						case 11:
							return lofar_udp_raw_loop<signed char, signed char, 4011, 0>(meta);



							// Reversed Beamlet-major modes
						case 20:
							return lofar_udp_raw_loop<signed char, signed char, 4020, 0>(meta);
						case 21:
							return lofar_udp_raw_loop<signed char, signed char, 4021, 0>(meta);



							// Time-major modes
						case 30:
							return lofar_udp_raw_loop<signed char, signed char, 4030, 0>(meta);
						case 31:
							return lofar_udp_raw_loop<signed char, signed char, 4031, 0>(meta);
						case 32:
							return lofar_udp_raw_loop<signed char, signed char, 4032, 0>(meta);
						case 35:
							return lofar_udp_raw_loop<signed char, float, 4035, 0>(meta);



							// Non-decimated Stokes
						case 100:
							return lofar_udp_raw_loop<signed char, float, 4100, 0>(meta);
						case 110:
							return lofar_udp_raw_loop<signed char, float, 4110, 0>(meta);
						case 120:
							return lofar_udp_raw_loop<signed char, float, 4120, 0>(meta);
						case 130:
							return lofar_udp_raw_loop<signed char, float, 4130, 0>(meta);
						case 150:
							return lofar_udp_raw_loop<signed char, float, 4150, 0>(meta);
						case 160:
							return lofar_udp_raw_loop<signed char, float, 4160, 0>(meta);



							// Decimated Stokes I
						case 101:
							return lofar_udp_raw_loop<signed char, float, 4101, 0>(meta);
						case 102:
							return lofar_udp_raw_loop<signed char, float, 4102, 0>(meta);
						case 103:
							return lofar_udp_raw_loop<signed char, float, 4103, 0>(meta);
						case 104:
							return lofar_udp_raw_loop<signed char, float, 4104, 0>(meta);


							// Deciates Stokes Q
						case 111:
							return lofar_udp_raw_loop<signed char, float, 4111, 0>(meta);
						case 112:
							return lofar_udp_raw_loop<signed char, float, 4112, 0>(meta);
						case 113:
							return lofar_udp_raw_loop<signed char, float, 4113, 0>(meta);
						case 114:
							return lofar_udp_raw_loop<signed char, float, 4114, 0>(meta);


							// Decimated Stokes U
						case 121:
							return lofar_udp_raw_loop<signed char, float, 4121, 0>(meta);
						case 122:
							return lofar_udp_raw_loop<signed char, float, 4122, 0>(meta);
						case 123:
							return lofar_udp_raw_loop<signed char, float, 4123, 0>(meta);
						case 124:
							return lofar_udp_raw_loop<signed char, float, 4124, 0>(meta);


							// Decimated Stokes V
						case 131:
							return lofar_udp_raw_loop<signed char, float, 4131, 0>(meta);
						case 132:
							return lofar_udp_raw_loop<signed char, float, 4132, 0>(meta);
						case 133:
							return lofar_udp_raw_loop<signed char, float, 4133, 0>(meta);
						case 134:
							return lofar_udp_raw_loop<signed char, float, 4134, 0>(meta);

							// Decimated Full Stokes
						case 151:
							return lofar_udp_raw_loop<signed char, float, 4151, 0>(meta);
						case 152:
							return lofar_udp_raw_loop<signed char, float, 4152, 0>(meta);
						case 153:
							return lofar_udp_raw_loop<signed char, float, 4153, 0>(meta);
						case 154:
							return lofar_udp_raw_loop<signed char, float, 4154, 0>(meta);


							// Decimated Useful Stokes
						case 161:
							return lofar_udp_raw_loop<signed char, float, 4161, 0>(meta);
						case 162:
							return lofar_udp_raw_loop<signed char, float, 4162, 0>(meta);
						case 163:
							return lofar_udp_raw_loop<signed char, float, 4163, 0>(meta);
						case 164:
							return lofar_udp_raw_loop<signed char, float, 4164, 0>(meta);


							// Non-decimated Stokes
						case 200:
							return lofar_udp_raw_loop<signed char, float, 4200, 0>(meta);
						case 210:
							return lofar_udp_raw_loop<signed char, float, 4210, 0>(meta);
						case 220:
							return lofar_udp_raw_loop<signed char, float, 4220, 0>(meta);
						case 230:
							return lofar_udp_raw_loop<signed char, float, 4230, 0>(meta);
						case 250:
							return lofar_udp_raw_loop<signed char, float, 4250, 0>(meta);
						case 260:
							return lofar_udp_raw_loop<signed char, float, 4260, 0>(meta);



							// Decimated Stokes I
						case 201:
							return lofar_udp_raw_loop<signed char, float, 4201, 0>(meta);
						case 202:
							return lofar_udp_raw_loop<signed char, float, 4202, 0>(meta);
						case 203:
							return lofar_udp_raw_loop<signed char, float, 4203, 0>(meta);
						case 204:
							return lofar_udp_raw_loop<signed char, float, 4204, 0>(meta);


							// Deciates Stokes Q
						case 211:
							return lofar_udp_raw_loop<signed char, float, 4211, 0>(meta);
						case 212:
							return lofar_udp_raw_loop<signed char, float, 4212, 0>(meta);
						case 213:
							return lofar_udp_raw_loop<signed char, float, 4213, 0>(meta);
						case 214:
							return lofar_udp_raw_loop<signed char, float, 4214, 0>(meta);


							// Decimated Stokes U
						case 221:
							return lofar_udp_raw_loop<signed char, float, 4221, 0>(meta);
						case 222:
							return lofar_udp_raw_loop<signed char, float, 4222, 0>(meta);
						case 223:
							return lofar_udp_raw_loop<signed char, float, 4223, 0>(meta);
						case 224:
							return lofar_udp_raw_loop<signed char, float, 4224, 0>(meta);


							// Decimated Stokes V
						case 231:
							return lofar_udp_raw_loop<signed char, float, 4231, 0>(meta);
						case 232:
							return lofar_udp_raw_loop<signed char, float, 4232, 0>(meta);
						case 233:
							return lofar_udp_raw_loop<signed char, float, 4233, 0>(meta);
						case 234:
							return lofar_udp_raw_loop<signed char, float, 4234, 0>(meta);

							// Decimated Full Stokes
						case 251:
							return lofar_udp_raw_loop<signed char, float, 4251, 0>(meta);
						case 252:
							return lofar_udp_raw_loop<signed char, float, 4252, 0>(meta);
						case 253:
							return lofar_udp_raw_loop<signed char, float, 4253, 0>(meta);
						case 254:
							return lofar_udp_raw_loop<signed char, float, 4254, 0>(meta);


							// Decimated Useful Stokes
						case 261:
							return lofar_udp_raw_loop<signed char, float, 4261, 0>(meta);
						case 262:
							return lofar_udp_raw_loop<signed char, float, 4262, 0>(meta);
						case 263:
							return lofar_udp_raw_loop<signed char, float, 4263, 0>(meta);
						case 264:
							return lofar_udp_raw_loop<signed char, float, 4264, 0>(meta);

						default:
							fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode,
									inputBitMode, calibrateData);
							return 1;
					}


				case 8:
					switch (processingMode) {
						case 0:
							return lofar_udp_raw_loop<signed char, signed char, 0, 0>(meta);
						case 1:
							return lofar_udp_raw_loop<signed char, signed char, 1, 0>(meta);
						case 2:
							return lofar_udp_raw_loop<signed char, signed char, 4002, 0>(meta);

							// Beamlet-major modes
						case 10:
							return lofar_udp_raw_loop<signed char, signed char, 10, 0>(meta);
						case 11:
							return lofar_udp_raw_loop<signed char, signed char, 11, 0>(meta);



							// Reversed Beamlet-major modes
						case 20:
							return lofar_udp_raw_loop<signed char, signed char, 20, 0>(meta);
						case 21:
							return lofar_udp_raw_loop<signed char, signed char, 21, 0>(meta);



							// Time-major modes
						case 30:
							return lofar_udp_raw_loop<signed char, signed char, 30, 0>(meta);
						case 31:
							return lofar_udp_raw_loop<signed char, signed char, 31, 0>(meta);
						case 32:
							return lofar_udp_raw_loop<signed char, signed char, 32, 0>(meta);
						case 35:
							return lofar_udp_raw_loop<signed char, float, 35, 0>(meta);



							// Non-decimated Stokes
						case 100:
							return lofar_udp_raw_loop<signed char, float, 100, 0>(meta);
						case 110:
							return lofar_udp_raw_loop<signed char, float, 110, 0>(meta);
						case 120:
							return lofar_udp_raw_loop<signed char, float, 120, 0>(meta);
						case 130:
							return lofar_udp_raw_loop<signed char, float, 130, 0>(meta);
						case 150:
							return lofar_udp_raw_loop<signed char, float, 150, 0>(meta);
						case 160:
							return lofar_udp_raw_loop<signed char, float, 160, 0>(meta);



							// Decimated Stokes I
						case 101:
							return lofar_udp_raw_loop<signed char, float, 101, 0>(meta);
						case 102:
							return lofar_udp_raw_loop<signed char, float, 102, 0>(meta);
						case 103:
							return lofar_udp_raw_loop<signed char, float, 103, 0>(meta);
						case 104:
							return lofar_udp_raw_loop<signed char, float, 104, 0>(meta);


							// Deciates Stokes Q
						case 111:
							return lofar_udp_raw_loop<signed char, float, 111, 0>(meta);
						case 112:
							return lofar_udp_raw_loop<signed char, float, 112, 0>(meta);
						case 113:
							return lofar_udp_raw_loop<signed char, float, 113, 0>(meta);
						case 114:
							return lofar_udp_raw_loop<signed char, float, 114, 0>(meta);


							// Decimated Stokes U
						case 121:
							return lofar_udp_raw_loop<signed char, float, 121, 0>(meta);
						case 122:
							return lofar_udp_raw_loop<signed char, float, 122, 0>(meta);
						case 123:
							return lofar_udp_raw_loop<signed char, float, 123, 0>(meta);
						case 124:
							return lofar_udp_raw_loop<signed char, float, 124, 0>(meta);


							// Decimated Stokes V
						case 131:
							return lofar_udp_raw_loop<signed char, float, 131, 0>(meta);
						case 132:
							return lofar_udp_raw_loop<signed char, float, 132, 0>(meta);
						case 133:
							return lofar_udp_raw_loop<signed char, float, 133, 0>(meta);
						case 134:
							return lofar_udp_raw_loop<signed char, float, 134, 0>(meta);


							// Decimated Full Stokes
						case 151:
							return lofar_udp_raw_loop<signed char, float, 151, 0>(meta);
						case 152:
							return lofar_udp_raw_loop<signed char, float, 152, 0>(meta);
						case 153:
							return lofar_udp_raw_loop<signed char, float, 153, 0>(meta);
						case 154:
							return lofar_udp_raw_loop<signed char, float, 154, 0>(meta);


							// Decimated Useful Stokes
						case 161:
							return lofar_udp_raw_loop<signed char, float, 161, 0>(meta);
						case 162:
							return lofar_udp_raw_loop<signed char, float, 162, 0>(meta);
						case 163:
							return lofar_udp_raw_loop<signed char, float, 163, 0>(meta);
						case 164:
							return lofar_udp_raw_loop<signed char, float, 164, 0>(meta);

							// Non-decimated Stokes
						case 200:
							return lofar_udp_raw_loop<signed char, float, 200, 0>(meta);
						case 210:
							return lofar_udp_raw_loop<signed char, float, 210, 0>(meta);
						case 220:
							return lofar_udp_raw_loop<signed char, float, 220, 0>(meta);
						case 230:
							return lofar_udp_raw_loop<signed char, float, 230, 0>(meta);
						case 250:
							return lofar_udp_raw_loop<signed char, float, 250, 0>(meta);
						case 260:
							return lofar_udp_raw_loop<signed char, float, 260, 0>(meta);



							// Decimated Stokes I
						case 201:
							return lofar_udp_raw_loop<signed char, float, 201, 0>(meta);
						case 202:
							return lofar_udp_raw_loop<signed char, float, 202, 0>(meta);
						case 203:
							return lofar_udp_raw_loop<signed char, float, 203, 0>(meta);
						case 204:
							return lofar_udp_raw_loop<signed char, float, 204, 0>(meta);


							// Deciates Stokes Q
						case 211:
							return lofar_udp_raw_loop<signed char, float, 211, 0>(meta);
						case 212:
							return lofar_udp_raw_loop<signed char, float, 212, 0>(meta);
						case 213:
							return lofar_udp_raw_loop<signed char, float, 213, 0>(meta);
						case 214:
							return lofar_udp_raw_loop<signed char, float, 214, 0>(meta);


							// Decimated Stokes U
						case 221:
							return lofar_udp_raw_loop<signed char, float, 221, 0>(meta);
						case 222:
							return lofar_udp_raw_loop<signed char, float, 222, 0>(meta);
						case 223:
							return lofar_udp_raw_loop<signed char, float, 223, 0>(meta);
						case 224:
							return lofar_udp_raw_loop<signed char, float, 224, 0>(meta);


							// Decimated Stokes V
						case 231:
							return lofar_udp_raw_loop<signed char, float, 231, 0>(meta);
						case 232:
							return lofar_udp_raw_loop<signed char, float, 232, 0>(meta);
						case 233:
							return lofar_udp_raw_loop<signed char, float, 233, 0>(meta);
						case 234:
							return lofar_udp_raw_loop<signed char, float, 234, 0>(meta);


							// Decimated Full Stokes
						case 251:
							return lofar_udp_raw_loop<signed char, float, 251, 0>(meta);
						case 252:
							return lofar_udp_raw_loop<signed char, float, 252, 0>(meta);
						case 253:
							return lofar_udp_raw_loop<signed char, float, 253, 0>(meta);
						case 254:
							return lofar_udp_raw_loop<signed char, float, 254, 0>(meta);


							// Decimated Useful Stokes
						case 261:
							return lofar_udp_raw_loop<signed char, float, 261, 0>(meta);
						case 262:
							return lofar_udp_raw_loop<signed char, float, 262, 0>(meta);
						case 263:
							return lofar_udp_raw_loop<signed char, float, 263, 0>(meta);
						case 264:
							return lofar_udp_raw_loop<signed char, float, 264, 0>(meta);

						default:
							fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode,
									inputBitMode, calibrateData);
							return 1;
					}


				case 16:
					switch (processingMode) {
						case 0:
							return lofar_udp_raw_loop<signed char, signed short, 0, 0>(meta);
						case 1:
							return lofar_udp_raw_loop<signed char, signed short, 1, 0>(meta);
						case 2:
							return lofar_udp_raw_loop<signed char, signed short, 4002, 0>(meta);

							// Beamlet-major modes
						case 10:
							return lofar_udp_raw_loop<signed char, signed short, 10, 0>(meta);
						case 11:
							return lofar_udp_raw_loop<signed char, signed short, 11, 0>(meta);



							// Reversed Beamlet-major modes
						case 20:
							return lofar_udp_raw_loop<signed char, signed short, 20, 0>(meta);
						case 21:
							return lofar_udp_raw_loop<signed char, signed short, 21, 0>(meta);



							// Time-major modes
						case 30:
							return lofar_udp_raw_loop<signed char, signed short, 30, 0>(meta);
						case 31:
							return lofar_udp_raw_loop<signed char, signed short, 31, 0>(meta);
						case 32:
							return lofar_udp_raw_loop<signed char, signed short, 32, 0>(meta);
						case 35:
							return lofar_udp_raw_loop<signed char, float, 35, 0>(meta);



							// Non-decimated Stokes
						case 100:
							return lofar_udp_raw_loop<signed short, float, 100, 0>(meta);
						case 110:
							return lofar_udp_raw_loop<signed short, float, 110, 0>(meta);
						case 120:
							return lofar_udp_raw_loop<signed short, float, 120, 0>(meta);
						case 130:
							return lofar_udp_raw_loop<signed short, float, 130, 0>(meta);
						case 150:
							return lofar_udp_raw_loop<signed short, float, 150, 0>(meta);
						case 160:
							return lofar_udp_raw_loop<signed short, float, 160, 0>(meta);



							// Decimated Stokes I
						case 101:
							return lofar_udp_raw_loop<signed short, float, 101, 0>(meta);
						case 102:
							return lofar_udp_raw_loop<signed short, float, 102, 0>(meta);
						case 103:
							return lofar_udp_raw_loop<signed short, float, 103, 0>(meta);
						case 104:
							return lofar_udp_raw_loop<signed short, float, 104, 0>(meta);


							// Deciates Stokes Q
						case 111:
							return lofar_udp_raw_loop<signed short, float, 111, 0>(meta);
						case 112:
							return lofar_udp_raw_loop<signed short, float, 112, 0>(meta);
						case 113:
							return lofar_udp_raw_loop<signed short, float, 113, 0>(meta);
						case 114:
							return lofar_udp_raw_loop<signed short, float, 114, 0>(meta);


							// Decimated Stokes U
						case 121:
							return lofar_udp_raw_loop<signed short, float, 121, 0>(meta);
						case 122:
							return lofar_udp_raw_loop<signed short, float, 122, 0>(meta);
						case 123:
							return lofar_udp_raw_loop<signed short, float, 123, 0>(meta);
						case 124:
							return lofar_udp_raw_loop<signed short, float, 124, 0>(meta);


							// Decimated Stokes V
						case 131:
							return lofar_udp_raw_loop<signed short, float, 131, 0>(meta);
						case 132:
							return lofar_udp_raw_loop<signed short, float, 132, 0>(meta);
						case 133:
							return lofar_udp_raw_loop<signed short, float, 133, 0>(meta);
						case 134:
							return lofar_udp_raw_loop<signed short, float, 134, 0>(meta);


							// Decimated Full Stokes
						case 151:
							return lofar_udp_raw_loop<signed short, float, 151, 0>(meta);
						case 152:
							return lofar_udp_raw_loop<signed short, float, 152, 0>(meta);
						case 153:
							return lofar_udp_raw_loop<signed short, float, 153, 0>(meta);
						case 154:
							return lofar_udp_raw_loop<signed short, float, 154, 0>(meta);


							// Decimated Useful Stokes
						case 161:
							return lofar_udp_raw_loop<signed short, float, 161, 0>(meta);
						case 162:
							return lofar_udp_raw_loop<signed short, float, 162, 0>(meta);
						case 163:
							return lofar_udp_raw_loop<signed short, float, 163, 0>(meta);
						case 164:
							return lofar_udp_raw_loop<signed short, float, 164, 0>(meta);


							// Non-decimated Stokes
						case 200:
							return lofar_udp_raw_loop<signed short, float, 200, 0>(meta);
						case 210:
							return lofar_udp_raw_loop<signed short, float, 210, 0>(meta);
						case 220:
							return lofar_udp_raw_loop<signed short, float, 220, 0>(meta);
						case 230:
							return lofar_udp_raw_loop<signed short, float, 230, 0>(meta);
						case 250:
							return lofar_udp_raw_loop<signed short, float, 250, 0>(meta);
						case 260:
							return lofar_udp_raw_loop<signed short, float, 260, 0>(meta);



							// Decimated Stokes I
						case 201:
							return lofar_udp_raw_loop<signed short, float, 201, 0>(meta);
						case 202:
							return lofar_udp_raw_loop<signed short, float, 202, 0>(meta);
						case 203:
							return lofar_udp_raw_loop<signed short, float, 203, 0>(meta);
						case 204:
							return lofar_udp_raw_loop<signed short, float, 204, 0>(meta);


							// Deciates Stokes Q
						case 211:
							return lofar_udp_raw_loop<signed short, float, 211, 0>(meta);
						case 212:
							return lofar_udp_raw_loop<signed short, float, 212, 0>(meta);
						case 213:
							return lofar_udp_raw_loop<signed short, float, 213, 0>(meta);
						case 214:
							return lofar_udp_raw_loop<signed short, float, 214, 0>(meta);


							// Decimated Stokes U
						case 221:
							return lofar_udp_raw_loop<signed short, float, 221, 0>(meta);
						case 222:
							return lofar_udp_raw_loop<signed short, float, 222, 0>(meta);
						case 223:
							return lofar_udp_raw_loop<signed short, float, 223, 0>(meta);
						case 224:
							return lofar_udp_raw_loop<signed short, float, 224, 0>(meta);


							// Decimated Stokes V
						case 231:
							return lofar_udp_raw_loop<signed short, float, 231, 0>(meta);
						case 232:
							return lofar_udp_raw_loop<signed short, float, 232, 0>(meta);
						case 233:
							return lofar_udp_raw_loop<signed short, float, 233, 0>(meta);
						case 234:
							return lofar_udp_raw_loop<signed short, float, 234, 0>(meta);


							// Decimated Full Stokes
						case 251:
							return lofar_udp_raw_loop<signed short, float, 251, 0>(meta);
						case 252:
							return lofar_udp_raw_loop<signed short, float, 252, 0>(meta);
						case 253:
							return lofar_udp_raw_loop<signed short, float, 253, 0>(meta);
						case 254:
							return lofar_udp_raw_loop<signed short, float, 254, 0>(meta);


							// Decimated Useful Stokes
						case 261:
							return lofar_udp_raw_loop<signed short, float, 261, 0>(meta);
						case 262:
							return lofar_udp_raw_loop<signed short, float, 262, 0>(meta);
						case 263:
							return lofar_udp_raw_loop<signed short, float, 263, 0>(meta);
						case 264:
							return lofar_udp_raw_loop<signed short, float, 264, 0>(meta);

						default:
							fprintf(stderr, "Unknown processing mode %d (%d, %d). Exiting.\n", processingMode,
									inputBitMode, calibrateData);
							return 1;
					}

				default:
					fprintf(stderr, "Unexpected bitmode %d (%d, %d). Exiting.\n", inputBitMode, processingMode,
							calibrateData);
					return 1;
			}

		default:
			fprintf(stderr, "Unexpected calibration mode (%d). Exiting.\n", calibrateData);
			return 1;
	}
}


// LUT for 4-bit data, faster than re-calculating upper/lower nibble for every sample.
//@formatter:off
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
//@formatter:on