#ifndef LOFAR_UDP_BACKENDS_H
#define LOFAR_UDP_BACKENDS_H

#ifdef __cplusplus
extern "C" {
#endif
	#include "lofar_udp_reader.h"
	#include "lofar_udp_misc.h"
#ifdef __cplusplus
}
#endif

#include <omp.h>


// 4-bit LUT for faster decoding of 4-bit data
#ifndef __LOFAR_4BITLUT
#define __LOFAR_4BITLUT
extern const char bitmodeConversion[256][2];
#endif

// Jones matrix size for calibration
#define JONESMATSIZE 8


#ifndef __LOFAR_UDP_VOLTAGE_MANIP
#define __LOFAR_UDP_VOLTAGE_MANIP
#ifdef __cplusplus
extern "C" {
#endif
// Include CPP functions
float calibrateSample(float c_1, float c_2, float c_3, float c_4, float c_5, float c_6, float c_7, float c_8);
// Declare the Stokes Vector Functions
typedef float(StokesFuncType)(float, float, float, float);
inline float stokesI(float Xr, float Xi, float Yr, float Yi);
inline float stokesQ(float Xr, float Xi, float Yr, float Yi);
inline float stokesU(float Xr, float Xi, float Yr, float Yi);
inline float stokesV(float Xr, float Xi, float Yr, float Yi);

// Index calculation shorthands
inline long input_offset_index(long lastInputPacketOffset, int beamlet, int timeStepSize);
inline long frequency_major_index(long outputPacketOffset, int totalBeamlets, int beamlet, int baseBeamlet, int cumulativeBeamlets);
inline long time_major_index(int beamlet, int baseBeamlet, int cumulativeBeamlets, long packetsPerIteration, long outputTimeIdx);

#ifdef __cplusplus
}
#endif
#endif

// Define the C-access interfaces
#ifndef __LOFAR_UDP_CPPBRIDGE
#define __LOFAR_UDP_CPPBRIDGE
#ifdef __cplusplus
extern "C" {
#endif
int lofar_udp_cpp_loop_interface(lofar_udp_meta *meta);
#ifdef __cplusplus
}
#endif
#endif
#endif


#ifndef LOFAR_UDP_BACKEND_INLINES
#define LOFAR_UDP_BACKEND_INLINES

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
	return  (Xr * Xr) + (Xi * Xi) + (Yr * Yr) + (Yi * Yi);
}

#pragma omp declare simd
inline float stokesQ(float Xr, float Xi, float Yr, float Yi) {
	return  ((Xr * Xr) + (Xi * Xi) - (Yr * Yr) - (Yi * Yi));
}

#pragma omp declare simd
inline float stokesU(float Xr, float Xi, float Yr, float Yi) {
	return  (2.0 * (Xr * Yr) - 3.0 * (Xi * Yi));
}

#pragma omp declare simd
inline float stokesV(float Xr, float Xi, float Yr, float Yi) {
	return 2.0 * ((Xr * Yi) - (Xi * Yr));
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
inline long input_offset_index(long lastInputPacketOffset, int beamlet, int timeStepSize) {
	return lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
}

/**
 * @brief      Get the output index for an output in frequency-reversed major order
 *
 * @param[in]  outputPacketOffset  The output packet offset
 * @param[in]  totalBeamlets       The total beamlets
 * @param[in]  beamlet             The beamlet
 * @param[in]  baseBeamlet         The base beamlet
 * @param[in]  cumulativeBeamlets  The cumulative beamlets
 *
 * @return     { description_of_the_return_value }
 */
inline long frequency_major_index(long outputPacketOffset, int totalBeamlets, int beamlet, int baseBeamlet, int cumulativeBeamlets) {
	return outputPacketOffset + (totalBeamlets - 1 - beamlet + baseBeamlet - cumulativeBeamlets);
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
inline long time_major_index(int beamlet, int baseBeamlet, int cumulativeBeamlets, long packetsPerIteration, long outputTimeIdx) {
	return (((beamlet - baseBeamlet + cumulativeBeamlets) * packetsPerIteration * UDPNTIMESLICE ) + outputTimeIdx);
}

#endif



// Setup templates for each processing mode
#ifndef LOFAR_UDP_BACKEND_TEMPLATES
#define LOFAR_UDP_BACKEND_TEMPLATES

#ifdef __cplusplus

// Apply the calibration from a Jones matrix to a set of X/Y samples
template<typename I, typename O>
void inline calibrateDataFunc(O *Xr, O *Xi, O *Yr, O *Yi, float *beamletJones, char *inputPortData, long tsInOffset, int timeStepSize) {
	(*Xr) = calibrateSample(beamletJones[0], *((I*) &(inputPortData[tsInOffset])), \
							-1.0 * beamletJones[1], *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), \
							beamletJones[2], *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), \
							-1.0 * beamletJones[3], *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));

	(*Xi) = calibrateSample(beamletJones[0], *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), \
							beamletJones[1], *((I*) &(inputPortData[tsInOffset])), \
							beamletJones[2], *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])), \
							beamletJones[3], *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])));

	(*Yr) = calibrateSample(beamletJones[4], *((I*) &(inputPortData[tsInOffset])), \
							-1.0 * beamletJones[5], *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), \
							beamletJones[6], *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), \
							-1.0 * beamletJones[7], *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));

	(*Yi) = calibrateSample(beamletJones[4], *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), \
							beamletJones[5], *((I*) &(inputPortData[tsInOffset])), \
							beamletJones[6], *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])), \
							beamletJones[7], *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])));
}


