#include "lofar_udp_metadata.h"

/**
 * @brief 			Provide a guess at the intended output metadata based on a filename
 * @param optargc 	Input filename
 * @return 			metadata_t: Best guess at type
 */
metadata_t lofar_udp_metadata_parse_type_output(const char optargc[]) {

	if (!strlen(optargc)) {
		return NO_META;
	}

	if (strstr(optargc, "GUPPI") != NULL) {
		return GUPPI;
	} else if (strstr(optargc, "DADA") != NULL) {
		return DADA;
	} else if (strstr(optargc, ".fil") != NULL ||
				strstr(optargc, "SIGPROC") != NULL) {
		return SIGPROC;
	} else if (strstr(optargc, ".h5") != NULL ||
				strstr(optargc, ".hdf5") != NULL ||
				strstr(optargc, "HDF5") != NULL) {
		return HDF5_META;
	}

	return NO_META;

}

/**
 * @brief
 * @param metadata
 * @param reader
 * @param config
 * @return
 */
int lofar_udp_metadata_setup(lofar_udp_metadata *metadata, const lofar_udp_reader *reader, const metadata_config *config) {

	// Sanity check the metadata and reader structs
	if (metadata == NULL) {
		fprintf(stderr, "ERROR %s: Metadata pointer is null, exiting.\n", __func__);
		return -1;
	}

	// Ensure we are meant to generate metadata
	if (metadata->type == NO_META) {
		if (!strnlen(config->metadataLocation, DEF_STR_LEN)) {
			fprintf(stderr, "ERROR %s: No metadata type specified, exiting.\n", __func__);
			return -1;
		}
		metadata->type = DEFAULT_META;
	}

	// Ensure we have a valid type of metadata
	switch(metadata->type) {
		//case NO_META:
		case DEFAULT_META:
		case GUPPI:
		case SIGPROC:
		case DADA:
		case HDF5_META:
			break;

		default:
			fprintf(stderr, "ERROR %s: Unknown metadata type %d, exiting.\n", __func__, metadata->type);
			return -1;
	}

	// Setup defaults
	if (_lofar_udp_metdata_setup_BASE(metadata) < 0) {
		fprintf(stderr, "ERROR %s: Failed to set default on metadata struct, exiting.\n", __func__);
		return -1;
	}

	// Parse the input metadata file with beamctl and other information
	if (config != NULL && strnlen(config->metadataLocation, DEF_STR_LEN)) {
		if (_lofar_udp_metadata_parse_input_file(metadata, config->metadataLocation) < 0) {
			return -1;
		}
	} else if (config != NULL && config->metadataType > DEFAULT_META){
		fprintf(stderr, "WARNING %s: Insufficient information provided to determine input pointing and frequency information.\n", __func__);
	}

	// Parse the input reader to refine the information related to the processing strategy
	if (reader != NULL) {
		if (_lofar_udp_metadata_parse_reader(metadata, reader) < 0) {
			return -1;
		}
	} else {
		fprintf(stderr, "WARNING %s: Reader struct was null, insufficient information provided to determine output properties.\n", __func__);
	}

	// Handle external modifiers or copy raw values to channel_bw/tsamp
	if (_lofar_udp_metadata_handle_external_factors(metadata, config) < 0) {
		return -1;
	}

	// If the output format isn't PSRDADA or HDF5, setup the target struct
	const int guppiMode = TIME_MAJOR_FULL;
	const int stokesMin = STOKES_I;
	switch(metadata->type) {
		case GUPPI:
			if (metadata->upm_procmode != guppiMode) {
				fprintf(stderr, "WARNING %s: GUPPI Raw Headers are intended to be attached to mode %d data (currently in mode %d).\n", __func__, guppiMode, metadata->upm_procmode);
			}
			return _lofar_udp_metadata_setup_GUPPI(metadata);

		case SIGPROC:
			if (metadata->upm_procmode < stokesMin) {
				fprintf(stderr, "WARNING %s: Sigproc headers are intended to be attached to Stokes data (mode > %d, currently in %d).\n", __func__, stokesMin, metadata->upm_procmode);
			}
			return _lofar_udp_metadata_setup_SIGPROC(metadata);

		// The DADA header/HDF5 metadata are formed from the reference metadata struct
		case DEFAULT_META:
		case DADA:
		case HDF5_META:
			return 0;

		case NO_META:
		default:
			__builtin_unreachable();
			fprintf(stderr, "ERROR %s: Unknown metadata type %d, exiting.\n", __func__, metadata->type);
			return -1;
	}
}

int lofar_udp_metadata_update(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, int newObs) {
	if (reader == NULL) {
		fprintf(stderr, "ERROR %s: Input reader is null, exiting.\n", __func__);
		return -1;
	}

	if (metadata == NULL || metadata->type == NO_META) {
		return 0;
	}

	if (newObs) {
		metadata->output_file_number += 1;
	}

	if (_lofar_udp_metadata_update_BASE(reader, metadata, newObs) < 0) {
		return -1;
	}

	switch(metadata->type) {
		case GUPPI:
			if (_lofar_udp_metadata_update_GUPPI(metadata, newObs) < 0) {
				return -1;
			}
			return 0;

		// DADA/HDF5 is the base case, just return
		case DADA:
		case HDF5_META:
			return 0;

		case SIGPROC:
			if (_lofar_udp_metadata_update_SIGPROC(metadata, newObs) < 0) {
				return -1;
			}
			return 0;


		default:
			fprintf(stderr, "ERROR %s: Unknown metadata type %d, exiting.\n", __func__, metadata->type);
			return -1;
	}


}

int _lofar_udp_metadata_update_BASE(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, int newObs) {

	if (reader == NULL || metadata == NULL) {
		fprintf(stderr, "ERROR %s: Input struct is null (reader: %p, metadata: %p), exiting.\n", __func__, reader, metadata);
		return -1;
	}

	// DADA headers are only really meant to be used once at the start of the file, so they aren't meant to change too much
	// But since we use this struct as the base, we'll need to update the bulk of the contents on every iteration.

	if (newObs) {
		lofar_udp_time_get_current_isot(reader, metadata->obs_utc_start, sizeof(metadata->obs_utc_start) / sizeof(metadata->obs_utc_start[0]));
		metadata->obs_mjd_start = lofar_udp_time_get_packet_time_mjd(reader->meta->inputData[0]);
		metadata->upm_processed_packets = 0;
		metadata->upm_dropped_packets = 0;
	}

	lofar_udp_time_get_daq(reader, metadata->upm_daq, sizeof(metadata->upm_daq) / sizeof(metadata->upm_daq[0]));
	metadata->upm_pack_per_iter = reader->meta->packetsPerIteration;
	metadata->upm_blocksize = metadata->upm_pack_per_iter * reader->meta->packetOutputLength[0];
	metadata->upm_processed_packets += metadata->upm_pack_per_iter * metadata->upm_num_inputs;

	metadata->upm_last_dropped_packets = 0;
	for (int port = 0; port < metadata->upm_num_inputs; port++) {
		metadata->upm_last_dropped_packets += reader->meta->portLastDroppedPackets[port];
	}
	metadata->upm_dropped_packets += metadata->upm_last_dropped_packets;

	return 0;
}

int64_t _lofar_udp_metadata_write_buffer(const lofar_udp_reader *reader, lofar_udp_metadata *const metadata, int8_t *headerBuffer, int64_t headerBufferSize, int8_t newObs) {
	return _lofar_udp_metadata_write_buffer_force(reader, metadata, headerBuffer, headerBufferSize, newObs, 0);
}

// 0: no work performed, >1: success + length, -1: failure
int64_t
_lofar_udp_metadata_write_buffer_force(const lofar_udp_reader *reader, lofar_udp_metadata *const metadata, int8_t *headerBuffer, int64_t headerBufferSize, int8_t newObs, int8_t force) {
	if (metadata == NULL || metadata->type <= DEFAULT_META) {
		return 0;
	}

	switch(metadata->type) {
		// GUPPI header should be written for every block
		case GUPPI:
			if (lofar_udp_metadata_update(reader, metadata, newObs || force)) {
				return -1;
			}
			break;

		// Other headers do not need to be updated after every iteration
		case HDF5_META:
		case DADA:
		case SIGPROC:
			if (newObs || force) {
				if (lofar_udp_metadata_update(reader, metadata, newObs || force)) {
					return -1;
				}
			} else {
				return 0;
			}
			break;

		default:
			fprintf(stderr, "ERROR %s_prep: Unknown metadata type %d, exiting.\n", __func__, metadata->type);
			return -1;
	}

	switch(metadata->type) {
		// GUPPI header should be written for every block
		case GUPPI:
			return _lofar_udp_metadata_write_GUPPI(metadata->output.guppi, headerBuffer, headerBufferSize);

		// DADA and sigproc headers are only written at the start of a file
		// HDF5 files take the DADA header into the PROCESS_HISTORY groups
		case DADA:
		case HDF5_META:
			return _lofar_udp_metadata_write_DADA(metadata, headerBuffer, headerBufferSize);

		// Sigproc headers should only be written at the start of a file
		case SIGPROC:
			return _lofar_udp_metadata_write_SIGPROC(metadata->output.sigproc, headerBuffer, headerBufferSize);

		default:
			fprintf(stderr, "ERROR %s_post: Unknown metadata type %d, exiting.\n", __func__, metadata->type);
			return -1;
	}

}

