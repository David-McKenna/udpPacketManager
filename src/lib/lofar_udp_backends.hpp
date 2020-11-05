#ifndef LOFAR_UDP_BACKENDS_H
#define LOFAR_UDP_BACKENDS_H

typedef float(StokesFuncType)(float, float, float, float);

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


float stokesI(float Xr, float Xi, float Yr, float Yi);
float stokesQ(float Xr, float Xi, float Yr, float Yi);
float stokesU(float Xr, float Xi, float Yr, float Yi);
float stokesV(float Xr, float Xi, float Yr, float Yi);

int lofar_udp_raw_udp_copy_cpp(lofar_udp_meta *meta);


// Define the C-access interfaces
#ifdef __cplusplus
extern "C" {
#endif
int lofar_udp_raw_udp_copy(lofar_udp_meta *meta);
int lofar_udp_raw_udp_copy_nohdr(lofar_udp_meta *meta);
int lofar_udp_raw_udp_copy_split_pols(lofar_udp_meta *meta);
int lofar_udp_raw_udp_channel_major(lofar_udp_meta *meta);
int lofar_udp_raw_udp_channel_major_split_pols(lofar_udp_meta *meta);
int lofar_udp_raw_udp_reversed_channel_major(lofar_udp_meta *meta);
int lofar_udp_raw_udp_reversed_channel_major_split_pols(lofar_udp_meta *meta);
int lofar_udp_raw_udp_time_major(lofar_udp_meta *meta);
int lofar_udp_raw_udp_time_major_split_pols(lofar_udp_meta *meta);
int lofar_udp_raw_udp_time_major_dual_pols(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesI(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesQ(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesU(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesV(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesI_sum2(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesQ_sum2(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesU_sum2(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesV_sum2(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesI_sum4(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesQ_sum4(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesU_sum4(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesV_sum4(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesI_sum8(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesQ_sum8(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesU_sum8(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesV_sum8(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesI_sum16(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesQ_sum16(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesU_sum16(lofar_udp_meta *meta);
int lofar_udp_raw_udp_stokesV_sum16(lofar_udp_meta *meta);
int lofar_udp_raw_udp_full_stokes(lofar_udp_meta *meta);
int lofar_udp_raw_udp_full_stokes_sum2(lofar_udp_meta *meta);
int lofar_udp_raw_udp_full_stokes_sum4(lofar_udp_meta *meta);
int lofar_udp_raw_udp_full_stokes_sum8(lofar_udp_meta *meta);
int lofar_udp_raw_udp_full_stokes_sum16(lofar_udp_meta *meta);
int lofar_udp_raw_udp_useful_stokes(lofar_udp_meta *meta);
int lofar_udp_raw_udp_useful_stokes_sum2(lofar_udp_meta *meta);
int lofar_udp_raw_udp_useful_stokes_sum4(lofar_udp_meta *meta);
int lofar_udp_raw_udp_useful_stokes_sum8(lofar_udp_meta *meta);
int lofar_udp_raw_udp_useful_stokes_sum16(lofar_udp_meta *meta);
#ifdef __cplusplus
}
#endif

#endif

// Setup templates for each processing mode
#ifndef LOFAR_UDP_BACKEND_TEMPLATES
#define LOFAR_UDP_BACKEND_TEMPLATES

#ifdef __cplusplus
template<typename I, typename O>
void inline udp_copy(long iLoop, char *inputPortData, O **outputData, int port, long lastInputPacketOffset, long packetOutputLength) {
	long outputPacketOffset = iLoop * packetOutputLength;
	memcpy(&(outputData[port][outputPacketOffset]), &(inputPortData[lastInputPacketOffset]), packetOutputLength);
}


template <typename I, typename O>
void inline udp_copyNoHdr(long iLoop, char *inputPortData, O **outputData, int port, long lastInputPacketOffset, long packetOutputLength) {
	long outputPacketOffset = iLoop * packetOutputLength;
	memcpy(&(outputData[port][outputPacketOffset]), &(inputPortData[lastInputPacketOffset]), packetOutputLength);
}


template <typename I, typename O>
void inline udp_copySplitPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (beamlet - baseBeamlet + cumulativeBeamlets) * UDPNTIMESLICE;

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
			outputData[1][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
			outputData[2][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
			outputData[3][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += 1;
		}
	}
}


template <typename I, typename O>
void inline udp_channelMajor(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (beamlet - baseBeamlet + cumulativeBeamlets) * UDPNPOL;

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
			outputData[0][tsOutOffset + 1] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
			outputData[0][tsOutOffset + 2] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
			outputData[0][tsOutOffset + 3] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += totalBeamlets * UDPNPOL;
		}
	}
}

template <typename I, typename O>
void inline udp_channelMajorSplitPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + beamlet - baseBeamlet + cumulativeBeamlets;

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
			outputData[1][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
			outputData[2][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
			outputData[3][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += totalBeamlets;
		}
	}
}

template <typename I, typename O>
void inline udp_reversedChannelMajor(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - (beamlet - baseBeamlet + cumulativeBeamlets)) * UDPNPOL;

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
			outputData[0][tsOutOffset + 1] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
			outputData[0][tsOutOffset + 2] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
			outputData[0][tsOutOffset + 3] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += totalBeamlets * UDPNPOL;
		}
	}
}

template <typename I, typename O>
void inline udp_reversedChannelMajorSplitPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - beamlet + baseBeamlet - cumulativeBeamlets);
		
		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
			outputData[1][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
			outputData[2][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
			outputData[3][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += totalBeamlets;
		}
	}
}

template <typename I, typename O>
void inline udp_timeMajor(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, int timeStepSize, int upperBeamlet, int cumulativeBeamlets, long packetsPerIteration, int baseBeamlet) {
	long outputTimeIdx = iLoop * UDPNTIMESLICE / sizeof(O);
	long tsInOffset, tsOutOffset;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = 4 * (((beamlet - baseBeamlet + cumulativeBeamlets) * packetsPerIteration * UDPNTIMESLICE ) + outputTimeIdx);
		
		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
			outputData[0][tsOutOffset + 1] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
			outputData[0][tsOutOffset + 2] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
			outputData[0][tsOutOffset + 3] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += 4;
		}
	}
}

template <typename I, typename O>
void inline udp_timeMajorSplitPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, int timeStepSize, int upperBeamlet, int cumulativeBeamlets, long packetsPerIteration, int baseBeamlet) {
	long outputTimeIdx = iLoop * UDPNTIMESLICE / sizeof(O);
	long tsInOffset, tsOutOffset;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = ((beamlet - baseBeamlet + cumulativeBeamlets) * packetsPerIteration * UDPNTIMESLICE ) + outputTimeIdx;
		
		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
			outputData[1][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
			outputData[1][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
			outputData[3][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += 1;
		}
	}
}


// FFTW format
template <typename I, typename O>
void inline udp_timeMajorDualPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, int timeStepSize, int upperBeamlet, int cumulativeBeamlets, long packetsPerIteration, int baseBeamlet) {
	long outputTimeIdx = iLoop * UDPNTIMESLICE / sizeof(O);
	long tsInOffset, tsOutOffset;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = 2 * ((beamlet - baseBeamlet + cumulativeBeamlets) * packetsPerIteration * UDPNTIMESLICE + outputTimeIdx);
		
		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			outputData[0][tsOutOffset] = *((I*) &(inputPortData[tsInOffset])); // Xr
			outputData[0][tsOutOffset + 1] = *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])); // Xi
			outputData[1][tsOutOffset] = *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])); // Yr
			outputData[1][tsOutOffset + 1] = *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])); // Yi

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += 2;
		}
	}
}


