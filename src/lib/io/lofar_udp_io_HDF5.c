
// This file is build on macros to try and keep it in some way clean
// Yes, it is ugly, but without it the file would be 2-3x longer.

// HDF5 group open/error handling macro
#define HDF_OPEN_GROUP(outGroupVar, inputHDF, groupName)        \
VERBOSE(printf("Opening HDF5 group %s\n", groupName);)              \
if (((outGroupVar) = H5Gopen(inputHDF, groupName, 0)) < 0) {    \
	H5Eprint(outGroupVar, stderr);                              \
	fprintf(stderr, "ERROR %s: Failed to open h5 groupName group, exiting.\n", __func__); \
	return -1;                                                  \
};


// Group/Dataset attribute setter
// Get the number of attributes in the struct, call the correct wrapper function, cleanup on error
#define _H5_SET_ATTRS(groupId, toSetAttrs, funcName, closeFunc)  \
numAttrs = VAR_ARR_SIZE(toSetAttrs);                        \
if (funcName(groupId, toSetAttrs, numAttrs) < 0) {   \
    closeFunc(groupId);                                                               \
	return -1;                                                                                  \
};

// Group wrapper to _H5_SET_ATTRS
#define H5G_SET_ATTRS(groupId, toSetAttrs, funcName)  \
	_H5_SET_ATTRS(groupId, toSetAttrs, funcName, H5Gclose)

// Dataset wrapper to _H5_SET_ATTRS
#define H5D_SET_ATTRS(groupId, toSetAttrs, funcName)  \
	_H5_SET_ATTRS(groupId, toSetAttrs, funcName, H5Dclose)

// Error check every call, raise warnings if an issue is encountered
// This is for soft errors, such as initialising a struct, that aren't essential
#define H5_ERR_CHECK(variable, statement) \
	H5_ERR_CHECK_RETURN(variable, statement, "", 0)

// This is forhard errors, where an actual operation has failed and we must exit
#define H5_ERR_CHECK_RETURN(variable, statement, msg, returnCode) \
if (((variable) = (statement)) < 0) {         \
	fprintf(stderr, "ERROR %s: HDF5 Error encountered %s: ", __func__, msg); \
	H5Eprint((variable), stderr);                                      \
    if (returnCode) {                                                              \
        return (returnCode);                                            \
    }                                                                  \
};

// Metadata function for ICD003
const char* get_rcumode_str(const int32_t rcumode);

// F#!* everything about this.
// This is the only way I've found to write a value that h5py reads as a bool, by
//  matching the hdf5 internal system.
//  0/1 values is not enough, this needs a unique named type.
typedef enum bool_t {
	FALSE = 0,
	TRUE = 1
} bool_t;


// Simplify attribute creation through key/value structs
#define shortStrLen  64
typedef struct strKeyStrVal {
	char key[shortStrLen];
	char val[shortStrLen];
} strKeyStrVal;

typedef struct strKeyStPtrVal {
	char key[shortStrLen];
	const char *val;
} strKeyStrPtrVal;

typedef struct strKeyStrArrVal {
	char key[shortStrLen];
	uint32_t num;
	char **const val;
} strKeyStrArrVal;

typedef struct strKeyLongVal {
	char key[shortStrLen];
	int64_t val;
} strKeyLongVal;

typedef struct strKeyLongArrVal {
	char key[shortStrLen];
	uint32_t num;
	int64_t *const val;
} strKeyLongArrVal;

typedef struct strKeyDoubleVal {
	char key[shortStrLen];
	double val;
} strKeyDoubleVal;

typedef struct strKeyDoubleArrVal {
	char key[shortStrLen];
	uint32_t num;
	double *const val;
} strKeyDoubleArrVal;

typedef struct strKeyBoolVal {
	char key[shortStrLen];
	bool_t val;
} strKeyBoolVal;

__attribute__((unused)) typedef struct strKeyBoolArrVal {
	char key[shortStrLen];
	uint32_t num;
	bool_t *const val;
} strKeyBoolArrVal;


/**
 * @brief      (NOT IMPLEMENTED) Setup the read I/O struct to handle HDF5 data
 *
 * @param      input   The input
 * @param[in]  inputLocation    The input file location
 * @param[in]  port    The index offset from the base file
 *
 * @return     0: Success, <0: Failure
 */
__attribute__((unused)) int32_t _lofar_udp_io_read_setup_HDF5(__attribute__((unused)) lofar_udp_io_read_config *const input, __attribute__((unused)) const char *inputLocation,
                                                              __attribute__((unused)) int8_t port) {
	return -1;
}


/**
 * @brief      (NOT IMPLEMENTED) Cleanup HDF5 file references for the read I/O struct
 *
 * @param      input  The input
 * @param[in]  port   The index offset from the base file
 */
__attribute__((unused)) int64_t _lofar_udp_io_read_HDF5(__attribute__((unused)) lofar_udp_io_read_config *const input, __attribute__((unused)) int8_t port, __attribute__((unused)) int8_t *targetArray, __attribute__((unused)) int64_t nchars) {
	return -1;
}

/**
 * @brief      Cleanup HDF5 file references for the read I/O struct
 *
 * @param      input  The input
 * @param[in]  port   The index offset from the base file
 */
__attribute__((unused)) void _lofar_udp_io_read_cleanup_HDF5(__attribute__((unused)) lofar_udp_io_read_config *const input, __attribute__((unused)) int8_t port) {
	return; // NOLINT(readability-redundant-control-flow)
}

/**
 * @brief      (NOT IMPLEMENTED) Temporarily read in num bytes from a HDF5 file
 *
 * @param      outbuf     The output buffer pointer
 * @param[in]  size       The size of words to read
 * @param[in]  num        The number of words to read
 * @param[in]  inputFile  The input file
 * @param[in]  resetSeek  Do (1) / Don't (0) reset back to the original location
 *                        in FILE* after performing a read operation
 *
 * @return     >0: bytes read, <=-1: Failure
 */
__attribute__((unused)) int64_t
_lofar_udp_io_read_temp_HDF5(__attribute__((unused)) void *outbuf, __attribute__((unused)) const int64_t size, __attribute__((unused)) const int8_t num, __attribute__((unused)) const char inputFile[],
                             __attribute__((unused)) const int8_t resetSeek) {
	//int64_t readlen = -1;
	return -1;
}


/**
 * @brief      Setup the write I/O struct to handle HDF5 data
 *
 * @param      config  The configuration
 * @param[in]  outp    The outp
 * @param[in]  iter    The iteration of the output file
 *
 * @return     0: Success, <0: Failure
 */
int32_t _lofar_udp_io_write_setup_HDF5(lofar_udp_io_write_config *const config, __attribute__((unused)) int8_t outp, const int32_t iter) {

	// Documentation referenced
	//
	// https://support.hdfgroup.org/ftp/HDF5/examples/examples-by-api/hdf5-examples/1_10/C/H5D/h5ex_d_unlimgzip.c
	// https://bitbucket.hdfgroup.org/projects/HDFFV/repos/hdf5-examples/browse/1_10/C/H5D/h5ex_d_extern.c
	// https://support.hdfgroup.org/ftp/HDF5/examples/examples-by-api/hdf5-examples/1_6/C/H5G/h5ex_g_create.c
	// 
	char h5Name[DEF_STR_LEN];
	if (lofar_udp_io_parse_format(h5Name, config->outputFormat, -1, iter, 0, config->firstPacket) < 0) {
		return -1;
	}

	if (!config->hdf5Writer.initialised) {
		hid_t group;
		herr_t status;

		H5_ERR_CHECK_RETURN(config->hdf5Writer.file, H5Fcreate(h5Name, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT), "Failed to create base HDF5 file", -1);

		char *groupNames[] = {  "/PROCESS_HISTORY",
								"/SUB_ARRAY_POINTING_000",
						        "/SUB_ARRAY_POINTING_000/PROCESS_HISTORY",
						        "/SUB_ARRAY_POINTING_000/BEAM_000",
						        "/SUB_ARRAY_POINTING_000/BEAM_000/PROCESS_HISTORY",
						        "/SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES",
						        "/SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES/SPECTRAL",
						        "/SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES/TIME",
						        "/SYS_LOG"
		};
		int32_t numGroups = VAR_ARR_SIZE(groupNames);

		// Create the default group/dataset structures
		for (int32_t groupIdx = 0; groupIdx < numGroups; groupIdx++) {
			VERBOSE(printf("HDF5: Creating group  %d/%d %s\n", groupIdx, numGroups, groupNames[groupIdx]));

			H5_ERR_CHECK_RETURN(group, H5Gcreate(config->hdf5Writer.file, groupNames[groupIdx], H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT), "Failed to create group", -1);
			H5_ERR_CHECK_RETURN(status, H5Gclose(group), "Failed to close group", -1);
		}

		if (strncpy(config->outputLocations[0], h5Name, DEF_STR_LEN) != config->outputLocations[0]) {
			fprintf(stderr, "ERROR: Failed to copy output file name to struct, exiting.\n");
			return -1;
		}

		config->hdf5Writer.initialised = 1;
		VERBOSE(printf("HDF5: base groups were created.\n");)
	}

	// We can't initialise the datasets just yet as we don't know their shapes -- they'll be created when the metadata is written for the first time.

	VERBOSE(printf("HDF5: Exiting file creation.\n"));
	return 0;
}