int64_t
lofar_udp_metadata_write_file(const lofar_udp_reader *reader, lofar_udp_io_write_config *const outConfig, int8_t outp, lofar_udp_metadata *const metadata, int8_t *headerBuffer,
                              int64_t headerBufferSize, int8_t newObs) {
	return _lofar_udp_metadata_write_file_force(reader, outConfig, outp, metadata, headerBuffer, headerBufferSize, newObs, 0);
}

int64_t _lofar_udp_metadata_write_file_force(const lofar_udp_reader *reader, lofar_udp_io_write_config *const outConfig, int8_t outp, lofar_udp_metadata *const metadata,
                                             int8_t *const headerBuffer, int64_t headerBufferSize, int8_t newObs, int8_t force) {
	int64_t returnVal = _lofar_udp_metadata_write_buffer_force(reader, metadata, headerBuffer, headerBufferSize, newObs, force);

	if (returnVal > 0) {
		return lofar_udp_io_write_metadata(outConfig, outp, metadata, headerBuffer, headerBufferSize);
	}

	return returnVal;
}

void lofar_udp_metadata_cleanup(lofar_udp_metadata *meta) {
	if (meta != NULL) {
		if (meta->output.sigproc != NULL) {
			FREE_NOT_NULL(meta->output.sigproc->fchannel);
		}

		FREE_NOT_NULL(meta->output.sigproc);
		FREE_NOT_NULL(meta->output.guppi);
	}
	FREE_NOT_NULL(meta);
}


int _lofar_udp_metdata_setup_BASE(lofar_udp_metadata *metadata) {

	if (metadata->type > DEFAULT_META && metadata->type < HDF5_META) {
		metadata->headerBuffer = calloc(DEF_HDR_LEN, sizeof(int8_t));
		CHECK_ALLOC(metadata->headerBuffer, -1, ;);
	}

	// Reset all arrays
	ARR_INIT(metadata->subbands, MAX_NUM_PORTS * UDPMAXBEAM, -1);
	STR_INIT(metadata->rawfile, MAX_NUM_PORTS);
	STR_INIT(metadata->upm_outputfmt, MAX_NUM_PORTS);

	// Minimum samples that can be parsed by DSPSR per operation
	// Not 100% sure what this is controlling, may need to vary based on processing mode
	metadata->resolution = 1;

	// LOFAR antenna are linear, not circular
	if (strncpy(metadata->basis, "Linear", META_STR_LEN) != metadata->basis) {
		fprintf(stderr, "ERROR: Failed to set antenna basis in the metadata struct, exiting.\n");
		return -1;
	}

	if (gethostname(metadata->hostname, META_STR_LEN)) {
		fprintf(stderr, "ERROR: Failed to get system hostname (errno %d: %s), exiting.\n", errno, strerror(errno));
		return -1;
	}

	if (strncpy(metadata->receiver, "LOFAR-RSP", META_STR_LEN) != metadata->receiver) {
		fprintf(stderr, "ERROR: Failed to copy receiver to metadata struct, exiting.\n");
		return -1;
	}

	// First file will reset value to 0
	metadata->output_file_number = -1;

	// Few other variables should never change
	metadata->obs_offset = 0;
	metadata->obs_overlap = 0;
	if (strncpy(metadata->mode, "Unknown", META_STR_LEN) != metadata->mode) {
		fprintf(stderr, "ERROR %s: Failed to copy mode to metadata struct, exiting.\n", __func__);
		return -1;
	}

	// Initialise all the packet trackers to 0
	metadata->upm_processed_packets = 0;
	metadata->upm_dropped_packets = 0;
	metadata->upm_last_dropped_packets = 0;


	return 0;
}

int _lofar_udp_metadata_parse_input_file(lofar_udp_metadata *metadata, const char inputFile[]) {
	if (inputFile == NULL || !strnlen(inputFile, DEF_STR_LEN)) {
		fprintf(stderr, "ERROR %s: Input file pointer is null, exiting.\n", __func__);
		return -1;
	}

	// Defaults
	strncpy(metadata->obs_id, "UNKNOWN", META_STR_LEN);
	strncpy(metadata->observer, "UNKNOWN", META_STR_LEN);
	strncpy(metadata->source, "UNKNOWN", META_STR_LEN);

	FILE *input = fopen(inputFile, "r");
	if (input == NULL) {
		fprintf(stderr, "ERROR %s: Failed to open metadata file at '%s' exiting.\n", __func__, inputFile);
		return -1;
	}


	int32_t beamctlData[9] = {
		-1,         // Last RCU mode
		INT16_MAX,    // Minimum subband
		0,          // Sum of subbands
		-1,         // Maximum subband
		0,          // Count of subbands
		INT16_MAX,    // Minimum beamlet
		0,          // Sum of beamlets
		-1,         // Maximum beamlet
		0           // Count of beamlets
	};

	// Reset the number of parsed beamlets
	metadata->upm_rawbeamlets = 0;
	const char yamlSignature[] = ".yml";
	if (strstr(inputFile, yamlSignature) != NULL) {
		if (_lofar_udp_metadata_parse_yaml_file(metadata, input, beamctlData) < 0) {
			fclose(input);
			return -1;
		}
	} else {
		if (_lofar_udp_metadata_parse_normal_file(metadata, input, beamctlData) < 0) {
			fclose(input);
			return -1;
		}
	}
	// Close the metadata file now that we have parsed it
	fclose(input);

	// Ensure we parsed either an --rcumode=<x> or --band=<x>_<y> parameter
	// Should be handled by lofar_udp_metadata_parse_subbands, double check...
	if (beamctlData[0] < 1) {
		__builtin_unreachable();
		fprintf(stderr, "ERROR: Beamctl parsing did not detect the RCU mode (%d), exiting.\n", beamctlData[0]);
		return -1;
	}

	// Ensure the total number of beamlets and subband match (otherwise the input line(s) were invalid)
	// Should be handled by lofar_udp_metadata_parse_subbands, double check...
	if (beamctlData[4] != beamctlData[8]) {
		__builtin_unreachable();
		fprintf(stderr, "ERROR: Number of subbands does not match number of beamlets (%d vs %d), exiting.\n", beamctlData[4], beamctlData[8]);
		return -1;
	}

	// Warning raised by lofar_udp_metadata_parse_subbands, exiting here as we need some amount of subband information
	if (beamctlData[4] < 1) {
		fprintf(stderr, "ERROR: Failed to parse any subband information, exiting.\n");
		return -1;
	}

	// Update the metadata struct to describe the input commands
	metadata->upm_lowerbeam = (int16_t) beamctlData[5];
	metadata->nsubband = (int16_t) beamctlData[8];
	metadata->upm_upperbeam = (int16_t) beamctlData[7];
	metadata->upm_rcuclock = _lofar_udp_metadata_get_clockmode((int16_t) beamctlData[0]);

	// Remaining few setup bits
	if (strncpy(metadata->upm_version, UPM_VERSION, META_STR_LEN) != metadata->upm_version) {
		fprintf(stderr, "ERROR: Failed to copy UPM version into metadata struct, exiting.\n");
		return -1;
	}

	if (_lofar_udp_metadata_update_frequencies(metadata, &(beamctlData[1])) < 0) {
		return -1;
	}

	return 0;
}

int _lofar_udp_metadata_parse_normal_file(lofar_udp_metadata *const metadata, FILE *const input, int32_t *const beamctlData) {

	char *inputLine = NULL, *strPtr = NULL;
	size_t buffLen = 0;
	ssize_t lineLength;
	// For each line in the file
	while ((lineLength = getline(&inputLine, &buffLen, input)) != -1) {

		VERBOSE(printf("Parsing line (%ld): %s\n", lineLength, inputLine));
		if ((strPtr = strstr(inputLine, "beamctl ")) != NULL) {

			VERBOSE(printf("Beamctl detected, passing on.\n"));
			if (_lofar_udp_metadata_parse_beamctl(metadata, strPtr, beamctlData) < 0) {
				FREE_NOT_NULL(inputLine);
				return -1;
			}

			// Append the line to the metadata struct
			char *fixPtr;
			while((fixPtr = strstr(inputLine, "\n"))) {
				*(fixPtr) = '\t';
			}
			strncat(metadata->upm_beamctl, inputLine, 2 * DEF_STR_LEN - 1 - strlen(metadata->upm_beamctl));


		} else if ((strPtr = strstr(inputLine, "SOURCE")) != NULL) {

			VERBOSE(printf("Source detected, passing on.\n"));
			if (_lofar_udp_metadata_get_tsv(strPtr, "SOURCE", metadata->source) < 0) {
				FREE_NOT_NULL(inputLine);
				return -1;
			}

			// Don't set mode for the time being
			/* if ((strPtr = strstr(metadata->source, "PSR") != NULL || ) {
			 *  if (strncpy(metadata->mode, META_STR_LEN, "PSR") != metadata->mode) {
			 *      fprintf(stderr, "ERROR: Failed to set metadata mode, exiting.\n");
			 *      return -1;
			 *  }
			 * } */
		} else if ((strPtr = strstr(inputLine, "OBS-ID")) != NULL) {

			VERBOSE(printf("Observation ID detected, passing on.\n"));
			if (_lofar_udp_metadata_get_tsv(strPtr, "OBS-ID", metadata->obs_id) < 0) {
				FREE_NOT_NULL(inputLine);
				return -1;
			}
		} else if ((strPtr = strstr(inputLine, "OBSERVER")) != NULL) {

			VERBOSE(printf("Observer detected, passing on.\n"));
			if (_lofar_udp_metadata_get_tsv(strPtr, "OBSERVER", metadata->observer) < 0) {
				FREE_NOT_NULL(inputLine);
				return -1;
			}
		} /*
		else if ((strPtr = strstr()) != NULL) {

		} */


		VERBOSE(printf("Beamctl data state: %d, %d, %d, %d, %d, %d\n", beamctlData[0], beamctlData[1], beamctlData[2], beamctlData[3], beamctlData[4], beamctlData[5]);
			        VERBOSE(printf("Getting next line.\n")));

	}
	// Free the getline buffer if it still exists (during testing this was raised as a non-free'd pointer)
	FREE_NOT_NULL(inputLine);

	return 0;
}


