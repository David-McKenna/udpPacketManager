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

// Main public functions
// Type parsers
metadata_t lofar_udp_metadata_parse_type_output(const char optargc[]);
metadata_t lofar_udp_metadata_string_to_meta(const char input[]);


int32_t lofar_udp_metadata_setup(lofar_udp_metadata *metadata, const lofar_udp_reader *reader, const metadata_config *config);
int32_t lofar_udp_metadata_update(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, int8_t newObs);
int64_t lofar_udp_metadata_write_file(const lofar_udp_reader *reader, lofar_udp_io_write_config *const outConfig, int8_t outp, lofar_udp_metadata *const metadata, int8_t *headerBuffer, // NOLINT(readability-avoid-const-params-in-decls)
                                      int64_t headerBufferSize, int8_t newObs);

// Internal representations

// Useful elsewhere
int32_t _lofar_udp_metadata_get_station_name(int stationID, char *stationCode);

//int lofar_udp_metadata_setup_DADA(lofar_udp_metadata *metadata); // Main setup call populates the DADA struct
int32_t _lofar_udp_metadata_setup_GUPPI(lofar_udp_metadata *metadata);
int32_t _lofar_udp_metadata_setup_SIGPROC(lofar_udp_metadata *metadata);
int32_t _lofar_udp_metadata_setup_DADA(__attribute__((unused)) lofar_udp_metadata *metadata);

int32_t _lofar_udp_metadata_update_BASE(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, int8_t newObs);
int32_t _lofar_udp_metadata_update_DADA(__attribute__((unused)) lofar_udp_metadata *const metadata, __attribute__((unused)) int8_t newObs);
int32_t _lofar_udp_metadata_update_GUPPI(lofar_udp_metadata *const metadata, int8_t newObs);
int32_t _lofar_udp_metadata_update_SIGPROC(lofar_udp_metadata *const metadata, int8_t newObs);
int32_t _lofar_udp_metadata_update_HDF5(lofar_udp_metadata *const metadata, int8_t newObs);

int64_t _lofar_udp_metadata_write_buffer(const lofar_udp_reader *reader, lofar_udp_metadata *const metadata, int8_t *headerBuffer, int64_t headerBufferSize, int8_t newObs);
int64_t
_lofar_udp_metadata_write_buffer_force(const lofar_udp_reader *reader, lofar_udp_metadata *const metadata, int8_t *headerBuffer, int64_t headerBufferSize, int8_t newObs, int8_t force);
int64_t _lofar_udp_metadata_write_file_force(const lofar_udp_reader *reader, lofar_udp_io_write_config *const outConfig, int8_t outp, lofar_udp_metadata *const metadata,
                                             int8_t *const headerBuffer, int64_t headerBufferSize, int8_t newObs, int8_t force);
int64_t _lofar_udp_metadata_write_DADA(const lofar_udp_metadata *hdr, int8_t *const headerBuffer, int64_t headerLength);
int64_t _lofar_udp_metadata_write_GUPPI(const guppi_hdr *hdr, int8_t *const headerBuffer, int64_t headerLength);
int64_t _lofar_udp_metadata_write_SIGPROC(const sigproc_hdr *hdr, int8_t *const headerBuffer, int64_t headerLength);
int32_t _lofar_udp_metadata_write_HDF5(const sigproc_hdr *hdr, char * const headerBuffer, size_t headerLength);


