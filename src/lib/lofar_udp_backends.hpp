#ifndef LOFAR_UDP_BACKENDS_HPP
#define LOFAR_UDP_BACKENDS_HPP

// We extensively use OpenMP, include it
#include <omp.h>

// For memcpy
#include <string.h>

// For assert
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif // End of __cplusplus

// Include the C library headers
#include "lofar_udp_structs.h"
#include "lofar_udp_time.h"


// Define the C-access interfaces
int32_t lofar_udp_cpp_loop_interface(lofar_udp_obs_meta *meta);

// Declare the Stokes Vector Functions
typedef float(StokesFuncType)(float, float, float, float);

// Define the 4-bit LUT
extern const int8_t bitmodeConversion[256][2];

// Don't pass the rest of this header to any C code


// Declare the function to calculate a calibrated sample
// Calculates one output component of:
// [(x_1, y_1i), (x_2, y_2i)] . [(s_1, s_2i)] = J . (X,Y).T
// [(x_3, y_3i), (x_4, y_4i)]   [(s_3, s_4i)]
//
// ===
// [(x_1 * s_1 - y_1 * s_2 + x_2 * s_3 - y_2 * s_4), i (x_1 * s_2 + y_1 * s_1 + x_2 * s_4 + y_2 * s_3)] = (X_r, X_i)
// [(x_3 * s_1 - y_3 * s_2 + x_4 * s_3 - y_4 * s_4), i (x_3 * s_2 + y_3 * s_1 + x_4 * s_4 + y_4 * s_3)] = (Y_r, Y_i)
#pragma omp declare simd
inline float calibrateSample(float c_1, float c_2, float c_3, float c_4, float c_5, float c_6, float c_7, float c_8) {
	return (c_1 * c_2) + (c_3 * c_4) + (c_5 * c_6) + (c_7 * c_8);
}

// Declare the Stokes Vector Functions
#pragma omp declare simd
inline float stokesI(float Xr, float Xi, float Yr, float Yi) {
	return (Xr * Xr) + (Xi * Xi) + (Yr * Yr) + (Yi * Yi);
}

#pragma omp declare simd
inline float stokesQ(float Xr, float Xi, float Yr, float Yi) {
	return ((Xr * Xr) + (Xi * Xi) - (Yr * Yr) - (Yi * Yi));
}

#pragma omp declare simd
inline float stokesU(float Xr, float Xi, float Yr, float Yi) {
	return 2.0f * ((Xr * Yr) + (Xi * Yi));
}

#pragma omp declare simd
inline float stokesV(float Xr, float Xi, float Yr, float Yi) {
	return 2.0f * ((Xr * Yi) - (Xi * Yr));
}

#ifdef __cplusplus
}

/**
 * @brief      Get the input index for a given packet, frequency
 *
 * @param[in]  lastInputPacketOffset  The last input packet offset
 * @param[in]  beamlet                The beamlet
 * @param[in]  timeStepSize           The time step size
 *
 * @return     { description_of_the_return_value }
 */
#pragma omp declare simd
inline int64_t input_offset_index(int64_t lastInputPacketOffset, int32_t beamlet, int32_t timeStepSize) {
	return lastInputPacketOffset + (int64_t) (beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize);
}

/**
 * @brief      Get the output index for an output in frequency-reversed major
 *             order
 *
 * @param[in]  outputPacketOffset  The output packet offset
 * @param[in]  beamlet             The beamlet
 * @param[in]  baseBeamlet         The base beamlet
 * @param[in]  cumulativeBeamlets  The cumulative beamlets
 * @param[in]  offset              The offset
 * @param[in]  totalBeamlets  The total beamlets
 *
 * @return     { description_of_the_return_value }
 */
#pragma omp declare simd
inline int64_t
frequency_major_index(int64_t outputPacketOffset, int32_t beamlet, int32_t baseBeamlet, int32_t cumulativeBeamlets, int32_t offset) {
	return outputPacketOffset + (int64_t) (beamlet - baseBeamlet + cumulativeBeamlets) * (int64_t) offset;
}


/**
 * @brief      Get the output index for an output in frequency-reversed major
 *             order
 *
 * @param[in]  totalBeamlets       The total beamlets
 * @param[in]  beamlet             The beamlet
 * @param[in]  baseBeamlet         The base beamlet
 * @param[in]  cumulativeBeamlets  The cumulative beamlets
 * @param[in]  offset              The offset
 * @param[in]  outputPacketOffset  The output packet offset
 *
 * @return     { description_of_the_return_value }
 */
#pragma omp declare simd
inline int64_t reversed_frequency_major_index(int64_t outputPacketOffset, int32_t totalBeamlets, int32_t beamlet, int32_t baseBeamlet,
										   int32_t cumulativeBeamlets, int32_t offset) {
	return outputPacketOffset + (int64_t) (totalBeamlets - 1 - beamlet + baseBeamlet - cumulativeBeamlets) * (int64_t) offset;
}


/**
 * @brief      Get the output index for an hour in time-major order
 *
 * @param[in]  beamlet              The beamlet
 * @param[in]  baseBeamlet          The base beamlet
 * @param[in]  cumulativeBeamlets   The cumulative beamlets
 * @param[in]  packetsPerIteration  The packets per iteration
 * @param[in]  outputTimeIdx        The output time index
 *
 * @return     { description_of_the_return_value }
 */
#pragma omp declare simd
inline int64_t
time_major_index(int32_t beamlet, int32_t baseBeamlet, int32_t cumulativeBeamlets, int64_t packetsPerIteration, int64_t outputTimeIdx) {
	return (((int64_t) (beamlet - baseBeamlet + cumulativeBeamlets) * (int64_t) (packetsPerIteration * UDPNTIMESLICE)) + outputTimeIdx);
}




// Setup templates for each processing mode

// Apply the calibration from a Jones matrix to a set of X/Y samples
#pragma omp declare simd
template<typename I, typename O>
void inline calibrateDataFunc(O *Xr, O *Xi, O *Yr, O *Yi, const float *beamletJones, const int8_t *inputPortData,
							  const int64_t tsInOffset, const int32_t timeStepSize) {
	(*Xr) = calibrateSample(beamletJones[0], *((I *) &(inputPortData[tsInOffset])), \
                            -1.0f * beamletJones[1], *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])), \
                            beamletJones[2], *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])), \
                            -1.0f * beamletJones[3], *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));

	(*Xi) = calibrateSample(beamletJones[0], *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])), \
                            beamletJones[1], *((I *) &(inputPortData[tsInOffset])), \
                            beamletJones[2], *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])), \
                            beamletJones[3], *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])));

	(*Yr) = calibrateSample(beamletJones[4], *((I *) &(inputPortData[tsInOffset])), \
                            -1.0f * beamletJones[5], *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])), \
                            beamletJones[6], *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])), \
                            -1.0f * beamletJones[7], *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));

	(*Yi) = calibrateSample(beamletJones[4], *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])), \
                            beamletJones[5], *((I *) &(inputPortData[tsInOffset])), \
                            beamletJones[6], *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])), \
                            beamletJones[7], *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])));
}

#pragma clang diagnostic push

// All of our processing modes
// This is going to take a while
// See docs/newProcessingMode.md for documentation of the structure of these kernels

// Copy: No change, packet goes in, packet goes out, missing packets are inserted through the configured padding scheme.
template<typename I, typename O>
void inline udp_copy(int64_t iLoop, int8_t *inputPortData, O **outputData, int32_t port, int64_t lastInputPacketOffset,
					 int64_t packetOutputLength) {
	int64_t outputPacketOffset = iLoop * packetOutputLength;
	memcpy(&(outputData[port][outputPacketOffset]), &(inputPortData[lastInputPacketOffset]), packetOutputLength);
}