/**
 * @brief Set a number of string attributes on a HDF5 Group
 *
 * @param group Group reference
 * @param attrs Array of attribute pairs
 * @param numEntries Number of attributes on attrs
 *
 * @return 0: Success, <0: Failure
 */
static int32_t hdf5SetupStrAttrs(const hid_t group, const strKeyStrVal attrs[], const size_t numEntries) {
	VERBOSE(printf("Str attrs\n"));

	hid_t filetype;
	herr_t status;
	hid_t memtype;
	hsize_t dims[1] = { 0 };
	hid_t space;

	// Initialise an empty space reference (single value, not an array)
	H5_ERR_CHECK(space, H5Screate_simple(0, dims, NULL));

	VERBOSE(printf("Begin Loop for %ld\n", numEntries));
	for (size_t atIdx = 0; atIdx < numEntries; atIdx++) {
		VERBOSE(printf("%s: %s (%ld)\n", attrs[atIdx].key, attrs[atIdx].val, strlen(attrs[atIdx].val)));
		// Get the string length to determine the attribute length
		// Add 1 for the null byte
		const int32_t strlenv = (int32_t) strnlen(attrs[atIdx].val, shortStrLen) + 1;
		VERBOSE(printf("strlenv: %d\n", strlenv));

		// Initialise the attribute variables
		H5_ERR_CHECK(filetype, H5Tcopy(H5T_FORTRAN_S1));
		H5_ERR_CHECK(status, H5Tset_size(filetype, H5T_VARIABLE));
		H5_ERR_CHECK(memtype, H5Tcopy (H5T_C_S1));
		H5_ERR_CHECK(status, H5Tset_size(memtype, H5T_VARIABLE));
		H5_ERR_CHECK(status, H5Tset_size(filetype, strlenv - 1));
		H5_ERR_CHECK(status, H5Tset_size(memtype, strlenv));

		if (H5Aexists(group, attrs[atIdx].key) != 0) {
			// Cannot find documentation on modifying VLA string attributes, taking the safe option:
			//  - Delete the existing attribute and re-write
			H5_ERR_CHECK_RETURN(status, H5Adelete(group, attrs[atIdx].key), "Failed to delete str attr", -1);
		}

		// Create the new attribute
		hid_t attr;
		VERBOSE(printf("attrs[%ld].key = %s, attrs[%ld].val = %s\n", atIdx, attrs[atIdx].key, atIdx, attrs[atIdx].val));
		H5_ERR_CHECK_RETURN(attr, H5Acreate(group, attrs[atIdx].key, filetype, space, H5P_DEFAULT, H5P_DEFAULT), "Failed to create str attr", -1);

		// Write the value to the new attribute
		VERBOSE(printf("Write\n"));
		H5_ERR_CHECK_RETURN(status, H5Awrite(attr, memtype, attrs[atIdx].val), "Failed to set str attr", -1);

		// Cleanup the attribute
		VERBOSE(printf("Close\n"));
		H5_ERR_CHECK(status, H5Aclose(attr));
		H5_ERR_CHECK(status, H5Tclose(filetype));
		H5_ERR_CHECK(status, H5Tclose(memtype));
	}

	// Cleanup and exit
	VERBOSE(printf("Str attrs end loop\n"));
	H5_ERR_CHECK(status, H5Sclose(space));
	VERBOSE(printf("Str attrs finished\n"));

	return 0;
}

/**
 * @brief Set a number of string attributes on a HDF5 Group
 *
 * @param group Group reference
 * @param attrs Array of attribute pairs
 * @param numEntries Number of attributes on attrs
 *
 * @return 0: Success, <0: Failure
 */
static int32_t hdf5SetupStrPtrAttrs(const hid_t group, const strKeyStrPtrVal attrs[], const size_t numEntries) {
	VERBOSE(printf("Str attrs\n"));

	hid_t filetype;
	herr_t status;
	hid_t memtype;
	hsize_t dims[1] = { 0 };
	hid_t space;

	// Initialise an empty space reference (single value, not an array)
	H5_ERR_CHECK(space, H5Screate_simple(0, dims, NULL));

	VERBOSE(printf("Begin Loop for %ld\n", numEntries));
	for (size_t atIdx = 0; atIdx < numEntries; atIdx++) {
		VERBOSE(printf("%s: %s (%ld)\n", attrs[atIdx].key, attrs[atIdx].val, strlen(attrs[atIdx].val)));
		// Get the string length to determine the attribute length
		// Add 1 for the null byte
		const int32_t strlenv = (int32_t) strlen(attrs[atIdx].val) + 1; // Can't use strnlen as we don't have limits on the input
		VERBOSE(printf("strlenv: %d\n", strlenv));

		// Initialise the attribute variables
		H5_ERR_CHECK(filetype, H5Tcopy (H5T_FORTRAN_S1));
		H5_ERR_CHECK(status, H5Tset_size(filetype, H5T_VARIABLE));
		H5_ERR_CHECK(memtype, H5Tcopy (H5T_C_S1));
		H5_ERR_CHECK(status, H5Tset_size(memtype, H5T_VARIABLE));
		H5_ERR_CHECK(status, H5Tset_size(filetype, strlenv - 1));
		H5_ERR_CHECK(status, H5Tset_size(memtype, strlenv));

		if (H5Aexists(group, attrs[atIdx].key) != 0) {
			// Cannot find documentation on modifying VLA string attributes, taking the safe option:
			//  - Delete the existing attribute and re-write
			H5_ERR_CHECK_RETURN(status, H5Adelete(group, attrs[atIdx].key), "Failed to delete str attr", -1);
		}

		// Create the new attribute
		hid_t attr;
		VERBOSE(printf("attrs[%ld].key = %s, attrs[%ld].val = %s\n", atIdx, attrs[atIdx].key, atIdx, attrs[atIdx].val));
		H5_ERR_CHECK_RETURN(attr, H5Acreate(group, attrs[atIdx].key, filetype, space, H5P_DEFAULT, H5P_DEFAULT), "Failed to create str attr", -1);

		// Write the value to the new attribute
		VERBOSE(printf("Write\n"));
		H5_ERR_CHECK_RETURN(status, H5Awrite(attr, memtype, attrs[atIdx].val), "Failed to set str attr", -1);

		// Cleanup the attribute
		VERBOSE(printf("Close\n"));
		H5_ERR_CHECK(status, H5Aclose(attr));
		H5_ERR_CHECK(status, H5Tclose(filetype));
		H5_ERR_CHECK(status, H5Tclose(memtype));
	}

	// Cleanup and exit
	VERBOSE(printf("Str attrs end loop\n"));
	H5_ERR_CHECK(status, H5Sclose(space));
	VERBOSE(printf("Str attrs finished\n"));

	return 0;
}

