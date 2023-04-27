#ifndef LOFAR_UDP_READER_H
#define LOFAR_UDP_READER_H

#include "lofar_udp_calibration.h"
#include "lofar_udp_metadata.h"
#include "lofar_udp_io.h"
#include "lofar_udp_backends.hpp"


// Extra required includes
#include <omp.h>
#include <limits.h>
#include <time.h>
#include <fcntl.h>



// Function Prototypes

// Allow C++ imports too
#ifdef __cplusplus
extern "C" {
#endif

// Reader/meta struct initialisation
lofar_udp_reader *lofar_udp_reader_setup(lofar_udp_config *config);
int32_t lofar_udp_file_reader_reuse(lofar_udp_reader *reader, int64_t startingPacket, int64_t packetsReadMax);
// Iteration handlers
int32_t lofar_udp_reader_step(lofar_udp_reader *reader);
int32_t lofar_udp_reader_step_timed(lofar_udp_reader *reader, double timing[2]);
// Reader struct cleanup
void lofar_udp_reader_cleanup(lofar_udp_reader *reader);

// Internal functions
void _lofar_udp_parse_header_extract_metadata(int8_t port, lofar_udp_obs_meta *meta, const int8_t header[16], const int16_t beamletLimits[2]);
void _lofar_udp_reader_config_patch(lofar_udp_config *config);
int32_t _lofar_udp_reader_malformed_header_checks(const int8_t header[16]);
int32_t _lofar_udp_parse_header_buffers(lofar_udp_obs_meta *meta, const int8_t header[4][16], const int16_t beamletLimits[2]);
int32_t _lofar_udp_setup_parse_headers(lofar_udp_config *config, lofar_udp_obs_meta *meta, int8_t inputHeaders[MAX_NUM_PORTS][UDPHDRLEN]);
int32_t _lofar_udp_skip_to_packet(lofar_udp_reader *reader);
int32_t _lofar_udp_setup_processing(lofar_udp_obs_meta *meta);
int32_t _lofar_udp_setup_processing_output_buffers(lofar_udp_obs_meta *meta);
int32_t _lofar_udp_get_first_packet_alignment(lofar_udp_reader *reader);
int32_t _lofar_udp_shift_remainder_packets(lofar_udp_reader *reader, const int64_t shiftPackets[], int8_t handlePadding);
int32_t _lofar_udp_reader_config_check(const lofar_udp_config *config);
int32_t _lofar_udp_reader_internal_read_step(lofar_udp_reader *reader);
lofar_udp_obs_meta* _lofar_udp_configure_obs_meta(const lofar_udp_config *config);
//int _lofar_udp_realign_data(lofar_udp_reader *reader);

#ifdef __cplusplus
}
#endif

#endif

/**
 * Copyright (C) 2023 David McKenna
 * This file is part of udpPacketManager <https://github.com/David-McKenna/udpPacketManager>.
 *
 * udpPacketManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * udpPacketManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with udpPacketManager.  If not, see <http://www.gnu.org/licenses/>.
 **/