// Copy No Header: Strip the header, copy the rest of the packet to the output.
template<typename I, typename O>
void inline udp_copyNoHdr(int64_t iLoop, int8_t *inputPortData, O **outputData, int32_t port, int64_t lastInputPacketOffset,
						  int64_t packetOutputLength) {
	int64_t outputPacketOffset = iLoop * packetOutputLength;
	memcpy(&(outputData[port][outputPacketOffset]), &(inputPortData[lastInputPacketOffset]), packetOutputLength);
}


// Copy Split Pols: Take the data across all ports, merge them, then split based on the X/Y real/imaginary state.
template<typename I, typename O, const int8_t calibrateData>
void inline
udp_copySplitPols(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset, int64_t packetOutputLength,
				  int32_t timeStepSize, int32_t upperBeamlet, int32_t cumulativeBeamlets, int32_t baseBeamlet, float *jonesMatrix) {
	int64_t outputPacketOffset = iLoop * packetOutputLength / sizeof(O);

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;


	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase = frequency_major_index(outputPacketOffset, beamlet, baseBeamlet, cumulativeBeamlets,
											UDPNTIMESLICE);

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (4 * timeStepSize);
			int64_t tsOutOffset = tsOutOffsetBase + ts;

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr[ts];
				outputData[1][tsOutOffset] = Xi[ts];
				outputData[2][tsOutOffset] = Yr[ts];
				outputData[3][tsOutOffset] = Yi[ts];
			} else {
				outputData[0][tsOutOffset] = *((I *) &(inputPortData[tsInOffset])); // Xr
				outputData[1][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[2][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[3][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}
		}
	}
}


// Channel Major: Change from being packet major (16 time samples, N beamlets) to channel major (M time samples, N beamlets).
template<typename I, typename O, const int8_t calibrateData>
void inline
udp_channelMajor(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset, int64_t packetOutputLength,
				 int32_t timeStepSize, int32_t totalBeamlets, int32_t upperBeamlet, int32_t cumulativeBeamlets, int32_t baseBeamlet,
				 float *jonesMatrix) {
	int64_t outputPacketOffset = iLoop * packetOutputLength / sizeof(O);

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;


	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase = frequency_major_index(outputPacketOffset, beamlet, baseBeamlet, cumulativeBeamlets, UDPNPOL);

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (4 * timeStepSize);
			int64_t tsOutOffset = tsOutOffsetBase + ts * (totalBeamlets * UDPNPOL);

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr[ts];
				outputData[0][tsOutOffset + 1] = Xi[ts];
				outputData[0][tsOutOffset + 2] = Yr[ts];
				outputData[0][tsOutOffset + 3] = Yi[ts];
			} else {
				outputData[0][tsOutOffset] = *((I *) &(inputPortData[tsInOffset])); // Xr
				outputData[0][tsOutOffset + 1] = *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[0][tsOutOffset + 2] = *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[0][tsOutOffset + 3] = *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}
		}
	}
}

// Channel Major, Split Pols: Change from packet major to beamlet major (described previously), then split data across X/Y real/imaginary state.
template<typename I, typename O, const int8_t calibrateData>
void inline udp_channelMajorSplitPols(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset,
									  int64_t packetOutputLength, int32_t timeStepSize, int32_t totalBeamlets, int32_t upperBeamlet,
									  int32_t cumulativeBeamlets, int32_t baseBeamlet, float *jonesMatrix) {
	int64_t outputPacketOffset = iLoop * packetOutputLength / sizeof(O);

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;


	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase = frequency_major_index(outputPacketOffset, beamlet, baseBeamlet, cumulativeBeamlets, 1);

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (4 * timeStepSize);
			int64_t tsOutOffset = tsOutOffsetBase + ts * totalBeamlets;

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr[ts];
				outputData[1][tsOutOffset] = Xi[ts];
				outputData[2][tsOutOffset] = Yr[ts];
				outputData[3][tsOutOffset] = Yi[ts];
			} else {
				outputData[0][tsOutOffset] = *((I *) &(inputPortData[tsInOffset])); // Xr
				outputData[1][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[2][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[3][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}
		}
	}
}

// Reversed Channel Major: Change from being packet major (16 time samples, N beamlets) to channel major (M time samples, N beamlets), with the beamlet other reversed.
template<typename I, typename O, const int8_t calibrateData>
void inline udp_reversedChannelMajor(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset,
									 int64_t packetOutputLength, int32_t timeStepSize, int32_t totalBeamlets, int32_t upperBeamlet,
									 int32_t cumulativeBeamlets, int32_t baseBeamlet, float *jonesMatrix) {
	int64_t outputPacketOffset = iLoop * packetOutputLength / sizeof(O);

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;


	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase = reversed_frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet,
													 cumulativeBeamlets, UDPNPOL);

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (4 * timeStepSize);
			int64_t tsOutOffset = tsOutOffsetBase + ts * (totalBeamlets * UDPNPOL);

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr[ts];
				outputData[0][tsOutOffset + 1] = Xi[ts];
				outputData[0][tsOutOffset + 2] = Yr[ts];
				outputData[0][tsOutOffset + 3] = Yi[ts];
			} else {
				outputData[0][tsOutOffset] = *((I *) &(inputPortData[tsInOffset])); // Xr
				outputData[0][tsOutOffset + 1] = *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[0][tsOutOffset + 2] = *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[0][tsOutOffset + 3] = *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}
		}
	}
}

// Reversed Channel Major, Split Pols: Change from packet major to beamlet major (described previously), then split data across X/Y real/imaginary state, with the beamlet order reversed.
template<typename I, typename O, const int8_t calibrateData>
void inline
udp_reversedChannelMajorSplitPols(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset,
								  int64_t packetOutputLength, int32_t timeStepSize, int32_t totalBeamlets, int32_t upperBeamlet,
								  int32_t cumulativeBeamlets, int32_t baseBeamlet, float *jonesMatrix) {
	int64_t outputPacketOffset = iLoop * packetOutputLength / sizeof(O);

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;


	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase = reversed_frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet,
													 cumulativeBeamlets, 1);

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (4 * timeStepSize);
			int64_t tsOutOffset = tsOutOffsetBase + ts * totalBeamlets;

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr[ts];
				outputData[1][tsOutOffset] = Xi[ts];
				outputData[2][tsOutOffset] = Yr[ts];
				outputData[3][tsOutOffset] = Yi[ts];
			} else {
				outputData[0][tsOutOffset] = *((I *) &(inputPortData[tsInOffset])); // Xr
				outputData[1][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[2][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[3][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}
		}
	}
}

// Time Major: Change from packet major to time major (N time samples from a beamlet, followed by the next beamlet, etc.)
template<typename I, typename O, const int8_t calibrateData>
void inline udp_timeMajor(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset, int32_t timeStepSize,
						  int32_t upperBeamlet, int32_t cumulativeBeamlets, int64_t packetsPerIteration, int32_t baseBeamlet,
						  float *jonesMatrix) {
	int64_t outputTimeIdx = iLoop * UDPNTIMESLICE;

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;


	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase =
				4 * time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputTimeIdx);

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (4 * timeStepSize);
			int64_t tsOutOffset = tsOutOffsetBase + ts * 4;

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr[ts];
				outputData[0][tsOutOffset + 1] = Xi[ts];
				outputData[0][tsOutOffset + 2] = Yr[ts];
				outputData[0][tsOutOffset + 3] = Yi[ts];
			} else {
				outputData[0][tsOutOffset] = *((I *) &(inputPortData[tsInOffset])); // Xr
				outputData[0][tsOutOffset + 1] = *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[0][tsOutOffset + 2] = *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[0][tsOutOffset + 3] = *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}
		}
	}
}