// All of our processing modes
// This is going to take a while
// See docs/newProcessingMode.md for documentation of the structure of these kernels

// Copy: No change, packet goes in, packet goes out, missing packets are inserted through the configured padding scheme.
template<typename I, typename O>
void inline udp_copy(long iLoop, char *inputPortData, O **outputData, int port, long lastInputPacketOffset, long packetOutputLength) {
	long outputPacketOffset = iLoop * packetOutputLength;
	memcpy(&(outputData[port][outputPacketOffset]), &(inputPortData[lastInputPacketOffset]), packetOutputLength);
}


// Copy No Header: Strip the header, copy the rest of the packet to the output.
template <typename I, typename O>
void inline udp_copyNoHdr(long iLoop, char *inputPortData, O **outputData, int port, long lastInputPacketOffset, long packetOutputLength) {
	long outputPacketOffset = iLoop * packetOutputLength;
	memcpy(&(outputData[port][outputPacketOffset]), &(inputPortData[lastInputPacketOffset]), packetOutputLength);
}


// Copy Split Pols: Take the data across all ports, merge them, then split based on the X/Y real/imaginary state.
template <typename I, typename O, const int calibrateData>
void inline udp_copySplitPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet, float *jonesMatrix) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop

	

	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		tsOutOffset = outputPacketOffset + (beamlet - baseBeamlet + cumulativeBeamlets) * UDPNTIMESLICE;
		
		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr;
				outputData[1][tsOutOffset] = Xi;
				outputData[2][tsOutOffset] = Yr;
				outputData[3][tsOutOffset] = Yi;
			} else {
				outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
				outputData[1][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[2][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[3][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += 1;
		}
	}
}


// Channel Major: Change from being packet majour (16 time samples, N beamlets) to channel majour (M time samples, N beamlets).
template <typename I, typename O, const int calibrateData>
void inline udp_channelMajor(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet, float *jonesMatrix) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		tsOutOffset = outputPacketOffset + (beamlet - baseBeamlet + cumulativeBeamlets) * UDPNPOL;
		
		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr;
				outputData[0][tsOutOffset + 1] = Xi;
				outputData[0][tsOutOffset + 2] = Yr;
				outputData[0][tsOutOffset + 3] = Yi;
			} else {
				outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
				outputData[0][tsOutOffset + 1] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[0][tsOutOffset + 2] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[0][tsOutOffset + 3] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += totalBeamlets * UDPNPOL;
		}
	}
}

// Channel Majour, Split Pols: Change from packet major to beamlet major (described previously), then split data across X/Y real/imaginary state.
template <typename I, typename O, const int calibrateData>
void inline udp_channelMajorSplitPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet, float *jonesMatrix) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		tsOutOffset = outputPacketOffset + beamlet - baseBeamlet + cumulativeBeamlets;
		
		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr;
				outputData[1][tsOutOffset] = Xi;
				outputData[2][tsOutOffset] = Yr;
				outputData[3][tsOutOffset] = Yi;
			} else {
				outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
				outputData[1][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[2][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[3][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += totalBeamlets;
		}
	}
}

template <typename I, typename O, const int calibrateData>
void inline udp_reversedChannelMajor(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet, float *jonesMatrix) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - (beamlet - baseBeamlet + cumulativeBeamlets)) * UDPNPOL;
	
		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr;
				outputData[0][tsOutOffset + 1] = Xi;
				outputData[0][tsOutOffset + 2] = Yr;
				outputData[0][tsOutOffset + 3] = Yi;
			} else {
				outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
				outputData[0][tsOutOffset + 1] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[0][tsOutOffset + 2] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[0][tsOutOffset + 3] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += totalBeamlets * UDPNPOL;
		}
	}
}