template <typename I, typename O, StokesFuncType stokesFunc>
void inline udp_stokes(long iLoop, char *inputPortData, O **outputData,  long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - beamlet + baseBeamlet - cumulativeBeamlets);

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			outputData[0][tsOutOffset] = (*stokesFunc)(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += totalBeamlets;
		}
	}
}

template <typename I, typename O, StokesFuncType stokesFunc, int factor>
void inline udp_stokesDecimation(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;
	O tempVal;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - beamlet + baseBeamlet - cumulativeBeamlets);
		tempVal = 0.0;

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			tempVal += (*stokesFunc)(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));

			tsInOffset += 4 * timeStepSize;
			if ((ts + 1) % factor == 0) {
				outputData[0][tsOutOffset] = tempVal;
				tempVal = 0.0;
				tsOutOffset += totalBeamlets;
			}
		}
	}
}

template <typename I, typename O>
void inline udp_fullStokes(long iLoop, char *inputPortData, O **outputData,  long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - beamlet + baseBeamlet - cumulativeBeamlets);

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			outputData[0][tsOutOffset] = stokesI(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			outputData[1][tsOutOffset] = stokesQ(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			outputData[2][tsOutOffset] = stokesU(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			outputData[3][tsOutOffset] = stokesV(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += totalBeamlets;
		}
	}
}

template <typename I, typename O, int factor>
void inline udp_fullStokesDecimation(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;
	O tempValI, tempValQ, tempValU, tempValV;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - beamlet + baseBeamlet - cumulativeBeamlets);


		// This is split into 2 inner loops as ICC generates garbage outputs when the loop is run on the full inner loop.
		// Should still be relatively efficient as 16 time samples fit inside one cache line on REALTA, so they should never be dropped
		tempValI = 0.0;
		tempValQ = 0.0;

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			tempValI += stokesI(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			tempValQ += stokesQ(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));

			tsInOffset += 4 * timeStepSize;
			if ((ts + 1) % factor == 0) {
				outputData[0][tsOutOffset] = tempValI;
				outputData[1][tsOutOffset] = tempValQ;
				tempValI = 0.0;
				tempValQ = 0.0;

				tsOutOffset += totalBeamlets;
			}
		}

		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - beamlet + baseBeamlet - cumulativeBeamlets);
		tempValU = 0.0;
		tempValV = 0.0;

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			tempValU += stokesU(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			tempValV += stokesV(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));

			tsInOffset += 4 * timeStepSize;
			if ((ts + 1) % factor == 0) {
				outputData[2][tsOutOffset] = tempValU;
				outputData[3][tsOutOffset] = tempValV;
				tempValU = 0.0;
				tempValV = 0.0;

				tsOutOffset += totalBeamlets;
			}
		}
	}


	

}

