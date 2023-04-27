#include "lofar_udp_metadata.h"

/**
 * @brief 			Provide a guess at the intended output metadata based on a filename
 *
 * @param[in] optargc 	Input filename
 *
 * @return 			metadata_t: Best guess at type, or NO_META
 */
metadata_t lofar_udp_metadata_parse_type_output(const char optargc[]) {
	if (optargc == NULL || !strlen(optargc)) {
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
 * @brief Setup a metadata struct based on the current reader and metadata configurations
 *
 * @param metadata		Output metadata struct
 * @param[in] reader	Input reader struct
 * @param[in] config	Input configuration struct
 *
 * @return 0: success, <0: failure
 */
int32_t lofar_udp_metadata_setup(lofar_udp_metadata *const metadata, const lofar_udp_reader *reader, const metadata_config *config) {

	// Sanity check the metadata and reader structs
	if (metadata == NULL || config == NULL) {
		fprintf(stderr, "ERROR %s: Metadata pointer (%p) or config pointer (%p) is null we cannot progress, exiting.\n", __func__, metadata, config);
		return -1;
	}

	// Ensure we are meant to generate metadata
	if (metadata->type == NO_META) {
		if (!strnlen(config->metadataLocation, DEF_STR_LEN)) {
			fprintf(stderr, "WARNING %s: No metadata type or location specified, exiting.\n", __func__);
			return 0;
		}
		// So there was a metadata input? Change type to basic parsing
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
	if (strnlen(config->metadataLocation, DEF_STR_LEN) > 0) {
		if (_lofar_udp_metadata_parse_input_file(metadata, config->metadataLocation) < 0) {
			return -1;
		}
	} else if (config->metadataType >= DEFAULT_META) {
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

		// Shouldn't be reachable; we exit earlier if there's no metadata to process
		case NO_META:
		default:
			__builtin_unreachable();
			fprintf(stderr, "ERROR %s: Unknown metadata type %d, exiting.\n", __func__, metadata->type);
			return -1;
	}
}

/**
 * @brief Update the metadata struct based on the current reader state
 *
 * @param[in] reader	Input reader struct
 * @param metadata		Output struct
 * @param[in] newObs	Handle update for a new observation / output
 *
 * @return 0: success, <0: failure
 */
int32_t lofar_udp_metadata_update(const lofar_udp_reader *reader, lofar_udp_metadata *const metadata, const int8_t newObs) {
	if (reader == NULL) {
		fprintf(stderr, "ERROR %s: Input reader is null, metadata cannot be updated, exiting.\n", __func__);
		return -1;
	}

	// Return if these parameters have not been initialised, likely intentional
	if (metadata == NULL || metadata->type == NO_META) {
		return 0;
	}

	if (newObs) {
		metadata->output_file_number += 1;
	}

	// Update the base information before performing specific updates
	if (_lofar_udp_metadata_update_BASE(reader, metadata, newObs) < 0) {
		return -1;
	}

	switch(metadata->type) {
		case GUPPI:
			if (_lofar_udp_metadata_update_GUPPI(metadata, newObs) < 0) {
				return -1;
			}
			return 0;

		// DADA/HDF5 is the base case we ran previously, just return
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

	__builtin_unreachable();

}

/**
 * @brief Update the metadata struct based on the current reader configuration
 *
 * @param[in] reader 	The input reader
 * @param metadata 		The output struct
 * @param[in] newObs 	Handle update for a new observation / output
 *
 * @return 0: success, <0: Failure
 */
int32_t _lofar_udp_metadata_update_BASE(const lofar_udp_reader *reader, lofar_udp_metadata *const metadata, int8_t newObs) {

	if (reader == NULL || metadata == NULL) {
		fprintf(stderr, "ERROR %s: Input struct is null (reader: %p, metadata: %p), exiting.\n", __func__, reader, metadata);
		return -1;
	}

	// DADA headers are only really meant to be used once at the start of the file, so they aren't meant to change too much
	// But since we use this struct as the base, we'll need to update the bulk of the contents on every iteration.

	if (newObs) {
		lofar_udp_time_get_current_isot(reader, metadata->obs_utc_start, VAR_ARR_SIZE(metadata->obs_utc_start));
		metadata->obs_mjd_start = lofar_udp_time_get_packet_time_mjd(reader->meta->inputData[0]);
		metadata->upm_processed_packets = 0;
		metadata->upm_dropped_packets = 0;
	}

	lofar_udp_time_get_daq(reader, metadata->upm_daq, VAR_ARR_SIZE(metadata->upm_daq));
	metadata->upm_pack_per_iter = reader->meta->packetsPerIteration;
	metadata->upm_blocksize = metadata->upm_pack_per_iter * reader->meta->packetOutputLength[0];
	metadata->upm_processed_packets += metadata->upm_pack_per_iter * metadata->upm_num_inputs;

	metadata->upm_last_dropped_packets = 0;
	for (int8_t port = 0; port < metadata->upm_num_inputs; port++) {
		metadata->upm_last_dropped_packets += reader->meta->portLastDroppedPackets[port];
	}
	metadata->upm_dropped_packets += metadata->upm_last_dropped_packets;

	return 0;
}

int64_t _lofar_udp_metadata_write_buffer(const lofar_udp_reader *reader, lofar_udp_metadata *const metadata, int8_t *headerBuffer, int64_t headerBufferSize, int8_t newObs) {
	return _lofar_udp_metadata_write_buffer_force(reader, metadata, headerBuffer, headerBufferSize, newObs, 0);
}

/**
 * @brief Update and write the configured metadata output to the given header buffer
 *
 * @param[in] reader			The input reader
 * @param metadata				The struct to be updated
 * @param headerBuffer			The output buffer
 * @param[in] headerBufferSize	The output buffer length
 * @param[in] newObs 			Handle update for a new observation / output
 * @param[in] force				Force a write (for metadata that's normally only at the start of a file)
 *
 * @return 0: no work performed, >1: success, represents output length, -1: failure
 */
int64_t
_lofar_udp_metadata_write_buffer_force(const lofar_udp_reader *reader, lofar_udp_metadata *const metadata, int8_t *const headerBuffer, const int64_t headerBufferSize, const int8_t newObs, const int8_t force) {
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

	__builtin_unreachable();
}

/**
 * @brief Perform a normal header write, to an intermediate buffer than then an output file
 *
 * @param[in] reader			The input reader
 * @param outConfig				The output wrtier struct
 * @param outp					The index of the output writer
 * @param metadata				The struct to be updated
 * @param headerBuffer			The output buffer
 * @param[in] headerBufferSize	The output buffer length
 * @param[in] newObs 			Handle update for a new observation / output
 *
 * @return < 0: failure, > 0: success, output length
 */
int64_t
lofar_udp_metadata_write_file(const lofar_udp_reader *reader, lofar_udp_io_write_config *const outConfig, const int8_t outp, lofar_udp_metadata *const metadata, int8_t *const headerBuffer,
                              const int64_t headerBufferSize, const int8_t newObs) {
	return _lofar_udp_metadata_write_file_force(reader, outConfig, outp, metadata, headerBuffer, headerBufferSize, newObs, 0);
}

/**
 * @brief Perform a (optionally) forced header write, to an intermediate buffer than then an output file
 *
 * @param[in] reader			The input reader
 * @param outConfig				The output wrtier struct
 * @param outp					The index of the output writer
 * @param metadata				The struct to be updated
 * @param headerBuffer			The output buffer
 * @param[in] headerBufferSize	The output buffer length
 * @param[in] newObs 			Handle update for a new observation / output
 * @param[in] force				Foce the write to happen, regardless of metadata type's conventions
 *
 * @return
 */
int64_t _lofar_udp_metadata_write_file_force(const lofar_udp_reader *reader, lofar_udp_io_write_config *const outConfig, const int8_t outp, lofar_udp_metadata *const metadata,
                                             int8_t *const headerBuffer, const int64_t headerBufferSize, const int8_t newObs, const int8_t force) {
	int64_t returnVal = _lofar_udp_metadata_write_buffer_force(reader, metadata, headerBuffer, headerBufferSize, newObs, force);

	if (returnVal > 0) {
		return lofar_udp_io_write_metadata(outConfig, outp, metadata, headerBuffer, headerBufferSize);
	}

	return returnVal;
}

/**
 * @brief Perform basic initialisation of parameters of a metadata struct
 *
 * @param metadata	Struct to be initialised
 *
 * @return 0: success, <0: failure
 */
int32_t _lofar_udp_metdata_setup_BASE(lofar_udp_metadata *const metadata) {
	if (metadata == NULL) {
		fprintf(stderr, "ERROR %s: Input metadata struct is null, exiting.\n", __func__);
		return -1;
	}

	// Exclude DEFAULT_META and HDF5_META as they should not need buffers
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

	// By default, we don't know the expected bandwidth order, assume it matches the beamctl input
	metadata->upm_bandflip = 0;


	return 0;
}

/**
 * @brief Parse an input metadata file containing beamctl commands and TSVs and configure the struct
 *
 * @param metadata	The struct to configure
 * @param inputFile	The input file
 *
 * @return 0: success, <0: failure
 */
int32_t _lofar_udp_metadata_parse_input_file(lofar_udp_metadata *const metadata, const char inputFile[]) {
	if (metadata == NULL || inputFile == NULL || !strnlen(inputFile, DEF_STR_LEN)) {
		fprintf(stderr, "ERROR %s: Metadata (%p), Input file pointer (%p) or value (%s) is null, exiting.\n", __func__, metadata, inputFile, inputFile);
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


	int32_t beamctlData[9] = { // int32_t due to summation elements, rest are well-contained by int16_t
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
	if (strcasestr(inputFile, yamlSignature) != NULL) {
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

/**
 * @brief Parse an input file that contains beamctl commands and TSVs
 *
 * @param metadata 		The struct to configure
 * @param input 		The input file
 * @param beamctlData 	Limits on beam data from previously parsed files
 *
 * @return 0: success, <0: failure
 */
int32_t _lofar_udp_metadata_parse_normal_file(lofar_udp_metadata *const metadata, FILE *const input, int32_t *const beamctlData) {

	char *inputLine = NULL, *strPtr = NULL;
	size_t buffLen = 0;
	ssize_t lineLength;
	// For each line in the file
	while ((lineLength = getline(&inputLine, &buffLen, input)) != -1) {

		VERBOSE(printf("Parsing line (%ld): %s\n", lineLength, inputLine));
		if ((strPtr = strcasestr(inputLine, "beamctl ")) != NULL) {

			VERBOSE(printf("Beamctl detected, passing on.\n"));
			if (_lofar_udp_metadata_parse_beamctl(metadata, strPtr, beamctlData) < 0) {
				FREE_NOT_NULL(inputLine);
				return -1;
			}

			// Append the line to the metadata struct
			char *fixPtr;
			while((fixPtr = strcasestr(inputLine, "\n"))) {
				*(fixPtr) = '\t';
			}
			strncat(metadata->upm_beamctl, inputLine, 2 * DEF_STR_LEN - 1 - strlen(metadata->upm_beamctl));


		} else if ((strPtr = strcasestr(inputLine, "SOURCE\t")) != NULL) {

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
		} else if ((strPtr = strcasestr(inputLine, "OBS-ID\t")) != NULL) {

			VERBOSE(printf("Observation ID detected, passing on.\n"));
			if (_lofar_udp_metadata_get_tsv(strPtr, "OBS-ID", metadata->obs_id) < 0) {
				FREE_NOT_NULL(inputLine);
				return -1;
			}
		} else if ((strPtr = strcasestr(inputLine, "OBSERVER\t")) != NULL) {

			VERBOSE(printf("Observer detected, passing on.\n"));
			if (_lofar_udp_metadata_get_tsv(strPtr, "OBSERVER", metadata->observer) < 0) {
				FREE_NOT_NULL(inputLine);
				return -1;
			}
		} /*
		else if ((strPtr = strstr()) != NULL) {

		} */


		VERBOSE(
			printf("Beamctl data state: %d, %d, %d, %d, %d, %d\n", beamctlData[0], beamctlData[1], beamctlData[2], beamctlData[3], beamctlData[4], beamctlData[5]);
			        VERBOSE(printf("Getting next line.\n"))
		);

	}
	// Free the getline buffer if it still exists (during testing this was raised as a non-free'd pointer)
	FREE_NOT_NULL(inputLine);

	return 0;
}


__attribute__((unused)) int32_t _lofar_udp_metadata_parse_yaml_file(lofar_udp_metadata *const metadata, FILE *const input, int32_t *const beamctlData) {
	fprintf(stderr, "WARNING: YAML files not currently supported, will attempt to fallback to plain text parser.\n");

	return _lofar_udp_metadata_parse_normal_file(metadata, input, beamctlData);
}

/**
 * @brief Parse data from the sreader struct and update the metadata accordingly
 *
 * @param metadata 		Stuct to update
 * @param[in] reader	Reader to parse
 *
 * @return 0: success, <0: failure
 */
int32_t _lofar_udp_metadata_parse_reader(lofar_udp_metadata *const metadata, const lofar_udp_reader *reader) {
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

		// Verify clock bit matches expectations
		int16_t beamletsPerPort;
		int16_t clockAlias = reader->meta->clockBit ? 200 : 160; // 1 -> 200Mhz, 0 -> 160MHz, verify, sample data doesn't change the bit...
		if (clockAlias != metadata->upm_rcuclock) {
			fprintf(stderr, "WARNING: Clock bit mismatch (metadata suggests the %d clock, raw data suggests the %d clock\n", metadata->upm_rcuclock,
			        reader->meta->clockBit);
		}

		// Sanity check beam counts
		if ((beamletsPerPort = _lofar_udp_metadata_get_beamlets(reader->meta->inputBitMode)) < 0) {
			return -1;
		}

		VERBOSE(
			printf("%d, %d, %d, %d, %d\n", reader->input->numInputs, (reader->input->offsetPortCount), (beamletsPerPort), reader->meta->baseBeamlets[0],
		               reader->meta->upperBeamlets[reader->meta->numPorts - 1])
	    );
		// Calculate the true beam limits from processing limitations
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
		for (int16_t beamlet = metadata->lowerBeamlet; beamlet < metadata->upperBeamlet; beamlet++) {
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

		// Copy over processing parameters
		metadata->baseport = reader->input->basePort;
		metadata->upm_reader = reader->input->readerType;
		metadata->upm_replay = reader->meta->replayDroppedPackets;
		metadata->upm_pack_per_iter = reader->packetsPerIteration;
		metadata->upm_blocksize = metadata->upm_pack_per_iter * reader->meta->packetOutputLength[0];
		metadata->upm_procmode = reader->meta->processingMode;
		metadata->upm_input_bitmode = reader->meta->inputBitMode;
		metadata->upm_calibrated = reader->meta->calibrateData;

		if (_lofar_udp_metadata_processing_mode_metadata(metadata) < 0) {
			return -1;
		}

		if (_lofar_udp_metadata_update_frequencies(metadata, subbandData) < 0) {
			return -1;
		}

		// Get the Telescope ID/Name
		metadata->telescope_rsp_id = reader->meta->stationID;
		if (_lofar_udp_metadata_get_station_name(metadata->telescope_rsp_id, metadata->telescope) < 0) {
			return -1;
		}

		metadata->upm_num_inputs = reader->input->numInputs;
		metadata->upm_num_outputs = reader->meta->numOutputs;


		// Compute start time in multiple formats (ISOT, MJD, DAQ)
		lofar_udp_time_get_current_isot(reader, metadata->obs_utc_start, VAR_ARR_SIZE(metadata->obs_utc_start));
		metadata->obs_mjd_start = lofar_udp_time_get_packet_time_mjd(reader->meta->inputData[0]);
		lofar_udp_time_get_daq(reader, metadata->upm_daq, VAR_ARR_SIZE(metadata->upm_daq));

		for (int8_t port = 0; port < metadata->upm_num_inputs; port++) {
			if (strncpy(metadata->rawfile[port], reader->input->inputLocations[port], META_STR_LEN) != metadata->rawfile[port]) {
				fprintf(stderr, "ERROR: Failed to copy raw filename %d to metadata struct, exiting.\n", port);
				return -1;
			}
		}
	}

	return 0;
}

/**
 * @brief Parse a beamctl command to extract clock, beamlet and frequency information about the observation
 *
 * @param metadata		Struct to update
 * @param[in] inputLine	Beamctl line to parse
 * @param beamData		Beamlet information output
 *
 * @return 0: success, -1: failure
 */
int32_t _lofar_udp_metadata_parse_beamctl(lofar_udp_metadata *const metadata, const char *inputLine, int32_t *const beamData) {
	if (metadata == NULL || inputLine == NULL || beamData == NULL) {
		fprintf(stderr, "ERROR %s: Passed null input pointer (%p, %p, %p), exiting.\n", __func__, metadata, inputLine, beamData);
		return -1;
	}

	char *strCopy = calloc(strlen(inputLine) + 1, sizeof(char));
	CHECK_ALLOC_NOCLEAN(strCopy, -1);
	if (memcpy(strCopy, inputLine, strlen(inputLine)) != strCopy) {
		fprintf(stderr, "ERROR: Failed to copy line to intermediate buffer, exiting.\n");
		free(strCopy);
		return -1;
	}

	// Iterate through the words in the command
	char *workingPtr, *tokenPtr;
	char token[2] = " ";
	char *inputPtr = strtok_r(strCopy, token, &tokenPtr);

	if (inputPtr == NULL) {
		// Should be unreachable; we search for "beamctl " when calling this function
		fprintf(stderr, "ERROR: Failed to find space in beamctl command, exiting.\n");
		free(strCopy);
		return -1;
	}

	// Loop over every token (word)
	do {
		VERBOSE(printf("Starting beamctl loop for token %s\n", inputPtr));
		// Number of RCUs
		if ((workingPtr = strstr(inputPtr, "--rcus=")) != NULL) {
			// Count the number of RCUs used; actual values don't matter
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

	// Repeat the process to extract the subband and beamlet information
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
		metadata->subband_bw = ((double) metadata->upm_rcuclock) / 1024.0;
	} else if (metadata->subband_bw != (((double) metadata->upm_rcuclock) / 1024.0)) {
		fprintf(stderr, "ERROR: Subband bandwidth mismatch (was %lf, now %lf), exiting.\n", metadata->subband_bw, ((double) metadata->upm_rcuclock) / 1024);
		return -1;
	}

	return 0;
}

/**
 * @brief Convert a rcumode/beand to clock and frequency information
 *
 * @param metadata 		Struct to update
 * @param inputStr 		A string containing an observation mode/band
 * @param beamctlData 	Beamlet information output
 *
 * @return 0: success, <0: failure
 */
int32_t _lofar_udp_metadata_parse_rcumode(lofar_udp_metadata *const metadata, const char *inputStr, int32_t *const beamctlData) {
	if (metadata == NULL || inputStr == NULL || beamctlData == NULL) {
		fprintf(stderr, "ERROR %s: Passed null pointer (%p, %p, %p), exiting.\n", __func__, metadata, inputStr, beamctlData);
		return -1;
	}

	int16_t workingInt = -1;
	// Extract the set rcumode/band value
	if (sscanf(inputStr, "%*[^=]=%hd", &(workingInt)) < 0) {
		return -1;
	}

	int16_t lastClock = metadata->upm_rcuclock;
	metadata->upm_rcuclock = _lofar_udp_metadata_get_clockmode(workingInt);
	metadata->upm_rcumode = _lofar_udp_metadata_get_rcumode(workingInt);
	lastClock = lastClock > 0 ? lastClock : metadata->upm_rcuclock;

	// Only mode 6 can use the 160MHz clock
	if (lastClock == 160 && metadata->upm_rcumode != 6) {
		fprintf(stderr, "ERROR: Conflicting RCU clock/mode information (160MHz clock, but mode %d, exiting.\n", metadata->upm_rcuclock);
		return -1;
	}

	// Extract the current RCU mode for offsetting subbands to the correct frequencies
	beamctlData[0] = (int32_t) metadata->upm_rcumode;

	if (beamctlData[0] == -1) {
		fprintf(stderr, "ERROR: Failed to determine RCU mode, exiting.\n");
		return -1;
	}

	// Ensure we are not mixing the 160MHz / 200MHz clock (fairly certain this is impossible anyway)
	if (lastClock != metadata->upm_rcuclock) {
		fprintf(stderr, "ERROR: 160/200MHz clock mixing detected (previously %d, now %d), this is not currently supported. Exiting.\n", lastClock, metadata->upm_rcuclock);
		return -1;
	}

	return 0;
}

/**
 * @brief Parse a pointing string and update the metadata struct
 *
 * @param metadata 	The struct to update
 * @param inputStr 	The input pointing string <double>,<double>,<basis>
 * @param[in] digi	Whether we are parsing a digidir, or anadir
 *
 * @return 0: success, <0: failure
 */
int32_t _lofar_udp_metadata_parse_pointing(lofar_udp_metadata *const metadata, const char inputStr[], const int8_t digi) {
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
	if (strnlen(metadata->coord_basis, META_STR_LEN) > 0) {
		VERBOSE(printf("Basis already set, comparing...\n"));
		if (strncmp(basis, metadata->coord_basis, 32) != 0) {
			fprintf(stderr, "ERROR: Coordinate basis mismatch while parsing directions (bool digdir=%d), parsed %s vs struct %s, exiting.\n", digi, basis, metadata->coord_basis);
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

/**
 * @brief Parse a subband string and update the metadata struct
 *
 * @param metadata		Struct to update
 * @param[in] inputLine Subband string to parse
 * @param results		Output metadata struct
 *
 * @return
 */
int32_t _lofar_udp_metadata_parse_subbands(lofar_udp_metadata *const metadata, const char *inputLine, int32_t *const results) {
	if (metadata == NULL || inputLine == NULL || results == NULL) {
		fprintf(stderr, "ERROR %s: Input is unallocated (%p, %p, %p), exiting.\n", __func__, metadata, inputLine, results);
		return -1;
	}

	// Removing 1/2 for now
	if (results[0] < 3 || results[0] > 7) {
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

		//case 1:
		//case 2:
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

	// Should be impossible as we search for "beamctl " before calling this command, so there should be 1 space regardless, but check anyway...
	if (inputPtr == NULL) {
		//__builtin_unreachable();
		fprintf(stderr, "ERROR: Failed to find space in beamctl command, exiting.\n");
		return -1;
	}

	int16_t subbandCount = 0, beamletCount = 0;
	do {
		VERBOSE(printf("Entering subband/beamlet parse loop for token %s\n", inputPtr));
		// Parse subbands
		if ((workingPtr = strstr(inputPtr, "--subbands=")) != NULL) {
			VERBOSE(printf("Subbands detected\n"));
			if ((subbandCount = _lofar_udp_metadata_parse_csv(workingPtr + strlen("--subbands="), subbands, &(results[1]), subbandOffset)) < 0) {
				return -1;
			}

		// Parse beamlets
		} else if ((workingPtr = strstr(inputPtr, "--beamlets=")) != NULL) {
			VERBOSE(printf("Beamlets detected\n"));
			if ((beamletCount = _lofar_udp_metadata_parse_csv(workingPtr + strlen("--beamlets="), beamlets, &(results[5]), 0)) < 0) {
				return -1;
			}

		}

	} while ((inputPtr = strtok_r(NULL, token, &tokenPtr)) != NULL);
	VERBOSE(printf("End subband/beamlet search loop\n"));

	// Sanity check parsed values
	if (subbandCount != beamletCount) {
		fprintf(stderr, "ERROR: Failed to parse matching numbers of both subbands and beamlets (%d/%d), exiting.\n", subbandCount, beamletCount);
		return -1;
	}

	if (subbandCount == 0 || subbandCount > MAX_NUM_PORTS * UDPMAXBEAM) {
		fprintf(stderr, "WARNING: Failed to parse sane amount of beams (expected between 1 and %d, got %d).\n", MAX_NUM_PORTS * UDPMAXBEAM, subbandCount);
	}

	// Add the parsed beams to the running total
	metadata->upm_rawbeamlets += beamletCount;

	// As a result of the above switch, for mode 5, subbands 512 - 1023, we have (512 - 1023) * (0.19) = 100 - 200MHz
	// Similarly no offset for mode 3, double the offset for mode 7, etc.
	// This is why I raise an error when mixing mode 6 (160MHz clock) and anything else (all on the 200MHz clock)
	for (int i = 0; i < metadata->upm_rawbeamlets; i++) {
		if (beamlets[i] != -1) {
			metadata->subbands[beamlets[i]] = (int16_t) (subbandOffset + subbands[i]);
		}
	}

	return 0;
}

/**
 * @brief Parse an input line to extract a result for a given key
 *
 * @param[in] inputStr 	String to parse
 * @param[in] keyword 	Input keyword
 * @param result	Output results
 *
 * @return
 */
int32_t _lofar_udp_metadata_get_tsv(const char *inputStr, const char *keyword, char *const result) {
	char key[META_STR_LEN + 1];
	if (sscanf(inputStr, "%64s\t%s", key, result) != 2) { // TODO: Inline STRINGIFY(META_STR_LEN), refusing to work forme right now.
		fprintf(stderr, "ERROR: Failed to get value for input '%s' keyword (parsed key/val of %s / %s), exiting.\n", keyword, key, result);
		return -1;
	}

	// Sanity check that we have gotten the correct key
	if (strncasecmp(key, keyword, META_STR_LEN * 2) != 0) {
		fprintf(stderr, "ERROR: Requested keyword %s does not match parsed key %s, exiting.\n", keyword, key);
		return -2;
	}

	VERBOSE(printf("Parsed for %s: %s / %s\n", keyword, key, result));

	return 0;
}

/**
 * @brief Count the number of CSV values in a given string
 *
 * @param[in] inputStr	Input string to parse
 *
 * @return < 1: Failure, > 1: number of elements
 */
int32_t _lofar_udp_metadata_count_csv(const char *inputStr) {
	return _lofar_udp_metadata_parse_csv(inputStr, NULL, NULL, 0);
}

/**
 * @brief Parse a string to extract and count the amount of CSV elements (ints expected)
 *
 * @param[in] inputStr	String to parse
 * @param values 		Stores raw extract values
 * @param data 			Stores counts, minima and maxima of the values parsed
 * @param[in] offset	Apply an offset to every value encountered (summed with value)
 *
 * @return < 1: failure, > 1 number of elements counted
 */
int16_t _lofar_udp_metadata_parse_csv(const char *inputStr, int16_t *const values, int32_t *const data, const int16_t offset) {
	if (inputStr == NULL) {
		fprintf(stderr, "ERROR %s: Input is unallocated, exiting.\n", __func__);
		return -1;
	}

	// TODO: While we use strtok_r to split up the input, an arbitrary input will fail if there
	//  is extra data with commas after the current word
	size_t bufferLen = strlen(inputStr) + 1;
	char *strCpy = calloc(bufferLen, sizeof(char));
	CHECK_ALLOC_NOCLEAN(strCpy, -1);
	if (memcpy(strCpy, inputStr, bufferLen - 1) != strCpy) {
		fprintf(stderr, "ERROR: Failed to make a copy of the csv string %s, exiting.\n", inputStr);
		FREE_NOT_NULL(strCpy);
		return -1;
	}

	VERBOSE(printf("Parse: %s\n", inputStr));

	// Working counters, final results will be copied to data
	int32_t sum = 0;
	int16_t counter = 0, lower = -1, upper = -1;
	int16_t minimum = INT16_MAX, maximum = -1;

	// Iterate through every comma-separated-value
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

		// Expand index notation, simplify to sumon range for counters
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

		// Extract a single int
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

	// Include the offset on all newly observed values
	minimum += offset;
	maximum += offset;
	sum += counter * offset;

	// Update limits, counters if needed
	if (data != NULL) {
		data[0] = (minimum < data[0]) ? minimum : data[0];
		data[1] += sum;
		data[2] = (maximum > data[0]) ? maximum : data[0];
		data[3] += counter;
	}

	FREE_NOT_NULL(strCpy);
	return counter;
}

/**
 * @brief Convert a band/rcumode t the corresponding clock in MHz
 *
 * @param input	Lower Band value/RCU mode
 *
 * @return 160: 160MHz clock, 200: 200MHz clock, -1: failure
 */
int16_t _lofar_udp_metadata_get_clockmode(const int16_t input) {
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

/**
 * @brief Convert an input into an RCU mode (from band or rcu mode)
 *
 * @param input	Input lower band/RCU mode
 *
 * @return 2 - 7: success, <0: failure
 */
int8_t _lofar_udp_metadata_get_rcumode(const int16_t input) {
	if (input <= 2) {
		fprintf(stderr, "ERROR: Invalid input RCU mode %d, exiting.\n", input);
		return -1;
	}

	// Less than 8, but greater than 2? We can just return the given value
	if (input <= 7) {
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

/**
 * @brief Convert a string to a metadata enum
 *
 * @param input	Input string
 *
 * @return metadata_t enum
 */
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

/**
 * @brief Return the maximum number of beamlets per port for a given bitmode
 *
 * @param bitmode	The input bitmode
 *
 * @return > 0: success & value, <0: failure
 */
int16_t _lofar_udp_metadata_get_beamlets(const int8_t bitmode) {
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

/**
 * @brief Given a parsed beamlet/subband array, update the metadata struct
 *
 * @param metadata 		Struct to configure
 * @param subbandData 	Parsed subband data
 *
 * @return 0: success, <0: failure
 */
int32_t _lofar_udp_metadata_update_frequencies(lofar_udp_metadata *const metadata, const int32_t *subbandData) {
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

	// Convert subband data to frequency values
	// These top and bottom variables need to be expanded by half a subband, as the subband number
	//  represents the centre of the subband in our offset system
	metadata->ftop = metadata->subband_bw * subbandData[0];
	metadata->freq_raw = metadata->subband_bw * meanSubband;
	metadata->fbottom = metadata->subband_bw * subbandData[2];

	// Define the observation bandwidth as the bandwidth between the centre of the top and bottom channels,
	//  plus a subband to account for the expanded bandwidth from the centre to the edges of the band
	metadata->bw = (metadata->fbottom - metadata->ftop);
	int8_t signCorrection = (metadata->bw > 0) ? 1 : -1;
	if (metadata->upm_bandflip == (signCorrection == 1)) {
		fprintf(stderr, "ERROR: Bandwidth has been flipped unexpectedly during setup, exiting.\n");
		return -1;
	}

	metadata->bw += signCorrection * metadata->subband_bw;
	metadata->ftop -= signCorrection * 0.5 * metadata->subband_bw;
	metadata->fbottom += signCorrection * 0.5 * metadata->subband_bw;

	return 0;
}

/**
 * @brief	Update metadata information based on the set processing mode
 *
 * @param metadata	Struct to update
 *
 * @return 0: success, <0: failure
 */
int32_t _lofar_udp_metadata_processing_mode_metadata(lofar_udp_metadata *const metadata) {
	if (metadata == NULL) {
		fprintf(stderr, "ERROR %s: Passed null metadata struct, exiting.\n", __func__);
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
		case STOKES_I_TIME ... STOKES_I_DS16_TIME:
		case STOKES_Q_TIME ... STOKES_Q_DS16_TIME:
		case STOKES_U_TIME ... STOKES_U_DS16_TIME:
		case STOKES_V_TIME ... STOKES_V_DS16_TIME:
		case STOKES_IQUV_TIME ... STOKES_IQUV_DS16_TIME:
		case STOKES_IV_TIME ... STOKES_IV_DS16_TIME:
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

	// Account for frequency direction in the bandwidth parameters
	if (metadata->upm_bandflip && metadata->fbottom > metadata->ftop) {
		double tmp = metadata->ftop;
		metadata->ftop = metadata->fbottom;
		metadata->fbottom = tmp;
		metadata->bw *= -1;
	}

	if (metadata->upm_bandflip && metadata->subband_bw > 0) {
		metadata->subband_bw *= -1;
	}

	if (metadata->upm_bandflip && metadata->bw > 0) {
		metadata->bw *= -1;
	}

	double samplingTime = metadata->upm_rcuclock == 200 ? clock200MHzSampleTime : clock160MHzSampleTime;
	if (metadata->upm_procmode > 99) {
		samplingTime *= (double) (1 << (metadata->upm_procmode % 10));
	}
	metadata->tsamp_raw = samplingTime;


	for (int8_t i = 0; i < MAX_OUTPUT_DIMS; i++) {
		if (strncpy(metadata->upm_outputfmt[i], "", META_STR_LEN) != metadata->upm_outputfmt[i]) {
			fprintf(stderr, "ERROR: Failed to reset upm_outputfmt_comment field %d, exiting.\n", i);
			return -1;
		}
	}

	// TODO (General, but esp. nice here): Find a way to do mode concatenations in future? E.g rev is 1 bit, split_pol is 1 bit, etc.
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

		case STOKES_I_TIME ... STOKES_I_DS16_TIME:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "I-TME-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes I, time-major with %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_Q_TIME ... STOKES_Q_DS16_TIME:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "Q-TME-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes Q, time-major with %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_U_TIME ... STOKES_U_DS16_TIME:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "U-TME-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes U, time-major with %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_V_TIME ... STOKES_V_DS16_TIME:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "V-TME-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes V, time-major with %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_IQUV_TIME ... STOKES_IQUV_DS16_TIME:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "I-TME-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt[1], META_STR_LEN, "Q-TME-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt[2], META_STR_LEN, "U-TME-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt[3], META_STR_LEN, "V-TME-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes IQUV, time-major with %dx downsampling", 1 << (metadata->upm_procmode % 10));
			break;
		case STOKES_IV_TIME ... STOKES_IV_DS16_TIME:
			snprintf(metadata->upm_outputfmt[0], META_STR_LEN, "I-TME-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt[1], META_STR_LEN, "V-TME-%dx", 1 << metadata->upm_procmode % 10);
			snprintf(metadata->upm_outputfmt_comment, META_STR_LEN, "Stokes IV, time-major with %dx downsampling", 1 << (metadata->upm_procmode % 10));
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
		case STOKES_I_TIME ... STOKES_I_DS16_TIME:
		case STOKES_Q_TIME ... STOKES_Q_DS16_TIME:
		case STOKES_U_TIME ... STOKES_U_DS16_TIME:
		case STOKES_V_TIME ... STOKES_V_DS16_TIME:
		case STOKES_IQUV_TIME ... STOKES_IQUV_DS16_TIME:
		case STOKES_IV_TIME ... STOKES_IV_DS16_TIME:
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
		case STOKES_I_TIME ... STOKES_I_DS16_TIME:
		case STOKES_Q_TIME ... STOKES_Q_DS16_TIME:
		case STOKES_U_TIME ... STOKES_U_DS16_TIME:
		case STOKES_V_TIME ... STOKES_V_DS16_TIME:
			metadata->upm_rel_outputs[0] = 1;
			for (int8_t i = 1; i < MAX_OUTPUT_DIMS; i++) {
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
		case STOKES_IQUV_TIME ... STOKES_IQUV_DS16_TIME:
			for (int8_t i = 0; i < MAX_OUTPUT_DIMS; i++) {
				metadata->upm_rel_outputs[i] = 1;
			}
			break;

		// Split by antenna polarisation, but not real/complex
		case TIME_MAJOR_ANT_POL:
		case TIME_MAJOR_ANT_POL_FLOAT:
			for (int8_t i = 0; i < MAX_OUTPUT_DIMS; i++) {
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
		case STOKES_IV_TIME ... STOKES_IV_DS16_TIME:
			for (int8_t i = 0; i < MAX_OUTPUT_DIMS; i++) {
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
		case STOKES_I_TIME ... STOKES_I_DS16_TIME:
		case STOKES_Q_TIME ... STOKES_Q_DS16_TIME:
		case STOKES_U_TIME ... STOKES_U_DS16_TIME:
		case STOKES_V_TIME ... STOKES_V_DS16_TIME:
		case STOKES_IQUV_TIME ... STOKES_IQUV_DS16_TIME:
		case STOKES_IV_TIME ... STOKES_IV_DS16_TIME:
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
		case STOKES_I_TIME ... STOKES_I_DS16_TIME:
		case STOKES_Q_TIME ... STOKES_Q_DS16_TIME:
		case STOKES_U_TIME ... STOKES_U_DS16_TIME:
		case STOKES_V_TIME ... STOKES_V_DS16_TIME:
		case STOKES_IQUV_TIME ... STOKES_IQUV_DS16_TIME:
		case STOKES_IV_TIME ... STOKES_IV_DS16_TIME:
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

/**
 * @brief Apply the external channelisation and downsampling factors to the metadata struct
 *
 * @param metadata	Struct to update
 * @param[in] config	Config to parse
 *
 * @return
 */
int32_t _lofar_udp_metadata_handle_external_factors(lofar_udp_metadata *const metadata, const metadata_config *config) {
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

	// Reset variables for the re-use case
	metadata->tsamp = metadata->tsamp_raw;
	metadata->channel_bw = metadata->subband_bw;
	metadata->nchan = metadata->nsubband;

	if (metadata->external_channelisation > 1) {
		// Apply relevant scaling
		metadata->tsamp *= metadata->external_channelisation;
		metadata->channel_bw /= metadata->external_channelisation;
		metadata->nchan *= metadata->external_channelisation;

		double signCorrection = metadata->bw > 0 ? 1.0 : -1.0;

		// This is the correct way to account for channelisation with FFT on a PFB output
		// It's messy, but accurate
		// ftop/fbottom are absolute top/bottom, not fch1 top/bottom
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

	// Apply additional scaling as required
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
int32_t _lofar_udp_metadata_get_station_name(const int stationID, char *const stationCode) {
	if (stationCode == NULL || stationID < 0) {
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

/**
 * @brief Return true on empty string
 * @param string
 * @return 1: empty string, 0: non-empty string
 */
int32_t _isEmpty(const char *string) {
	if (string == NULL) {
		return 1;
	}
	return string[0] == '\0';
}

/**
 * @brief Return 1 on unset int (-1)
 * @param input
 * @return 1: int is -1, 0: otherwise
 */
int32_t _intNotSet(int32_t input) {
	return input == -1;
}

/**
 * @brief Return 1 on unset long (-1)
 * @param input
 * @return 1: long is -1, 0: otherwise
 */
int32_t _longNotSet(int64_t input) {
	return input == -1;
}

/**
 * @brief Return 1 on unset float (-1.0 or 0.0)
 * @param input
 * @param exception Swap check to 0.0
 * @return 1: float is unset, 0: otherwise
 */
int32_t _floatNotSet(float input, int8_t exception) {
	// Exception: it might be sane for some values to be -1.0f, so add a non-zero check instead
	if (exception) {
		return input == 0.0f;
	}

	return input == -1.0f;
}

/**
 * @brief Return 1 on unset double (-1.0 or 0.0)
 * @param input
 * @param exception Swap check to 0.0
 * @return 1: double is unset, 0: otherwise
 */
int32_t _doubleNotSet(double input, int8_t exception) {
	// Exception: it might be sane for some values to be -1.0, so add a non-zero check instead
	if (exception) {
		return input == 0.0;
	}

	return input == -1.0;
}

#include "./metadata/lofar_udp_metadata_GUPPI.c" // NOLINT(bugprone-suspicious-include)
#include "./metadata/lofar_udp_metadata_DADA.c" // NOLINT(bugprone-suspicious-include)
#include "./metadata/lofar_udp_metadata_SIGPROC.c" // NOLINT(bugprone-suspicious-include)

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
