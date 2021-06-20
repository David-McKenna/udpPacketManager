
// Read interface

/**
 * @brief      { function_description }
 *
 * @param      input   The input
 * @param[in]  config  The configuration
 * @param[in]  port    The port
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_read_setup_HDF5(lofar_udp_io_read_config *input, const char *inputLocation, const int port) {

	return -1;
}


/**
 * @brief      { function_description }
 *
 * @param      input        The input
 * @param[in]  port         The port
 * @param      targetArray  The target array
 * @param[in]  nchars       The nchars
 *
 * @return     { description_of_the_return_value }
 */
long lofar_udp_io_read_HDF5(lofar_udp_io_read_config *input, const int port, char *targetArray, const long nchars) {

	return -1;
}

/**
 * @brief      { function_description }
 *
 * @param      input  The input
 * @param[in]  port   The port
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_read_cleanup_HDF5(lofar_udp_io_read_config *input, const int port) {

	return -1;
}



/**
 * @brief      { function_description }
 *
 * @param      outbuf     The outbuf
 * @param[in]  size       The size
 * @param[in]  num        The number
 * @param[in]  inputHDF5  The input HDF5
 * @param[in]  resetSeek  The reset seek
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_read_temp_HDF5(void *outbuf, const size_t size, const int num, const char inputHDF5[],
                                const int resetSeek) {
	long readlen = -1;
	return readlen;
}


/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 * @param[in]  iter    The iterator
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_setup_HDF5(lofar_udp_io_write_config *config, int outp, int iter) {

	// https://support.hdfgroup.org/ftp/HDF5/examples/examples-by-api/hdf5-examples/1_10/C/H5D/h5ex_d_unlimgzip.c
	// https://bitbucket.hdfgroup.org/projects/HDFFV/repos/hdf5-examples/browse/1_10/C/H5D/h5ex_d_extern.c
	// https://support.hdfgroup.org/ftp/HDF5/examples/examples-by-api/hdf5-examples/1_6/C/H5G/h5ex_g_create.c
	// 
	char h5Name[DEF_STR_LEN];
	if (lofar_udp_io_parse_format(h5Name, config->outputFormat, -1, iter, 0, config->firstPacket) < 0) {
		return -1;
	}


	hid_t       file, space, dset, dcpl, group;
	herr_t      status;
	hsize_t     dims[2] = { 0, 0 };

    htri_t          avail;
    H5Z_filter_t    filter_type;
    hsize_t         extdims[2] = { 488, 512 },
                    maxdims[2] = { H5S_UNLIMITED, H5S_UNLIMITED },
                    chunk[2] = { 16, 16 },
                    start[2],
                    count[2];
    size_t          nelmts;
    unsigned int    flags,
                    filter_info;


	

	if (!config->hdf5Writer.initialised) {
		if ((config->hdf5Writer.file = H5Fcreate(h5Name, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
			H5Eprint(config->hdf5Writer.file, stderr);
			fprintf(stderr, "ERROR: Failed to create base HDF5 file, exiting.\n");
			return -1;
		}

		char *groupNames[] = {  "/PROCESS_HISTORY",
								"/SUB_ARRAY_POINTING_0",
						        "/SUB_ARRAY_POINTING_0/PROCESS_HISTORY",
						        "/SUB_ARRAY_POINTING_0/BEAM_0",
						        "/SUB_ARRAY_POINTING_0/BEAM_0/PROCESS_HISTORY",
						        "/SUB_ARRAY_POINTING_0/BEAM_0/COORDINATES",
						        "/SUB_ARRAY_POINTING_0/BEAM_0/COORDINATES/COORDINATE_0",
						        "/SUB_ARRAY_POINTING_0/BEAM_0/COORDINATES/COORDINATE_1",
						        "/SYS_LOG"
		};
		int numGroups = sizeof(groupNames) / sizeof(groupNames[0]);

		// Create the default group/dataset structures
		for (int groupIdx = 0; groupIdx < numGroups; groupIdx++) {
			VERBOSE(printf("Creating group  %d/%d %s\n", groupIdx, numGroups, groupNames[groupIdx]));
			if ((group = H5Gcreate(config->hdf5Writer.file, groupNames[groupIdx], H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
				H5Eprint(group, stderr);
				fprintf(stderr, "ERROR: Failed to create group '%s', exiting.\n", groupNames[groupIdx]);
				return -1;
			}

			if ((status = H5Gclose(group)) < 0) {
				H5Eprint(status, stderr);
				fprintf(stderr, "ERROR: Failed to close group '%s', exiting.\n", groupNames[groupIdx]);
				return -1;
			}
		}

		config->hdf5Writer.initialised = 1;
	}
	VERBOSE(printf("HDF5 base groups created.\n");)


	char dsetFile[DEF_STR_LEN + 16];
	if (snprintf(dsetFile, DEF_STR_LEN + 15, "%s.data_%d", h5Name, outp) < 0) {
		fprintf(stderr, "ERROR: Failed to write HDF5 dataset file name to buffer, exiting.\n");
		return -1;
	}

	char dsetName[64];
	if (snprintf(dsetName, 63, "/SUB_ARRAY_POINTING_0/BEAM_0/STOKES_%d", outp) < 0) {
		fprintf(stderr, "ERROR: Failed to write HDF5 dataset name to buffer, exiting.\n");
		return -1;
	}


    space = H5Screate_simple(2, dims, maxdims);


	config->hdf5DSetWriter[outp].dcpl = H5Pcreate(H5P_DATASET_CREATE);
	//status = H5Pset_external(config->hdf5DSetWriter[outp].dcpl, dsetFile, 0, H5F_UNLIMITED);
	status = H5Pset_chunk(config->hdf5DSetWriter[outp].dcpl, 2, chunk);

	if (strncpy(config->outputLocations[outp], dsetFile, DEF_STR_LEN) != config->outputLocations[outp]) {
		fprintf(stderr, "ERROR: Failed to copy dataset file name to buffer, exiting.\n");
		return -1;
	}

	config->hdf5DSetWriter[outp].dset = H5Dcreate(config->hdf5Writer.file, dsetName, H5T_IEEE_F32LE, space, H5P_DEFAULT, config->hdf5DSetWriter[outp].dcpl, H5P_DEFAULT);

	status = H5Sclose(space);
	VERBOSE(printf("Exiting HDF5 file creation.\n"));

	return 0;
}


int hdf5SetupStrAttrs(hid_t group, char *stringEntries[], size_t numEntries) {

	VERBOSE(printf("Str attrs\n"));
	// Needs error checking
	hid_t filetype = H5Tcopy (H5T_FORTRAN_S1);
	herr_t status = H5Tset_size (filetype, H5T_VARIABLE);
	hid_t memtype = H5Tcopy (H5T_C_S1);
	status = H5Tset_size (memtype, H5T_VARIABLE);
	hsize_t dims[1] = { 0 };
	hid_t space = H5Screate_simple(0, dims, NULL);

	hid_t attr;

	for (size_t atIdx = 0; atIdx < numEntries; atIdx++) {
		printf("%s: %s\n", stringEntries[atIdx * 2], stringEntries[atIdx * 2 + 1]);
		int strlenv = strlen(stringEntries[atIdx * 2 + 1]) + 1;
		printf("%d\n", strlenv);
		filetype = H5Tcopy (H5T_FORTRAN_S1);
		status = H5Tset_size(filetype, strlenv - 1);
		memtype = H5Tcopy (H5T_C_S1);
		status = H5Tset_size(memtype, strlenv);

		if ((attr = H5Acreate(group, stringEntries[atIdx * 2], filetype, space, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
			H5Eprint(attr, stderr);
			fprintf(stderr, "ERROR %s: Failed to create str attr %s: %s in root group, exiting.\n", __func__, stringEntries[atIdx * 2], stringEntries[atIdx * 2 + 1]);
			return -1;
		}
		printf("Write\n");
		if ((status = H5Awrite(attr, memtype, stringEntries[atIdx * 2 + 1])) < 0) {
			H5Eprint(status, stderr);
			fprintf(stderr, "ERROR %s: Failed to set str attr %s: %s in root group, exiting.\n", __func__, stringEntries[atIdx * 2], stringEntries[atIdx * 2 + 1]);
			return -1;
		}
		printf("Close\n");
		H5Aclose(attr);

		H5Tclose(filetype);
		H5Tclose(memtype);
	}
	H5Sclose(space);

	VERBOSE(printf("Str attrs finished\n"));

	return 0;
}

int hdf5SetupLongAttrs(hid_t group, char **stringEntries, long *longValues, size_t numEntries) {
	VERBOSE(printf("long attrs\n"));
	hid_t attr;
	herr_t status;

	hsize_t dims[1] = { 1 };
	hid_t space = H5Screate_simple(0, dims, NULL);

	for (size_t atIdx = 0; atIdx < numEntries; atIdx++) {
		printf("%s: %ld\n", stringEntries[atIdx], longValues[atIdx]);
		if ((attr = H5Acreate(group, stringEntries[atIdx], H5T_STD_I64BE, space, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
			H5Eprint(status, stderr);
			fprintf(stderr, "ERROR %s: Failed to create str attr %s: %ld in root group, exiting.\n", __func__, stringEntries[atIdx], longValues[atIdx]);
			return -1;
		}

		if ((status = H5Awrite(attr, H5T_NATIVE_LONG, &(longValues[atIdx]))) < 0) {
			H5Eprint(attr, stderr);
			fprintf(stderr, "ERROR %s: Failed to set str attr %s: %ld in root group, exiting.\n", __func__, stringEntries[atIdx], longValues[atIdx]);
			return -1;
		}
		H5Aclose(attr);
	}

	status = H5Sclose(space);
	return 0;
}

int hdf5SetupDoubleAttrs(hid_t group, char *stringEntries[], double doubleValues[], size_t numEntries) {
	VERBOSE(printf("Double attrs\n"));
	hid_t attr;
	herr_t status;

	hsize_t dims[1] = { 1 };
	hid_t space = H5Screate_simple(0, dims, NULL);

	for (size_t atIdx = 0; atIdx < numEntries; atIdx++) {
		printf("%s: %lf\n", stringEntries[atIdx], doubleValues[atIdx]);
		if ((attr = H5Acreate(group, stringEntries[atIdx], H5T_NATIVE_DOUBLE, space, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
			H5Eprint(status, stderr);
			fprintf(stderr, "ERROR %s: Failed to create str attr %s: %lf in root group, exiting.\n", __func__, stringEntries[atIdx], doubleValues[atIdx]);
			return -1;
		}

		if ((status = H5Awrite(attr, H5T_NATIVE_DOUBLE, &(doubleValues[atIdx]))) < 0) {
			H5Eprint(attr, stderr);
			fprintf(stderr, "ERROR %s: Failed to set str attr %s: %lf in root group, exiting.\n", __func__, stringEntries[atIdx], doubleValues[atIdx]);
			return -1;
		}
		status = H5Aclose(attr);
	}

	status = H5Sclose(space);
	return 0;
}

long lofar_udp_io_write_metadata_HDF5(lofar_udp_io_write_config *config, lofar_udp_metadata *metadata, char *headerBuffer, size_t headerLength) {
	hid_t group;
	hid_t filetype, memtype;
	hsize_t dims[1] = { 1 };
	hsize_t space = H5Screate_simple (1, dims, NULL);

	herr_t status;
	size_t numAttrs;

	if (!config->hdf5Writer.metadataInitialised) {


		// Root group
		// Missing entries: 
		/*
			OBSERVATION_END_UTC = "",
			FILTER_SELECTION = "", // LBA_10_70, LBA_30_70, LBA_10_90, LBA_30_90, HBA_110_190, HBA_170_230, HBA_210_250
			BF_VERSION = "",
			OBSERVATION_DATATYPE = "", // "Searching, timing, etc"
			// SUB_ARRAY_POINTING_DIAMETER_UNIT = "arcmin";

			// BEAM_DIAMETER = "arcmin";
			OBSERVATION_STATION_LIST = { "" };
			TARGETS = { "" };
			OBSERVATION_END_MJD = dbl;
			TOTAL_INTEGRATION_TIME = dbl;
			// SUB_ARRAY_POINTING_DIAMETER = dbl; // FWHM of SAP at centre / fcentre
			// BEAM_DIAMETER = dbl; // FWHM of beams at zenith at the centre frequency
		*/
		{

			if ((group = H5Gopen(config->hdf5Writer.file, "/", 0)) < 0) {
				H5Eprint(group, stderr);
				fprintf(stderr, "ERROR %s: Failed to open h5 root group, exiting.\n", __func__);
				return -1;
			}

			char *stringEntries[] = { "GROUPTYPE", "Root",
			                          "FILENAME", config->outputFormat,
			                          "FILEDATE", metadata->upm_daq,
			                          "FILETYPE", "bf",
			                          "TELESCOPE", "LOFAR",
			                          "PROJECT_ID", metadata->obs_id,
			                          "PROJECT_TITLE", metadata->obs_id,
			                          "PROJECT_PI", metadata->observer,
			                          "PROJECT_CO_I", "UNKNOWN",
			                          "PROJECT_CONTACT", metadata->observer,
			                          "OBSERVER", metadata->observer,
			                          "OBSERVATION_ID", metadata->obs_id,
			                          "OBSERVATION_START_UTC", metadata->obs_utc_start,
			                          "OBSERVATION_FREQUENCY_UNIT", "MHz",
			                          "CLOCK_FREQUENCY_UNIT", "MHz",
			                          "ANTENNA_SET", metadata->freq > 100 ? "HBA_JOINED" : "LBA_OUTER",
			                          "SYSTEM_VERSION", UPM_VERSION,
			                          "PIPELINE_NAME", "udpPacketManager",
			                          "PIPELINE_VERSION", UPM_VERSION,
			                          "DOC_NAME", "ICD003",
			                          "DOC_VERSION", "2.6",
			                          "NOTES", "INIT",
			                          "CREATE_ONLINE_OFFLINE", metadata->upm_reader == DADA_ACTIVE ? "ONLINE" : "OFFLINE",
			                          "BF_FORMAT", "RAW",
			                          "TOTAL_INTEGRATION_TIME_UNIT", "s",
			                          "BANDWIDTH_UNIT", "MHz",
			};

			numAttrs = sizeof(stringEntries) / sizeof(stringEntries[0]);

			// With key/val in the same array it should always be a multiple of 2 long
			if (numAttrs % 2) {
				fprintf(stderr, "ERROR: Odd number of values provided to stringEntries for the ROOT group, exiting.\n");
				H5Gclose(group);
				return -1;
			}

			numAttrs /= 2;

			if (hdf5SetupStrAttrs(group, stringEntries, numAttrs) < 0) {
				return -1;
			}


			// ROOT group attributes
			char *doubleEntriesKeys[] = {
				"OBSERVATION_START_MJD",
				"OBSERVATION_FREQUENCY_MIN",
				"OBSERVATION_FREQUENCY_CENTER",
				"OBSERVATION_FREQUENCY_MAX",
				"CLOCK_FREQUENCY",
				"BANDWIDTH"
			};
			double doubleEntriesValues[] = {
				metadata->obs_mjd_start,
				metadata->fbottom,
				metadata->freq,
				metadata->ftop,
				metadata->upm_rcuclock,
				metadata->channel_bw * metadata->nchan // Integrated bandwidth, not ftop - fbot
			};

			numAttrs = sizeof(doubleEntriesValues) / sizeof(doubleEntriesValues[0]);
			if (numAttrs != (sizeof(doubleEntriesKeys) / sizeof(doubleEntriesKeys[0]))) {
				fprintf(stderr, "ERROR: Number of double attribute names does not match number of double attribute values, exiting.\n");
				return -1;
			}

			if (hdf5SetupDoubleAttrs(group, doubleEntriesKeys, doubleEntriesValues, numAttrs) < 0) {
				return -1;
			}




			// TODO: OBSERVATION_NOF_STATIONS is meant to be unsigned, not signed.
			char *longEntriesKeys[] = {
				"OBSERVATION_NOF_STATIONS",
				"OBSERVATION_NOF_BITS_PER_SAMPLE",
				"OBSERVATION_NOF_SUB_ARRAY_POINTINGS",
				"NOF_SUB_ARRAY_POINTINGS"
			};
			long longEntriesValues[] = {
				1,
				metadata->upm_input_bitmode,
				1,
				1
			};

			numAttrs = sizeof(longEntriesValues) / sizeof(longEntriesValues[0]);
			if (numAttrs != (sizeof(longEntriesKeys) / sizeof(longEntriesKeys[0]))) {
				fprintf(stderr, "ERROR: Number of int attribute names does not match number of double attribute values, exiting.\n");
				return -1;
			}

			if (hdf5SetupLongAttrs(group, longEntriesKeys, longEntriesValues, numAttrs) < 0) {
				return -1;
			}

			status = H5Gclose(group);
		}

		// SUB_ARRAY_POINTING_0
		// Missing:
		// "EXPTIME_END_UTC", "",
		//EXPTIME_END_MJD = dbl;
		//TOTAL_INTEGRATION_TIME = dbl;
		// POINT_ALTITUDE = dbl; // Deg
		// POINT_AZIMUTH = dbl; // Deg

		{
			if ((group = H5Gopen(config->hdf5Writer.file, "/SUB_ARRAY_POINTING_0", 0)) < 0) {
				H5Eprint(group, stderr);
				fprintf(stderr, "ERROR %s: Failed to open h5 /SUB_ARRAY_POINTING_0 group, exiting.\n", __func__);
				return -1;
			}

			char *stringEntries[] = {
				"GROUPTYPE", "SubArrayPointing",
				"EXPTIME_START_UTC", metadata->obs_utc_start,
				"TOTAL_INTEGRATION_TIME_UNIT", "s",
				"POINT_RA_UNIT", "deg",
				"POINT_DEC_UNIT", "deg",
				"POINT_ALTITUDE", "deg",
				"POINT_AZIMUTH", "deg"
			};

			numAttrs = sizeof(stringEntries) / sizeof(stringEntries[0]);

			// With key/val in the same array it should always be a multiple of 2 long
			if (numAttrs % 2) {
				fprintf(stderr, "ERROR: Odd number of values provided to stringEntries for the SUB_ARRAY_POINTING_0 group, exiting.\n");
				H5Gclose(group);
				return -1;
			}

			numAttrs /= 2;

			if (hdf5SetupStrAttrs(group, stringEntries, numAttrs) < 0) {
				return -1;
			}



			char *doubleEntriesKeys[] = {
				"EXPTIME_START_MJD",
				"POINT_RA",
				"POINT_DEC"
			};

			double doubleEntriesValues[] = {
				metadata->obs_mjd_start,
				metadata->ra_rad * 57.2958, // Convert radians to degrees
				metadata->dec_rad * 57.2958  // Convert radians to degrees
			};

			numAttrs = sizeof(doubleEntriesValues) / sizeof(doubleEntriesValues[0]);
			if (numAttrs != (sizeof(doubleEntriesKeys) / sizeof(doubleEntriesKeys[0]))) {
				fprintf(stderr, "ERROR: Number of double attribute names does not match number of double attribute values, exiting.\n");
				return -1;
			}

			if (hdf5SetupDoubleAttrs(group, doubleEntriesKeys, doubleEntriesValues, numAttrs) < 0) {
				return -1;
			}


			char *longEntriesKeys[] = {
				"OBSERVATION_NOF_BEAMS",
				"NOF_BEAMS"
			};
			long longEntriesValues[] = {
				1,
				1
			};

			numAttrs = sizeof(longEntriesValues) / sizeof(longEntriesValues[0]);
			if (numAttrs != (sizeof(longEntriesKeys) / sizeof(longEntriesKeys[0]))) {
				fprintf(stderr, "ERROR: Number of int attribute names does not match number of double attribute values, exiting.\n");
				return -1;
			}

			if (hdf5SetupLongAttrs(group, longEntriesKeys, longEntriesValues, numAttrs) < 0) {
				return -1;
			}


			status = H5Gclose(group);
		}


		// PROCESS_HISTORY
		{
			if ((group = H5Gopen(config->hdf5Writer.file, "/PROCESS_HISTORY", 0)) < 0) {
				H5Eprint(group, stderr);
				fprintf(stderr, "ERROR %s: Failed to open h5 /PROCESS_HISTORY group, exiting.\n", __func__);
				return -1;
			}

			status = H5Gclose(group);
		}


		// SUB_ARRAY_POINTING_0/BEAM_0
		// Missing:
		//	TARGETS = { "" };
		//	STATIONS_LIST = { "" };
		//	STOKES_COMPONENTS { "" };
		//	"BEAM_DIAMETER_RA" = dbl,
		//	"BEAM_DIAMETER_DEC" = dbl,
		//	NOF_SAMPLES = int;
		{
			if ((group = H5Gopen(config->hdf5Writer.file, "/SUB_ARRAY_POINTING_0/BEAM_0", 0)) < 0) {
				H5Eprint(group, stderr);
				fprintf(stderr, "ERROR %s: Failed to open h5 /SUB_ARRAY_POINTING_0/BEAM_0 group, exiting.\n", __func__);
				return -1;
			}

			char *stringEntries[] = {
				"GROUPTYPE", "Beam",
				"SAMPLING_RATE_UNIT", "Hz",
				"SAMPLING_TIME_UNIT", "s",
				"SUBBAND_WIDTH_UNIT", "Hz",
				"TRACKING", metadata->coord_basis,
				"POINT_RA_UNIT", "deg",
				"POINT_DEC_UNIT", "deg",
				"POINT_OFFSET_RA_UNIT", "deg",
				"POINT_OFFSET_DEC_UNIT", "deg",
				"BEAM_DIAMETER_RA_UNIT", "arcmin",
				"BEAM_DIAMETER_DEC_UNIT", "arcmin",
				"BEAM_FREQUENCY_CENTER_UNIT", "MHz",
				"FOLD_PERIOD_UNIT", "s",
				"DISPERSION_MEASURE_UNIT", "pc/cm^3",
				"SIGNAL_SUM", "INCOHERENT"
			};

			numAttrs = sizeof(stringEntries) / sizeof(stringEntries[0]);

			// With key/val in the same array it should always be a multiple of 2 long
			if (numAttrs % 2) {
				fprintf(stderr, "ERROR: Odd number of values provided to stringEntries for the SUB_ARRAY_POINTING_0/BEAM_0 group, exiting.\n");
				H5Gclose(group);
				return -1;
			}

			numAttrs /= 2;

			if (hdf5SetupStrAttrs(group, stringEntries, numAttrs) < 0) {
				return -1;
			}



			char *doubleEntriesKeys[] = {
				"SAMPLING_RATE",
				"SAMPLING_TIME",
				"SUBBAND_WIDTH",
				"POINT_RA",
				"POINT_DEC",
				"POINT_OFFSET_RA",
				"POINT_OFFSET_DEC",
				"BEAM_FREQUENCY_CENTER",
				"FOLD_PERIOD",
				"DEDISPERSION",
				"DISPERSION_MEASURE"
			};

			double doubleEntriesValues[] = {
				1. / metadata->tsamp,
				metadata->tsamp,
				metadata->channel_bw * 1e6,
				metadata->ra_rad * 57.2958, // Convert radians to degrees
				metadata->dec_rad * 57.2958,  // Convert radians to degrees
				0.,
				0.,
				metadata->freq,
				0.,
				0.,
				0.
			};

			numAttrs = sizeof(doubleEntriesValues) / sizeof(doubleEntriesValues[0]);
			if (numAttrs != (sizeof(doubleEntriesKeys) / sizeof(doubleEntriesKeys[0]))) {
				fprintf(stderr, "ERROR: Number of double attribute names does not match number of double attribute values, exiting.\n");
				return -1;
			}

			if (hdf5SetupDoubleAttrs(group, doubleEntriesKeys, doubleEntriesValues, numAttrs) < 0) {
				return -1;
			}

			char *longEntriesKeys[] = {
				"NOF_STATIONS",
				"CHANNELS_PER_SUBBAND",
				"OBSERVATION_NOF_STOKES",
				"NOF_STOKES",
				"FOLDED_DATA",
				"BARYCENTERED",
				"COMPLEX_VOLTAGE"
			};

			long longEntriesValues[] = {
				1,
				1,
				metadata->upm_num_outputs,
				metadata->upm_num_outputs,
				0,
				0,
				(metadata->upm_procmode < 100)
			};

			numAttrs = sizeof(longEntriesValues) / sizeof(longEntriesValues[0]);
			if (numAttrs != (sizeof(longEntriesKeys) / sizeof(longEntriesKeys[0]))) {
				fprintf(stderr, "ERROR: Number of int attribute names does not match number of double attribute values, exiting.\n");
				return -1;
			}

			if (hdf5SetupLongAttrs(group, longEntriesKeys, longEntriesValues, numAttrs) < 0) {
				return -1;
			}


			status = H5Gclose(group);

		}



		// SUB_ARRAY_POINTING_0/BEAM_0/STOKES[0..3]
		// Missing:
		{
			/*
			GROUP STOKES_[0...3]
				GROUPTYPE = "bfData";
				DATATYPE = "float"; // variable
				STOKES_COMPONENT = "I";

				NOF_SAMPLES = int;
				NOF_SUBBANDS = int;
               
				NOF_CHANNELS = { 1, 1, 1...}; // Confirm, not 100% sure 
			*/
		}

		// SUB_ARRAY_POINTING_0/BEAM_0/COORDINATES
		// Missing:
		// COORDINATES_TYPES = { "Time", "Spectral" };
		// REF_LOCATION_UNIT = { "m", "m", "m" };
		// REF_LOCATION_VALUE = { dbl };
		{
			if ((group = H5Gopen(config->hdf5Writer.file, "/SUB_ARRAY_POINTING_0/BEAM_0/COORDINATES", 0)) < 0) {
				H5Eprint(group, stderr);
				fprintf(stderr, "ERROR %s: Failed to open h5 /SUB_ARRAY_POINTING_0/BEAM_0 group, exiting.\n", __func__);
				return -1;
			}

			char *stringEntries[] = {
				"GROUPTYPE", "Coordinates",
				"REF_LOCATION_FRAME",
				"ITRF", // GEOCENTER, BARYCENTER, HELIOCENTER, TOPOCENTER, LSRK, LSRD, GALACTIC, LOCAL_GROUP, RELOCATABLE... Someone is optimistic as to where LOFAR stations will end up
				"REF_TIME_UNIT", "d",
				"REF_TIME_FRAME", "MJD"
			};

			numAttrs = sizeof(stringEntries) / sizeof(stringEntries[0]);

			// With key/val in the same array it should always be a multiple of 2 long
			if (numAttrs % 2) {
				fprintf(stderr, "ERROR: Odd number of values provided to stringEntries for the SUB_ARRAY_POINTING_0/BEAM_0 group, exiting.\n");
				H5Gclose(group);
				return -1;
			}

			numAttrs /= 2;

			if (hdf5SetupStrAttrs(group, stringEntries, numAttrs) < 0) {
				return -1;
			}



			char *doubleEntriesKeys[] = {
				"REF_TIME_VALUE"
			};

			double doubleEntriesValues[] = {
				metadata->obs_mjd_start
			};

			numAttrs = sizeof(doubleEntriesValues) / sizeof(doubleEntriesValues[0]);
			if (numAttrs != (sizeof(doubleEntriesKeys) / sizeof(doubleEntriesKeys[0]))) {
				fprintf(stderr, "ERROR: Number of double attribute names does not match number of double attribute values, exiting.\n");
				return -1;
			}

			if (hdf5SetupDoubleAttrs(group, doubleEntriesKeys, doubleEntriesValues, numAttrs) < 0) {
				return -1;
			}

			char *longEntriesKeys[] = {
				"NOF_AXIS",
				"NOF_COORDINATES",
			};

			long longEntriesValues[] = {
				2,
				2
			};

			numAttrs = sizeof(longEntriesValues) / sizeof(longEntriesValues[0]);
			if (numAttrs != (sizeof(longEntriesKeys) / sizeof(longEntriesKeys[0]))) {
				fprintf(stderr, "ERROR: Number of int attribute names does not match number of double attribute values, exiting.\n");
				return -1;
			}

			if (hdf5SetupLongAttrs(group, longEntriesKeys, longEntriesValues, numAttrs) < 0) {
				return -1;
			}


			status = H5Gclose(group);
		}

		// SUB_ARRAY_POINTING_0/BEAM_0/COORDINATES/COORDINATE_0
		// Missing:
		// STORAGE_TYPE = { "Linear" };
		//	AXIS_NAMES = { "Time" };
		//	AXIS_UNITS = { "s" };
		//	PC = { 1. };
		// AXIS_VALUES_PIXEL = { 0 };
		// AXIS_VALUES_WORLD = { 0. };
		{
			if ((group = H5Gopen(config->hdf5Writer.file, "/SUB_ARRAY_POINTING_0/BEAM_0/COORDINATES/COORDINATE_0", 0)) <
			    0) {
				H5Eprint(group, stderr);
				fprintf(stderr, "ERROR %s: Failed to open h5 /SUB_ARRAY_POINTING_0/BEAM_0 group, exiting.\n", __func__);
				return -1;
			}

			char *stringEntries[] = {
				"GROUPTYPE", "TimeCoord",
				"COORDINATE_TYPE", "Time"
			};

			numAttrs = sizeof(stringEntries) / sizeof(stringEntries[0]);

			// With key/val in the same array it should always be a multiple of 2 long
			if (numAttrs % 2) {
				fprintf(stderr, "ERROR: Odd number of values provided to stringEntries for the SUB_ARRAY_POINTING_0/BEAM_0 group, exiting.\n");
				H5Gclose(group);
				return -1;
			}

			numAttrs /= 2;

			if (hdf5SetupStrAttrs(group, stringEntries, numAttrs) < 0) {
				return -1;
			}



			char *doubleEntriesKeys[] = {
				"REFERENCE_VALUE",
				"REFERENCE_PIXEL",
				"INCREMENT"
			};

			double doubleEntriesValues[] = {
				0.,
				0.,
				metadata->tsamp
			};

			numAttrs = sizeof(doubleEntriesValues) / sizeof(doubleEntriesValues[0]);
			if (numAttrs != (sizeof(doubleEntriesKeys) / sizeof(doubleEntriesKeys[0]))) {
				fprintf(stderr, "ERROR: Number of double attribute names does not match number of double attribute values, exiting.\n");
				return -1;
			}

			if (hdf5SetupDoubleAttrs(group, doubleEntriesKeys, doubleEntriesValues, numAttrs) < 0) {
				return -1;
			}

			char *longEntriesKeys[] = {
				"NOF_AXIS",
			};

			long longEntriesValues[] = {
				1
			};

			numAttrs = sizeof(longEntriesValues) / sizeof(longEntriesValues[0]);
			if (numAttrs != (sizeof(longEntriesKeys) / sizeof(longEntriesKeys[0]))) {
				fprintf(stderr, "ERROR: Number of int attribute names does not match number of double attribute values, exiting.\n");
				return -1;
			}

			if (hdf5SetupLongAttrs(group, longEntriesKeys, longEntriesValues, numAttrs) < 0) {
				return -1;
			}


			status = H5Gclose(group);

		}

		// SUB_ARRAY_POINTING_0/BEAM_0/COORDINATES/COORDINATE_1
		// Missing:
		// 	STORAGE_TYPE = { "Tabular" };
		// AXIS_NAMES = { "Frequency" };
		//	AXIS_UNITS = { "Hz" };
		//	PC = { 1. };

		// AXIS_VALUES_PIXEL = { 0, 1, 2, 3... beamlet-1 }; // Frequncy channels in dataset
		// AXIS_VALUES_WORLD = { 101e6, 102e6, 103e6... }; 
		{
			if ((group = H5Gopen(config->hdf5Writer.file, "/SUB_ARRAY_POINTING_0/BEAM_0/COORDINATES/COORDINATE_1", 0)) <
			    0) {
				H5Eprint(group, stderr);
				fprintf(stderr, "ERROR %s: Failed to open h5 /SUB_ARRAY_POINTING_0/BEAM_0 group, exiting.\n", __func__);
				return -1;
			}

			char *stringEntries[] = {
				"GROUPTYPE", "SpectralCoord",
				"COORDINATE_TYPE", "Spectral"
			};

			numAttrs = sizeof(stringEntries) / sizeof(stringEntries[0]);

			// With key/val in the same array it should always be a multiple of 2 long
			if (numAttrs % 2) {
				fprintf(stderr, "ERROR: Odd number of values provided to stringEntries for the SUB_ARRAY_POINTING_0/BEAM_0 group, exiting.\n");
				H5Gclose(group);
				return -1;
			}

			numAttrs /= 2;

			if (hdf5SetupStrAttrs(group, stringEntries, numAttrs) < 0) {
				return -1;
			}



			char *doubleEntriesKeys[] = {
				"REFERENCE_VALUE",
				"REFERENCE_PIXEL",
				"INCREMENT"
			};

			double doubleEntriesValues[] = {
				0.,
				0.,
				0.
			};

			numAttrs = sizeof(doubleEntriesValues) / sizeof(doubleEntriesValues[0]);
			if (numAttrs != (sizeof(doubleEntriesKeys) / sizeof(doubleEntriesKeys[0]))) {
				fprintf(stderr, "ERROR: Number of double attribute names does not match number of double attribute values, exiting.\n");
				return -1;
			}

			if (hdf5SetupDoubleAttrs(group, doubleEntriesKeys, doubleEntriesValues, numAttrs) < 0) {
				return -1;
			}

			char *longEntriesKeys[] = {
				"NOF_AXIS",
			};

			long longEntriesValues[] = {
				1
			};

			numAttrs = sizeof(longEntriesValues) / sizeof(longEntriesValues[0]);
			if (numAttrs != (sizeof(longEntriesKeys) / sizeof(longEntriesKeys[0]))) {
				fprintf(stderr, "ERROR: Number of int attribute names does not match number of double attribute values, exiting.\n");
				return -1;
			}

			if (hdf5SetupLongAttrs(group, longEntriesKeys, longEntriesValues, numAttrs) < 0) {
				return -1;
			}


			status = H5Gclose(group);
		}

		printf("Complete.\n");
	}




	status = H5Sclose(space);

	return 0;
}

/**
 * @brief      { function_description }
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 * @param      src     The source
 * @param[in]  nchars  The nchars
 *
 * @return     { description_of_the_return_value }
 */
long lofar_udp_io_write_HDF5(lofar_udp_io_write_config *config, int outp, const char *src, const long nchars) {
	return -1;
}

/**
 * @brief      { function_description }
 *
 * @param      config     The configuration
 * @param[in]  outp       The outp
 * @param[in]  fullClean  The full clean
 *
 * @return     { description_of_the_return_value }
 */
int lofar_udp_io_write_cleanup_HDF5(lofar_udp_io_write_config *config, int outp, int fullClean) {
	return -1;
}