/**
 * @brief Set a number of string array attributes on a HDF5 Group
 *
 * @param group Group reference
 * @param attrs Array of attribute pairs
 * @param numEntries Number of attributes on attrs
 *
 * @return 0: Success, <0: Failure
 */
static int32_t hdf5SetupStrArrayAttrs(hid_t group, const strKeyStrArrVal attrs[], size_t numEntries) {
	VERBOSE(printf("Str array attrs\n"));

	hid_t filetype;
	hid_t memtype;
	herr_t status;

	for (size_t atIdx = 0; atIdx < numEntries; atIdx++) {
		if (attrs[atIdx].num < 1) continue;
		VERBOSE(printf("%s[0]: %s\n", attrs[atIdx].key, attrs[atIdx].val[0]));

		H5_ERR_CHECK(filetype, H5Tcopy(H5T_FORTRAN_S1));
		H5_ERR_CHECK(status, H5Tset_size(filetype, H5T_VARIABLE));
		H5_ERR_CHECK(memtype, H5Tcopy(H5T_C_S1));
		H5_ERR_CHECK(status, H5Tset_size(memtype, H5T_VARIABLE));

		hsize_t dims[1] = { attrs[atIdx].num };
		hid_t space;

		// Initialise a non-empty space variable for the array
		H5_ERR_CHECK(space, H5Screate_simple(1, dims, NULL));

		if (H5Aexists(group, attrs[atIdx].key) != 0) {
			// Cannot find documentation on modifying VLA string array attributes, taking the safe option:
			//  - Delete the existing attribute and re-write
			H5_ERR_CHECK_RETURN(status, H5Adelete(group, attrs[atIdx].key), "Failed to delete str arr attr", -1);
		}

		// Create the attribute
		hid_t attr;
		H5_ERR_CHECK_RETURN(attr, H5Acreate(group, attrs[atIdx].key, filetype, space, H5P_DEFAULT, H5P_DEFAULT), "Failed to create str arr attr", -1);

		// Write the attribute
		VERBOSE(printf("Write\n"));
		H5_ERR_CHECK_RETURN(status, H5Awrite(attr, memtype, attrs[atIdx].val), "Failed to write str arr attr", -1);

		// Cleanup the attribute
		VERBOSE(printf("Close\n"));
		H5_ERR_CHECK(status, H5Aclose(attr));
		H5_ERR_CHECK(status, H5Tclose(filetype));
		H5_ERR_CHECK(status, H5Tclose(memtype));
		H5_ERR_CHECK(status, H5Sclose(space));
	}

	// Nothing to cleanup -- exit
	VERBOSE(printf("Str array attrs finished\n"));

	return 0;
}

/**
 * @brief Set a number of long attributes on a HDF5 Group
 *
 * @param group Group reference
 * @param attrs Array of attribute pairs
 * @param numEntries Number of attributes on attrs
 *
 * @return 0: Success, <0: Failure
 */
static int32_t hdf5SetupLongAttrs(hid_t group, const strKeyLongVal attrs[], size_t numEntries) {
	VERBOSE(printf("Long attrs\n"));

	hid_t attr;
	herr_t status;
	hsize_t dims[1] = { 1 };
	hid_t space;

	// Initialise an empty space reference (single value, not an array)
	H5_ERR_CHECK(space, H5Screate_simple(0, dims, NULL));

	for (size_t atIdx = 0; atIdx < numEntries; atIdx++) {
		VERBOSE(printf("%s: %ld\n", attrs[atIdx].key, attrs[atIdx].val));
		if (H5Aexists(group, attrs[atIdx].key) == 0) {
			H5_ERR_CHECK_RETURN(attr, H5Acreate(group, attrs[atIdx].key, H5T_NATIVE_INT64, space, H5P_DEFAULT, H5P_DEFAULT), "Failed to create int64_t attr", -1);
		} else {
			H5_ERR_CHECK_RETURN(attr, H5Aopen(group, attrs[atIdx].key, H5P_DEFAULT), "Failed to open int64_t attr", -1);
		}

		H5_ERR_CHECK_RETURN(status, H5Awrite(attr, H5T_NATIVE_INT64, &(attrs[atIdx].val)), "Failed to set int64_t attr", -1);
		H5_ERR_CHECK(status, H5Aclose(attr));
	}
	H5_ERR_CHECK(status, H5Sclose(space));

	return 0;
}

/**
 * @brief Set a number of long array attributes on a HDF5 Group
 *
 * @param group Group reference
 * @param attrs Array of attribute pairs
 * @param numEntries Number of attributes on attrs
 *
 * @return 0: Success, <0: Failure
 */
__attribute__((unused)) static int32_t hdf5SetupLongArrayAttrs(hid_t group, const strKeyLongArrVal attrs[], size_t numEntries) {
	VERBOSE(printf("Long arr attrs\n"));

	for (size_t atIdx = 0; atIdx < numEntries; atIdx++) {
		if (attrs[atIdx].num < 1) continue;
		hid_t attr;
		herr_t status;
		hsize_t dims[1] = { attrs[atIdx].num };
		hid_t space;
		H5_ERR_CHECK(space, H5Screate_simple(1, dims, NULL));


		VERBOSE(printf("%s[0]: %ld\n", attrs[atIdx].key, attrs[atIdx].val[0]));
		if (H5Aexists(group, attrs[atIdx].key) == 0) {
			H5_ERR_CHECK_RETURN(attr, H5Acreate(group, attrs[atIdx].key, H5T_NATIVE_INT64, space, H5P_DEFAULT, H5P_DEFAULT), "Failed to create int64_t arr attr", -1);
		} else {
			H5_ERR_CHECK_RETURN(attr, H5Aopen(group, attrs[atIdx].key, H5P_DEFAULT), "Failed to open int64_t arr attr", -1);
		}


		if ((status = H5Awrite(attr, H5T_NATIVE_INT64, attrs[atIdx].val)) < 0) {
			H5Eprint(status, stderr);
			fprintf(stderr, "ERROR %s: Failed to set long attr %s[0]: %ld, exiting.\n", __func__, attrs[atIdx].key, attrs[atIdx].val[0]);
			return -1;
		}
		H5_ERR_CHECK(status, H5Aclose(attr));
		H5_ERR_CHECK(status, H5Sclose(space));
	}

	return 0;
}

/**
 * @brief Set a number of double attributes on a HDF5 Group
 *
 * @param group Group reference
 * @param attrs Array of attribute pairs
 * @param numEntries Number of attributes on attrs
 *
 * @return 0: Success, <0: Failure
 */
static int32_t hdf5SetupDoubleAttrs(hid_t group, const strKeyDoubleVal attrs[], size_t numEntries) {
	VERBOSE(printf("Double attrs\n"));
	hid_t attr;
	herr_t status;

	hsize_t dims[1] = { 1 };
	hid_t space;

	// Initialise an empty space reference (single value, not an array)
	H5_ERR_CHECK(space, H5Screate_simple(0, dims, NULL));

	for (size_t atIdx = 0; atIdx < numEntries; atIdx++) {
		VERBOSE(printf("%s: %lf\n", attrs[atIdx].key, attrs[atIdx].val));
		if (H5Aexists(group, attrs[atIdx].key) == 0) {
			H5_ERR_CHECK_RETURN(attr, H5Acreate(group, attrs[atIdx].key, H5T_NATIVE_DOUBLE, space, H5P_DEFAULT, H5P_DEFAULT), "Failed to create double attr", -1);
		} else {
			H5_ERR_CHECK_RETURN(attr, H5Aopen(group, attrs[atIdx].key, H5P_DEFAULT), "Failed to open double attr", -1);
		}

		H5_ERR_CHECK_RETURN(status, H5Awrite(attr, H5T_NATIVE_DOUBLE, &(attrs[atIdx].val)), "Failed to set double attr", -1);
		H5_ERR_CHECK(status, H5Aclose(attr));
	}
	H5_ERR_CHECK(status, H5Sclose(space));

	return 0;
}

