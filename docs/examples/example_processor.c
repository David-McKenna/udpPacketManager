#include "lofar_udp_reader.h"


static void handleData(int8_t **arr, int8_t numArrs, int32_t numBeamlets, int64_t numTimestamps) {
	UNUSED(arr); UNUSED(numArrs); UNUSED(numBeamlets); UNUSED(numTimestamps);
	// Do some kind of processing
	return;
}

// Heavily based upon the sample in README_INTEGRATION.md, and README_io.md
int main(int argc, char *argv[]) {
	// Allocate the configuration structs
	lofar_udp_config *readerConfig = lofar_udp_config_alloc();
	lofar_udp_io_write_config *writer = lofar_udp_io_write_alloc();
	int8_t *headerBuffer = (int8_t*) calloc(DEF_HDR_LEN, sizeof(int8_t));

	if (readerConfig == NULL || writer == NULL || headerBuffer == NULL) {
		return 1;
	}

	readerConfig->processingMode = STOKES_I; // Choose an output data product

	const int8_t numPorts = 4;
	readerConfig->numPorts = numPorts; // Process 4 files of data
	// char* array of input file pattern
	char inputFileFormat[DEF_STR_LEN] = "udp_1613[[port]].ucc1.2022-06-29T01:30:00.000.zst";
	// Parse the input format and set readerConfig->readerType based on the file string
	if (lofar_udp_io_read_parse_optarg(readerConfig, inputFileFormat) < 0) {
		lofar_udp_config_cleanup(readerConfig);
		return 1;
	}

	readerConfig->packetsPerIteration = 65536; // Number of packets to read for each operation
	readerConfig->replayDroppedPackets = 1; // Copy last packet instead of 0-padding

	readerConfig->beamletLimits[0] = 0;
	readerConfig->beamletLimits[1] = 0; // Process all beamlets

	readerConfig->calibrateData = APPLY_CALIBRATION; // Calibrate data with dreamBeam, see alternative options with calibrate_t in lofar_udp_general.h
	char myMetaLocation[] = "./metatdata.h";
	strncpy(readerConfig->metadata_config.metadataLocation, myMetaLocation,
	        DEF_STR_LEN);

	readerConfig->packetsReadMax = -1; // Process the file entirely

	// Set-up an output writer


	// Output format for a DADA ringbuffer, starting at key 16130 and increasing by 2 for each output
	const char outputFormat[] = "DADA:16130,2";
	if (lofar_udp_io_write_parse_optarg(writer, outputFormat) < 0) {
		lofar_udp_config_cleanup(readerConfig);
		lofar_udp_io_write_cleanup(writer, 1);
		return 1;
	}
	// Produce matching metadata to our writer if we can determine a pattern
	// Given the "DADA:" prefix, it will produce a DADA header
	readerConfig->metadata_config.metadataType = lofar_udp_metadata_parse_type_output(outputFormat);
	writer->progressWithExisting = 1; // Join / recreate a ringbuffer if it already exists on the given key

	// Generate the reader object -- this is out main interface to the library
	lofar_udp_reader *reader =  lofar_udp_reader_setup(readerConfig);
	if (reader == NULL) {
		lofar_udp_config_cleanup(readerConfig);
		lofar_udp_io_write_cleanup(writer,1);
		return 1;
	}

	// Generate the writer object, use a helper function to initialise the config
	if (_lofar_udp_io_write_internal_lib_setup_helper(writer, reader, 0) < 0) {
		lofar_udp_config_cleanup(readerConfig);
		lofar_udp_io_write_cleanup(writer, 1);
		lofar_udp_reader_cleanup(reader);
		return 1;
	}

	// Operate
	int32_t localLoops = 0;
	while (lofar_udp_reader_step(reader) <= 0) {
		int64_t nsamps_processed = reader->meta->packetsPerIteration * UDPNTIMESLICE;
		handleData(reader->meta->outputData, numPorts, reader->meta->totalProcBeamlets, nsamps_processed);

		for (int8_t outp = 0; outp < writer->numOutputs; outp++) {
			int64_t outputLength = reader->meta->packetsPerIteration * reader->meta->packetOutputLength[outp];

			if (lofar_udp_metadata_write_file(reader, writer, outp, reader->metadata, headerBuffer, DEF_HDR_LEN, localLoops == 0) < 0) {
				fprintf(stderr, "ERROR: Failed to write header to disk.\n");
				break;
			}

			if (outputLength != lofar_udp_io_write(writer, outp, reader->meta->outputData[outp], outputLength)) {
				fprintf(stderr, "ERROR: Failed to write out data.\n");
				break;
			}
		}
		localLoops++;
	}


	// Cleanup
	lofar_udp_reader_cleanup(reader);
	lofar_udp_config_cleanup(readerConfig);
	lofar_udp_io_write_cleanup(writer, 1);
	free(headerBuffer);
}


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