// Internal functions
int32_t _lofar_udp_metadata_parse_input_file(lofar_udp_metadata *metadata, const char inputFile[]);
int32_t _lofar_udp_metadata_parse_normal_file(lofar_udp_metadata *const metadata, FILE *const input, int32_t *const beamctlData);
int32_t _lofar_udp_metadata_parse_yaml_file(lofar_udp_metadata *const metadata, FILE *const input, int32_t *const beamctlData);
int32_t _lofar_udp_metadata_parse_reader(lofar_udp_metadata *metadata, const lofar_udp_reader *reader);
int32_t _lofar_udp_metadata_parse_subbands(lofar_udp_metadata *const metadata, const char *inputLine, int32_t *const results);
int32_t _lofar_udp_metadata_parse_pointing(lofar_udp_metadata *metadata, const char inputStr[], int8_t digi);
int32_t _lofar_udp_metadata_parse_rcumode(lofar_udp_metadata *const metadata, const char *inputStr, int32_t *const beamctlData);
int16_t _lofar_udp_metadata_parse_csv(const char *inputStr, int16_t *const values, int32_t *const data, int16_t offset);
int32_t _lofar_udp_metadata_count_csv(const char *inputStr);
int32_t _lofar_udp_metadata_get_tsv(const char *inputStr, const char *keyword, char *result);
int32_t _lofar_udp_metadata_parse_beamctl(lofar_udp_metadata *const metadata, const char *inputLine, int32_t *const beamData);
int16_t _lofar_udp_metadata_get_clockmode(int16_t input);
int8_t _lofar_udp_metadata_get_rcumode(int16_t input);
int16_t _lofar_udp_metadata_get_beamlets(int8_t bitmode);
int32_t _lofar_udp_metadata_processing_mode_metadata(lofar_udp_metadata *metadata);
int32_t _lofar_udp_metadata_update_frequencies(lofar_udp_metadata *const metadata, const int32_t *subbandData);
int32_t _lofar_udp_metadata_handle_external_factors(lofar_udp_metadata *metadata, const metadata_config *config);
int32_t _lofar_udp_metdata_setup_BASE(lofar_udp_metadata *metadata);

// Internal variable checks
int32_t _isEmpty(const char *string);
int32_t _intNotSet(int32_t input);
int32_t _longNotSet(int64_t input);
int32_t _floatNotSet(float input, int8_t exception);
int32_t _doubleNotSet(double input, int8_t exception);

// DADA buffer writers
int32_t _writeStr_DADA(char *header, const char *key, const char *value);
int32_t _writeInt_DADA(char *header, const char *key, int32_t value);
int32_t _writeLong_DADA(char *header, const char *key, int64_t value);
__attribute__((unused)) int32_t _writeFloat_DADA(char *header, const char *key, float value, int8_t exception); // Not in spec
int32_t _writeDouble_DADA(char *header, const char *key, double value, int8_t exception);

// GUPPI buffer writers
char *_writeStr_GUPPI(char *headerBuffer, int64_t headerLength, const char *key, const char *val);
char *_writeInt_GUPPI(char *headerBuffer, int64_t headerLength, const char *key, int32_t val);
char *_writeLong_GUPPI(char *headerBuffer, int64_t headerLength, const char *key, int64_t val);
__attribute__((unused)) char *_writeFloat_GUPPI(char *headerBuffer, int64_t headerLength, const char *key, float val, int8_t exception);
char *_writeDouble_GUPPI(char *headerBuffer, int64_t headerLength, const char *key, double val, int8_t exception);

// SigProc buffer writers
char *_writeKey_SIGPROC(char *buffer, int64_t *bufferLength, const char *name);
char *_writeStr_SIGPROC(char *buffer, int64_t bufferLength, const char *name, const char *value);
char *_writeInt_SIGPROC(char *buffer, int64_t bufferLength, const char *name, int32_t value);
__attribute__((unused)) char *_writeLong_SIGPROC(char *buffer, int64_t bufferLength, const char *name, int64_t value); // Not in spec
__attribute__((unused)) char *_writeFloat_SIGPROC(char *buffer, int64_t bufferLength, const char *name, float value, int8_t exception); // Not in spec
char *_writeDouble_SIGPROC(char *buffer, int64_t bufferLength, const char *name, double value, int8_t exception);

// SigProc helpers
int32_t _sigprocStationID(int32_t telescopeId);
__attribute__((unused)) int32_t _sigprocMachineID(const char *machineName); // Currently not implemented
double _sigprocStr2Pointing(const char *input);


// Old GUPPI configuration interface
//__attribute__((unused)) int lofar_udp_metdata_GUPPI_configure_from_file(const char *inputFile, guppi_hdr *header);

#ifdef __cplusplus
}
#endif

#endif // End of LOFAR_UDP_METADATA_H

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