/**
 * @brief Set a number of double array attributes on a HDF5 Group
 *
 * @param group Group reference
 * @param attrs Array of attribute pairs
 * @param numEntries Number of attributes on attrs
 *
 * @return 0: Success, <0: Failure
 */
static int32_t hdf5SetupDoubleArrayAttrs(hid_t group, const strKeyDoubleArrVal attrs[], size_t numEntries) {
	VERBOSE(printf("double arr attrs\n"));
	herr_t status;

	for (size_t atIdx = 0; atIdx < numEntries; atIdx++) {
		if (attrs[atIdx].num < 1) continue;

		hid_t attr;
		hsize_t dims[1] = { attrs[atIdx].num };
		hid_t space;
		H5_ERR_CHECK(space, H5Screate_simple(1, dims, NULL));

		VERBOSE(printf("%s[0]: %lf\n", attrs[atIdx].key, attrs[atIdx].val[0]));
		if (H5Aexists(group, attrs[atIdx].key) == 0) {
			H5_ERR_CHECK_RETURN(attr, H5Acreate(group, attrs[atIdx].key, H5T_NATIVE_DOUBLE, space, H5P_DEFAULT, H5P_DEFAULT), "Failed to create double arr attr", -1);
		} else {
			H5_ERR_CHECK_RETURN(attr, H5Aopen(group, attrs[atIdx].key, H5P_DEFAULT), "Failed to open double arr attr", -1);
		}

		H5_ERR_CHECK_RETURN(status, H5Awrite(attr, H5T_NATIVE_DOUBLE, attrs[atIdx].val), "Failed to set double arr attr", -1);
		H5_ERR_CHECK(status, H5Aclose(attr));
		H5_ERR_CHECK(status, H5Sclose(space));
	}

	return 0;
}

/**
 * @brief Set a number of bool attributes on a HDF5 Group
 *
 * @param group Group reference
 * @param attrs Array of attribute pairs
 * @param numEntries Number of attributes on attrs
 *
 * @return 0: Success, <0: Failure
 */
static int32_t hdf5SetupBoolAttrs(hid_t group, const strKeyBoolVal attrs[], size_t numEntries) {
	VERBOSE(printf("Bool attrs\n"));

	hid_t attr;
	herr_t status;
	hsize_t dims[1] = { 1 };
	hid_t space;

	// Initialise an empty space reference (single value, not an array)
	H5_ERR_CHECK(space, H5Screate_simple(0, dims, NULL));

	// Register the bool enum with HDF5
	bool_t tmpVal;
	hid_t boolenum;
	H5_ERR_CHECK(boolenum, H5Tcreate(H5T_ENUM, sizeof(bool_t)));
	H5_ERR_CHECK(status, H5Tenum_insert(boolenum, "FALSE", ((tmpVal)=(FALSE),&(tmpVal))));
	H5_ERR_CHECK(status, H5Tenum_insert(boolenum, "TRUE", ((tmpVal)=(TRUE),&(tmpVal))));

	for (size_t atIdx = 0; atIdx < numEntries; atIdx++) {
		VERBOSE(printf("%s: %c\n", attrs[atIdx].key, attrs[atIdx].val));

		if (H5Aexists(group, attrs[atIdx].key) == 0) {
			H5_ERR_CHECK_RETURN(attr, H5Acreate(group, attrs[atIdx].key, boolenum, space, H5P_DEFAULT, H5P_DEFAULT), "Failed to create bool attr", -1);
		} else {
			H5_ERR_CHECK_RETURN(attr, H5Aopen(group, attrs[atIdx].key, H5P_DEFAULT), "Failed to open bool attr", -1);
		}

		H5_ERR_CHECK_RETURN(status, H5Awrite(attr, boolenum, &(attrs[atIdx].val)), "Failed to set bool attr", -1);
		H5_ERR_CHECK(status, H5Aclose(attr));
	}

	H5_ERR_CHECK(status, H5Sclose(space));
	return 0;
}