template <typename I, typename O>
void inline udp_usefulStokes(long iLoop, char *inputPortData, O **outputData,  long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - beamlet + baseBeamlet - cumulativeBeamlets);

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			outputData[0][tsOutOffset] = stokesI(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			outputData[1][tsOutOffset] = stokesV(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));

			tsInOffset += 4 * timeStepSize;
			tsOutOffset += totalBeamlets;

		}
	}
}

template <typename I, typename O, int factor>
void inline udp_usefulStokesDecimation(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int upperBeamlet, int cumulativeBeamlets, int baseBeamlet) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;
	O tempValI, tempValV;

	
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = baseBeamlet; beamlet < upperBeamlet; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - beamlet + baseBeamlet - cumulativeBeamlets);
		tempValI = 0.0;
		tempValV = 0.0;

		#ifdef __INTEL_COMPILER
		#pragma omp simd
		#else
		#pragma GCC unroll 16
		#endif
		for (int ts = 0; ts < UDPNTIMESLICE; ts++) {
			tempValI += stokesI(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));
			tempValV += stokesV(*((I*) &(inputPortData[tsInOffset])), *((I*) &(inputPortData[tsInOffset + 1 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 2 * timeStepSize])), *((I*) &(inputPortData[tsInOffset + 3 * timeStepSize])));

			tsInOffset += 4 * timeStepSize;
			if ((ts + 1) % factor == 0) {
				outputData[0][tsOutOffset] = tempValI;
				outputData[1][tsOutOffset] = tempValV;
				tempValI = 0.0;
				tempValV = 0.0;

				tsOutOffset += totalBeamlets;
			}
		}
	}
}