// Time Major, Split Pols: Change from packet major to time major (as described before), then split data across X/Y real/imaginary state.
template<typename I, typename O, const int8_t calibrateData>
void inline
udp_timeMajorSplitPols(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset, int32_t timeStepSize,
					   int32_t upperBeamlet, int32_t cumulativeBeamlets, int64_t packetsPerIteration, int32_t baseBeamlet,
					   float *jonesMatrix) {
	int64_t outputTimeIdx = iLoop * UDPNTIMESLICE;

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;


	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputTimeIdx);

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (4 * timeStepSize);
			int64_t tsOutOffset = tsOutOffsetBase + ts;

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr[ts];
				outputData[1][tsOutOffset] = Xi[ts];
				outputData[1][tsOutOffset] = Yr[ts];
				outputData[3][tsOutOffset] = Yi[ts];
			} else {
				outputData[0][tsOutOffset] = *((I *) &(inputPortData[tsInOffset])); // Xr
				outputData[1][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[1][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[3][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}
		}
	}
}


// Time Major, Dual Pols: Change from packet major to time major (as described before), then split data across X/Y.
// This output matches the input format required by FFTW
template<typename I, typename O, const int8_t calibrateData>
void inline
udp_timeMajorDualPols(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset, int32_t timeStepSize,
					  int32_t upperBeamlet, int32_t cumulativeBeamlets, int64_t packetsPerIteration, int32_t baseBeamlet,
					  float *jonesMatrix) {
	int64_t outputTimeIdx = iLoop * UDPNTIMESLICE;

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;


	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase =
				2 * time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputTimeIdx);

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (4 * timeStepSize);
			int64_t tsOutOffset = tsOutOffsetBase + ts * 2;

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr[ts];
				outputData[0][tsOutOffset + 1] = Xi[ts];
				outputData[1][tsOutOffset] = Yr[ts];
				outputData[1][tsOutOffset + 1] = Yi[ts];
			} else {
				outputData[0][tsOutOffset] = *((I *) &(inputPortData[tsInOffset])); // Xr
				outputData[0][tsOutOffset + 1] = *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[1][tsOutOffset] = *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[1][tsOutOffset + 1] = *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}
		}
	}
}

// Stokes Parameter output, in a given order (varies wildly based on input parameters)
template<typename I, typename O, StokesFuncType stokesFunc, const int32_t order, const int8_t calibrateData>
void inline
udp_stokes(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset, int64_t packetOutputLength,
		   int32_t timeStepSize, int32_t totalBeamlets, int32_t upperBeamlet, int32_t cumulativeBeamlets, int64_t packetsPerIteration,
		   int32_t baseBeamlet, float *jonesMatrix) {

	int64_t outputPacketOffset;
	if constexpr (order < 2) {
		outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	} else if constexpr (order < 4) {
		outputPacketOffset = iLoop * UDPNTIMESLICE;
	} else {
		__builtin_unreachable();
	}

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;

	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase;

		if constexpr (order == 0) {
			tsOutOffsetBase = reversed_frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet,
														 cumulativeBeamlets, 1);
		} else if constexpr (order == 1) {
			tsOutOffsetBase = frequency_major_index(outputPacketOffset, beamlet, baseBeamlet, cumulativeBeamlets,
			                                        1);
		} else if constexpr (order == 2) {
			tsOutOffsetBase = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
		} else if constexpr (order == 3) {
			__builtin_unreachable();
		}

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (4 * timeStepSize);
			int64_t tsOutOffset;

			if constexpr (order < 2) {
				tsOutOffset = tsOutOffsetBase + ts * totalBeamlets;
			} else if constexpr (order < 4) {
				tsOutOffset = tsOutOffsetBase + ts;
			}

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = (*stokesFunc)(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
			} else {
				outputData[0][tsOutOffset] = (*stokesFunc)(*((I *) &(inputPortData[tsInOffset])),
														   *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
														   *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
														   *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}
		}
	}
}

// Stokes Parameter output, in a given order (varies wildly based on input parameters), downsampled by a factor up to 16
template<typename I, typename O, StokesFuncType stokesFunc, const int32_t order, const int32_t factor, const int8_t calibrateData>
void inline udp_stokesDecimation(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset,
								 int64_t packetOutputLength, int32_t timeStepSize, int32_t totalBeamlets, int32_t upperBeamlet,
								 int32_t cumulativeBeamlets, int64_t packetsPerIteration, int32_t baseBeamlet,
								 float *jonesMatrix) {

	int64_t outputPacketOffset;
	if constexpr (order < 2) {
		outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	} else if constexpr (order < 4) {
		outputPacketOffset = iLoop * UDPNTIMESLICE;
	} else {
		__builtin_unreachable();
	}

	O tempVal;

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;

	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase;
		if constexpr (order == 0) {
			tsOutOffsetBase = reversed_frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet,
														 cumulativeBeamlets, 1);
		} else if constexpr (order == 1) {
			tsOutOffsetBase = frequency_major_index(outputPacketOffset, beamlet, baseBeamlet, cumulativeBeamlets,
			                                        1);
		} else if constexpr (order == 2) {
			tsOutOffsetBase = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
		} else if constexpr (order == 3) {
			__builtin_unreachable();
		}

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		tempVal = (float) 0.0;

		int64_t tsOutOffset = tsOutOffsetBase;

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (4 * timeStepSize);

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				tempVal += (*stokesFunc)(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
			} else {
				tempVal += (*stokesFunc)(*((I *) &(inputPortData[tsInOffset])),
										 *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
										 *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
										 *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}

			if ((ts + 1) % factor == 0) {
				outputData[0][tsOutOffset] = tempVal;
				tempVal = (float) 0.0;
				if constexpr (order == 0) {
					tsOutOffset += totalBeamlets;
				} else {
					tsOutOffset += 1;
				}
			}
		}
	}
}