__attribute__((unused)) int _lofar_udp_metadata_parse_yaml_file(lofar_udp_metadata *const metadata, FILE *const input, int32_t *const beamctlData) {
	fprintf(stderr, "WARNING: YAML files not currently supported, will attempt to fallback to plain text parser.\n");

	return _lofar_udp_metadata_parse_normal_file(metadata, input, beamctlData);
}

int _lofar_udp_metadata_parse_reader(lofar_udp_metadata *metadata, const lofar_udp_reader *reader) {
	// Sanity check the inputs
	if (metadata == NULL || reader == NULL) {
		fprintf(stderr, "ERROR: Input metadata/reader struct was null, exiting.\n");
		return -1;
	}

	// Check the metadata state -- we need it to have been populated with beamctl data before we can do anything useful
	if (metadata->upm_rcuclock == -1 || metadata->upm_upperbeam == -1 || metadata->upm_lowerbeam == -1) {
		if (metadata->type > DEFAULT_META) {
			fprintf(stderr, "WARNING: Unable to update frequency information from reader. "
			                "Either the rcuclock (%d),  lower (%d) or upper (%d) beam was undefined.\n",
			        metadata->upm_rcuclock, metadata->upm_lowerbeam, metadata->upm_upperbeam);
		}
		if (metadata->upm_rcuclock == -1) {
			metadata->upm_rcuclock = reader->meta->clockBit ? 200 : 160;
		}
		return 0;
	} else {

		int16_t beamletsPerPort;
		int16_t clockAlias = reader->meta->clockBit ? 200 : 160; // 1 -> 200Mhz, 0 -> 160MHz, verify, sample data doesn't change the bit...
		if (clockAlias != metadata->upm_rcuclock) {
			fprintf(stderr, "WARNING: Clock bit mismatch (metadata suggests the %d clock, raw data suggests the %d clock\n", metadata->upm_rcuclock,
			        reader->meta->clockBit);
		}

		if ((beamletsPerPort = _lofar_udp_metadata_get_beamlets(reader->meta->inputBitMode)) < 0) {
			return -1;
		}

		VERBOSE(printf("%d, %d, %d, %d, %d\n", reader->input->numInputs, (reader->input->offsetPortCount), (beamletsPerPort), reader->meta->baseBeamlets[0],
		               reader->meta->upperBeamlets[reader->meta->numPorts - 1]));
		metadata->lowerBeamlet = (reader->input->offsetPortCount * beamletsPerPort) + reader->meta->baseBeamlets[0];
		// Upper beamlet is exclusive in UPM, not inclusive like LOFAR inputs
		metadata->upperBeamlet =
			((reader->input->offsetPortCount + (reader->input->numInputs - 1)) * beamletsPerPort) + reader->meta->upperBeamlets[reader->meta->numPorts - 1];


		// Sanity check the beamlet limits
		if ((metadata->upperBeamlet - metadata->lowerBeamlet) != reader->meta->totalProcBeamlets) {
			fprintf(stderr,
			        "WARNING: Mismatch between beamlets limits (lower/upper %d->%d=%d vs totalProcBeamlets %d), are beamlets non-sequential? Metadata may be compromised..\n",
			        metadata->lowerBeamlet, metadata->upperBeamlet, metadata->upperBeamlet - metadata->lowerBeamlet, reader->meta->totalProcBeamlets);
		}

		int32_t subbandData[3] = { INT16_MAX, 0, -1 };
		// Parse the new sub-set of beamlets
		for (int beamlet = metadata->lowerBeamlet; beamlet < metadata->upperBeamlet; beamlet++) {
			// Edge case: undefined subband is used as a beamlet
			VERBOSE(printf("Beamlet %d: Subband %d\n", beamlet, metadata->subbands[beamlet]));
			if (metadata->subbands[beamlet] != -1) {
				if (metadata->subbands[beamlet] > subbandData[2]) {
					subbandData[2] = metadata->subbands[beamlet];
				}

				if (metadata->subbands[beamlet] < subbandData[0]) {
					subbandData[0] = metadata->subbands[beamlet];
				}

				subbandData[1] += metadata->subbands[beamlet];
			}
		}

		// Update number of beamlets
		metadata->nsubband = reader->meta->totalProcBeamlets;
		VERBOSE(printf("Reader call\n"));
		if (_lofar_udp_metadata_update_frequencies(metadata, subbandData) < 0) {
			return -1;
		}

		metadata->telescope_rsp_id = reader->meta->stationID;
		if (_lofar_udp_metadata_get_station_name(metadata->telescope_rsp_id, metadata->telescope) < 0) {
			return -1;
		}

		metadata->baseport = reader->input->basePort;
		metadata->upm_reader = reader->input->readerType;
		metadata->upm_replay = reader->meta->replayDroppedPackets;
		metadata->upm_pack_per_iter = reader->packetsPerIteration;
		metadata->upm_blocksize = metadata->upm_pack_per_iter * reader->meta->packetOutputLength[0];


		// These are needed for the next function
		metadata->upm_procmode = reader->meta->processingMode;
		metadata->upm_input_bitmode = reader->meta->inputBitMode;
		metadata->upm_calibrated = reader->meta->calibrateData;
		if (_lofar_udp_metadata_processing_mode_metadata(metadata) < 0) {
			return -1;
		}

		metadata->upm_num_inputs = reader->input->numInputs;
		metadata->upm_num_outputs = reader->meta->numOutputs;


		lofar_udp_time_get_current_isot(reader, metadata->obs_utc_start, sizeof(metadata->obs_utc_start) / sizeof(metadata->obs_utc_start[0]));
		metadata->obs_mjd_start = lofar_udp_time_get_packet_time_mjd(reader->meta->inputData[0]);
		lofar_udp_time_get_daq(reader, metadata->upm_daq, sizeof(metadata->upm_daq) / sizeof(metadata->upm_daq[0]));

		for (int port = 0; port < metadata->upm_num_inputs; port++) {
			if (strncpy(metadata->rawfile[port], reader->input->inputLocations[port], META_STR_LEN) != metadata->rawfile[port]) {
				fprintf(stderr, "ERROR: Failed to copy raw filename %d to metadata struct, exiting.\n", port);
				return -1;
			}
		}
	}

	return 0;
}

