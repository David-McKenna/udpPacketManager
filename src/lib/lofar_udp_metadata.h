#ifndef LOFAR_UDP_METADATA_H
#define LOFAR_UDP_METADATA_H


#include "lofar_udp_structs.h"
#include "lofar_udp_time.h"
#include "lofar_udp_io.h"

#include <errno.h>
#include <getopt.h>
#include <string.h>

// gethostname(2)
#include <unistd.h>

// PSRDADA header interface
#include <ascii_header.h>

// YAML Library for parsing iLISA metadata
//#include "yaml.h"

// Shorthand for the number of arguments we support + 1 for endline
#define ASCII_HDR_MEMBS 35


// Allow C++ imports too
#ifdef __cplusplus
extern "C" {
#endif
// Useful elsewhere
int lofar_udp_metadata_get_station_name(int stationID, char *stationCode);

// Main public functions
metadata_t lofar_udp_metadata_parse_type_output(const char optargc[]);
int lofar_udp_metadata_setup(lofar_udp_metadata *metadata, const lofar_udp_reader *reader, const metadata_config *config);
int lofar_udp_metadata_update(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, int newObs);
int lofar_udp_metadata_write_buffer(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, char *headerBuffer, size_t headerBufferSize, int newObs);
int lofar_udp_metadata_write_buffer_force(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, char *headerBuffer, size_t headerBufferSize, int newObs, int force);
int
lofar_udp_metadata_write_file(const lofar_udp_reader *reader, lofar_udp_io_write_config *outConfig, int outp, lofar_udp_metadata *metadata, char *headerBuffer,
                              size_t headerBufferSize, int newObs);
int lofar_udp_metadata_write_file_force(const lofar_udp_reader *reader, lofar_udp_io_write_config *outConfig, int outp, lofar_udp_metadata *metadata,
                                        char *headerBuffer, size_t headerBufferSize, int newObs, int force);
int lofar_udp_metadata_cleanup(lofar_udp_metadata *metadata);

// Internal representations
//int lofar_udp_metadata_setup_DADA(lofar_udp_metadata *metadata); // Main setup call populates the DADA struct
int lofar_udp_metadata_setup_GUPPI(lofar_udp_metadata *metadata);
int lofar_udp_metadata_setup_SIGPROC(lofar_udp_metadata *metadata);

int lofar_udp_metadata_update_DADA(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, int newObs);
int lofar_udp_metadata_update_GUPPI(lofar_udp_metadata *metadata, int newObs);
int lofar_udp_metadata_update_SIGPROC(lofar_udp_metadata *metadata, int newObs);
int lofar_udp_metadata_update_HDF5(lofar_udp_metadata *metadata, int newObs);

int lofar_udp_metadata_write_DADA(const lofar_udp_metadata *hdr, char *headerBuffer, size_t headerLength);
int lofar_udp_metadata_write_GUPPI(const guppi_hdr *hdr, char *headerBuffer, size_t headerLength);
int lofar_udp_metadata_write_SIGPROC(const sigproc_hdr *hdr, char *headerBuffer, size_t headerLength);
int lofar_udp_metadata_write_HDF5(const sigproc_hdr *hdr, char *headerBuffer, size_t headerLength);


// Internal functions
int lofar_udp_metadata_parse_input_file(lofar_udp_metadata *metadata, const char inputFile[]);
int lofar_udp_metadata_parse_normal_file(lofar_udp_metadata *metadata, FILE *input, int *beamctlData);
int lofar_udp_metadata_parse_yaml_file(lofar_udp_metadata *metadata, FILE *input, int *beamctlData);
int lofar_udp_metadata_parse_reader(lofar_udp_metadata *metadata, const lofar_udp_reader *reader);
int lofar_udp_metadata_parse_subbands(lofar_udp_metadata *metadata, const char *inputLine, int *results);
int lofar_udp_metadata_parse_pointing(lofar_udp_metadata *metadata, const char inputStr[], int digi);
int lofar_udp_metadata_parse_rcumode(lofar_udp_metadata *metadata, const char *inputStr, int *beamctlData);
int lofar_udp_metadata_parse_csv(const char *inputStr, int *values, int *data);
int lofar_udp_metadata_count_csv(const char *inputStr);
int lofar_udp_metadata_get_tsv(const char *inputStr, const char *keyword, char *result);
int lofar_udp_metadata_parse_beamctl(lofar_udp_metadata *metadata, const char *inputLine, int *rcuMode);
int lofar_udp_metadata_get_clockmode(int input);
int lofar_udp_metadata_get_rcumode(int input);
int lofar_udp_metadata_get_beamlets(int bitmode);
int lofar_udp_metadata_processing_mode_metadata(lofar_udp_metadata *metadata);
metadata_t lofar_udp_metadata_string_to_meta(const char input[]);
int lofar_udp_metadata_update_frequencies(lofar_udp_metadata *metadata, int *subbandData);
int lofar_udp_metdata_handle_external_factors(lofar_udp_metadata *metadata, const metadata_config *config);
int lofar_udp_metdata_set_default(lofar_udp_metadata *metadata);


// Old GUPPI configuration interface
__attribute__((unused)) int lofar_udp_metdata_GUPPI_configure_from_file(const char *inputFile, guppi_hdr *header);

#ifdef __cplusplus
}
#endif

#endif // End of LOFAR_UDP_METADATA_H
