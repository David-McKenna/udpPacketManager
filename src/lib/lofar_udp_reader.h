#ifndef LOFAR_UDP_READER_H
#define LOFAR_UDP_READER_H

#include "lofar_udp_io.h"
#include "lofar_udp_time.h"
#include "lofar_udp_backends.hpp"
#include "lofar_udp_metadata.h"

// Extra required includes
#include <omp.h>
#include <limits.h>
#include <time.h>


// Calibration extra requirements
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <spawn.h>


// For posix_spawnp
extern char **environ;


// Function Prototypes

// Allow C++ imports too
#ifdef __cplusplus
extern "C" {
#endif

// Reader/meta struct initialisation
//lofar_udp_reader* lofar_udp_reader_alloc() // NOT A PUBLIC FUNCTION
lofar_udp_reader *lofar_udp_reader_setup(lofar_udp_config *config);
int lofar_udp_file_reader_reuse(lofar_udp_reader *reader, long startingPacket, long packetsReadMax);

// Initialisation helpers
int lofar_udp_parse_headers(lofar_udp_input_meta *meta, char header[MAX_NUM_PORTS][UDPHDRLEN], const int beamletLimits[2]);
int lofar_udp_setup_processing(lofar_udp_input_meta *meta);
int lofar_udp_get_first_packet_alignment(lofar_udp_reader *reader);

// Raw input data handlers
int lofar_udp_reader_step(lofar_udp_reader *reader);
int lofar_udp_reader_step_timed(lofar_udp_reader *reader, double timing[2]);
int lofar_udp_reader_read_step(lofar_udp_reader *reader);
int lofar_udp_shift_remainder_packets(lofar_udp_reader *reader, const long shiftPackets[], int handlePadding);
//int lofar_udp_realign_data(lofar_udp_reader *reader);

// Reader struct cleanup
void lofar_udp_reader_cleanup(lofar_udp_reader *reader);

#ifdef __cplusplus
}
#endif

#endif