template <typename I, typename O, const int calibrateData>
void inline udp_reversedChannelMajorSplitPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet, float *jonesMatrix) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		tsOutOffset = frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet, cumulativeBeamlets);
	
		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}	

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr;
				outputData[1][tsOutOffset] = Xi;
				outputData[2][tsOutOffset] = Yr;
				outputData[3][tsOutOffset] = Yi;
			} else {
				outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
				outputData[1][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[2][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[3][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += totalBeamlets;
		}
	}
}

template <typename I, typename O, const int calibrateData>
void inline udp_timeMajor(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, int timeStepSize, int upperBeamlet, int cumulativeBeamlets, long packetsPerIteration, int baseBeamlet, float *jonesMatrix) {
	long outputTimeIdx = iLoop * UDPNTIMESLICE / sizeof(O);
	long tsInOffset, tsOutOffset;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop


	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		tsOutOffset = 4 * time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputTimeIdx);

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}	

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr; 
				outputData[0][tsOutOffset + 1] = Xi;
				outputData[0][tsOutOffset + 2] = Yr;
				outputData[0][tsOutOffset + 3] = Yi;
			} else {
				outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
				outputData[0][tsOutOffset + 1] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[0][tsOutOffset + 2] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[0][tsOutOffset + 3] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}
			tsInOffset += 4 * timeStepSize;
			tsOutOffset += 4;
		}
	}
}

template <typename I, typename O, const int calibrateData>
void inline udp_timeMajorSplitPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, int timeStepSize, int upperBeamlet, int cumulativeBeamlets, long packetsPerIteration, int baseBeamlet, float *jonesMatrix) {
	long outputTimeIdx = iLoop * UDPNTIMESLICE / sizeof(O);
	long tsInOffset, tsOutOffset;
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop
	
	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		tsOutOffset = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputTimeIdx);

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}	

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);
				
				outputData[0][tsOutOffset] = Xr;
				outputData[1][tsOutOffset] = Xi;
				outputData[1][tsOutOffset] = Yr;
				outputData[3][tsOutOffset] = Yi;
			} else {
				outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
				outputData[1][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[1][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[3][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += 1;
		}
	}
}


// FFTW format
template <typename I, typename O, const int calibrateData>
void inline udp_timeMajorDualPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, int timeStepSize, int upperBeamlet, int cumulativeBeamlets, long packetsPerIteration, int baseBeamlet, float *jonesMatrix) {
	long outputTimeIdx = iLoop * UDPNTIMESLICE / sizeof(O);
	long tsInOffset, tsOutOffset;
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);
		tsOutOffset = 2 * time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputTimeIdx);
	
		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}	

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = Xr;
				outputData[0][tsOutOffset + 1] = Xi; 
				outputData[1][tsOutOffset] = Yr;
				outputData[1][tsOutOffset + 1] = Yi;
			} else {
				outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
				outputData[0][tsOutOffset + 1] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
				outputData[1][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
				outputData[1][tsOutOffset + 1] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi
			}

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += 2;
		}
	}
}


template <typename I, typename O, StokesFuncType stokesFunc, const int order, const int calibrateData>
void inline udp_stokes(long iLoop, char *inputPortData, O **outputData,  long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, long packetsPerIteration, int baseBeamlet, float *jonesMatrix) {
	
	long outputPacketOffset;
	if constexpr (order == 0) {
		outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	} else {
		outputPacketOffset = iLoop * UDPNTIMESLICE / sizeof(O);
	}

	long tsInOffset, tsOutOffset;
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);

		if constexpr (order == 0) {
			tsOutOffset = frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet, cumulativeBeamlets);
	 	} else {
	 		tsOutOffset = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
	 	}

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = (*stokesFunc)(Xr, Xi, Yr, Yi);
			} else {
				outputData[0][tsOutOffset] = (*stokesFunc)(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}

			tsInOffset += 4 * timeStepSize;
			if constexpr (order == 0) {
				tsOutOffset += totalBeamlets;
			} else {
				tsOutOffset += 4;
			}
		}
	}
}

