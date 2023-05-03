#include "lofar_udp_reader.h"


void handleData(int8_t **arr, int8_t numArrs, int32_t numBeamlets, int64_t numTimestamps) {
	UNUSED(arr); UNUSED(numArrs); UNUSED(numBeamlets); UNUSED(numTimestamps);
	// Do some kind of processing
	return;
}

// Heavily based upon the sample in README_INTEGRATION.md, and README_io.md
int main() {
	// Setup the configuration struct
	lofar_udp_config *readerConfig = lofar_udp_config_alloc();

	readerConfig->processingMode = STOKES_I; // Choose an output data product

	const int8_t numPorts = 4;
	readerConfig->numPorts = numPorts; // Process 4 files of data
	// char* array of input file pattern
	char inputFileFormat[DEF_STR_LEN] = "udp_1613[[port]].ucc1.2022-06-29T01:30:00.000.zst";
	// Parse the input format and set readerConfig->readerType based on the file string
	if (_lofar_udp_io_read_internal_lib_parse_optarg(readerConfig, inputFileFormat) < 0) {
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

	// Generate the reader object -- this is out main interface to the library
	lofar_udp_reader *reader =  lofar_udp_reader_setup(readerConfig);

	// Set-up an output writer
	lofar_udp_io_write_config *writer = lofar_udp_io_write_alloc();

	// TODO: Configure writer


	// Operate
	while (lofar_udp_reader_step(reader) <= 0) {
		int64_t nsamps_processed = reader->meta->packetsPerIteration * UDPNTIMESLICE;
		handleData(reader->meta->outputData, numPorts, reader->meta->totalProcBeamlets, nsamps_processed);

		for (int8_t outp = 0; outp < writer->numOutputs; outp++) {
			int64_t outputLength = reader->meta->packetsPerIteration * reader->meta->packetOutputLength[outp];
			if (outputLength != lofar_udp_io_write(writer, outp, reader->meta->outputData[outp], outputLength)) {
				fprintf(stderr, "ERROR: Failed to write out data.\n");
				break;
			}
		}
	}


	// Cleanup
	lofar_udp_reader_cleanup(reader);
	lofar_udp_config_cleanup(readerConfig);
	lofar_udp_io_write_cleanup(writer, 1);
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