int _lofar_udp_metadata_parse_beamctl(lofar_udp_metadata *const metadata, const char *inputLine, int32_t *const beamData) {
	char *strCopy = calloc(strlen(inputLine) + 1, sizeof(char));
	if (strCopy == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate buffer for intermediate string copy, exiting.\n");
		return -1;
	}
	if (memcpy(strCopy, inputLine, strlen(inputLine)) != strCopy) {
		fprintf(stderr, "ERROR: Failed to copy line to intermediate buffer, exiting.\n");
		free(strCopy);
		return -1;
	}

	char *workingPtr;

	char *tokenPtr;
	char token[2] = " ";
	char *inputPtr = strtok_r(strCopy, token, &tokenPtr);

	if (inputPtr == NULL) {
		// Should be unreachable; we search for "beamctl " when calling this function
		fprintf(stderr, "ERROR: Failed to find space in beamctl command, exiting.\n");
		free(strCopy);
		return -1;
	}

	do {
		VERBOSE(printf("Starting beamctl loop for token %s\n", inputPtr));
		// Number of RCUs
		if ((workingPtr = strstr(inputPtr, "--rcus=")) != NULL) {
			metadata->nrcu = (int16_t) _lofar_udp_metadata_count_csv(workingPtr + strlen("--rcus="));

			VERBOSE(printf("Nrcu: %d\n", metadata->nrcu));
			if (metadata->nrcu == -1) {
				free(strCopy);
				return -1;
			}

		// RCU configuration
		} else if ((workingPtr = strstr(inputPtr, "--band=")) != NULL) {
			if (_lofar_udp_metadata_parse_rcumode(metadata, workingPtr, beamData) < 0) {
				free(strCopy);
				return -1;
			}
			VERBOSE(printf("Band: %d\n", beamData[0]));

		} else if ((workingPtr = strstr(inputPtr, "--rcumode=")) != NULL) {
			if (_lofar_udp_metadata_parse_rcumode(metadata, workingPtr, beamData) < 0) {
				free(strCopy);
				return -1;
			}
			VERBOSE(printf("Rcumode: %d\n", beamData[0]));

		// Pointing information
		} else if ((workingPtr = strstr(inputPtr, "--digdir=")) != NULL) {
			if (_lofar_udp_metadata_parse_pointing(metadata, workingPtr, 1) < 0) {
				free(strCopy);
				return -1;
			}
			VERBOSE(printf("Digdir: %s, %s, %s\n", metadata->ra, metadata->dec, metadata->coord_basis));

		} else if ((workingPtr = strstr(inputPtr, "--anadir=")) != NULL) {
			if (_lofar_udp_metadata_parse_pointing(metadata, workingPtr, 0) < 0) {
				free(strCopy);
				return -1;
			}
			VERBOSE(printf("Anadir: %s, %s, %s\n", metadata->ra_analog, metadata->dec_analog, metadata->coord_basis));

		}

		VERBOSE(printf("End beamctl loop for token %s\n", inputPtr));
	} while ((inputPtr = strtok_r(NULL, token, &tokenPtr)) != NULL);
	VERBOSE(printf("End strok beamctl loop\n"));

	if (memcpy(strCopy, inputLine, strlen(inputLine)) != strCopy) {
		fprintf(stderr, "ERROR: Failed to copy line to intermediate buffer, exiting.\n");
		FREE_NOT_NULL(strCopy);
		return -1;
	}
	workingPtr = strCopy;
	VERBOSE(printf("Sanity check states: \n%s\n%s\n\n", workingPtr, inputLine));
	if (_lofar_udp_metadata_parse_subbands(metadata, workingPtr, beamData) < 0) {
		FREE_NOT_NULL(strCopy);
		return -1;
	}

	FREE_NOT_NULL(strCopy);

	// Parse the combined beamctl data to determine the frequency information
	if (metadata->subband_bw == 0.0) {
		metadata->subband_bw = ((double) metadata->upm_rcuclock) / 1024;
	} else if (metadata->subband_bw != (((double) metadata->upm_rcuclock) / 1024.0)) {
		fprintf(stderr, "ERROR: Channel bandwidth mismatch (was %lf, now %lf), exiting.\n", metadata->subband_bw, ((double) metadata->upm_rcuclock) / 1024);
		return -1;
	}

	return 0;
}

int _lofar_udp_metadata_parse_rcumode(lofar_udp_metadata *const metadata, const char *inputStr, int32_t *const beamctlData) {
	int16_t workingInt = -1;
	if (sscanf(inputStr, "%*[^=]=%hd", &(workingInt)) < 0) {
		return -1;
	}

	int16_t lastClock = metadata->upm_rcuclock;
	metadata->upm_rcuclock = _lofar_udp_metadata_get_clockmode(workingInt);
	metadata->upm_rcumode = _lofar_udp_metadata_get_rcumode(workingInt);

	if (metadata->upm_rcuclock == 160 && metadata->upm_rcumode != 6) {
		fprintf(stderr, "ERROR: Conflicting RCU clock/mode information (160MHz clock, but mode %d, exiting.\n", metadata->upm_rcuclock);
		return -1;
	}

	beamctlData[0] = (int32_t) metadata->upm_rcumode;

	if (lastClock > 100 && lastClock != metadata->upm_rcuclock) {
		fprintf(stderr, "ERROR: 160/200MHz clock mixing detected (previously %d, now %d), this is not currently supported. Exiting.\n", lastClock, metadata->upm_rcuclock);
		return -1;
	}

	if (beamctlData[0] == -1) {
		fprintf(stderr, "ERROR: Failed to determine RCU mode, exiting.\n");
		return -1;
	}

	return 0;
}

int _lofar_udp_metadata_parse_pointing(lofar_udp_metadata *metadata, const char inputStr[], int digi) {
	double lon, lon_work, lat, lat_work, ra_s, dec_s;
	int16_t ra[2], dec[2];
	char basis[32];

	// 2pi / 24h -> 0.26179938779 rad/hr
	const double rad2HA = 0.2617993877991494;
	// 2pi / 360deg -> 0.01745329251 rad/deg
	const double rad2Deg = 0.017453292519943295;

	// Parse a comma separate long,lat,basis string
	char formatStr[] = "%*[^0-9]%lf%*[^0-9]%lf%*[^a-zA-Z]%[a-zA-Z0-9]32s%*[^0-9a-zA-Z]";
	char *formatStrPtr = &(formatStr[0]);
	// The above format will not parse correctly if the first element is numeric
	if (inputStr[0] >= '0' && inputStr[0] <= '9') {
		formatStrPtr += strlen("%*[^0-9]");
	}
	if (sscanf(inputStr, formatStrPtr, &lon, &lat, basis) < 2) {
		fprintf(stderr, "ERROR: Failed to parse pointing string %s, exiting.\n", inputStr);
		return -1;
	}

	// Avoiding libmath imports, so manual modulo-s ahead.
	// Take a copy of the values to work with
	lon_work = lon;
	lat_work = lat;

	// TODO: Do we need to sanity check these inputs?

	// Radians -> hh:mm:ss.sss
	ra[0] = (int16_t) (lon_work / rad2HA);
	lon_work -= ra[0] * rad2HA;
	ra[1] = (int16_t) (lon_work / (rad2HA / 60));
	lon_work -= ra[1] * (rad2HA / 60);
	ra_s = (lon_work / (rad2HA / 60 / 60));

	// Radians -> dd:mm:ss.sss
	dec[0] = (int16_t) (lat_work / rad2Deg);
	lat_work -= dec[0] * rad2Deg;
	dec[1] = (int16_t) (lat_work / (rad2Deg / 60));
	lat_work -= dec[1] * (rad2Deg / 60);
	dec_s = (lat_work / (rad2Deg / 60 / 60));

	// Save the radian values to the struct
	// Swap string destinations based on the digi flag
	char *dest[2];
	if (digi) {
		metadata->ra_rad = lon;
		metadata->dec_rad = lat;
		dest[0] = metadata->ra;
		dest[1] = metadata->dec;
	} else {
		metadata->ra_rad_analog = lon;
		metadata->dec_rad_analog = lat;
		dest[0] = metadata->ra_analog;
		dest[1] = metadata->dec_analog;
	}

	// Print 0-padded dd:mm:ss.sss.. values to strings
	if (snprintf(dest[0], META_STR_LEN, "%02d:%02d:%07.4lf", ra[0], ra[1], ra_s) < 0) {
		fprintf(stderr, "ERROR: Failed to print RA/Lon to metadata, exiting.\n");
		return -2;
	}

	if (snprintf(dest[1], META_STR_LEN, "%02d:%02d:%07.4lf", dec[0], dec[1], dec_s) < 0) {
		fprintf(stderr, "ERROR: Failed to print Dec/Lat to metadata, exiting.\n");
		return -3;
	}

	// Verify or set the basis
	if (strlen(metadata->coord_basis) > 0) {
		VERBOSE(printf("Basis already set, comparing...\n"));
		if (strcmp(basis, metadata->coord_basis) != 0) {
			fprintf(stderr, "ERROR: Coordinate basis mismatch while parsing directions (digdir=%d), parsed %s vs struct %s, exiting.\n", digi, basis, metadata->coord_basis);
			return -4;
		}
	} else {
		VERBOSE(printf("Basis not set, copying...\n"));
		if (strncpy(metadata->coord_basis, basis, META_STR_LEN) != metadata->coord_basis) {
			fprintf(stderr, "ERROR: Failed to copy coordinate basis to metadata struct, exiting.\n");
			return -5;
		}
	}

	return 0;
}