int64_t _lofar_udp_io_write_metadata_HDF5(lofar_udp_io_write_config *const config, const lofar_udp_metadata *metadata) {
	hid_t group;
	hsize_t dims[2] = { 1 };
	hid_t space;
	herr_t status;
	H5_ERR_CHECK(space, H5Screate_simple(1, dims, NULL));

	size_t numAttrs;

	if (!config->hdf5Writer.metadataInitialised) {
		// Root group
		{
			HDF_OPEN_GROUP(group, config->hdf5Writer.file, "/");

			char filterSelection[16];
			if (strncpy(filterSelection, get_rcumode_str(metadata->upm_rcumode), 15) != filterSelection) {
				fprintf(stderr, "ERROR: Failed to copy filter selection while generating HDF5 metadata, exiting.\n");
				H5Gclose(group);
				return -1;
			}

			const strKeyStrVal rootStrAttrs[] = {
				{ "GROUPTYPE", "Root" },
				{ "FILETYPE", "bf" },
				{ "TELESCOPE", "LOFAR" },
				{ "PROJECT_CO_I", "UNKNOWN" }, // TODO
				{ "OBSERVATION_START_TAI", "NULL" }, // TODO
				{ "EXPTIME_START_TAI", "NULL" }, // TODO
				{ "OBSERVATION_FREQUENCY_UNIT", "MHz" },
				{ "CLOCK_FREQUENCY_UNIT", "MHz" },
				{ "PIPELINE_NAME", "udpPacketManager" },
				{ "ICD_NUMBER", "ICD003" },
				{ "ICD_VERSION", "2.6" },
				{ "NOTES", "NULL" }, // TODO
				{ "BF_FORMAT", "RAW" },
				{ "TOTAL_INTEGRATION_TIME_UNIT", "s" },
				{ "BANDWIDTH_UNIT", "MHz" },
				{ "OBSERVATION_DATATYPE", "NULL" }, // TODO
				{ "SYSTEM_VERSION", UPM_VERSION },
				{ "PIPELINE_VERSION", UPM_VERSION }, // TODO?
				{ "BF_VERSION", "NULL" }, // TODO
			};

			H5G_SET_ATTRS(group, rootStrAttrs, hdf5SetupStrAttrs);


			const strKeyStrPtrVal rootStrPtrAttrs[] = {
				{ "FILENAME", config->outputFormat },
				{ "FILEDATE", metadata->upm_daq },
				{ "PROJECT_ID", metadata->obs_id },
				{ "PROJECT_TITLE", metadata->obs_id }, // TODO
				{ "PROJECT_PI", metadata->observer }, // TODO
				{ "PROJECT_CONTACT", metadata->observer }, // TODO
				{ "OBSERVER", metadata->observer }, // TODO
				{ "OBSERVATION_ID", metadata->obs_id },
				{ "OBSERVATION_START_UTC", metadata->obs_utc_start },
				{ "EXPTIME_START_UTC", metadata->obs_utc_start },
				{ "ANTENNA_SET", metadata->freq_raw > 100 ? "HBA_JOINED" : "LBA_OUTER" },
				{ "CREATE_OFFLINE_ONLINE", metadata->upm_reader == DADA_ACTIVE ? "ONLINE" : "OFFLINE" },
				{ "TARGET", metadata->source },
				{ "FILTER_SELECTION", filterSelection },
			};
			H5G_SET_ATTRS(group, rootStrPtrAttrs, hdf5SetupStrPtrAttrs);


			const strKeyStrArrVal rootStrArrVal[] = {
				{ "OBSERVATION_STATION_LIST", 1, (char **) &metadata->telescope },
				{ "TARGETS",                  1, (char **) &metadata->source },
			};
			H5G_SET_ATTRS(group, rootStrArrVal, hdf5SetupStrArrayAttrs);


			const strKeyDoubleVal rootDoubleAttrs[] = {
				{ "OBSERVATION_START_MJD", metadata->obs_mjd_start },
				{ "EXPTIME_START_MJD", metadata->obs_mjd_start },
				{ "OBSERVATION_FREQUENCY_MIN", metadata->fbottom },
				{ "OBSERVATION_FREQUENCY_CENTER", metadata->freq_raw },
				{ "OBSERVATION_FREQUENCY_MAX", metadata->ftop },
				{ "CLOCK_FREQUENCY", metadata->upm_rcuclock },
				{ "BANDWIDTH", metadata->subband_bw * metadata->nchan }, // Integrated bandwidth, not ftop - fbot
				{ "SUB_ARRAY_POINTING_DIAMETER", 0.0 }, // TODO
				{ "BEAM_DIAMETER", 0.0 }, // TODO
				{ "TOTAL_INTEGRATION_TIME", 0.0 },
				{ "OBSERVATION_END_MJD", 0.0 },
				{ "EXPTIME_STOP_MJD", 0.0 },
			};
			H5G_SET_ATTRS(group, rootDoubleAttrs, hdf5SetupDoubleAttrs);


			double doubleArrVal[] =  { -1.0 };
			const strKeyDoubleArrVal rootDoubleArrAttrs[] = {
				{ "WEATHER_TEMPERATURE", 1, (double *) &doubleArrVal },
				{ "WEATHER_HUMIDITY", 1,    (double *) &doubleArrVal },
				{ "SYSTEM_TEMPERATURE", 1,  (double *) &doubleArrVal },
			};
			H5G_SET_ATTRS(group, rootDoubleArrAttrs, hdf5SetupDoubleArrayAttrs);


			// TODO: OBSERVATION_NOF_STATIONS is meant to be unsigned, not signed.
			const strKeyLongVal rootLongAttrs[] = {
				{ "OBSERVATION_NOF_STATIONS", 1 },
				{ "OBSERVATION_NOF_BITS_PER_SAMPLE", metadata->upm_input_bitmode },
				{ "OBSERVATION_NOF_SUB_ARRAY_POINTINGS", 1 },
				{ "NOF_SUB_ARRAY_POINTINGS", 1 },
			};
			H5G_SET_ATTRS(group, rootLongAttrs, hdf5SetupLongAttrs);


			H5_ERR_CHECK(status, H5Gclose(group));
		}

		// SYS_LOG
		{
			HDF_OPEN_GROUP(group, config->hdf5Writer.file, "/SYS_LOG");

			const strKeyStrVal sysLogStrAttrs[] = {
				{ "GROUPTYPE", "SysLog" },
			};
			H5G_SET_ATTRS(group, sysLogStrAttrs, hdf5SetupStrAttrs);

			H5_ERR_CHECK(status, H5Gclose(group));
		}

		// SUB_ARRAY_POINTING_000
		{
			HDF_OPEN_GROUP(group, config->hdf5Writer.file, "/SUB_ARRAY_POINTING_000");

			const strKeyStrVal sap0StrAttrs[] = {
				{ "GROUPTYPE", "SubArrayPointing" },

				{ "TOTAL_INTEGRATION_TIME_UNIT", "s" },
				{ "POINT_RA_UNIT", "deg" },
				{ "POINT_DEC_UNIT", "deg" },
				{ "SAMPLING_RATE_UNIT", "MHz" },
				{ "SAMPLING_TIME_UNIT", "s" },
				{ "SUBBAND_WIDTH_UNIT", "MHz" },
				{ "CHANNEL_WIDTH_UNIT", "MHz" },
			};
			H5G_SET_ATTRS(group,sap0StrAttrs, hdf5SetupStrAttrs);

			const strKeyStrPtrVal sap0StrPtrAttrs[] = {
				{ "EXPTIME_START_UTC", metadata->obs_utc_start },
			};
			H5G_SET_ATTRS(group, sap0StrPtrAttrs, hdf5SetupStrPtrAttrs);


			const strKeyDoubleVal sap0DoubleAttrs[] = {
				{ "EXPTIME_START_MJD", metadata->obs_mjd_start },
				{ "POINT_RA", metadata->ra_rad * 57.2958 }, // Convert radians to degrees
				{ "POINT_DEC", metadata->dec_rad * 57.2958 },  // Convert radians to degrees
				{ "SAMPLING_RATE", 1. / metadata->tsamp / 1e6 },
				{ "SAMPLING_TIME", metadata->tsamp } ,
				{ "SUBBAND_WIDTH", metadata->subband_bw },
				{ "CHANNEL_WIDTH", metadata->subband_bw }, // TODO: channel_width
			};
			H5G_SET_ATTRS(group, sap0DoubleAttrs, hdf5SetupDoubleAttrs);


			// TODO
			double doubleArrVal[] =  { -1.0 };
			const strKeyDoubleArrVal sap0DoubleArrAttrs[] = {
				{ "POINT_AZIMUTH", 1, doubleArrVal },
				{ "POINT_ALTITUDE", 1, doubleArrVal },
			};
			H5G_SET_ATTRS(group, sap0DoubleArrAttrs, hdf5SetupDoubleArrayAttrs);


			const strKeyLongVal sap0LongAttrs[] = {
				{ "OBSERVATION_NOF_BEAMS", 1 },
				{ "NOF_BEAMS", 1 },
				{ "CHANNELS_PER_SUBBAND", 1 }, // TODO: Channels
				{ "NOF_SAMPLES", 0 }, // TODO: Modify at end?
			};
			H5G_SET_ATTRS(group, sap0LongAttrs, hdf5SetupLongAttrs);


			H5_ERR_CHECK(status, H5Gclose(group));
		}


		// PROCESS_HISTORY
		// /SUB_ARRAY_POINTING_000/PROCESS_HISTORY
		// /SUB_ARRAY_POINTING_000/BEAM_000/PROCESS_HISTORY
		{
			const char *groups[] = {
				"/PROCESS_HISTORY",
				"/SUB_ARRAY_POINTING_000/PROCESS_HISTORY",
				"/SUB_ARRAY_POINTING_000/BEAM_000/PROCESS_HISTORY"
			};
			int32_t numGroups = sizeof(groups) / sizeof(groups[0]);

			const strKeyStrVal processHistStrAttrs[] = {
				{ "GROUPTYPE", "ProcessHistory" },
			};


			const strKeyBoolVal processHistoryBoolAttrs[] = {
				{ "OBSERVATION_PARSET", FALSE },
				{ "OBSERVATION_LOG", FALSE },
				{ "PRESTO_PARSET", FALSE },
				{ "PRESTO_LOG", FALSE },
			};

			for (int32_t groupIdx = 0; groupIdx < numGroups; groupIdx++) {
				HDF_OPEN_GROUP(group, config->hdf5Writer.file, groups[groupIdx]);
				H5G_SET_ATTRS(group, processHistStrAttrs, hdf5SetupStrAttrs);
				H5G_SET_ATTRS(group, processHistoryBoolAttrs, hdf5SetupBoolAttrs);

				H5_ERR_CHECK(status, H5Gclose(group));

			}

		}


		// SUB_ARRAY_POINTING_000/BEAM_000
		// Missing:
		{
			VERBOSE(printf("\"/SUB_ARRAY_POINTING_000/BEAM_000\"\n"));
			HDF_OPEN_GROUP(group, config->hdf5Writer.file, "/SUB_ARRAY_POINTING_000/BEAM_000");


			const strKeyStrVal sap0beam0StrAttrs[] = {
				{ "GROUPTYPE", "Beam" },
				{ "SAMPLING_RATE_UNIT", "Hz" },
				{ "SAMPLING_TIME_UNIT", "s" },
				{ "SUBBAND_WIDTH_UNIT", "Hz" },
				{ "POINT_RA_UNIT", "deg" },
				{ "POINT_DEC_UNIT", "deg" },
				{ "POINT_OFFSET_RA_UNIT", "deg" },
				{ "POINT_OFFSET_DEC_UNIT", "deg" },
				{ "BEAM_DIAMETER_RA_UNIT", "arcmin" },
				{ "BEAM_DIAMETER_DEC_UNIT", "arcmin" },
				{ "BEAM_FREQUENCY_CENTER_UNIT", "MHz" },
				{ "FOLD_PERIOD_UNIT", "s" },
				{ "DISPERSION_MEASURE_UNIT", "pc/cm3" },
				{ "SIGNAL_SUM", "INCOHERENT" },
				{ "DEDISPERSION", "NONE" }, // TODO
			};
			H5G_SET_ATTRS(group, sap0beam0StrAttrs, hdf5SetupStrAttrs);

			const strKeyStrPtrVal sap0beam0StrPtrAttrs[] = {
				{ "TRACKING", metadata->coord_basis },
			};
			H5G_SET_ATTRS(group, sap0beam0StrPtrAttrs, hdf5SetupStrPtrAttrs);


			// Variable length array strings
			{
				const char voltagesArray[4][3] = {
					"Xi",
					"Xr",
					"Yi",
					"Yr"
				};
				const char stokesArray[4][2] = {
					"I", "Q", "U", "V"
				};
				char **chosenArray;

				if (metadata->upm_procmode < 100) {
					chosenArray = (char **) voltagesArray;
				} else {
					chosenArray = (char **) stokesArray;
				}


				const strKeyStrArrVal sap0beam0StrArrAttrs[] = {
					{ "STATION_LIST", 1, (char **) &metadata->telescope },
					{ "TARGETS", 1, (char **) &metadata->source },
					{ "STOKES_COMPONENTS", 4, chosenArray },
				};

				//sap0beam0StrArrAttrs[0].val = &metadata->telescope;
				//sap0beam0StrArrAttrs[1].val = &metadata->source;
				//sap0beam0StrArrAttrs[2].val = chosenArray;

				H5G_SET_ATTRS(group, sap0beam0StrArrAttrs, hdf5SetupStrArrayAttrs);
			}

			const strKeyDoubleVal sap0beam0DoubleAttrs[] = {
				{ "SAMPLING_RATE", 1. / metadata->tsamp },
				{ "SAMPLING_TIME", metadata->tsamp },
				{ "SUBBAND_WIDTH", metadata->subband_bw * 1e6 },
				{ "POINT_RA", metadata->ra_rad * 57.2958 }, // Convert radians to degrees
				{ "POINT_DEC", metadata->dec_rad * 57.2958 },  // Convert radians to degrees
				{ "POINT_OFFSET_RA", 0. },
				{ "POINT_OFFSET_DEC", 0. },
				{ "BEAM_FREQUENCY_CENTER", metadata->freq_raw },
				{ "FOLD_PERIOD", 0. },
				{ "DISPERSION_MEASURE", 0. },
				{ "POSITION_OFFSET_RA", 0. },
				{ "POSITION_OFFSET_DEC", 0. },
				{ "BEAM_DIAMETER_RA", 0. },
				{ "BEAM_DIAMETER_DEC", 0. },
			};
			H5G_SET_ATTRS(group, sap0beam0DoubleAttrs, hdf5SetupDoubleAttrs);


			const strKeyLongVal sap0beam0LongAttrs[] = {
				{ "NOF_STATIONS", 1 },
				{ "CHANNELS_PER_SUBBAND", 1 },
				{ "OBSERVATION_NOF_STOKES", metadata->upm_num_outputs },
				{ "NOF_STOKES", metadata->upm_num_outputs },

			};
			H5G_SET_ATTRS(group, sap0beam0LongAttrs, hdf5SetupLongAttrs);


			const strKeyBoolVal sap0beam0BoolAttrs[] = {
				{ "FOLDED_DATA", 1 },
				{ "BARYCENTER", 0 },
				{ "COMPLEX_VOLTAGE", (metadata->upm_procmode < 100)},
			};
			H5G_SET_ATTRS(group, sap0beam0BoolAttrs, hdf5SetupBoolAttrs);

			H5_ERR_CHECK(status, H5Gclose(group));

		}


		// SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES
		// Missing:
		{
			HDF_OPEN_GROUP(group, config->hdf5Writer.file, "/SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES");

			const strKeyStrVal sap0beam0coordStrAttrs[] = {
				{ "GROUPTYPE", "Coordinates" },
				{ "REF_LOCATION_FRAME", "GEOCENTER" }, // GEOCENTER, BARYCENTER, HELIOCENTER, TOPOCENTER, LSRK, LSRD, GALACTIC, LOCAL_GROUP, RELOCATABLE... Someone is optimistic as to where LOFAR stations will end up
				{ "REF_TIME_UNIT", "d" },
				{ "REF_TIME_FRAME", "MJD" },
			};
			H5G_SET_ATTRS(group, sap0beam0coordStrAttrs, hdf5SetupStrAttrs);


			{
				char *coordinateTypes[] = {
					"Time",
					"Spectral"
				};
				char *refUnits[] = {
						"m", "m", "m"
				};

				const strKeyStrArrVal sap0beam0coordStrArrAttrs[] = {
					{ "COORDINATES_TYPES", VAR_ARR_SIZE(coordinateTypes), (char **) &coordinateTypes },
					{ "REF_LOCATION_UNIT", VAR_ARR_SIZE(refUnits), (char **) &refUnits }
				};
				//sap0beam0coordStrArrAttrs[0].val = coordinateTypes;
				//sap0beam0coordStrArrAttrs[1].val = refUnits;

				printf("Sanity check: %d vs %d, %d vs %d\n", 2, sap0beam0coordStrArrAttrs[0].num, 3, sap0beam0coordStrArrAttrs[1].num);

				H5G_SET_ATTRS(group, sap0beam0coordStrArrAttrs, hdf5SetupStrArrayAttrs);
			}


			const strKeyDoubleVal sap0beam0coordDoubleAttrs[] = {
				{ "REF_TIME_VALUE", metadata->obs_mjd_start },
			};
			H5G_SET_ATTRS(group, sap0beam0coordDoubleAttrs, hdf5SetupDoubleAttrs);


			// Variable length array double entries
			{
				double attrval[] = { -1.0, -1.0, -1.0 };
				const strKeyDoubleArrVal sap0beam0coordDoubleArrAttrs[] = {
					{ "REF_LOCATION_VALUE", VAR_ARR_SIZE(attrval),  attrval },
				};
				//sap0beam0coordDoubleArrAttrs[0].val = &attrval;

				H5G_SET_ATTRS(group, sap0beam0coordDoubleArrAttrs, hdf5SetupDoubleArrayAttrs);
			}

			const strKeyLongVal sap0beam0coordLongAttrs[] = {
				{ "NOF_AXES", 2 },
				{ "NOF_COORDINATES", 2 },
			};
			H5G_SET_ATTRS(group, sap0beam0coordLongAttrs, hdf5SetupLongAttrs);

			H5_ERR_CHECK(status, H5Gclose(group));
		}

		// SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES/TIME
		// Missing:
		// REFERENCE_VALUE = arr dbl
		// REFERENCE_PIXEL = arr dbl
		// AXES_VALUES_PIXEL = { 0 };
		// AXES_VALUES_WORLD = { 0. };
		// INCREMENT = arr dbl
		{
			HDF_OPEN_GROUP(group, config->hdf5Writer.file, "/SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES/TIME");

			const strKeyStrVal sap0beam0coordtimeStrAttrs[] = {
				{ "GROUPTYPE", "TimeCoord" },
				{ "COORDINATE_TYPE", "Time" },
			};
			H5G_SET_ATTRS(group, sap0beam0coordtimeStrAttrs, hdf5SetupStrAttrs);

			{
				char *arrayValues[] = {
					"Linear",
					"Time",
					"s",
				};
				const strKeyStrArrVal sap0beam0coordtimeStrArrAttrs[] = {
					{ "STORAGE_TYPE", 1, &(arrayValues[0]) },
					{ "AXES_NAMES", 1, &(arrayValues[1]) },
					{ "AXES_UNITS", 1, &(arrayValues[2]) },
				};
				//for (size_t i = 0; i < VAR_ARR_SIZE(arrayValues); i++) sap0beam0coordtimeStrArrAttrs[i].val = &arrayValues[i];
				H5G_SET_ATTRS(group, sap0beam0coordtimeStrArrAttrs, hdf5SetupStrArrayAttrs);
			}

			{
				const strKeyLongVal sap0beam0coordtimeLongAttrs[] = {
					{ "NOF_AXES", 1 },
				};
				H5G_SET_ATTRS(group, sap0beam0coordtimeLongAttrs, hdf5SetupLongAttrs);
			}

			{
				double doubleattrs[] = { 1. };
				const strKeyDoubleArrVal sap0beam0coordtimeDoubleAttrs[] = {
					{ "PC", 1, doubleattrs}
				};
				H5G_SET_ATTRS(group, sap0beam0coordtimeDoubleAttrs, hdf5SetupDoubleArrayAttrs);
			}

			H5_ERR_CHECK(status, H5Gclose(group));
		}

		// SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES/SPECTRAL
		// Missing:
		//	PC = { 1. };
		// REFERENCE_VALUE = arr dbl
		// REFERENCE_PIXEL = arr dbl

		// AXES_VALUES_PIXEL = { 0, 1, 2, 3... beamlet-1 }; // Frequncy channels in dataset
		// AXES_VALUES_WORLD = { 101e6, 102e6, 103e6... };
		// INCREMENTT = arr dbl
		{
			HDF_OPEN_GROUP(group, config->hdf5Writer.file, "/SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES/SPECTRAL");

			const strKeyStrVal sap0beam0coordtimeStrAttrs[] = {
				{ "GROUPTYPE",       "SpectralCoord" },
				{ "COORDINATE_TYPE", "Spectral" },
			};
			H5G_SET_ATTRS(group, sap0beam0coordtimeStrAttrs, hdf5SetupStrAttrs);

			{
				char *tabular[] = { "Tabular" };
				char *frequency[] = { "Frequency" };
				char *hz[] = { "Hz" };

				const strKeyStrArrVal sap0beam0coordtimeStrArrAttrs[] = {
					{ "STORAGE_TYPE", 1, tabular },
					{ "AXES_NAMES", 1, frequency},
					{"AXES_UNITS", 1, hz},
					};
				H5G_SET_ATTRS(group, sap0beam0coordtimeStrArrAttrs, hdf5SetupStrArrayAttrs);
			}

			const strKeyLongVal sap0beam0coordtimeLongAttrs[] = {
				{ "NOF_AXES", 1 },
			};
			H5G_SET_ATTRS(group, sap0beam0coordtimeLongAttrs, hdf5SetupLongAttrs);

			H5_ERR_CHECK(status, H5Gclose(group));
		}


		H5_ERR_CHECK(status, H5Sclose(space));


		char dtype[32] = "";
		switch (metadata->nbit) {
			case 8:
				config->hdf5Writer.dtype = H5Tcopy(H5T_NATIVE_INT8);
				config->hdf5Writer.elementSize = sizeof(int8_t);
				strncpy(dtype, "char", 31);
				break;

			case 16:
				config->hdf5Writer.dtype = H5Tcopy(H5T_NATIVE_INT16);
				config->hdf5Writer.elementSize = sizeof(int16_t);
				strncpy(dtype, "short", 31);
				break;

			case -32:
				config->hdf5Writer.dtype = H5Tcopy(H5T_NATIVE_FLOAT);
				config->hdf5Writer.elementSize = sizeof(float);
				strncpy(dtype, "float", 31);
				break;

			default:
				fprintf(stderr, "ERROR: Unable to initialise HDF5 dtype for unknown bitmode %d, exiting.\n", metadata->nbit);
				return -1;
		}

		if (config->hdf5Writer.dtype < 0) {
			H5Eprint(config->hdf5Writer.dtype, stderr);
			fprintf(stderr, "ERROR %s: Failed to set output dtype variable for %d bits, exiting.\n", __func__, metadata->nbit);
			return -1;
		}



		hid_t prop;
		H5_ERR_CHECK(prop, H5Pcreate(H5P_DATASET_CREATE));
		hid_t dataspace;
		int32_t rank = 2;
		hsize_t maxdims[2] = { H5S_UNLIMITED, H5S_UNLIMITED };
		hsize_t chunk_dims[2] = { 4096, 32 };
		H5_ERR_CHECK(status, H5Pset_chunk(prop, rank, chunk_dims));
		char dsetName[DEF_STR_LEN], componentStr[16] = "";
		const char delim = '-';
		char *component = NULL;

		if (bshuf_register_h5filter() < 0) {
			fprintf(stderr, "ERROR: Failed to register Bitshuffle HDF5 plugin.\n");
		} else {

			const uint32_t bitshuffleFlags[] = {
				0, // Automatically determine shape of shuffle
				BSHUF_H5_COMPRESS_ZSTD, // Use ZSTD as a post-shuffle compressor
				config->zstdConfig.compressionLevel, // ZSTD compression level
			};
			size_t numFlags = VAR_ARR_SIZE(bitshuffleFlags);

			if ((status = H5Pset_filter(prop, BSHUF_H5FILTER, H5Z_FLAG_OPTIONAL, numFlags, (const uint32_t *) bitshuffleFlags)) < 0) {
				H5Eprint(status, stderr);
				fprintf(stderr, "ERROR: Failed to find default HDF5 compression plugin (BitShuffle, %d), falling back to no compression.\n", BSHUF_H5FILTER);
			}
		}

		// SUB_ARRAY_POINTING_000/BEAM_000/STOKES[0..3]
		// Missing:
		{
			/*
			GROUP STOKES_[0...3]

				NOF_CHANNELS = { 1, 1, 1...}; // Confirm, not 100% sure
			*/
		}
		int8_t outputs = 0;
		VERBOSE(printf("Loop\n"));
		for (int8_t i = 0; i < MAX_OUTPUT_DIMS; i++) {
			if (metadata->upm_rel_outputs[i]) {
				config->hdf5Writer.hdf5DSetWriter[outputs].dims[0] = 0;
				config->hdf5Writer.hdf5DSetWriter[outputs].dims[1] = metadata->nchan;
				H5_ERR_CHECK(dataspace, H5Screate_simple(rank, config->hdf5Writer.hdf5DSetWriter->dims, maxdims));

				if (snprintf(dsetName, DEF_STR_LEN - 1, "/SUB_ARRAY_POINTING_000/BEAM_000/STOKES_%d", i) < 1) {
					fprintf(stderr, "ERROR: Failed to print dataset name for dset %d, exiting.\n", i);
					return -1;
				}
				H5_ERR_CHECK(config->hdf5Writer.hdf5DSetWriter[outputs].dset, H5Dcreate(config->hdf5Writer.file, dsetName, config->hdf5Writer.dtype, dataspace, H5P_DEFAULT, prop, H5P_DEFAULT));
				H5_ERR_CHECK(status, H5Sclose(dataspace));

				if (strncpy(componentStr, metadata->upm_outputfmt[outputs], 15) != componentStr) {
					fprintf(stderr, "ERROR: Failed to make a copy of format comment %d (%d) for HDF5 dataset STOKES_COMPONENT, exiting.\n", i, outputs);
					return -1;
				}
				if ((component = strtok(componentStr, &delim)) == NULL) {
					fprintf(stderr, "ERROR: Failed to parse format comment %d (%d) for HDF5 dataset STOKES_COMPONENT, exiting.\n", i, outputs);
					return -1;
				}

				const strKeyStrVal dsetStrAttrs[] = {
					{ "GROUPTYPE", "bfData" },
				};
				H5D_SET_ATTRS(config->hdf5Writer.hdf5DSetWriter[outputs].dset, dsetStrAttrs, hdf5SetupStrAttrs);

				const strKeyStrPtrVal dsetStrPtrAttrs[] = {
					{ "DATATYPE", dtype },
					{ "STOKES_COMPONENT", component },
				};
				H5D_SET_ATTRS(config->hdf5Writer.hdf5DSetWriter[outputs].dset, dsetStrPtrAttrs, hdf5SetupStrPtrAttrs);

				const strKeyLongVal dsetLongAttrs[] = {
					{ "NOF_SAMPLES", 0 },
					{ "NOF_SUBBANDS", metadata->nchan },
				};
				H5D_SET_ATTRS(config->hdf5Writer.hdf5DSetWriter[outputs].dset, dsetLongAttrs, hdf5SetupLongAttrs);

				outputs++;
			}
		}
		config->hdf5Writer.metadataInitialised = 1;
		VERBOSE(printf("Complete.\n"));
	}

	// Variables that should be updated on every tick
	// Maybe make this a configurable option? e.g., only update every N writes?
	// Root group /

	// TOTAL_INTEGRATION_TIME dbl
	// OBSERVATION_END_MJD double
	// EXPTIME_STOP_MJD double


	HDF_OPEN_GROUP(group, config->hdf5Writer.file, "/");

	const strKeyStrVal rootStrAttrs[] = {
		{ "OBSERVATION_END_TAI", "NULL" }, // TODO
		{ "OBSERVATION_END_UTC", "NULL" }, // TODO
		{ "EXPTIME_STOP_TAI", "NULL" }, // TODO
		{ "EXPTIME_STOP_UTC", "NULL" }, // TODO
	};
	H5G_SET_ATTRS(group, rootStrAttrs, hdf5SetupStrAttrs);

	const double integrationTime = ((double) (metadata->upm_processed_packets * UDPNTIMESLICE / metadata->upm_num_inputs)) * metadata->tsamp_raw;
	const double endMjd = metadata->obs_mjd_start + integrationTime / 86400.0;
	const strKeyDoubleVal rootDoubleAttrs[] = {
		{"TOTAL_INTEGRATION_TIME", integrationTime},
		{"OBSERVATION_END_MJD", endMjd},
		{"EXPTIME_STOP_MJD", endMjd}
	};
	H5G_SET_ATTRS(group, rootDoubleAttrs, hdf5SetupDoubleAttrs);

	H5_ERR_CHECK(status, H5Gclose(group));


	// /SUB_ARRAY_POINTING_000
	HDF_OPEN_GROUP(group, config->hdf5Writer.file, "/SUB_ARRAY_POINTING_000");

	const strKeyLongVal sap0LongAttrs[] = {
		{ "NOF_SAMPLES", config->hdf5Writer.hdf5DSetWriter[0].dims[1] },
	};
	H5G_SET_ATTRS(group, sap0LongAttrs, hdf5SetupLongAttrs);

	H5_ERR_CHECK(status, H5Gclose(group));

	return 0;
}