template <typename I, typename O, StokesFuncType stokesFunc, const int order, const int factor, const int calibrateData>
void inline udp_stokesDecimation(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, long packetsPerIteration, int baseBeamlet, float *jonesMatrix) {
	
	long outputPacketOffset;
	if constexpr (order == 0) {
		outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	} else {
		outputPacketOffset = iLoop * UDPNTIMESLICE / sizeof(O);
	}

	long tsInOffset, tsOutOffset;
	O tempVal;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop
	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);

		if constexpr (order == 0) {
			tsOutOffset = frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet, cumulativeBeamlets);
		} else {
			tsOutOffset = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
		}

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}

		tempVal = (float) 0.0;

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				tempVal += (*stokesFunc)(Xr, Xi, Yr, Yi);
			} else {
				tempVal += (*stokesFunc)(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}


			tsInOffset += 4 * timeStepSize;
			if ((ts + 1) % factor == 0) {
				outputData[0][tsOutOffset] = tempVal;
				tempVal = (float) 0.0;
				if constexpr (order == 0) {
					tsOutOffset += totalBeamlets;
				} else {
					tsOutOffset += 4;
				}
			}
		}
	}
}

template <typename I, typename O, const int order, const int calibrateData>
void inline udp_fullStokes(long iLoop, char *inputPortData, O **outputData,  long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, long packetsPerIteration, int baseBeamlet, float *jonesMatrix) {
	
	long outputPacketOffset;
	if constexpr (order == 0) {
		outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	} else {
		outputPacketOffset = iLoop * UDPNTIMESLICE / sizeof(O);
	}

	long tsInOffset, tsOutOffset;
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop

	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);

		if constexpr (order == 0) {
			tsOutOffset = frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet, cumulativeBeamlets);
		} else {
			tsOutOffset = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
		}

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = stokesI(Xr, Xi, Yr, Yi);
				outputData[1][tsOutOffset] = stokesQ(Xr, Xi, Yr, Yi);
				outputData[2][tsOutOffset] = stokesU(Xr, Xi, Yr, Yi);
				outputData[3][tsOutOffset] = stokesV(Xr, Xi, Yr, Yi);

			 } else {
				outputData[0][tsOutOffset] = stokesI(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				outputData[1][tsOutOffset] = stokesQ(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				outputData[2][tsOutOffset] = stokesU(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				outputData[3][tsOutOffset] = stokesV(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}

			tsInOffset += 4 * timeStepSize;
			if constexpr (order == 0) {
				tsOutOffset += totalBeamlets;
			} else {
				tsOutOffset += 4;
			}
		}
	}
}

template <typename I, typename O, const int order, const int factor, const int calibrateData>
void inline udp_fullStokesDecimation(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, long packetsPerIteration, int baseBeamlet, float *jonesMatrix) {
	
	long outputPacketOffset;
	if constexpr (order == 0) {
		outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	} else {
		outputPacketOffset = iLoop * UDPNTIMESLICE / sizeof(O);
	}

	long tsInOffset, tsOutOffset;
	O tempValI, tempValQ, tempValU, tempValV;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop
	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);

		if constexpr (order == 0) {
			tsOutOffset = frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet, cumulativeBeamlets);
		} else {
			tsOutOffset = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
		}

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}

		// This is split into 2 inner loops as ICC generates garbage outputs when the loop is run on the full inner loop.
		// Should still be relatively efficient as 16 time samples fit inside one cache line on REALTA, so they should never be dropped
		// But we'll slow down when we are claibrating the data.
		tempValI = (float) 0.0;
		tempValQ = (float) 0.0;

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				tempValI += stokesI(Xr, Xi, Yr, Yi);
				tempValQ += stokesQ(Xr, Xi, Yr, Yi);
			} else {
				tempValI += stokesI(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				tempValQ += stokesQ(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}

			tsInOffset += 4 * timeStepSize;
			if ((ts + 1) % factor == 0) {
				outputData[0][tsOutOffset] = tempValI;
				outputData[1][tsOutOffset] = tempValQ;
				tempValI = (float) 0.0;
				tempValQ = (float) 0.0;

				if constexpr (order == 0) {
					tsOutOffset += totalBeamlets;
				} else {
					tsOutOffset += 4;
				}
			}
		}

		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);

		if constexpr (order == 0) {
			tsOutOffset = frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet, cumulativeBeamlets);
		} else {
			tsOutOffset = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
		}
		
		tempValU = (float) 0.0;
		tempValV = (float) 0.0;

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				tempValU += stokesU(Xr, Xi, Yr, Yi);
				tempValV += stokesV(Xr, Xi, Yr, Yi);
			} else {
				tempValU += stokesU(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				tempValV += stokesV(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}

			tsInOffset += 4 * timeStepSize;
			if ((ts + 1) % factor == 0) {
				outputData[2][tsOutOffset] = tempValU;
				outputData[3][tsOutOffset] = tempValV;
				tempValU = (float) 0.0;
				tempValV = (float) 0.0;

				if constexpr (order == 0) {
					tsOutOffset += totalBeamlets;
				} else {
					tsOutOffset += 4;
				}
			}
		}
	}
}