int _lofar_udp_metadata_parse_subbands(lofar_udp_metadata *const metadata, const char *inputLine, int32_t *const results) {

	if (metadata == NULL || inputLine == NULL || results == NULL) {
		fprintf(stderr, "ERROR %s: Input is unallocated (%p, %p, %p), exiting.\n", __func__, metadata, inputLine, results);
		return -1;
	}

	if (results[0] < 1 || results[0] > 7) {
		fprintf(stderr, "ERROR: RCU mode variable is uninitialised or invalid (%d), exiting.\n", results[0]);
		return -1;
	}

	// Subband offset
	// Simpler way of calculating the frequencies -- use an offset to emulate the true frequency across several RCU modes
	// Mode 1/2/3/4 -- 0 offset, subband (0-511), 0.19 MHz bandwidth, 0 - 100MHz
	// Mode 5 -- 512 offset, subband (512 - 1023), 0.19 MHz bandwidth, 100 - 200MHz
	// Mode 6/7 -- 1024 offset, subband (1024 - 1535), 0.19 / 0.16 MHz bandwidth, 160 - 240MHz and 200 - 300MHz
	int16_t subbandOffset;
	switch (results[0]) {

		case 1:
		case 2:
		case 3:
		case 4:
			subbandOffset = 0;
			break;

		case 5:
			subbandOffset = 512;
			break;

		case 6:
		case 7:
			subbandOffset = 1024;
			break;

		default:
			// (results[0] < 1 || results[0] > 7) checked
			__builtin_unreachable();
			fprintf(stderr, "ERROR: Invalid rcumode %d, exiting.\n", results[0]);
			return -1;
	}

	int16_t subbands[MAX_NUM_PORTS * UDPMAXBEAM] = { 0 };
	int16_t beamlets[MAX_NUM_PORTS * UDPMAXBEAM] = { 0 };
	ARR_INIT(subbands, MAX_NUM_PORTS * UDPMAXBEAM, -1);
	ARR_INIT(beamlets, MAX_NUM_PORTS * UDPMAXBEAM, -1);

	char *workingPtr = (char*) &(inputLine[0]), *tokenPtr;
	char token[2] = " ";
	VERBOSE(printf("Parsing input line %s for subbands and beamlets\n", workingPtr));
	char *inputPtr = strtok_r(workingPtr, token, &tokenPtr);

	// Should be impossible as we search for "beamctl ", but check anyway...
	if (inputPtr == NULL) {
		__builtin_unreachable();
		fprintf(stderr, "ERROR: Failed to find space in beamctl command, exiting.\n");
		return -1;
	}

	int16_t subbandCount = 0, beamletCount = 0;
	do {
		VERBOSE(printf("Entering subband/beamlet parse loop for token %s\n", inputPtr));
		if ((workingPtr = strstr(inputPtr, "--subbands=")) != NULL) {
			VERBOSE(printf("Subbands detected\n"));
			if ((subbandCount = _lofar_udp_metadata_parse_csv(workingPtr + strlen("--subbands="), subbands, &(results[1]), subbandOffset)) < 0) {
				return -1;
			}

		} else if ((workingPtr = strstr(inputPtr, "--beamlets=")) != NULL) {
			VERBOSE(printf("Beamlets detected\n"));
			if ((beamletCount = _lofar_udp_metadata_parse_csv(workingPtr + strlen("--beamlets="), beamlets, &(results[5]), 0)) < 0) {
				return -1;
			}

		}

	} while ((inputPtr = strtok_r(NULL, token, &tokenPtr)) != NULL);
	VERBOSE(printf("End subband/beamlet search loop\n"));

	if (subbandCount != beamletCount) {
		fprintf(stderr, "ERROR: Failed to parse matching numbers of both subbands and beamlets (%d/%d), exiting.\n", subbandCount, beamletCount);
		return -1;
	}

	if (subbandCount == 0 || subbandCount > MAX_NUM_PORTS * UDPMAXBEAM) {
		fprintf(stderr, "WARNING: Failed to parse sane amount of beams (expected between 1 and %d, got %d).\n", MAX_NUM_PORTS * UDPMAXBEAM, subbandCount);
	}

	metadata->upm_rawbeamlets += beamletCount;

	// As a result, for mode 5, subbands 512 - 1023, we have (512 - 1023) * (0.19) = 100 - 200MHz
	// Similarly no offset for mode 3, double the offset for mode 7, etc.
	// This is why I raise an error when mixing mode 6 (160MHz clock) and anything else (all on the 200MHz clock)
	for (int i = 0; i < metadata->upm_rawbeamlets; i++) {
		if (beamlets[i] != -1) {
			metadata->subbands[beamlets[i]] = (int16_t) (subbandOffset + subbands[i]);
		}
	}

	return 0;
}

int _lofar_udp_metadata_get_tsv(const char *inputStr, const char *keyword, char *result) {
	char key[META_STR_LEN + 1];
	if (sscanf(inputStr, "%64s\t%s", key, result) != 2) {
		fprintf(stderr, "ERROR: Failed to get value for input '%s' keyword (parsed key/val of %s / %s), exiting.\n", keyword, key, result);
		return -1;
	}

	if (strcmp(key, keyword) != 0) {
		fprintf(stderr, "ERROR: Keyword %s does not match parsed key %s, exiting.\n", keyword, key);
		return -2;
	}

	VERBOSE(printf("Parsed for %s: %s / %s\n", keyword, key, result));

	return 0;
}

int _lofar_udp_metadata_count_csv(const char *inputStr) {
	return _lofar_udp_metadata_parse_csv(inputStr, NULL, NULL, 0);
}

int16_t _lofar_udp_metadata_parse_csv(const char *inputStr, int16_t *const values, int32_t *const data, int16_t offset) {
	if (inputStr == NULL) {
		fprintf(stderr, "ERROR %s: Input is unallocated, exiting.\n", __func__);
		return -1;
	}

	// TODO: While we use strtok to split up the input, an arbitrary input will fail if there
	//  is extra data with commas after the current word
	size_t bufferLen = strlen(inputStr) + 1;
	char *strCpy = calloc(bufferLen, sizeof(char));
	if (strCpy == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate %ld bytes for input string %s, exiting.\n", bufferLen, inputStr);
	}

	if (memcpy(strCpy, inputStr, bufferLen - 1) != strCpy) {
		fprintf(stderr, "ERROR: Failed to make a copy of the csv string %s, exiting.\n", inputStr);
		FREE_NOT_NULL(strCpy);
		return -1;
	}

	VERBOSE(printf("Parse: %s\n", inputStr));

	int32_t sum = 0;
	int16_t counter = 0, lower = -1, upper = -1;
	int16_t minimum = INT16_MAX, maximum = -1;

	char *tokenPtr;
	char token[2] = ",";
	char *workingPtr = strtok_r(strCpy, token, &tokenPtr);

	// If the input isn't a list, just point to the start of the list and parse the 1 element.
	if (workingPtr == NULL) {
		fprintf(stderr, "WARNING: Commas not detected, attempting to parse first entry from the pointer header...\n");
		workingPtr = (char*) &(inputStr[0]);
	}

	char *fixPtr;
	do { //  while ((workingPtr = strtok_r(NULL, token, &tokenPtr)) != NULL);
		VERBOSE(printf("Parse entry %s\n", workingPtr));

		// Remove quotes
		while((fixPtr = strstr(workingPtr, "\'"))) {
			*(fixPtr) = ' ';
		}

		while((fixPtr = strstr(workingPtr, "\""))) {
			*(fixPtr) = ' ';
		}

		if (strstr(workingPtr, ":") != NULL) {
			VERBOSE(printf(": detected\n"));
			// Extremely unlikely we can't parse a value, but double check to be sure
			if (sscanf(workingPtr, "%hd:%hd", &lower, &upper) == EOF) {
				fprintf(stderr, "Things went wrong\n");
				FREE_NOT_NULL(strCpy);
				return -1;
			}

			VERBOSE(printf("Parse %s -> %d / %d\n", workingPtr, upper, lower));

			// Sum of a series
			sum += ((upper * (upper + 1) / 2) - ((lower - 1) * (lower) / 2));

			// LOFAR is inclusive of values on a range, so +1
			for (int16_t i = lower; i < upper + 1; i++) {
				if (values != NULL) {
					values[counter] = i;
				}
				counter++;
			}

		} else {
			sscanf(workingPtr, "%hd", &lower);

			// Extremely unlikely we can't parse a value, but double check to be sure
			if (lower == EOF) {
				fprintf(stderr, "ERROR: Failed to parse csv list (input: %s), exiting.\n", inputStr);
				FREE_NOT_NULL(strCpy);
				return -1;
			}

			VERBOSE(printf("Parsed %d\n", lower));
			upper = lower;
			sum += lower;

			// Single element, increment by 1
			if (values != NULL) {
				values[counter] = lower;
			}
			counter += 1;
		}

		if (lower < minimum) {
			minimum = lower;
		}

		if (upper > maximum) {
			maximum = upper;
		}

	} while ((workingPtr = strtok_r(NULL, token, &tokenPtr)) != NULL);

	minimum += offset;
	maximum += offset;
	sum += counter * offset;

	if (data != NULL) {
		data[0] = (minimum < data[0]) ? minimum : data[0];
		data[1] += sum;
		data[2] = (maximum > data[0]) ? maximum : data[0];
		data[3] += counter;
	}

	FREE_NOT_NULL(strCpy);
	return counter;
}

int16_t _lofar_udp_metadata_get_clockmode(int16_t input) {
	switch (input) {
		// RCU modes
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 7:
		// Base values of bands
		case 10:
		case 30:
		case 110:
		case 210:
			return 200;

		case 6:
		case 170:
			return 160;


		default:
			fprintf(stderr, "ERROR: Failed to parse RCU clockmode (base int of %d), exiting.\n", input);
			return -1;
	}
}

