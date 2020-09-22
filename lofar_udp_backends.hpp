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


float stokesI(float Xr, float Xi, float Yr, float Yi);
float stokesQ(float Xr, float Xi, float Yr, float Yi);
float stokesU(float Xr, float Xi, float Yr, float Yi);
float stokesV(float Xr, float Xi, float Yr, float Yi);

int lofar_udp_raw_udp_copy_cpp(lofar_udp_meta *meta);

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

#ifdef __cplusplus
}
#endif

#endif

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
void inline udp_copySplitPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int portBeamlets, int cumulativeBeamlets) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (beamlet + cumulativeBeamlets) * UDPNTIMESLICE;

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
void inline udp_channelMajor(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int portBeamlets, int cumulativeBeamlets) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (beamlet + cumulativeBeamlets) * UDPNPOL;

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
void inline udp_channelMajorSplitPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int portBeamlets, int cumulativeBeamlets) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + beamlet + cumulativeBeamlets;

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
void inline udp_reversedChannelMajor(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int portBeamlets, int cumulativeBeamlets) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - (beamlet + cumulativeBeamlets)) * UDPNPOL;

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
void inline udp_reversedChannelMajorSplitPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int portBeamlets, int cumulativeBeamlets) {
	long outputPacketOffset = iLoop * packetOutputLength;
	long tsInOffset, tsOutOffset;

	//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - beamlet - cumulativeBeamlets);
		
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
void inline udp_timeMajor(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, int timeStepSize, int totalBeamlets, int portBeamlets, int cumulativeBeamlets, long packetsPerIteration) {
	long outputTimeIdx = iLoop * UDPNTIMESLICE;
	long tsInOffset, tsOutOffset;

	//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = 4 * (((beamlet + cumulativeBeamlets) * packetsPerIteration * UDPNTIMESLICE ) + outputTimeIdx);
		
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
void inline udp_timeMajorSplitPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, int timeStepSize, int totalBeamlets, int portBeamlets, int cumulativeBeamlets, long packetsPerIteration) {
	long outputTimeIdx = iLoop * UDPNTIMESLICE;
	long tsInOffset, tsOutOffset;

	//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = ((beamlet + cumulativeBeamlets) * packetsPerIteration * UDPNTIMESLICE ) + outputTimeIdx;
		
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
void inline udp_timeMajorDualPols(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, int timeStepSize, int totalBeamlets, int portBeamlets, int cumulativeBeamlets, long packetsPerIteration) {
	long outputTimeIdx = iLoop * UDPNTIMESLICE;
	long tsInOffset, tsOutOffset;

	//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = 2 * ((beamlet + cumulativeBeamlets) * packetsPerIteration * UDPNTIMESLICE ) + outputTimeIdx;
		
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
void inline udp_stokes(long iLoop, char *inputPortData, O **outputData,  long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int portBeamlets, int cumulativeBeamlets) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O);
	long tsInOffset, tsOutOffset;

	//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - beamlet - cumulativeBeamlets);

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
void inline udp_stokesDecimation(long iLoop, char *inputPortData, O **outputData, long lastInputPacketOffset, long packetOutputLength, int timeStepSize, int totalBeamlets, int portBeamlets, int cumulativeBeamlets) {
	long outputPacketOffset = iLoop * packetOutputLength / sizeof(O) / factor;
	long tsInOffset, tsOutOffset;
	O tempVal;

	//#pragma omp parallel for schedule(dynamic, 31) // Expected sizes: 61, 122, 244
	#ifdef __INTEL_COMPILER
	#pragma omp simd
	#else
	#pragma GCC unroll 61
	#endif
	for (int beamlet = 0; beamlet < portBeamlets; beamlet++) {
		tsInOffset = lastInputPacketOffset + beamlet * UDPNTIMESLICE * UDPNPOL * timeStepSize;
		tsOutOffset = outputPacketOffset + (totalBeamlets - 1 - beamlet - cumulativeBeamlets);
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





template <typename I, typename O, const int state>
int lofar_udp_raw_loop(lofar_udp_meta *meta) {
	// Setup return variable
	int packetLoss = 0;

	// Setup decimation factor if needed
	constexpr int decimation = 1 << (state % 10);

	// Setup working variables
	
	VERBOSE(const int verbose = meta->VERBOSE);
	
	const int packetsPerIteration = meta->packetsPerIteration;
	const int replayDroppedPackets = meta->replayDroppedPackets;

	// For each port of data provided,
	omp_set_num_threads(OMP_THREADS);
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

		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-variable"
		#pragma GCC diagnostic push
		const int portBeamlets = meta->portBeamlets[port];
		const int cumulativeBeamlets = meta->portCumulativeBeamlets[port];
		const int totalBeamlets = meta->totalBeamlets;

		// Get the length of packets on the current port and reset the last packet variable encase
		// 	there is packet loss on the first packet
		const int portPacketLength = meta->portPacketLength[port];
		const int packetOutputLength = meta->packetOutputLength[0];
		const int timeStepSize = sizeof(I) / sizeof(char);
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
		VERBOSE(if (verbose) printf("Packet 0: %ld\n", currentPortPacket));
		
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

			switch(state) {

				case 0:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_copy<char, char>(iLoop, inputPortData, (char**) outputData, port, lastInputPacketOffset, packetOutputLength);
					break;
				case 1:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_copyNoHdr<char, char>(iLoop, inputPortData, (char**) outputData, port, lastInputPacketOffset, packetOutputLength);
					break;
				case 2:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_copySplitPols<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, portBeamlets, cumulativeBeamlets);
					break;



				case 10:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_channelMajor<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets);
					break;
				case 11:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_channelMajorSplitPols<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets);
					break;



				case 20:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_reversedChannelMajor<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets);
					break;
				case 21:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_reversedChannelMajorSplitPols<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets);
					break;



				case 30:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_timeMajor<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets, packetsPerIteration);
					break;
				case 31:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_timeMajorSplitPols<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets, packetsPerIteration);
					break;
				case 32:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_timeMajorDualPols<I, O>(iLoop, inputPortData, outputData, lastInputPacketOffset, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets, packetsPerIteration);
					break;



				case 100:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_stokes<I, O, stokesI>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets);
					break;
				case 110:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_stokes<I, O, stokesQ>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets);
					break;
				case 120:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_stokes<I, O, stokesU>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets);
					break;
				case 130:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_stokes<I, O, stokesV>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets);
					break;



				case 101 ... 104:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_stokesDecimation<I, O, stokesI, decimation>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets);
					break;

				case 111 ... 114:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_stokesDecimation<I, O, stokesQ, decimation>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets);
					break;


				case 121 ... 124:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_stokesDecimation<I, O, stokesU, decimation>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets);
					break;


				case 131 ... 134:
					#ifdef __INTEL_COMPILER
					#pragma omp task firstprivate(iLoop, lastInputPacketOffset)
					#endif
					udp_stokesDecimation<I, O, stokesV, decimation>(iLoop, inputPortData, outputData, lastInputPacketOffset, packetOutputLength, timeStepSize, totalBeamlets, portBeamlets, cumulativeBeamlets);
					break;




				default:
					fprintf(stderr, "Unknown processing mode %d, exiting.\n", state);
					exit(1);
			}

		}
		// Update the overall dropped packet count for this port

		meta->portLastDroppedPackets[port] = currentPacketsDropped;
		meta->portTotalDroppedPackets[port] += currentPacketsDropped;
		VERBOSE(if (verbose) printf("Current dropped packet count on port %d: %d\n", port, meta->portLastDroppedPackets[port]));

		#ifdef __INTEL_COMPILER
		#pragma omp taskwait
		#endif

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

	return packetLoss;
}

#endif
#endif