template <typename I, typename O, const int order, const int calibrateData>
void inline udp_usefulStokes(long iLoop, char *inputPortData, O **outputData,  long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, long packetsPerIteration, int baseBeamlet, float *jonesMatrix) {
	
	long outputPacketOffset;
	if constexpr (order == 0) {
		outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	} else {
		outputPacketOffset = iLoop * UDPNTIMESLICE / sizeof(O);
	}

	long tsInOffset, tsOutOffset;
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);

		if constexpr (order == 0) {
			tsOutOffset = frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet, cumulativeBeamlets);
		} else {
			tsOutOffset = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
		}

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				outputData[0][tsOutOffset] = stokesI(Xr, Xi, Yr, Yi);
				outputData[1][tsOutOffset] = stokesV(Xr, Xi, Yr, Yi);
			} else {
				outputData[0][tsOutOffset] = stokesI(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				outputData[1][tsOutOffset] = stokesV(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}

			tsInOffset += 4 * timeStepSize;
			if constexpr (order == 0) {
				tsOutOffset += totalBeamlets;
			} else {
				tsOutOffset += 4;
			}
		}
	}
}

template <typename I, typename O, const int order, const int factor, const int calibrateData>
void inline udp_usefulStokesDecimation(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, long packetsPerIteration, int baseBeamlet, float *jonesMatrix) {
	
	long outputPacketOffset;
	if constexpr (order == 0) {
		outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	} else {
		outputPacketOffset = iLoop * UDPNTIMESLICE / sizeof(O);
	}

	long tsInOffset, tsOutOffset;
	O tempValI, tempValV;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	O Xr, Xi, Yr, Yi;
	float *beamletJones;
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop
	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = input_offset_index(lastInputPacketOffset, beamlet, timeStepSize);

		if constexpr (order == 0) {
			tsOutOffset = frequency_major_index(outputPacketOffset, totalBeamlets, beamlet, baseBeamlet, cumulativeBeamlets);
		} else {
			tsOutOffset = time_major_index(beamlet, baseBeamlet, cumulativeBeamlets, packetsPerIteration, outputPacketOffset);
		}

		if constexpr (calibrateData) {
			beamletJones = &(jonesMatrix[(beamlet - baseBeamlet) * JONESMATSIZE]);
		}

		tempValI = (float) 0.0;
		tempValV = (float) 0.0;

		#pragma omp simd
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			if constexpr (calibrateData) {
				calibrateDataFunc<I, O>(&Xr, &Xi, &Yr, &Yi, beamletJones, inputPortData, tsInOffset, timeStepSize);

				tempValI += stokesI(Xr, Xi, Yr, Yi);
				tempValV += stokesV(Xr, Xi, Yr, Yi);
			} else {
				tempValI += stokesI(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
				tempValV += stokesV(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			}

			tsInOffset += 4 * timeStepSize;
			if ((ts + 1) % factor == 0) {
				outputData[0][tsOutOffset] = tempValI;
				outputData[1][tsOutOffset] = tempValV;
				tempValI = (float) 0.0;
				tempValV = (float) 0.0;

				if constexpr (order == 0) {
					tsOutOffset += totalBeamlets;
				} else {
					tsOutOffset += 4;
				}
			}
		}
	}
}