int8_t _lofar_udp_metadata_get_rcumode(int16_t input) {
	if (input < 1) {
		fprintf(stderr, "ERROR: Invalid input RCU mode %d, exiting.\n", input);
		return -1;
	}

	if (input < 8) {
		return (int8_t) input;
	}

	switch (input) {
		// Base values of bands
		case 10:
			return 3;
		case 30:
			return 4;

		case 110:
			return 5;
		case 210:
			return 7;

		case 170:
			return 6;


		default:
			fprintf(stderr, "ERROR: Failed to determine RCU mode (base int of %d), exiting.\n", input);
			return -1;
	}
}

metadata_t lofar_udp_metadata_string_to_meta(const char input[]) {
	if (strstr(input, "GUPPI") != NULL)  {
		return GUPPI;
	} else if (strstr(input, "DADA") != NULL) {
		return DADA;
	} else if (strstr(input, "SIGPROC") != NULL) {
		return SIGPROC;
	} else if (strstr(input, "HDF5") != NULL) {
		return HDF5_META;
	}

	fprintf(stderr, "ERROR: Unknown metadata format '%s', returning NO_META.\n", input);
	return NO_META;
}

int16_t _lofar_udp_metadata_get_beamlets(int8_t bitmode) {
	switch(bitmode) {
		case 4:
			return 244;

		case 8:
			return 122;

		case 16:
			return 61;

		default:
			fprintf(stderr, "ERROR: Unknown bitmode %d, exiting.\n", bitmode);
			return -1;
	}
}

int _lofar_udp_metadata_update_frequencies(lofar_udp_metadata *const metadata, int32_t *const subbandData) {
	// Update the metadata frequency / channel parameters

	if (metadata == NULL || subbandData == NULL) {
		fprintf(stderr, "ERROR %s: Passed null metadata struct (%p) / subband data (%p), exiting.\n", __func__, metadata, subbandData);
		return -1;
	}

	if (metadata->nsubband < 1) {
		fprintf(stderr, "ERROR %s: passed invalid number of channels (%d), exiting.\n", __func__, metadata->nsubband);
		return -1;
	}

	double meanSubband = (double) subbandData[1] / (double) metadata->nsubband;

	VERBOSE(printf("SubbandData: %d, %d, %d\n", subbandData[0], subbandData[1], subbandData[2]));

	metadata->ftop = metadata->subband_bw * subbandData[0];
	metadata->freq_raw = metadata->subband_bw * meanSubband;
	metadata->fbottom = metadata->subband_bw * subbandData[2];
	// Define the observation bandwidth as the bandwidth between the centre of the top and bottom channels,
	//  plus a subband to account for the expanded bandwidth from the centre to the edges of the band
	metadata->bw = (metadata->fbottom - metadata->ftop);
	double signCorrection = (metadata->bw > 0) ? 1.0 : -1.0;

	metadata->bw += signCorrection * metadata->subband_bw;
	metadata->ftop -= signCorrection * 0.5 * metadata->subband_bw;
	metadata->fbottom += signCorrection * 0.5 * metadata->subband_bw;

	return 0;
}