// Define the main processing loop
template <typename I, typename O, const int state>
int lofar_udp_raw_loop(lofar_udp_meta *meta) {
	// Setup return variable
	int packetLoss = 0;

	VERBOSE(const int verbose = meta->VERBOSE);
	constexpr int trueState = state % 4000;

	// Confirm number of OMP threads
	omp_set_num_threads(OMP_THREADS);

	// Setup decimation factor, 4-bit workspaces if needed
	// Silence compiler warnings as this variable is only needed for some processing modes
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
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



	// Setup working variables	
	VERBOSE(if (verbose) printf("Begining processing for state %d, with input %ld and output %ld\n", state, sizeof(I), sizeof(O)););
	
	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	// For each port of data provided,
	#pragma omp parallel for 
	for (int port = 0; port < meta->numPorts; port++) {
		VERBOSE(if (verbose) printf("Port: %d on thread %d\n", port, omp_get_thread_num()));

		long lastPortPacket, currentPortPacket, inputPacketOffset, lastInputPacketOffset, iWork, iLoop;

		// On GCC, keep a cache of the last inputPacketOffset while operating in 4-bit mode
		#ifndef __INTEL_COMPILER
		long LIPOCache;
		#endif

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
		const int baseBeamlet = meta->baseBeamlets[port];
		const int upperBeamlet = meta->upperBeamlets[port];

		const int cumulativeBeamlets = meta->portCumulativeBeamlets[port];
		const int totalBeamlets = meta->totalProcBeamlets;

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
					// Speedup: add 4 to the sequence, check if accurate. Doesn't work at rollover.
					if  (((char_unsigned_int*) &(inputPortData[inputPacketOffset + 12]))->ui  == (((char_unsigned_int*) &(inputPortData[lastInputPacketOffset -4])))->ui + 16)  {
						currentPortPacket += 1;
					} else {
						currentPortPacket = lofar_get_packet_number(&(inputPortData[inputPacketOffset]));
					}
				}

			}


			// Use firstprivate to lock 4-bit variables in a task, create a cache variable otherwise
			#ifdef __INTEL_COMPILER
			#pragma omp task firstprivate(iLoop, lastInputPacketOffset, inputPortData) shared(byteWorkspace, outputData)
			{
			#else
				LIPOCache = lastInputPacketOffset;
			#endif

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

				VERBOSE(if (verbose && port == 0 && iLoop == 0) {
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
				udp_copySplitPols<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			



			} else if constexpr (trueState == 10) {
				udp_channelMajor<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			} else if constexpr (trueState == 11) {
				udp_channelMajorSplitPols<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			



			} else if constexpr (trueState == 20) {
				udp_reversedChannelMajor<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			} else if constexpr (trueState == 21) {
				udp_reversedChannelMajorSplitPols<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			



			} else if constexpr (trueState == 30) {
				udp_timeMajor<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, timeStepSize, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet);
			} else if constexpr (trueState == 31) {
				udp_timeMajorSplitPols<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, timeStepSize, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet);
			} else if constexpr (trueState == 32) {
				udp_timeMajorDualPols<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, timeStepSize, upperBeamlet, cumulativeBeamlets, packetsPerIteration, baseBeamlet);
			



			} else if constexpr (trueState == 100) {
				udp_stokes<I, O, stokesI>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			} else if constexpr (trueState == 110) {
				udp_stokes<I, O, stokesQ>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			} else if constexpr (trueState == 120) {
				udp_stokes<I, O, stokesU>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			} else if constexpr (trueState == 130) {
				udp_stokes<I, O, stokesV>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			} else if constexpr (trueState == 150) {
				udp_fullStokes<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			} else if constexpr (trueState == 160) {
				udp_usefulStokes<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);




			} else if constexpr (trueState >= 101 && trueState <= 104) {
				udp_stokesDecimation<I, O, stokesI, decimation>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			} else if constexpr (trueState >= 111 && trueState <= 114) {
				udp_stokesDecimation<I, O, stokesQ, decimation>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			} else if constexpr (trueState >= 121 && trueState <= 124) {
				udp_stokesDecimation<I, O, stokesU, decimation>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			} else if constexpr (trueState >= 131 && trueState <= 134) {
				udp_stokesDecimation<I, O, stokesV, decimation>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			} else if constexpr (trueState >= 151 && trueState <= 154) {
				udp_fullStokesDecimation<I, O, decimation>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			} else if constexpr (trueState >= 161 && trueState <= 164) {
				udp_usefulStokesDecimation<I, O, decimation>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, upperBeamlet, cumulativeBeamlets, baseBeamlet);
			




			} else {
				fprintf(stderr, "Unknown processing mode %d, exiting.\n", state);
				exit(1);
			}


			// End ICC task block, update cached variables as needed
			#ifdef __INTEL_COMPILER
			}
			#else
			if constexpr (state >= 4000) {
				inputPortData = meta->inputData[port];
				lastInputPacketOffset = LIPOCache;
			}
			#endif
		}
		// Update the overall dropped packet count for this port

		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));

		#ifdef __INTEL_COMPILER
		#pragma omp taskwait
		#endif

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