// Define the main processing loop
template <typename I, typename O, const int state, const int calibrateData>
int lofar_udp_raw_loop(lofar_udp_meta *meta) {
	// Setup return variable
	int packetLoss = 0;

	VERBOSE(const int verbose = meta->VERBOSE);
	// Calculate the true processing mode (4-bit -> +4000)
	constexpr int trueState = state % 4000;

	// Setup decimation factor, 4-bit workspaces if needed
	// Silence compiler warnings as this variable is only needed for some processing modes
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	#pragma GCC diagnostic ignored "-Wuninitialized"
	#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	#pragma GCC diagnostic push
	constexpr int decimation = 1 << (state % 10);

	char **byteWorkspace;
	if constexpr (state >= 4010) {
		byteWorkspace = (char**) malloc(sizeof(char *) * OMP_THREADS);
		VERBOSE(if (verbose) printf("Allocating %ld bytes at %p\n", sizeof(char *) * OMP_THREADS, (void *) byteWorkspace););
		int maxPacketSize = 0;
		for (int port = 0; port < meta->numPorts; port++) {
			if (meta->portPacketLength[port] > maxPacketSize) {
				maxPacketSize = meta->portPacketLength[port];
			}
		}
		for (int i = 0; i < OMP_THREADS; i++) {
			byteWorkspace[i] = (char*) malloc(2 * maxPacketSize - 2 * UDPHDRLEN * sizeof(char));
			VERBOSE(if (verbose) printf("Allocating %d bytes at %p\n", 2 * maxPacketSize - 2 * UDPHDRLEN, (void *) byteWorkspace[i]););
		}
	}
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop


	// Setup working variables	
	VERBOSE(if (verbose) printf("Begining processing for state %d, with input %ld and output %ld\n", state, sizeof(I), sizeof(O)););
	
	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	// For each port of data provided,
	#pragma omp parallel for 
	for (int port = 0; port < meta->numPorts; port++) {

		VERBOSE(if (verbose) printf("Port: %d on thread %d\n", port, omp_get_thread_num()));

		long lastPortPacket, currentPortPacket, inputPacketOffset, lastInputPacketOffset, iWork, iLoop;

		// Reset the dropped packets counter
		meta->portLastDroppedPackets[port] = 0;
		int currentPacketsDropped = 0, nextSequence;

		// Reset last packet, reference data on the current port
		lastPortPacket = meta->lastPacket;
		char *inputPortData = meta->inputData[port];
		O  **outputData = (O**) meta->outputData;

		// Silence compiler warnings as this variable is only needed for some processing modes
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-variable"
		#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
		#pragma GCC diagnostic push
		#pragma GCC diagnostic push

		// Select beamlet parameters
		const int baseBeamlet = meta->baseBeamlets[port];
		const int upperBeamlet = meta->upperBeamlets[port];

		const int cumulativeBeamlets = meta->portCumulativeBeamlets[port];
		const int totalBeamlets = meta->totalProcBeamlets;


		// Select Jones Matrix if performing Calibration
		float *jonesMatrix;
		if constexpr (calibrateData) {
			printf("Beamlets %d: %d, %d\n", port, baseBeamlet, upperBeamlet);
			jonesMatrix = (float*) calloc((upperBeamlet - baseBeamlet) * JONESMATSIZE, sizeof(float));
			for (int i = 0; i < (upperBeamlet - baseBeamlet); i++) {
				for (int j = 0; j < JONESMATSIZE; j++) {
					jonesMatrix[i * JONESMATSIZE + j] = meta->jonesMatrices[meta->calibrationStep][(cumulativeBeamlets + i) * JONESMATSIZE + j];
				}
			}
		}

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int portPacketLength = meta->portPacketLength[port];
		const int packetOutputLength = meta->packetOutputLength[0];
		const int timeStepSize = sizeof(I) / sizeof(char);
		#pragma GCC diagnostic pop
		#pragma GCC diagnostic pop

		lastInputPacketOffset = (-2 + meta->replayDroppedPackets) * portPacketLength; 	// We request at least 2 packets are malloc'd before the array head pointer, so no SEGFAULTs here
														// -2 * PPL = 0s -1 * PPL = last processed packet -- used for calcuating offset in dropped case if 
														// 	we have packet loss on the boundary.
		VERBOSE(if (verbose) printf("LPP: %ld, PPL: %d, LIPO: %ld\n", lastPortPacket, portPacketLength, lastInputPacketOffset));
		// iWork -- input data offset calculations (account for dropped packets)
		// iLoop -- output data offsets
		// These are the same if there is no packet loss.
		// 
		// Reset iWork, inputPacketOffset, read in the first packet's number.
		iWork = 0, inputPacketOffset = 0;
		currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));

		VERBOSE(if (verbose) printf("Port %d: Packet %ld, iters %d, base %d, upper %d, cumulative %d, total %d, outputLength %d, timeStep %d, decimation %d, trueState %d\n", \
					port, currentPortPacket, packetsPerIteration, baseBeamlet, upperBeamlet, cumulativeBeamlets, totalBeamlets, packetOutputLength, timeStepSize, decimation, trueState););
		
		for (iLoop = 0; iLoop < packetsPerIteration; iLoop++) {
			VERBOSE(if (verbose == 2) printf("Loop %ld, Work %ld, packet %ld, target %ld\n", iLoop, iWork, currentPortPacket, lastPortPacket + 1));

			// Check for packet loss by ensuring we have sequential packet numbers
			if (currentPortPacket != (lastPortPacket + 1)) {
				// Check if a packet is out of order; if so, drop the packet
				// TODO: Better future option: check if the packet is in this block, copy and overwrite padded packed instead
				VERBOSE(if (verbose == 2) printf("Packet %ld is not the expected packet, %ld.\n", currentPortPacket, lastPortPacket + 1));
				if (currentPortPacket < lastPortPacket) {
					
					VERBOSE(if (verbose == 2) printf("Packet %ld on port %d is out of order; dropping.\n", currentPortPacket, port));
					// Dropped packet -> index not processed -> effectively an 'added' packet, decrement the dropped packet count
					// 	so that we don't include an extra packet in shift operations
					currentPacketsDropped -= 1;

					iWork++;
					if (iWork != packetsPerIteration) {
						inputPacketOffset = iWork * portPacketLength;
						currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					}
					continue;

				}

				// Packet dropped:
				//  Trip the return int
				//	Increment the port counter for this operation
				//	Increment the last packet offset so it can be used again next time, including the new offset
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
					nextSequence = lofar_get_next_packet_sequence(&(inputPortData[lastInputPacketOffset]));
					memcpy(&(inputPortData[inputPacketOffset + 12]), &nextSequence, 4);
					// The last bit of the 'source' short isn't used; leave a signature that this packet was modified
					// Mask off the rest of the byte before adding encase we've used this packet before
					inputPortData[inputPacketOffset + 2] = (inputPortData[inputPacketOffset + 2] & 127) + 128;
				}

				VERBOSE(if (verbose == 2) printf("Packet %ld on port %d is missing; padding.\n", lastPortPacket + 1, port));

			} else {
				VERBOSE(if (verbose == 2) printf("Packet %ld is the expected packet.\n", currentPortPacket));

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
					if  (*((unsigned int*) &(inputPortData[inputPacketOffset + 12]))  == (*((unsigned int*) &(inputPortData[lastInputPacketOffset -4]))) + 16)  {
						currentPortPacket += 1;
					} else {
						currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					}
				}

			}


			// Use firstprivate to lock 4-bit variables in a task, create a cache variable otherwise
			#pragma omp task firstprivate(iLoop, lastInputPacketOffset, inputPortData) shared(byteWorkspace, outputData)
			{

			// Unpacket 4-bit data into an array of chars, so it can be processed the same way we process 8-bit data
			if constexpr (state >= 4010) {
				// Get the workspace for the current packet
				inputPortData = byteWorkspace[omp_get_thread_num()];

				// Determine the number of (byte-sized) samples to process
				int numSamples = portPacketLength - UDPHDRLEN;

				// Use a LUT to extract the 4-bit signed ints from signed chars
				#ifdef __INTEL_COMPILER
				#pragma unroll(976)
				#else
				#pragma GCC unroll 976
				#endif
				for (int idx = 0; idx < numSamples; idx++) {
					#pragma GCC diagnostic push
					#pragma GCC diagnostic ignored "-Wchar-subscripts"
					const char *result = bitmodeConversion[(unsigned char) meta->inputData[port][lastInputPacketOffset + idx]];
					#pragma GCC diagnostic pop
					inputPortData[idx * 2] = result[0];
					inputPortData[idx * 2 + 1] = result[1];
				}

				VERBOSE(if (verbose == 2 && port == 0 && iLoop == 0) {
					for (int i = 0; i < numSamples - UDPHDRLEN; i++)
						printf("%d, ", inputPortData[i]);
					printf("\n");
				};);

				// We have data at the start of the array, reset the pointer offset to 0
				lastInputPacketOffset = 0;
			}

			// Effectively a large switch statement, but more performant as it's decided at compile time.
			if constexpr (trueState == 0) {
				udp_copy<char, char>(iLoop, inputPortData, (char**) outputData, port, lastInputPacketOffset, packetOutputLength);
			} else if constexpr (trueState == 1) {
				udp_copyNoHdr<char, char>(iLoop, inputPortData, (char**) outputData, port, lastInputPacketOffset, packetOutputLength);
			} else if constexpr (trueState == 2) {
				udp_copySplitPols<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, upperBeamlet, cumulativeBeamlets, baseBeamlet, jonesMatrix);
			



			} else if constexpr (trueState == 10) {
				udp_channelMajor<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 11) {
				udp_channelMajorSplitPols<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet, jonesMatrix);
			



			} else if constexpr (trueState == 20) {
				udp_reversedChannelMajor<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 21) {
				udp_reversedChannelMajorSplitPols<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet, jonesMatrix);
			



			} else if constexpr (trueState == 30) {
				udp_timeMajor<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, timeStepSize, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 31) {
				udp_timeMajorSplitPols<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, timeStepSize, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 32) {
				udp_timeMajorDualPols<I, O, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, timeStepSize, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			



			} else if constexpr (trueState == 100) {
				udp_stokes<I, O, stokesI, 0, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 110) {
				udp_stokes<I, O, stokesQ, 0, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 120) {
				udp_stokes<I, O, stokesU, 0, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 130) {
				udp_stokes<I, O, stokesV, 0, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 150) {
				udp_fullStokes<I, O, 0, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 160) {
				udp_usefulStokes<I, O, 0, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);




			} else if constexpr (trueState >= 101 && trueState <= 104) {
				udp_stokesDecimation<I, O, stokesI, 0, decimation, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState >= 111 && trueState <= 114) {
				udp_stokesDecimation<I, O, stokesQ, 0, decimation, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState >= 121 && trueState <= 124) {
				udp_stokesDecimation<I, O, stokesU, 0, decimation, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState >= 131 && trueState <= 134) {
				udp_stokesDecimation<I, O, stokesV, 0, decimation, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState >= 151 && trueState <= 154) {
				udp_fullStokesDecimation<I, O, 0, decimation, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState >= 161 && trueState <= 164) {
				udp_usefulStokesDecimation<I, O, 0, decimation, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			

			} else if constexpr (trueState == 200) {
				udp_stokes<I, O, stokesI, 1, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 210) {
				udp_stokes<I, O, stokesQ, 1, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 220) {
				udp_stokes<I, O, stokesU, 1, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 230) {
				udp_stokes<I, O, stokesV, 1, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 250) {
				udp_fullStokes<I, O, 1, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState == 260) {
				udp_usefulStokes<I, O, 1, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);




			} else if constexpr (trueState >= 201 && trueState <= 204) {
				udp_stokesDecimation<I, O, stokesI, 1, decimation, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState >= 211 && trueState <= 214) {
				udp_stokesDecimation<I, O, stokesQ, 1, decimation, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState >= 221 && trueState <= 224) {
				udp_stokesDecimation<I, O, stokesU, 1, decimation, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState >= 231 && trueState <= 234) {
				udp_stokesDecimation<I, O, stokesV, 1, decimation, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState >= 251 && trueState <= 254) {
				udp_fullStokesDecimation<I, O, 1, decimation, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			} else if constexpr (trueState >= 261 && trueState <= 264) {
				udp_usefulStokesDecimation<I, O, 1, decimation, calibrateData>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet, jonesMatrix);
			


			} else {
				fprintf(stderr, "Unknown processing mode %d, exiting.\n", state);
				exit(1);
			}


			// End task block, update cached variables as needed
			}
		}

		// Perform analysis of iteration, cleanup and wait for tasks to end.
		if constexpr (calibrateData) {
			printf("Free'ing calibration Jones\n");
			free(jonesMatrix);
		}

		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));

		#pragma omp taskwait

		VERBOSE(if (verbose) printf("Port %d finished loop.\n", port););

	}

	for (int port = 0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < (-0.001 * (float) packetsPerIteration)) {
			fprintf(stderr, "A large number of packets were out of order on port %d; this normally indicates a data integrity issue, exiting...", port);
			return 1;
		}
	}

	for (int port =0; port < meta->numPorts; port++) {
		if (meta->portLastDroppedPackets[port] < 0 && meta->replayDroppedPackets) {
			// TODO: pad the end of the arrays if the data doesn't fill it.
			// Or just re-loop with fresh data and report less data back?
			// Maybe rearchitect a bit, overread the amount of data needed, only return what's needed.
		}
	}

	// Update the last packet variable
	meta->lastPacket += packetsPerIteration;
	VERBOSE(if (verbose) printf("Exiting operation, last packet was %ld\n", meta->lastPacket));

	// Update the input/output states
	meta->inputDataReady = 0;
	meta->outputDataReady = 1;
	meta->calibrationStep += 1;


	// If needed, free the 4-bit workspace
	if constexpr (state >= 4010) {
		for (int i = 0; i < OMP_THREADS; i++) {
			VERBOSE(if (verbose) printf("freeing byteWorkspace data at %p\n", (void *) byteWorkspace[i]););
			free(byteWorkspace[i]);
		}
		VERBOSE(if (verbose) printf("byteWorkspaceSubArrays free'd\n"););
		free(byteWorkspace);
		VERBOSE(if (verbose) printf("byteWorkspace free'd\n"););
	}

	return packetLoss;
}


#endif
#endif