int _lofar_udp_metadata_processing_mode_metadata(lofar_udp_metadata *metadata) {

	if (metadata == NULL) {
		return -1;
	}

	switch (metadata->upm_procmode) {
		// Frequency order same as beamlet order
		case PACKET_FULL_COPY ... PACKET_SPLIT_POL:
		case BEAMLET_MAJOR_FULL ... BEAMLET_MAJOR_SPLIT_POL:
		case TIME_MAJOR_FULL ... TIME_MAJOR_ANT_POL:
		case TIME_MAJOR_ANT_POL_FLOAT:
		case STOKES_I ... STOKES_I_DS16:
		case STOKES_Q ... STOKES_Q_DS16:
		case STOKES_U ... STOKES_U_DS16:
		case STOKES_V ... STOKES_V_DS16:
		case STOKES_IQUV ... STOKES_IQUV_DS16:
		case STOKES_IV ... STOKES_IV_DS16:
			metadata->upm_bandflip = 0;
			break;

		// Frequency order flipped
		case BEAMLET_MAJOR_FULL_REV ... BEAMLET_MAJOR_SPLIT_POL_REV:
		case STOKES_I_REV ... STOKES_I_DS16_REV:
		case STOKES_Q_REV ... STOKES_Q_DS16_REV:
		case STOKES_U_REV ... STOKES_U_DS16_REV:
		case STOKES_V_REV ... STOKES_V_DS16_REV:
		case STOKES_IQUV_REV ... STOKES_IQUV_DS16_REV:
		case STOKES_IV_REV ... STOKES_IV_DS16_REV:
			metadata->upm_bandflip = 1;
			break;

		case TEST_INVALID_MODE:
		default:
			fprintf(stderr,"ERROR %s: Unknown processing mode %d, exiting.\n", __func__, metadata->upm_procmode);
			return -1;
	}

	if (metadata->upm_bandflip) {
		double tmp = metadata->ftop;
		metadata->ftop = metadata->fbottom;
		metadata->fbottom = tmp;
		metadata->subband_bw *= -1;
		metadata->bw *= -1;
	}

	double samplingTime = metadata->upm_rcuclock == 200 ? clock200MHzSampleTime : clock160MHzSampleTime;
	if (metadata->upm_procmode > 99) {
		samplingTime *= (double) (1 << (metadata->upm_procmode % 10));
	}
	metadata->tsamp_raw = samplingTime;


	for (int i = 0; i < MAX_OUTPUT_DIMS; i++) {
		if (strncpy(metadata->upm_outputfmt[i], "", META_STR_LEN) != metadata->upm_outputfmt[i]) {
			fprintf(stderr, "ERROR: Failed to reset upm_outputfmt_comment field %d, exiting.\n", i);
			return -1;
		}
	}

	// TODO (General, but esp. nice here): Find a way to do concatenations in future? E.g rev is 1 bit, split_pol is 1 bit, etc.
	// 		Would need a string -> processMode_t parser, as the values wouldn't make sense, but it would be nice.
	switch (metadata->upm_procmode) {
		// Frequency order same as beamlet order
		case PACKET_FULL_COPY:
			strncpy(metadata->upm_outputfmt[0], "PKT-HDR-XrXiYrYi", META_STR_LEN);
			strncpy(metadata->upm_outputfmt_comment, "Full Packet Copy", META_STR_LEN);
			break;
		case PACKET_NOHDR_COPY:
			strncpy(metadata->upm_outputfmt[0], "PKT-XrXiYrYi", META_STR_LEN);
			strncpy(metadata->upm_outputfmt_comment, "Headerless Packet Copy", META_STR_LEN);
			break;
		case PACKET_SPLIT_POL:
			strncpy(metadata->upm_outputfmt[0], "Xr-PKT", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[1], "Xi-PKT", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[2], "Yr-PKT", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[3], "Yi-PKT", META_STR_LEN);
			strncpy(metadata->upm_outputfmt_comment, "Headerless Packet, Split by Polarisation", META_STR_LEN);
			break;

		case BEAMLET_MAJOR_FULL:
			strncpy(metadata->upm_outputfmt[0], "FREQ-POS-XrXiYrYi", META_STR_LEN);
			strncpy(metadata->upm_outputfmt_comment, "Beamlet Major", META_STR_LEN);
			break;
		case BEAMLET_MAJOR_SPLIT_POL:
			strncpy(metadata->upm_outputfmt[0], "Xr-FREQ-POS", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[1], "Xi-FREQ-POS", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[2], "Yr-FREQ-POS", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[3], "Yi-FREQ-POS", META_STR_LEN);
			strncpy(metadata->upm_outputfmt_comment, "Beamlet Major, Split by Polarisation", META_STR_LEN);
			break;
		case BEAMLET_MAJOR_FULL_REV:
			strncpy(metadata->upm_outputfmt[0], "FREQ-NEG-XrXiYrYi", META_STR_LEN);
			strncpy(metadata->upm_outputfmt_comment, "Reversed Beamlet Major", META_STR_LEN);
			break;
		case BEAMLET_MAJOR_SPLIT_POL_REV:
			strncpy(metadata->upm_outputfmt[0], "Xr-FREQ-NEG", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[1], "Xi-FREQ-NEG", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[2], "Yr-FREQ-NEG", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[3], "Yi-FREQ-NEG", META_STR_LEN);
			strncpy(metadata->upm_outputfmt_comment, "Reversed Beamlet Major, Split by Polarisation", META_STR_LEN);
			break;

		case TIME_MAJOR_FULL:
			strncpy(metadata->upm_outputfmt[0], "TIME-XrXiYrYi", META_STR_LEN);
			strncpy(metadata->upm_outputfmt_comment, "Chunked Time Major", META_STR_LEN);
			break;
		case TIME_MAJOR_SPLIT_POL:
			strncpy(metadata->upm_outputfmt[0], "Xr-TIME", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[1], "Xi-TIME", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[2], "Yr-TIME", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[3], "Yi-TIME", META_STR_LEN);
			strncpy(metadata->upm_outputfmt_comment, "Chunked Time Major, Split by Polarisation", META_STR_LEN);
			break;
		case TIME_MAJOR_ANT_POL:
			strncpy(metadata->upm_outputfmt[0], "XrXi-TIME", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[1], "YrYi-TIME", META_STR_LEN);
			strncpy(metadata->upm_outputfmt_comment, "Chunked Time Major, Split by Antenna", META_STR_LEN);
			break;
		case TIME_MAJOR_ANT_POL_FLOAT:
			strncpy(metadata->upm_outputfmt[0], "XrXi-TIME", META_STR_LEN);
			strncpy(metadata->upm_outputfmt[1], "YrYi-TIME", META_STR_LEN);
			strncpy(metadata->upm_outputfmt_comment, "Chunked Time Major, Split by Antenna, Forced Float Output", META_STR_LEN);
			break;


		case STOKES_I_REV ... STOKES_I_DS16_REV:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "I-NEG-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes I, with reversed frequencies and %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_Q_REV ... STOKES_Q_DS16_REV:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "Q-NEG-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes Q, with reversed frequencies and %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_U_REV ... STOKES_U_DS16_REV:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "U-NEG-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes U, with reversed frequencies and %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_V_REV ... STOKES_V_DS16_REV:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "V-NEG-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes V, with reversed frequencies and %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_IQUV_REV ... STOKES_IQUV_DS16_REV:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "I-NEG-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt[1], META_STR_LEN, "Q-NEG-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt[2], META_STR_LEN, "U-NEG-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt[3], META_STR_LEN, "V-NEG-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes IQUV, with reversed frequencies and %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_IV_REV ... STOKES_IV_DS16_REV:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "I-NEG-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt[1], META_STR_LEN, "V-NEG-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes IV, with reversed frequencies and %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;


		case STOKES_I ... STOKES_I_DS16:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "I-POS-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes I, with %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_Q ... STOKES_Q_DS16:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "Q-POS-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes Q, with %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_U ... STOKES_U_DS16:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "U-POS-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes U, with %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_V ... STOKES_V_DS16:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "V-POS-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes V, with %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_IQUV ... STOKES_IQUV_DS16:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "I-POS-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt[1], META_STR_LEN, "Q-POS-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt[2], META_STR_LEN, "U-POS-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt[3], META_STR_LEN, "V-POS-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes IQUV, with %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_IV ... STOKES_IV_DS16:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "I-POS-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt[1], META_STR_LEN, "V-POS-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes IV, with %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;

		default:
			fprintf(stderr, "ERROR %s: Unknown processing mode %d, exiting.\n", __func__, metadata->upm_procmode);
			return -1;
	}

	// Data states / polarisations, psrchive -> Util/genutil/Types.C
	/*
	 * Nyquist, Analytic, Intensity, NthPower, PPQQ, Coherence, Stokes, PseudoStokes, PP, QQ
	 * http://psrchive.sourceforge.net/classes/Util/namespaceSignal.shtml
	 */
	switch (metadata->upm_procmode) {
		// Packet copies, no change
		case PACKET_FULL_COPY:
		case PACKET_NOHDR_COPY:
		// Data reordered, single output, no change
		case BEAMLET_MAJOR_FULL:
		case BEAMLET_MAJOR_FULL_REV:
		case TIME_MAJOR_FULL:
			metadata->ndim = 2;
			metadata->npol = 2;
			break;

		// Polarisations/real/complex values split into their own files
		case PACKET_SPLIT_POL:
		case BEAMLET_MAJOR_SPLIT_POL:
		case BEAMLET_MAJOR_SPLIT_POL_REV:
		case TIME_MAJOR_SPLIT_POL:
			metadata->ndim = 1;
			metadata->npol = 1;
			break;

		// Split by antenna polarisation, but not real/complex
		case TIME_MAJOR_ANT_POL:
		case TIME_MAJOR_ANT_POL_FLOAT:
			metadata->ndim = 2;
			metadata->npol = 1;
			break;


		// Stokes outputs
		// May need to double check, should npol be 1 here?
		case STOKES_I ... STOKES_I_DS16:
		case STOKES_Q ... STOKES_Q_DS16:
		case STOKES_U ... STOKES_U_DS16:
		case STOKES_V ... STOKES_V_DS16:
		case STOKES_IQUV ... STOKES_IQUV_DS16:
		case STOKES_IV ... STOKES_IV_DS16:
		case STOKES_I_REV ... STOKES_I_DS16_REV:
		case STOKES_Q_REV ... STOKES_Q_DS16_REV:
		case STOKES_U_REV ... STOKES_U_DS16_REV:
		case STOKES_V_REV ... STOKES_V_DS16_REV:
		case STOKES_IQUV_REV ... STOKES_IQUV_DS16_REV:
		case STOKES_IV_REV ... STOKES_IV_DS16_REV:
			metadata->ndim = 1;
			metadata->npol = 2;
			break;

		default:
			fprintf(stderr, "ERROR %s: Unknown processing mode %d, exiting.\n", __func__, metadata->upm_procmode);
			return -1;
	}

	switch (metadata->upm_procmode) {
		// Packet copies, no change
		case PACKET_FULL_COPY:
		case PACKET_NOHDR_COPY:
		// Data reordered, single output, no change
		case BEAMLET_MAJOR_FULL:
		case BEAMLET_MAJOR_FULL_REV:
		case TIME_MAJOR_FULL:
		// Single Stokes outputs
		case STOKES_I ... STOKES_I_DS16:
		case STOKES_Q ... STOKES_Q_DS16:
		case STOKES_U ... STOKES_U_DS16:
		case STOKES_V ... STOKES_V_DS16:
		case STOKES_I_REV ... STOKES_I_DS16_REV:
		case STOKES_Q_REV ... STOKES_Q_DS16_REV:
		case STOKES_U_REV ... STOKES_U_DS16_REV:
		case STOKES_V_REV ... STOKES_V_DS16_REV:
			metadata->upm_rel_outputs[0] = 1;
			for (int i = 1; i < MAX_OUTPUT_DIMS; i++) {
				metadata->upm_rel_outputs[i] = 0;
			}
			break;

		// Polarisations/real/complex values split into their own files
		case PACKET_SPLIT_POL:
		case BEAMLET_MAJOR_SPLIT_POL:
		case BEAMLET_MAJOR_SPLIT_POL_REV:
		case TIME_MAJOR_SPLIT_POL:
		// Stokes IQUV
		case STOKES_IQUV ... STOKES_IQUV_DS16:
		case STOKES_IQUV_REV ... STOKES_IQUV_DS16_REV:
			for (int i = 0; i < MAX_OUTPUT_DIMS; i++) {
				metadata->upm_rel_outputs[i] = 1;
			}
			break;

		// Split by antenna polarisation, but not real/complex
		case TIME_MAJOR_ANT_POL:
		case TIME_MAJOR_ANT_POL_FLOAT:
			for (int i = 0; i < MAX_OUTPUT_DIMS; i++) {
				if (i < 2) {
					metadata->upm_rel_outputs[i] = 1;
				} else {
					metadata->upm_rel_outputs[i] = 0;
				}
			}
			break;

		// Stokes IV
		case STOKES_IV ... STOKES_IV_DS16:
		case STOKES_IV_REV ... STOKES_IV_DS16_REV:
			for (int i = 0; i < MAX_OUTPUT_DIMS; i++) {
				if (i == 0 || i == 3) {
					metadata->upm_rel_outputs[i] = 1;
				} else {
					metadata->upm_rel_outputs[i] = 0;
				}
			}
			break;


		default:
			fprintf(stderr, "ERROR %s: Unknown processing mode %d, exiting.\n", __func__, metadata->upm_procmode);
			return -1;
	}

	switch (metadata->upm_procmode) {
		case PACKET_FULL_COPY ... PACKET_SPLIT_POL:
		case BEAMLET_MAJOR_FULL ... BEAMLET_MAJOR_SPLIT_POL:
		case BEAMLET_MAJOR_FULL_REV ... BEAMLET_MAJOR_SPLIT_POL_REV:
		case TIME_MAJOR_FULL ... TIME_MAJOR_ANT_POL:
		case TIME_MAJOR_ANT_POL_FLOAT:
			// Analytic: In-phase and Quadrature sampled voltages (complex)
			metadata->upm_output_voltages = 1;
			if (strncpy(metadata->state, "Analytic", META_STR_LEN) != metadata->state) {
				fprintf(stderr, "ERROR: Failed to set metadata state, exiting.\n");
				return -1;
			}
			break;


		// Stokes outputs
		case STOKES_I ... STOKES_I_DS16:
		case STOKES_Q ... STOKES_Q_DS16:
		case STOKES_U ... STOKES_U_DS16:
		case STOKES_V ... STOKES_V_DS16:
		case STOKES_IQUV ... STOKES_IQUV_DS16:
		case STOKES_IV ... STOKES_IV_DS16:
		case STOKES_I_REV ... STOKES_I_DS16_REV:
		case STOKES_Q_REV ... STOKES_Q_DS16_REV:
		case STOKES_U_REV ... STOKES_U_DS16_REV:
		case STOKES_V_REV ... STOKES_V_DS16_REV:
		case STOKES_IQUV_REV ... STOKES_IQUV_DS16_REV:
		case STOKES_IV_REV ... STOKES_IV_DS16_REV:
			// Intensity: Square-law detected total power.
			metadata->upm_output_voltages = 0;
			if (strncpy(metadata->state, "Intensity", META_STR_LEN) != metadata->state) {
				fprintf(stderr, "ERROR: Failed to set metadata state, exiting.\n");
				return -1;
			}
			break;

		default:
			fprintf(stderr, "ERROR %s: Unknown processing mode %d, exiting.\n", __func__, metadata->upm_procmode);
			return -1;
	}

	// Output bit mode
	switch (metadata->upm_procmode) {
		case PACKET_FULL_COPY ... PACKET_SPLIT_POL:
		case BEAMLET_MAJOR_FULL ... BEAMLET_MAJOR_SPLIT_POL:
		case BEAMLET_MAJOR_FULL_REV ... BEAMLET_MAJOR_SPLIT_POL_REV:
		case TIME_MAJOR_FULL ... TIME_MAJOR_ANT_POL:
			// Calibrated samples are always floats
			if (metadata->upm_calibrated) {
				metadata->nbit = -32;
			} else {
				metadata->nbit = metadata->upm_input_bitmode == 4 ? 8 : metadata->upm_input_bitmode;
			}
			break;

		// Mode 35 is the same as 32, but forced to be floats
		case TIME_MAJOR_ANT_POL_FLOAT:
		// Stokes outputs
		case STOKES_I ... STOKES_I_DS16:
		case STOKES_Q ... STOKES_Q_DS16:
		case STOKES_U ... STOKES_U_DS16:
		case STOKES_V ... STOKES_V_DS16:
		case STOKES_IQUV ... STOKES_IQUV_DS16:
		case STOKES_IV ... STOKES_IV_DS16:
		case STOKES_I_REV ... STOKES_I_DS16_REV:
		case STOKES_Q_REV ... STOKES_Q_DS16_REV:
		case STOKES_U_REV ... STOKES_U_DS16_REV:
		case STOKES_V_REV ... STOKES_V_DS16_REV:
		case STOKES_IQUV_REV ... STOKES_IQUV_DS16_REV:
		case STOKES_IV_REV ... STOKES_IV_DS16_REV:
			// Floats are defined as -32 since ints are also 32 bits wide
			// Sigproc will require the absolute value here
			metadata->nbit = -32;
			break;

		default:
			fprintf(stderr, "ERROR %s: Unknown processing mode %d, exiting.\n", __func__, metadata->upm_procmode);
			return -1;
	}

	return 0;
}