/**
 * @brief      Perform a data write for a HDF5 file
 *
 * @param[in]  config  The configuration
 * @param[in]  outp    The outp
 * @param[in]  src     The source
 * @param[in]  nchars  The nchars
 *
 * @return  >=0: Number of bytes written, <0: Failure
 */
int64_t _lofar_udp_io_write_HDF5(lofar_udp_io_write_config *const config, const int8_t outp, const int8_t *src, const int64_t nchars) {
	if (!config->hdf5Writer.initialised || !config->hdf5Writer.metadataInitialised) {
		fprintf(stderr, "ERROR %s: HDF5 writer not fully initialised (writer: %d, metadata: %d), exiting.\n", __func__, config->hdf5Writer.initialised, config->hdf5Writer.metadataInitialised);
		return -1;
	}
	hsize_t offsets[2];
	offsets[0] = config->hdf5Writer.hdf5DSetWriter[outp].dims[0];
	offsets[1] = 0;

	VERBOSE(printf("%ld, %lld, %ld\n", nchars, config->hdf5Writer.hdf5DSetWriter[outp].dims[1], config->hdf5Writer.elementSize));
	hsize_t extension[2] = { nchars / config->hdf5Writer.hdf5DSetWriter[outp].dims[1] / config->hdf5Writer.elementSize,
						        config->hdf5Writer.hdf5DSetWriter[outp].dims[1] };
	VERBOSE(printf("Preparing to extend HDF5 dataset %d by %lld samples (%ld bytes / %lld chans).\n", outp, extension[0], nchars, config->hdf5Writer.hdf5DSetWriter[outp].dims[1]));
	config->hdf5Writer.hdf5DSetWriter[outp].dims[0] += extension[0];

	VERBOSE(printf("Getting extension for (%lld, %lld)\n", config->hdf5Writer.hdf5DSetWriter[outp].dims[0], config->hdf5Writer.hdf5DSetWriter[outp].dims[1]));
	hid_t status;
	H5_ERR_CHECK(status, H5Dset_extent(config->hdf5Writer.hdf5DSetWriter[outp].dset, config->hdf5Writer.hdf5DSetWriter[outp].dims));


	VERBOSE(printf("Getting space\n"));
	hid_t filespace;
	H5_ERR_CHECK(filespace, H5Dget_space(config->hdf5Writer.hdf5DSetWriter[outp].dset));
	VERBOSE(printf("Getting hyperslab\n"));
	H5_ERR_CHECK(status, H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offsets, NULL, extension, NULL));
	VERBOSE(printf("Getting memspace\n"));
	hid_t memspace;
	H5_ERR_CHECK(memspace, H5Screate_simple(2, extension, NULL));

	VERBOSE(printf("Writing..\n"));
	if ((status = H5Dwrite(config->hdf5Writer.hdf5DSetWriter[outp].dset, config->hdf5Writer.dtype, memspace, filespace, H5P_DEFAULT, src)) < 0) {
		H5Eprint(status, stderr);
		fprintf(stderr, "ERROR: Failed to write %ld chars to HDF5 dataset %d, exiting.\n", nchars, outp);
		return -1;
	}

	H5_ERR_CHECK(status, H5Sclose(filespace));
	H5_ERR_CHECK(status, H5Sclose(memspace));

	return nchars;
}