// All Stokes Parameters output, in a given order (varies wildly based on input parameters)
template<typename I, typename O, const int32_t order, const int8_t calibrateData>
void inline
udp_fullStokes(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset, int64_t packetOutputLength,
			   int32_t timeStepSize, int32_t totalBeamlets, int32_t upperBeamlet, int32_t cumulativeBeamlets, int64_t packetsPerIteration,
			   int32_t baseBeamlet, float *jonesMatrix) {

	int64_t outputPacketOffset;
	if constexpr (order < 2) {
		outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	} else if constexpr (order < 4) {
		outputPacketOffset = iLoop * UDPNTIMESLICE;
	} else {
		__builtin_unreachable();
	}

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;

	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase;
		if constexpr (order == 0) {
			tsOutOffsetBase = reversed_frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet,
														 cumulativeBeamlets, 1);
		} else if constexpr (order == 1) {
			tsOutOffsetBase = frequency_major_index(outputPacketOffset, beamlet, baseBeamlet, cumulativeBeamlets,
			                                        1);
		} else if constexpr (order == 2) {
			tsOutOffsetBase = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
		} else if constexpr (order == 3) {
			__builtin_unreachable();
		}

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (4 * timeStepSize);
			int64_t tsOutOffset;

			if constexpr (order < 2) {
				tsOutOffset = tsOutOffsetBase + ts * totalBeamlets;
			} else if constexpr (order < 4) {
				tsOutOffset = tsOutOffsetBase + ts;
			}

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = stokesI(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
				outputData[1][tsOutOffset] = stokesQ(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
				outputData[2][tsOutOffset] = stokesU(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
				outputData[3][tsOutOffset] = stokesV(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);

			} else {
				outputData[0][tsOutOffset] = stokesI(*((I *) &(inputPortData[tsInOffset])),
													 *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
													 *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
													 *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				outputData[1][tsOutOffset] = stokesQ(*((I *) &(inputPortData[tsInOffset])),
													 *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
													 *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
													 *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				outputData[2][tsOutOffset] = stokesU(*((I *) &(inputPortData[tsInOffset])),
													 *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
													 *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
													 *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				outputData[3][tsOutOffset] = stokesV(*((I *) &(inputPortData[tsInOffset])),
													 *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
													 *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
													 *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}
		}
	}
}

// All Stokes Parameters output, in a given order (varies wildly based on input parameters), downsampled by a factor up to 16
template<typename I, typename O, const int32_t order, const int32_t factor, const int8_t calibrateData>
void inline udp_fullStokesDecimation(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset,
									 int64_t packetOutputLength, int32_t timeStepSize, int32_t totalBeamlets, int32_t upperBeamlet,
									 int32_t cumulativeBeamlets, int64_t packetsPerIteration, int32_t baseBeamlet,
									 float *jonesMatrix) {

	int64_t outputPacketOffset;
	if constexpr (order < 2) {
		outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	} else if constexpr (order < 4) {
		outputPacketOffset = iLoop * UDPNTIMESLICE;
	} else {
		__builtin_unreachable();
	}

	O tempValI, tempValQ, tempValU, tempValV;

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;

	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase;

		if constexpr (order == 0) {
			tsOutOffsetBase = reversed_frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet,
														 cumulativeBeamlets, 1);
		} else if constexpr (order == 1) {
			tsOutOffsetBase = frequency_major_index(outputPacketOffset, beamlet, baseBeamlet, cumulativeBeamlets,
			                                    1);
		} else if constexpr (order == 2) {
			tsOutOffsetBase = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
		} else if constexpr (order == 3) {
			__builtin_unreachable();
		}

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		// This is split into 2 inner loops as ICC generates garbage outputs when the loop is run on the full inner loop.
		// Should still be relatively efficient as 16 time samples fit inside one cache line on REALTA, so they should never be dropped
		// But there is some duplication of calibration.
		tempValI = (float) 0.0;
		tempValQ = (float) 0.0;

		int64_t tsOutOffset = tsOutOffsetBase;

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (UDPNPOL * timeStepSize);

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				tempValI += stokesI(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
				tempValQ += stokesQ(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
			} else {
				tempValI += stokesI(*((I *) &(inputPortData[tsInOffset])),
									*((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
									*((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
									*((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				tempValQ += stokesQ(*((I *) &(inputPortData[tsInOffset])),
									*((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
									*((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
									*((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}

			if ((ts + 1) % factor == 0) {
				outputData[0][tsOutOffset] = tempValI;
				outputData[1][tsOutOffset] = tempValQ;
				tempValI = (float) 0.0;
				tempValQ = (float) 0.0;

				if constexpr (order < 2) {
					tsOutOffset += totalBeamlets;
				} else if constexpr (order < 4) {
					tsOutOffset += 1;
				}
			}
		}

		tempValU = (float) 0.0;
		tempValV = (float) 0.0;

		tsOutOffset = tsOutOffsetBase;

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (UDPNPOL * timeStepSize);

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				tempValU += stokesU(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
				tempValV += stokesV(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
			} else {
				tempValU += stokesU(*((I *) &(inputPortData[tsInOffset])),
									*((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
									*((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
									*((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				tempValV += stokesV(*((I *) &(inputPortData[tsInOffset])),
									*((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
									*((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
									*((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}

			if ((ts + 1) % factor == 0) {
				outputData[2][tsOutOffset] = tempValU;
				outputData[3][tsOutOffset] = tempValV;
				tempValU = (float) 0.0;
				tempValV = (float) 0.0;

				if constexpr (order < 2) {
					tsOutOffset += totalBeamlets;
				} else if constexpr (order < 4) {
					tsOutOffset += 1;
				}
			}
		}
	}
}

// Stokes I/Q Parameter output, in a given order (varies wildly based on input parameters)
template<typename I, typename O, const int32_t order, const int8_t calibrateData>
void inline
udp_usefulStokes(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset, int64_t packetOutputLength,
				 int32_t timeStepSize, int32_t totalBeamlets, int32_t upperBeamlet, int32_t cumulativeBeamlets,
				 int64_t packetsPerIteration, int32_t baseBeamlet, float *jonesMatrix) {

	int64_t outputPacketOffset;

	if constexpr (order < 2) {
		outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	} else if constexpr (order < 4) {
		outputPacketOffset = iLoop * UDPNTIMESLICE;
	} else {
		__builtin_unreachable();
	}

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;


	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		int64_t tsOutOffsetBase;

		if constexpr (order == 0) {
			tsOutOffsetBase = reversed_frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet,
														 cumulativeBeamlets, 1);
		} else if constexpr (order == 1) {
			tsOutOffsetBase = frequency_major_index(outputPacketOffset, beamlet, baseBeamlet, cumulativeBeamlets,
			                                    1);
		} else if constexpr (order == 2) {
			tsOutOffsetBase = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
		} else if constexpr (order == 3) {
			// Unimplemented
			__builtin_unreachable();
		}

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (UDPNPOL * timeStepSize);
			int64_t tsOutOffset;

			if constexpr (order == 0) {
				tsOutOffset = tsOutOffsetBase + ts * totalBeamlets;
			} else {
				tsOutOffset = tsOutOffsetBase + ts;
			}

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = stokesI(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
				outputData[1][tsOutOffset] = stokesV(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
			} else {
				outputData[0][tsOutOffset] = stokesI(*((I *) &(inputPortData[tsInOffset])),
													 *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
													 *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
													 *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				outputData[1][tsOutOffset] = stokesV(*((I *) &(inputPortData[tsInOffset])),
													 *((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
													 *((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
													 *((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}
		}
	}
}

// Stokes I/Q Parameter output, in a given order (varies wildly based on input parameters), with downsampling by a factor up to 16
template<typename I, typename O, const int32_t order, const int32_t factor, const int8_t calibrateData>
void inline udp_usefulStokesDecimation(int64_t iLoop, int8_t *inputPortData, O **outputData, int64_t lastInputPacketOffset,
									   int64_t packetOutputLength, int32_t timeStepSize, int32_t totalBeamlets, int32_t upperBeamlet,
									   int32_t cumulativeBeamlets, int64_t packetsPerIteration, int32_t baseBeamlet,
									   float *jonesMatrix) {

	int64_t outputPacketOffset;
	if constexpr (order < 2) {
		outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	} else if constexpr (order < 4) {
		outputPacketOffset = iLoop * UDPNTIMESLICE;
	} else {
		__builtin_unreachable();
	}

	int64_t tsOutOffset;
	O tempValI, tempValV;

	O Xr[UDPNTIMESLICE], Xi[UDPNTIMESLICE], Yr[UDPNTIMESLICE], Yi[UDPNTIMESLICE];
	float *beamletJones;

	for (int32_t beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		int64_t tsInOffsetBase = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);

		if constexpr (order == 0) {
			tsOutOffset = reversed_frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet,
														 cumulativeBeamlets, 1);
		} else if constexpr (order == 1) {
			tsOutOffset = frequency_major_index(outputPacketOffset, beamlet, baseBeamlet, cumulativeBeamlets,
			                                    1);
		} else if constexpr (order == 2) {
			tsOutOffset = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
		} else if constexpr (order == 3) {
			// Unimplemented
			__builtin_unreachable();
		}

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		} else {
			UNUSED(beamletJones); UNUSED(jonesMatrix); UNUSED(Xr); UNUSED(Xi); UNUSED(Yr); UNUSED(Yi);
		}

		tempValI = (float) 0.0;
		tempValV = (float) 0.0;

		#pragma omp simd nontemporal(outputData)
		for (int32_t ts = 0; ts < UDPNTIMESLICE; ts++) {
			int64_t tsInOffset = tsInOffsetBase + ts * (UDPNPOL * timeStepSize);

			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr[ts], &Xi[ts], &Yr[ts], &Yi[ts], beamletJones, inputPortData, tsInOffset, timeStepSize);

				tempValI += stokesI(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
				tempValV += stokesV(Xr[ts], Xi[ts], Yr[ts], Yi[ts]);
			} else {
				tempValI += stokesI(*((I *) &(inputPortData[tsInOffset])),
									*((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
									*((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
									*((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				tempValV += stokesV(*((I *) &(inputPortData[tsInOffset])),
									*((I *) &(inputPortData[tsInOffset + 1 * timeStepSize])),
									*((I *) &(inputPortData[tsInOffset + 2 * timeStepSize])),
									*((I *) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}

			if ((ts + 1) % factor == 0) {
				outputData[0][tsOutOffset] = tempValI;
				outputData[1][tsOutOffset] = tempValV;
				tempValI = (float) 0.0;
				tempValV = (float) 0.0;

				if constexpr (order < 2) {
					tsOutOffset += totalBeamlets;
				} else if constexpr (order < 4) {
					tsOutOffset += 1;
				}
			}
		}
	}
}
#pragma clang diagnostic pop //  -Wpass-failed=transform-warning



// Define the main processing loop
template<typename I, typename O, const int32_t state, const int8_t calibrateData>
int32_t lofar_udp_raw_loop(lofar_udp_obs_meta *meta) {

	if (meta->numPorts < 1 || meta->numOutputs < 1 || meta->packetsPerIteration < 1) {
		fprintf(stderr, "ERROR %s: No work can be performed (ports: %d, outputs: %d, packets: %ld), returning.\n", __func__, meta->numPorts, meta->numOutputs, meta->packetsPerIteration);
		return 1;
	}

	assert(sizeof(O) == meta->outputBitMode / 8);
	if (meta->inputBitMode != 4) { // NOLINT(bugprone-branch-clone)
		assert(sizeof(I) == meta->inputBitMode / 8);
	} else {
		assert(sizeof(I) == meta->inputBitMode / 4);
	}

	// Setup return variable
	int32_t packetLoss = 0;

	// Get number of OpenMP Threads
	int32_t nThreads = omp_get_num_threads();

	VERBOSE(const int32_t verbose = meta->VERBOSE);
	// Calculate the true processing mode (4-bit -> +4000)
	constexpr int32_t trueState = state % 4000;

	// Setup decimation factor, 4-bit workspaces if needed
	// Silence compiler warnings as this variable is only needed for some processing modes
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic push
#pragma GCC diagnostic push
#pragma GCC diagnostic push
#pragma GCC diagnostic push
	constexpr int32_t decimation = 1 << (state % 10);

	int8_t **byteWorkspace = nullptr;
	if constexpr (state >= 4010) {
		VERBOSE(if (verbose) {
			printf("Allocating %ld bytes at %p\n", sizeof(int8_t *) * OMP_THREADS, (void *) byteWorkspace);
		});
		byteWorkspace = (int8_t **) malloc(sizeof(int8_t *) * nThreads);
		CHECK_ALLOC_NOCLEAN(byteWorkspace, 1);
		int32_t maxPacketSize = 0;
		for (int32_t port = 0; port < meta->numPorts; port++) {
			if (meta->portPacketLength[port] > maxPacketSize) {
				maxPacketSize = meta->portPacketLength[port];
			}
		}
		VERBOSE(if (verbose) {
			printf("Allocating %d bytes at %p\n", OMP_THREADS * 2 * maxPacketSize - 2 * UDPHDRLEN,
				   (void *) byteWorkspace[0]);
		});
		int32_t calSampleSize = 2 * maxPacketSize - 2 * UDPHDRLEN * (int32_t) sizeof(int8_t);
		byteWorkspace[0] = (int8_t *) malloc(nThreads * calSampleSize);
		CHECK_ALLOC(byteWorkspace[0], 1, free(byteWorkspace););

		for (int32_t i = 1; i < nThreads; i++) {
			byteWorkspace[i] = byteWorkspace[0] + i * calSampleSize;
		}
	}
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop


	// Setup working variables
	VERBOSE(if (verbose) {
		printf("Beginning processing for state %d, with input %ld and output %ld\n", state, sizeof(I),
			   sizeof(O));
	});

	const int64_t packetsPerIteration = meta->packetsPerIteration;
	const int8_t replayDroppedPackets = meta->replayDroppedPackets;

	// For each port of data provided,
#pragma omp parallel for default(shared) shared(byteWorkspace)
	for (int8_t port = 0; port < meta->numPorts; port++) {

		VERBOSE(if (verbose) { printf("Port: %d on thread %d\n", port, omp_get_thread_num()); });

		int64_t lastPortPacket, currentPortPacket, inputPacketOffset, lastInputPacketOffset, iWork, iLoop;

		// Reset the dropped packets counter
		meta->portLastDroppedPackets[port] = 0;
		int32_t nextSequence;
		int64_t currentPacketsDropped = 0, currentOutOfOrderPackets = 0;

		// Reset last packet, reference data on the current port
		lastPortPacket = meta->lastPacket;
		int8_t *inputPortData = meta->inputData[port];
		O **outputData = (O **) meta->outputData;

		// Silence compiler warnings as this variable is only needed for some processing modes
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic push
#pragma GCC diagnostic push

		// Select beamlet parameters
		const int32_t baseBeamlet = meta->baseBeamlets[port];
		const int32_t upperBeamlet = meta->upperBeamlets[port];

		const int32_t cumulativeBeamlets = meta->portCumulativeBeamlets[port];
		const int32_t totalBeamlets = meta->totalProcBeamlets;


		// Select Jones Matrix if performing Calibration
		float *jonesMatrix = nullptr;
		if constexpr (calibrateData) {
			VERBOSE(printf("Beamlets %d: %d, %d\n", port, baseBeamlet, upperBeamlet););
			jonesMatrix = (float *) calloc((upperBeamlet - baseBeamlet) * JONESMATSIZE, sizeof(float));
			if (jonesMatrix == NULL) {
				fprintf(stderr, "ERROR: Failed to allocate %ld bytes for Jones Matrix caching, attempting to fall back to normal operations.\n", (upperBeamlet - baseBeamlet) * JONESMATSIZE * sizeof(float));
				jonesMatrix = &(meta->jonesMatrices[meta->calibrationStep][cumulativeBeamlets]);
			} else {
				for (int32_t i = 0; i < (upperBeamlet - baseBeamlet); i++) {
					for (int32_t j = 0; j < JONESMATSIZE; j++) {
						jonesMatrix[i * JONESMATSIZE + j] = meta->jonesMatrices[meta->calibrationStep][
							(cumulativeBeamlets + i) * JONESMATSIZE + j];
					}
				}
			}
		}

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int32_t portPacketLength = meta->portPacketLength[port];
		const int32_t packetOutputLength = meta->packetOutputLength[0];
		const int32_t timeStepSize = sizeof(I) / sizeof(int8_t);
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

		lastInputPacketOffset = (-2 + meta->replayDroppedPackets) *
								portPacketLength;    // We request at least 2 packets are malloc'd before the array head pointer, so no SEGFAULTs here
		// -2 * PPL = 0s -1 * PPL = last processed packet -- used for calculating offset in dropped case if
		// 	we have packet loss on the boundary.
		VERBOSE(if (verbose) {
			printf("LPP: %ld, PPL: %d, LIPO: %ld\n", lastPortPacket, portPacketLength, lastInputPacketOffset);
		});
		// iWork -- input data offset calculations (account for dropped packets)
		// iLoop -- output data offsets
		// These are the same if there is no packet loss.
		//
		// Reset iWork, inputPacketOffset, read in the first packet's number.
		iWork = 0, inputPacketOffset = 0;
		currentPortPacket = lofar_udp_time_get_packet_number(&(inputPortData[inputPacketOffset]));

		VERBOSE(if (verbose) {
			printf("Port %d: Packet %ld, iters %ld, base %d, upper %d, cumulative %d, total %d, outputLength %d, timeStep %d, decimation %d, trueState %d\n", \
                    port, currentPortPacket, packetsPerIteration, baseBeamlet, upperBeamlet, cumulativeBeamlets,
				   totalBeamlets, packetOutputLength, timeStepSize, decimation, trueState);
		});

		//int64_t iLoopCache;
		for (iLoop = 0; iLoop < packetsPerIteration; iLoop++) {
			// iLoopCache = iLoop;
			VERBOSE(if (verbose == 2) {
				printf("Port %d. Loop %ld, Work %ld, packet %ld, target %ld\n", port, iLoop, iWork,
					   currentPortPacket, lastPortPacket + 1);
			});

			// Check for packet loss by ensuring we have sequential packet numbers
			if (currentPortPacket != (lastPortPacket + 1)) {
				// Check if a packet is out of order; if so, drop the packet
				// TODO: Better future option: check if the packet is in this block, copy and overwrite padded packed instead
				VERBOSE(if (verbose == 2) {
					printf("Port %d, Packet %ld (%ld / %ld) is not the expected packet, %ld.\n", port, currentPortPacket, iLoop, packetsPerIteration - 1,
					       lastPortPacket + 1);
				});
				if (currentPortPacket < lastPortPacket) {
					VERBOSE(if (verbose >= 1) {
						printf("Packet %ld on port %d is out of order; dropping.\n", currentPortPacket, port);
					});
					// Dropped packet -> index not processed -> effectively an 'added' packet, decrement the dropped packet count
					// 	so that we don't include an extra packet in shift operations
					currentPacketsDropped -= 1;
					currentOutOfOrderPackets += 1;

					iWork++;
					/* UNTESTED:
					if ((currentPortPacket > reader->meta->lastPacket + 1) && (currentPortPacket < reader->meta->lastPacket + reader->packetsPerIteration)) {
						lastInputPacketOffset = (currentPortPacket - (reader->meta->lastPacket + 1)) * portPacketLength;
						iLoopCache = (currentPortPacket - (reader->meta->lastPacket + 1));
					}
					*/
					if (iWork != packetsPerIteration) {
						inputPacketOffset = iWork * portPacketLength;
						currentPortPacket = lofar_udp_time_get_packet_number(&(inputPortData[inputPacketOffset]));
					}
					continue;

				}

				// Packet dropped:
				//  Trip the return int
				//	Increment the port counter for this operation
				//	Increment the last packet offset, so it can be used again next time while including the new offset
				packetLoss = -1;
				currentPacketsDropped += 1;
				lastPortPacket += 1;

				if (replayDroppedPackets) {
					// If we are replaying the last packet, change the array index to the last good packet index
					inputPacketOffset = lastInputPacketOffset;
				} else {
					// Array should be 0 padded at the start; copy data from there.
					inputPacketOffset = -2 * portPacketLength;
					if constexpr (state == 0) {
						// Move the last header into place
						memcpy(&(inputPortData[inputPacketOffset + 8]), &(inputPortData[lastInputPacketOffset + 8]), 8);
					}
				}

				if constexpr (state == 0) {
					// Increment the 'sequence' component of the header so that the packet number appears right in future packet reads
					nextSequence = (int32_t) lofar_udp_time_get_next_packet_sequence(&(inputPortData[lastInputPacketOffset]));
					memcpy(&(inputPortData[inputPacketOffset + 12]), &nextSequence, 4);
					// The last bit of the 'source' short isn't used; leave a signature that this packet was modified
					inputPortData[inputPacketOffset + CEP_HDR_SRC_OFFSET + 1] += 1;
				}

				VERBOSE(if (verbose >= 1) {
					printf("Packet %ld on port %d is missing; padding.\n", lastPortPacket + 1, port);
				});

			} else {
				VERBOSE(if (verbose == 2) {
					printf("Port %d, Packet %ld is the expected packet.\n", port, currentPortPacket);
				});

				// We have a sequential packet, therefore:
				//		Update the last legitimate packet number and array offset index
				//		Get the next packet number for the next loop
				//		Increment iWork (input data packet index) and determine the new input offset

				lastPortPacket = currentPortPacket;

				if constexpr (state == 0) {
					lastInputPacketOffset = inputPacketOffset;
				} else {
					lastInputPacketOffset = inputPacketOffset + UDPHDRLEN;
				}

				iWork++;
				inputPacketOffset = iWork * portPacketLength;

				// Ensure we don't attempt to access unallocated memory
				if (iLoop != packetsPerIteration - 1) {
					// Speedup: add 16 to the sequence, check if accurate. Doesn't work at rollover.
					int32_t nextSequence;
					if constexpr (state == 0) {
						nextSequence = (*((uint32_t *) &(inputPortData[lastInputPacketOffset + CEP_HDR_SEQ_OFFSET]))) + 16;
					} else {
						nextSequence = (*((uint32_t *) &(inputPortData[lastInputPacketOffset - (UDPHDRLEN - CEP_HDR_SEQ_OFFSET)]))) + 16;
					}

					if (*((uint32_t *) &(inputPortData[inputPacketOffset + 12])) == nextSequence) {
						currentPortPacket += 1;
					} else {
						currentPortPacket = lofar_udp_time_get_packet_number(&(inputPortData[inputPacketOffset]));
					}
				}

			}


			// Use firstprivate to lock the variables into the task, while the main loop continues to update them
#pragma omp task default(shared) firstprivate(iLoop, lastInputPacketOffset, inputPortData) shared(byteWorkspace, outputData)
			{

				// Unpack 4-bit data into an array of int8_t, so it can be processed the same way we process 8-bit data
				if constexpr (state >= 4010) {
					// Get the workspace for the current packet
					inputPortData = byteWorkspace[omp_get_thread_num()];

					// Determine the number of (byte-sized) samples to process
					int32_t numSamples = portPacketLength - UDPHDRLEN;

					// Use a LUT to extract the 4-bit signed ints from int8_t
#ifdef __INTEL_COMPILER
#pragma unroll(976)
#else
#pragma GCC unroll 976
#endif
					for (int32_t idx = 0; idx < numSamples; idx++) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"
						const int8_t *result = bitmodeConversion[(uint8_t) meta->inputData[port][
								lastInputPacketOffset + idx]];
#pragma GCC diagnostic pop
						inputPortData[idx * 2] = result[0];
						inputPortData[idx * 2 + 1] = result[1];
					}

					VERBOSE(if (verbose == 2 && port == 0 && iLoop == 0) {
						for (int32_t i = 0; i < numSamples - UDPHDRLEN; i++)
							printf("%d, ", inputPortData[i]);
						printf("\n");
					};);

					// We have data at the start of the array, reset the pointer offset to 0
					lastInputPacketOffset = 0;
				}

				// Effectively a large switch statement, but more performant as it's decided at compile time.
				if constexpr (trueState == PACKET_FULL_COPY) {
					udp_copy<int8_t, int8_t>(iLoop, inputPortData, (int8_t **) outputData, port, lastInputPacketOffset,
					                         packetOutputLength);
				} else if constexpr (trueState == PACKET_NOHDR_COPY) {
					udp_copyNoHdr<int8_t, int8_t>(iLoop, inputPortData, (int8_t **) outputData, port, lastInputPacketOffset,
					                              packetOutputLength);
				} else if constexpr (trueState == PACKET_SPLIT_POL) {
					udp_copySplitPols<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                       packetOutputLength, timeStepSize, upperBeamlet,
					                                       cumulativeBeamlets, baseBeamlet, jonesMatrix);


				} else if constexpr (trueState == BEAMLET_MAJOR_FULL) {
					udp_channelMajor<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                      packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet,
					                                      cumulativeBeamlets, baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == BEAMLET_MAJOR_SPLIT_POL) {
					udp_channelMajorSplitPols<I, O, calibrateData>(iLoop, inputPortData, outputData,
					                                               lastInputPacketOffset, packetOutputLength,
					                                               timeStepSize, totalBeamlets, upperBeamlet,
					                                               cumulativeBeamlets, baseBeamlet, jonesMatrix);

				} else if constexpr (trueState == BEAMLET_MAJOR_FULL_REV) {
					udp_reversedChannelMajor<I, O, calibrateData>(iLoop, inputPortData, outputData,
					                                              lastInputPacketOffset, packetOutputLength,
					                                              timeStepSize, totalBeamlets, upperBeamlet,
					                                              cumulativeBeamlets, baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == BEAMLET_MAJOR_SPLIT_POL_REV) {
					udp_reversedChannelMajorSplitPols<I, O, calibrateData>(iLoop, inputPortData, outputData,
					                                                       lastInputPacketOffset, packetOutputLength,
					                                                       timeStepSize, totalBeamlets, upperBeamlet,
					                                                       cumulativeBeamlets, baseBeamlet,
					                                                       jonesMatrix);


				} else if constexpr (trueState == TIME_MAJOR_FULL) {
					udp_timeMajor<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                   timeStepSize, upperBeamlet, cumulativeBeamlets,
					                                   packetsPerIteration, baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == TIME_MAJOR_SPLIT_POL) {
					udp_timeMajorSplitPols<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                            timeStepSize, upperBeamlet, cumulativeBeamlets,
					                                            packetsPerIteration, baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == 32 || trueState == TIME_MAJOR_ANT_POL_FLOAT) {
					udp_timeMajorDualPols<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                           timeStepSize, upperBeamlet, cumulativeBeamlets,
					                                           packetsPerIteration, baseBeamlet, jonesMatrix);


				} else if constexpr (trueState == STOKES_I) {
					udp_stokes<I, O, stokesI, 0, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                            packetOutputLength, timeStepSize, totalBeamlets,
					                                            upperBeamlet, cumulativeBeamlets, packetsPerIteration,
					                                            baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == STOKES_Q) {
					udp_stokes<I, O, stokesQ, 0, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                            packetOutputLength, timeStepSize, totalBeamlets,
					                                            upperBeamlet, cumulativeBeamlets, packetsPerIteration,
					                                            baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == STOKES_U) {
					udp_stokes<I, O, stokesU, 0, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                            packetOutputLength, timeStepSize, totalBeamlets,
					                                            upperBeamlet, cumulativeBeamlets, packetsPerIteration,
					                                            baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == STOKES_V) {
					udp_stokes<I, O, stokesV, 0, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                            packetOutputLength, timeStepSize, totalBeamlets,
					                                            upperBeamlet, cumulativeBeamlets, packetsPerIteration,
					                                            baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == STOKES_IQUV) {
					udp_fullStokes<I, O, 0, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                       packetOutputLength, timeStepSize, totalBeamlets,
					                                       upperBeamlet, cumulativeBeamlets, packetsPerIteration,
					                                       baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == STOKES_IV) {
					udp_usefulStokes<I, O, 0, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                         packetOutputLength, timeStepSize, totalBeamlets,
					                                         upperBeamlet, cumulativeBeamlets, packetsPerIteration,
					                                         baseBeamlet, jonesMatrix);

				} else if constexpr (trueState >= STOKES_I_DS2 && trueState <= STOKES_I_DS16) {
					udp_stokesDecimation<I, O, stokesI, 0, decimation, calibrateData>(iLoop, inputPortData, outputData,
					                                                                  lastInputPacketOffset,
					                                                                  packetOutputLength, timeStepSize,
					                                                                  totalBeamlets, upperBeamlet,
					                                                                  cumulativeBeamlets,
					                                                                  packetsPerIteration, baseBeamlet,
					                                                                  jonesMatrix);
				} else if constexpr (trueState >= STOKES_Q_DS2 && trueState <= STOKES_Q_DS16) {
					udp_stokesDecimation<I, O, stokesQ, 0, decimation, calibrateData>(iLoop, inputPortData, outputData,
					                                                                  lastInputPacketOffset,
					                                                                  packetOutputLength, timeStepSize,
					                                                                  totalBeamlets, upperBeamlet,
					                                                                  cumulativeBeamlets,
					                                                                  packetsPerIteration, baseBeamlet,
					                                                                  jonesMatrix);
				} else if constexpr (trueState >= STOKES_U_DS2 && trueState <= STOKES_U_DS16) {
					udp_stokesDecimation<I, O, stokesU, 0, decimation, calibrateData>(iLoop, inputPortData, outputData,
					                                                                  lastInputPacketOffset,
					                                                                  packetOutputLength, timeStepSize,
					                                                                  totalBeamlets, upperBeamlet,
					                                                                  cumulativeBeamlets,
					                                                                  packetsPerIteration, baseBeamlet,
					                                                                  jonesMatrix);
				} else if constexpr (trueState >= STOKES_V_DS2 && trueState <= STOKES_V_DS16) {
					udp_stokesDecimation<I, O, stokesV, 0, decimation, calibrateData>(iLoop, inputPortData, outputData,
					                                                                  lastInputPacketOffset,
					                                                                  packetOutputLength, timeStepSize,
					                                                                  totalBeamlets, upperBeamlet,
					                                                                  cumulativeBeamlets,
					                                                                  packetsPerIteration, baseBeamlet,
					                                                                  jonesMatrix);
				} else if constexpr (trueState >= STOKES_IQUV_DS2 && trueState <= STOKES_IQUV_DS16) {
					udp_fullStokesDecimation<I, O, 0, decimation, calibrateData>(iLoop, inputPortData, outputData,
					                                                             lastInputPacketOffset,
					                                                             packetOutputLength, timeStepSize,
					                                                             totalBeamlets, upperBeamlet,
					                                                             cumulativeBeamlets,
					                                                             packetsPerIteration, baseBeamlet,
					                                                             jonesMatrix);
				} else if constexpr (trueState >= STOKES_IV_DS2 && trueState <= STOKES_IV_DS16) {
					udp_usefulStokesDecimation<I, O, 0, decimation, calibrateData>(iLoop, inputPortData, outputData,
					                                                               lastInputPacketOffset,
					                                                               packetOutputLength, timeStepSize,
					                                                               totalBeamlets, upperBeamlet,
					                                                               cumulativeBeamlets,
					                                                               packetsPerIteration, baseBeamlet,
					                                                               jonesMatrix);

				} else if constexpr (trueState == STOKES_I_REV) {
					udp_stokes<I, O, stokesI, 1, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                            packetOutputLength, timeStepSize, totalBeamlets,
					                                            upperBeamlet, cumulativeBeamlets, packetsPerIteration,
					                                            baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == STOKES_Q_REV) {
					udp_stokes<I, O, stokesQ, 1, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                            packetOutputLength, timeStepSize, totalBeamlets,
					                                            upperBeamlet, cumulativeBeamlets, packetsPerIteration,
					                                            baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == STOKES_U_REV) {
					udp_stokes<I, O, stokesU, 1, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                            packetOutputLength, timeStepSize, totalBeamlets,
					                                            upperBeamlet, cumulativeBeamlets, packetsPerIteration,
					                                            baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == STOKES_V_REV) {
					udp_stokes<I, O, stokesV, 1, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                            packetOutputLength, timeStepSize, totalBeamlets,
					                                            upperBeamlet, cumulativeBeamlets, packetsPerIteration,
					                                            baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == STOKES_IQUV_REV) {
					udp_fullStokes<I, O, 1, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                       packetOutputLength, timeStepSize, totalBeamlets,
					                                       upperBeamlet, cumulativeBeamlets, packetsPerIteration,
					                                       baseBeamlet, jonesMatrix);
				} else if constexpr (trueState == STOKES_IV_REV) {
					udp_usefulStokes<I, O, 1, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset,
					                                         packetOutputLength, timeStepSize, totalBeamlets,
					                                         upperBeamlet, cumulativeBeamlets, packetsPerIteration,
					                                         baseBeamlet, jonesMatrix);

				} else if constexpr (trueState >= STOKES_I_DS2_REV && trueState <= STOKES_I_DS16_REV) {
					udp_stokesDecimation<I, O, stokesI, 1, decimation, calibrateData>(iLoop, inputPortData, outputData,
					                                                                  lastInputPacketOffset,
					                                                                  packetOutputLength, timeStepSize,
					                                                                  totalBeamlets, upperBeamlet,
					                                                                  cumulativeBeamlets,
					                                                                  packetsPerIteration, baseBeamlet,
					                                                                  jonesMatrix);
				} else if constexpr (trueState >= STOKES_Q_DS2_REV && trueState <= STOKES_Q_DS16_REV) {
					udp_stokesDecimation<I, O, stokesQ, 1, decimation, calibrateData>(iLoop, inputPortData, outputData,
					                                                                  lastInputPacketOffset,
					                                                                  packetOutputLength, timeStepSize,
					                                                                  totalBeamlets, upperBeamlet,
					                                                                  cumulativeBeamlets,
					                                                                  packetsPerIteration, baseBeamlet,
					                                                                  jonesMatrix);
				} else if constexpr (trueState >= STOKES_U_DS2_REV && trueState <= STOKES_U_DS16_REV) {
					udp_stokesDecimation<I, O, stokesU, 1, decimation, calibrateData>(iLoop, inputPortData, outputData,
					                                                                  lastInputPacketOffset,
					                                                                  packetOutputLength, timeStepSize,
					                                                                  totalBeamlets, upperBeamlet,
					                                                                  cumulativeBeamlets,
					                                                                  packetsPerIteration, baseBeamlet,
					                                                                  jonesMatrix);
				} else if constexpr (trueState >= STOKES_V_DS2_REV && trueState <= STOKES_V_DS16_REV) {
					udp_stokesDecimation<I, O, stokesV, 1, decimation, calibrateData>(iLoop, inputPortData, outputData,
					                                                                  lastInputPacketOffset,
					                                                                  packetOutputLength, timeStepSize,
					                                                                  totalBeamlets, upperBeamlet,
					                                                                  cumulativeBeamlets,
					                                                                  packetsPerIteration, baseBeamlet,
					                                                                  jonesMatrix);
				} else if constexpr (trueState >= STOKES_IQUV_DS2_REV && trueState <= STOKES_IQUV_DS16_REV) {
					udp_fullStokesDecimation<I, O, 1, decimation, calibrateData>(iLoop, inputPortData, outputData,
					                                                             lastInputPacketOffset,
					                                                             packetOutputLength, timeStepSize,
					                                                             totalBeamlets, upperBeamlet,
					                                                             cumulativeBeamlets,
					                                                             packetsPerIteration, baseBeamlet,
					                                                             jonesMatrix);
				} else if constexpr (trueState >= STOKES_IV_DS2_REV && trueState <= STOKES_IV_DS16_REV) {
					udp_usefulStokesDecimation<I, O, 1, decimation, calibrateData>(iLoop, inputPortData, outputData,
					                                                               lastInputPacketOffset,
					                                                               packetOutputLength, timeStepSize,
					                                                               totalBeamlets, upperBeamlet,
					                                                               cumulativeBeamlets,
					                                                               packetsPerIteration, baseBeamlet,
					                                                               jonesMatrix);

				} else {
					fprintf(stderr, "Unknown processing mode %d, exiting.\n", state);
					exit(1);
				}

			}
		}

		// Perform analysis of iteration, cleanup and wait for tasks to end.
		if constexpr (calibrateData) {
			VERBOSE(printf("Free'ing calibration Jones\n"););
			free(jonesMatrix);
		}

		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) {
			printf("Current dropped packet count on port %d: %ld\n", port, meta->portLastDroppedPackets[port]);
		});

		if (currentOutOfOrderPackets > 0) {
			fprintf(stderr, "WARNING (Port %d): %ld packets were out of the expected order.\n", port,
					currentOutOfOrderPackets);
		}

		// Wait until all tasks are finished before leaving the loop.
		#pragma omp taskwait

		VERBOSE(if (verbose) { printf("Port %d finished loop.\n", port); });

	}

	for (int32_t port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < (int64_t) (-0.2f * (float) packetsPerIteration < 0 ? -0.2f * (float) packetsPerIteration : -1)) {
			fprintf(stderr,
					"A large number of packets were out of order on port %d; this normally indicates a data integrity issue, exiting...",
					port);
			return 1;
		}
	}

	for (int32_t port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < 0 && meta->replayDroppedPackets) {
			// TODO: pad the end of the arrays if the data doesn't fill it.
			// Or just re-loop with fresh data and report less data back?
			// Maybe re-architect a bit, over-read the amount of data needed, only return what's needed.
		}
	}

	// Update the last packet variable
	meta->lastPacket += packetsPerIteration;
	VERBOSE(if (verbose) { printf("Exiting operation, last packet was %ld\n", meta->lastPacket); });

	// Update the input/output states
	meta->inputDataReady = 0;
	meta->outputDataReady = 1;



	// If needed, free the 4-bit workspace
	if constexpr (state >= 4010) {
		VERBOSE(if (verbose) { printf("freeing byteWorkspace data at %p\n", (void *) byteWorkspace[0]); });
		free(byteWorkspace[0]);

		free(byteWorkspace);
		VERBOSE(if (verbose) { printf("byteWorkspace free'd\n"); });
	}

	return packetLoss;
}

#endif // End of if __cplusplus
#endif // End of LOFAR_UDP_BACKENDS_HPP
