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
int lofar_udp_file_reader_reuse(lofar_udp_reader *reader, long startingPacket, long packetsReadMax);
// Iteration handlers
int lofar_udp_reader_step(lofar_udp_reader *reader);
int lofar_udp_reader_step_timed(lofar_udp_reader *reader, double timing[2]);
// Reader struct cleanup
void lofar_udp_reader_cleanup(lofar_udp_reader *reader);

// Internal functions
int _lofar_udp_parse_headers(lofar_udp_obs_meta *meta, const int8_t header[4][16], const int16_t beamletLimits[2]);
int _lofar_udp_setup_processing(lofar_udp_obs_meta *meta);
int _lofar_udp_get_first_packet_alignment(lofar_udp_reader *reader);
int _lofar_udp_shift_remainder_packets(lofar_udp_reader *reader, const long shiftPackets[], int handlePadding);
//int _lofar_udp_realign_data(lofar_udp_reader *reader);
int _lofar_udp_reader_config_check(lofar_udp_config *config);
int _lofar_udp_reader_internal_read_step(lofar_udp_reader *reader);
int _lofar_udp_reader_malformed_header_checks(const int8_t header[16]);
void _lofar_udp_configure_obs_meta(const lofar_udp_config *config, lofar_udp_obs_meta *meta);

#ifdef __cplusplus
}
#endif

#endif