/**
 * @brief      Cleanup HDF5 file references for the write I/O struct
 *
 * @param      config     The configuration
 * @param[in]  outp       The outp
 * @param[in]  fullClean  Perform a full clean, dealloc the ringbuffer
 */
void _lofar_udp_io_write_cleanup_HDF5(lofar_udp_io_write_config *const config, const int8_t outp, const int8_t fullClean) {
	if (config == NULL || config->hdf5Writer.file == -1) {
		return;
	}

	if (!fullClean) {
		H5Dclose(config->hdf5Writer.hdf5DSetWriter[outp].dset);
		config->hdf5Writer.hdf5DSetWriter[outp].dset = -1;
	} else {
		for (int8_t out = 0; out < config->numOutputs; out++) {
			H5Dclose(config->hdf5Writer.hdf5DSetWriter[out].dset);
			config->hdf5Writer.hdf5DSetWriter[out].dset = -1;
		}
		H5Fclose(config->hdf5Writer.file);
		H5Tclose(config->hdf5Writer.dtype);

		config->hdf5Writer.file = -1;
	}
}

/**
 * @brief Convert an RCU mode to a band string
 *
 * @param rcumode Active RCU mode
 *
 * @return Band string representation
 */
const char* get_rcumode_str(const int32_t rcumode) {
	switch (rcumode) {
		// Base values of bands
		case 3:
			return "LBA_10_90";
		case 4:
			return "LBA_30_90";

		case 5:
			return "HBA_110_190";
		case 7:
			return "HBA_210_250";

		case 6:
			return "HBA_170_230";


		default:
			fprintf(stderr, "ERROR: Failed to determine RCU mode (base int of %d).\n", rcumode);
			return "UNKNOWN";
	}
}

// Undefine all local macros
#undef HDF_OPEN_GROUP
#undef _H5_SET_ATTRS
#undef H5G_SET_ATTRS
#undef H5D_SET_ATTRS
#undef H5_ERR_CHECK
#undef H5_ERR_CHECK_RETURN
#undef shortStrLen

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