int _lofar_udp_metadata_handle_external_factors(lofar_udp_metadata *metadata, const metadata_config *config) {
	if (metadata == NULL || config == NULL) {
		fprintf(stderr, "ERROR %s: Passed parameter was null (metadata: %p, config %p), exiting.\n", __func__, metadata, config);
		return -1;
	}

	if (metadata->external_channelisation > 1 && metadata->external_channelisation != config->externalChannelisation) {
		fprintf(stderr, "WARNING: While setting up channelisation, a mismatch was detected between the previous configuration and the current configuration (%d vs %d).\n", metadata->external_channelisation, config->externalChannelisation);
	}
	if (metadata->external_downsampling > 1 && metadata->external_downsampling != config->externalDownsampling) {
		fprintf(stderr, "WARNING: While setting up downsampling, a mismatch was detected between the previous configuration and the current configuration (%d vs %d).\n", metadata->external_downsampling, config->externalDownsampling);
	}

	if (config->externalChannelisation > 1) {
		metadata->external_channelisation = config->externalChannelisation;
	}
	if (config->externalDownsampling > 1) {
		metadata->external_downsampling = config->externalDownsampling;
	}

	metadata->tsamp = metadata->tsamp_raw;
	metadata->channel_bw = metadata->subband_bw;
	metadata->nchan = metadata->nsubband;

	if (metadata->external_channelisation > 1) {
		metadata->tsamp *= metadata->external_channelisation;
		metadata->channel_bw /= metadata->external_channelisation;
		metadata->nchan *= metadata->external_channelisation;

		double signCorrection = metadata->bw > 0 ? 1.0 : -1.0;

		double highestFreq = metadata->freq_raw // Centre frequency
								+ (((double) (metadata->nsubband - 1) / (double) (2 * metadata->nsubband)) * signCorrection * metadata->bw) // Centre of the top subband
								+ (((double) ((int32_t) (metadata->external_channelisation / 2))) * signCorrection * metadata->channel_bw) // Centre of the top channel
								+ (0.5 * signCorrection *  metadata->channel_bw); // top of the final channel
		double lowestFreq = highestFreq - (metadata->nchan * signCorrection * metadata->channel_bw);
		metadata->freq = (highestFreq + lowestFreq) / 2;

		if (signCorrection == 1.0) {
			metadata->ftop = lowestFreq;
			metadata->fbottom = highestFreq;
		} else {
			metadata->ftop = highestFreq;
			metadata->fbottom = lowestFreq;
		}
	} else {
		metadata->freq = metadata->freq_raw;
	}

	if (metadata->external_downsampling > 1) {
		metadata->tsamp *= metadata->external_downsampling;
	}

	return 0;
}


/**
 * @brief      Convert the station ID to the station code
 * 				RSP station ID != intl station ID. See
 * 				https://git.astron.nl/ro/lofar/-/raw/master/MAC/Deployment/data/StaticMetaData/StationInfo.dat
 * 				RSP hdr byte 4 reports 32 * ID code in result above. Divide by 32 on any port to round down to target code.
 *
 * @param[in]  stationID    The station id
 * @param      stationCode  The output station code (min size: 5 bytes)
 *
 * @return     0: Success, 1: Failure
 */
int _lofar_udp_metadata_get_station_name(int stationID, char *stationCode) {
	if (stationCode == NULL) {
		return -1;
	}
	switch (stationID) {
		// Core Stations
		case 1 ... 7:
		case 11:
		case 13:
		case 17:
		case 21:
		case 24:
		case 26:
		case 28:
		case 30:
		case 31:
		case 32:
		case 101:
		case 103:
			sprintf(stationCode, "CS%03d", stationID);
			break;

		case 121:
			sprintf(stationCode, "CS201");
			break;

		case 141 ... 142:
			sprintf(stationCode, "CS%03d", 301 + (stationID % 141));
			break;

		case 161:
			sprintf(stationCode, "CS401");
			break;

		case 181:
			sprintf(stationCode, "CS501");
			break;


		// Remote Stations
		case 106:
			sprintf(stationCode, "RS%03d", stationID);
			break;

		case 125:
		case 128:
		case 130:
			sprintf(stationCode, "RS%03d", 205 + (stationID % 125));
			break;

		case 145 ... 147:
		case 150:
			sprintf(stationCode, "RS%03d", 305 + (stationID % 145));
			break;

		case 166 ... 167:
		case 169:
			sprintf(stationCode, "RS%03d", 406 + (stationID % 166));
			break;

		case 183:
		case 188 ... 189:
			sprintf(stationCode, "RS%03d", 503 + (stationID % 183));
			break;


		// Intl Stations
		// DE
		case 201 ... 205:
			sprintf(stationCode, "DE%03d", 601 + (stationID % 201));
			break;

		case 210:
			sprintf(stationCode, "DE609");
			break;


		// FR
		case 206:
			sprintf(stationCode, "FR606");
			break;

		// SE
		case 207:
			sprintf(stationCode, "SE607");
			break;

		// UK
		case 208:
			sprintf(stationCode, "UK608");
			break;

		// PL
		case 211 ... 213:
			sprintf(stationCode, "PL%03d", 610 + (stationID % 211));
			break;

		// IE
		case 214:
			sprintf(stationCode, "IE613");
			break;

		// LV
		case 215:
			sprintf(stationCode, "LV614");
			break;

		// KAIRA
		case 901:
			sprintf(stationCode, "FI901");
			break;

		// LOFAR4SW test station
		case 902:
			sprintf(stationCode, "UK902");
			break;

		default:
			fprintf(stderr, "Unknown telescope ID %d. Was a new station added to the array? Update %s\n",
			        stationID, __func__);
			return 1;
	}

	return !(strnlen(stationCode, 6) == 5);
}


int _isEmpty(const char *string) {
	if (string == NULL) {
		return 1;
	}
	return string[0] == '\0';
}

int _intNotSet(int input) {
	return input == -1;
}

int _longNotSet(long input) {
	return input == -1;
}

int _floatNotSet(float input, int exception) {
	// Exception: it might be sane for some values to be -1.0f, so add a non-zero check instead
	if (exception) {
		return input == 0.0f;
	}

	return input == -1.0f;
}

int _doubleNotSet(double input, int exception) {
	// Exception: it might be sane for some values to be -1.0, so add a non-zero check instead
	if (exception) {
		return input == 0.0;
	}

	return input == -1.0;
}

#include "./metadata/lofar_udp_metadata_GUPPI.c" // NOLINT(bugprone-suspicious-include)
#include "./metadata/lofar_udp_metadata_DADA.c" // NOLINT(bugprone-suspicious-include)
#include "./metadata/lofar_udp_metadata_SIGPROC.c" // NOLINT(bugprone-suspicious-include)