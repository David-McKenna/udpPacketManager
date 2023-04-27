#include "lofar_udp_calibration.h"

/**
 * @brief Verify the reader configuration can support calibration
 *
 * @param reader The input reader
 *
 * @return 0: Success, other:failure
 */
int32_t _lofar_udp_calibration_sanity_checks(const lofar_udp_reader *reader) {
	// Ensure we are meant to be calibrating data
	if (reader == NULL || reader->meta == NULL || reader->metadata == NULL) {
		fprintf(stderr, "ERROR: Calibration cannot proceed as we have been passed a nullptr, exiting.\n");
		return 1;
	}

	if (reader->meta->calibrateData == NO_CALIBRATION) {
		fprintf(stderr, "ERROR: Requested calibration while calibration is disabled. Exiting.\n");
		return -1;
	}

	if (reader->metadata->ra_rad == -1.0 || reader->metadata->dec_rad == -1.0) {
		fprintf(stderr, "ERROR %s: Metadata not initialised, cannot generate calibration data, exiting.\n", __func__);
		return 1;
	}

	if (!reader->meta->clockBit) {
		fprintf(stderr, "ERROR: Mode 6 is currently not supported for calibration, exiting.\n");
		return 1;
	}

	if (reader->calibration->calibrationDuration <= 0.0f) {
		fprintf(stderr, "ERROR: Passed invalid duration for calibration (%fs), exiting.\n", reader->calibration->calibrationDuration);
		return 1;
	}

	return 0;
}

/**
 * @brief Searches the current path directories for the working script name
 *
 * @param jonesGeneratorName Name of the script to search for
 * @param outputJonesGenerator A buffer to store the path of the first found matching location
 *
 * @return 0: Success, <0: Failure
 */
int32_t _lofar_udp_calibration_find_script(const char jonesGeneratorName[], char outputJonesGenerator[DEF_STR_LEN]) {

	// Make a copy of the path so that we don't destroy it with strtok
	char pathPath[DEF_STR_LEN];
	if (strncpy(pathPath, getenv("PATH"), DEF_STR_LEN) != pathPath) {
		fprintf(stderr, "ERROR: Failed to make a copy of path variable, exiting.\n");
		return -1;
	}

	// Split directories by ':', not portable, but good enough
	char *workingPtr, *currPtr = strtok_r(pathPath, ":", &workingPtr);

	// Iterate through the paths, checking to see if an executable script with the given name is available
	while (currPtr) {
		snprintf(outputJonesGenerator, DEF_STR_LEN - 1, "%s/%s", currPtr, jonesGeneratorName);
		if (!access(outputJonesGenerator, X_OK)) {
			break;
		}
		currPtr = strtok_r(NULL, ":", &workingPtr);
	}

	// currPtr will be NULL on failure
	if (!currPtr) {
		fprintf(stderr, "ERROR %s: Unable to find executable %s on path.\n", __func__, jonesGeneratorName);
		fprintf(stderr, "Current PATH variable: %s\n", pathPath);
		fprintf(stderr, "Exiting.\n");
		return -1;
	}

	return 0;
}

/**
 * @brief Setup a shmData struct to hold a shared memory buffer of a given size at a given location
 *
 * @param sharedData The struct to configure
 *
 * @return 0: Success, <0: Failure
 */
int32_t _lofar_udp_calibration_shmData_setup(shmData *sharedData) {
	// Re-seed randomness based on the given time
	srand(time(0));

	// Fill the remaining characters in the string buffer with random ASCII letters for security
	const int32_t shmNameLen = strnlen(sharedData->shmName, VAR_ARR_SIZE(sharedData->shmName)) + 1;
	sharedData->shmName[shmNameLen - 1] = '_';
	const int32_t shmNameLimit = VAR_ARR_SIZE(sharedData->shmName) - 1;
	for (int32_t i =  shmNameLen; i < shmNameLimit; i++) {
		sharedData->shmName[i + 1] = '\0';
		sharedData->shmName[i] = (uint8_t) ('a' + rand() % 26 + ('A' - 'a') * (rand() % 2));
	}

	// Attempt to open the given path
	sharedData->shmFd = shm_open(sharedData->shmName, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);

	if (sharedData->shmFd == -1) {
		// If it exists, attempt to re-use and re-allocate the buffer where possible
		if (errno == EEXIST) {
			fprintf(stderr, "WARNING: Shared memory for calibration already existed at %s, closing and attempting to continue.\n", sharedData->shmName);

			sharedData->shmFd = shm_open(sharedData->shmName, O_RDWR, S_IRUSR | S_IWUSR);
			if (sharedData->shmFd == -1) {
				fprintf(stderr, "ERROR: Failed to open previously existing calibration buffer at %s  (%d: %s), exiting.\n", sharedData->shmName, errno, strerror(errno));
				shm_unlink(sharedData->shmName);
				return 1;
			}
			struct stat shmStat;
			if (fstat(sharedData->shmFd, &shmStat) == -1) {
				fprintf(stderr, "ERROR: Failed to probe previous calibration buffer at %s  (%d: %s), exiting.\n", sharedData->shmName, errno, strerror(errno));
				shm_unlink(sharedData->shmName);
				return 1;
			}

			void* tmpPtr = mmap(NULL, shmStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, sharedData->shmFd, 0);
			if (tmpPtr == MAP_FAILED) {
				fprintf(stderr, "ERROR: Failed to remap shared memory at %s (%d:%s), exiting.\n", sharedData->shmName, errno, strerror(errno));
				shm_unlink(sharedData->shmName);
				return 1;
			}
			munmap(tmpPtr, shmStat.st_size);
			shm_unlink(sharedData->shmName);
			return _lofar_udp_calibration_shmData_setup(sharedData);

		}
		fprintf(stderr, "ERROR: Failed to open shared memory at %s (%d: %s), exiting.\n", sharedData->shmName, errno, strerror(errno));
		return 1;
	}

	// Expand the shared memory to the necessary size
	if (ftruncate(sharedData->shmFd, sharedData->shmSize)) {
		fprintf(stderr, "ERROR: Failed to resize shared memory at %s to %ld bytes (%d: %s), exiting.\n", sharedData->shmName, sharedData->shmSize, errno, strerror(errno));
		return 1;
	}

	// mmap the sharedmemory into a generic pointer
	sharedData->shmPtr = mmap(NULL, sharedData->shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, sharedData->shmFd, 0);
	if (sharedData->shmPtr == MAP_FAILED) {
		fprintf(stderr, "ERROR: Failed to mmap shared memory array (%d: %s), exiting.\n", errno, strerror(errno));
		return 1;
	}

	return 0;
}

/**
 * @brief Spawn the calibration script and pass it the required arguments
 *
 * @param reader Reader configuration
 * @param jonesGenerator Script location
 * @param numTimesteps Number of time steps to generate
 * @param integrationTime The integration time of each step
 * @param sharedData The shmstruct
 *
 * @return 0: Success, <0: Failure
 */
int32_t _lofar_udp_calibration_spawn_python_script(const lofar_udp_reader *reader, char jonesGenerator[DEF_STR_LEN], int32_t numTimesteps, float integrationTime, shmData *sharedData) {
	const int32_t shortStr = 31, longStr = 511, longestStr = 2 * DEF_STR_LEN - 1;
	char stationID[shortStr + 1], mjdTime[shortStr + 1], duration[shortStr + 1], shmSize[shortStr + 1], integration[shortStr + 1], pointing[longStr + 1];


	// Copy strings into working arrays
	_lofar_udp_metadata_get_station_name(reader->meta->stationID, &(stationID[0]));
	snprintf(mjdTime, shortStr,  "%.16lf", lofar_udp_time_get_packet_time_mjd(reader->meta->inputData[0]));
	snprintf(duration, shortStr, "%.10f", reader->calibration->calibrationDuration + 0.5 * integrationTime); // Work around float precision loss for small values
	snprintf(shmSize, shortStr, "%ld", sharedData->shmSize);
	snprintf(integration, shortStr, "%15.10f", integrationTime);
	snprintf(pointing, longStr, "%f,%f,%s", reader->metadata->ra_rad, reader->metadata->dec_rad, reader->metadata->coord_basis);


	// Build a list of comma separated subbands, in their offset beamlet order (accounting for rcumodes)
	char calibrationSubbands[longestStr + 1];
	snprintf(calibrationSubbands, longestStr, "%d", reader->metadata->subbands[reader->metadata->lowerBeamlet]);
	int32_t strOffset = strnlen(calibrationSubbands, longestStr);
	for (int16_t sbb = reader->metadata->lowerBeamlet + 1; sbb < reader->metadata->upperBeamlet; sbb++) {
		int16_t sub = reader->metadata->subbands[sbb];
		// TODO: Better memory safety / raise errors if string is too small
		strOffset += snprintf(&(calibrationSubbands[strOffset]), longestStr - strOffset, ",%d", sub);
	}


	// Generate the argv array
	char * const argv[] = { jonesGenerator, "--stn", stationID, "--time", mjdTime,
	                        "--sub", calibrationSubbands,
	                        "--dur", duration, "--int", integration, "--pnt", pointing,
	                        "--shm_key", sharedData->shmName, "--shm_size", shmSize,
	                        NULL };
	VERBOSE(printf("Generating %d Jones matrices (%s-8 bytes) via %s @ %s...\n", numTimesteps, shmSize, jonesGenerator, sharedData->shmName);
		{char * const *tmpPtr = argv; while (*tmpPtr != NULL) {
		printf("%s ", *tmpPtr); tmpPtr++;
	}; printf("\n");});


	// Spawn and check that the thread executes as expected
	pid_t pid;
	int32_t returnVal = posix_spawnp(&pid, jonesGenerator, NULL, NULL, argv, environ);

	// Verify the process launched
	if (returnVal == 0) {
		VERBOSE(printf("%s has been launched.\n", jonesGenerator));
	} else if (returnVal < 0) {
		fprintf(stderr, "ERROR: Unable to create child process to call %s. Exiting.\n", jonesGenerator);
		return -1;
	}

	// Wait for the process to exit, check if there were any errors.
	int32_t processReturnVal;
	if ((returnVal = waitpid(pid, &processReturnVal, 0)) < 0 || (WIFEXITED(processReturnVal) && WEXITSTATUS(processReturnVal) != 0)) {
		fprintf(stderr, "ERROR: Child %s raised error %d/%d, exiting.\n", jonesGenerator, returnVal, processReturnVal);
		return -1;
	}

	return 0;
}

/**
 * @brief Process the Jones Matrices from the shared memory buffer into a local calloc'd buffer
 *
 * @param reader Reader struct
 * @param sharedData shm struct
 * @param expectedBeamlets Expected number of subbands of data
 * @param expectedTimesamples Expected number of time samples of data
 *
 * @return 0: Success, <0: Failure
 */
int32_t _lofar_udp_calibration_shmData_process(lofar_udp_reader *reader, shmData *sharedData, int32_t expectedBeamlets, int32_t expectedTimesamples) {
	// Ensure the calibration strategy matches the number of subbands we are processing
	// The first 8 bytes contain the number of written beamlets / time samples
	int32_t writtenTimesamples = (((float *)sharedData->shmPtr)[0]);
	int32_t writtenBeamlets = (((float *)sharedData->shmPtr)[1]);
	if (expectedTimesamples != writtenTimesamples || expectedBeamlets != writtenBeamlets) {
		fprintf(stderr, "ERROR: Mismatch in requested calibration data and provided calibration data (%d/%d, %d/%d), exiting.\n", writtenTimesamples, expectedTimesamples, writtenBeamlets, expectedBeamlets);
		return -1;
	}

	float **tmpPtr;
	// Check if we already allocated storage for the calibration data, re-use already alloc'd data where possible
	if (reader->meta->jonesMatrices == NULL) {
		// Allocate expectedTimesamples * numBeamlets * (4 matrix elements) * (2 complex values per element)
		reader->meta->jonesMatrices = calloc(expectedTimesamples, sizeof(float *));
		for (int timeIdx = 0; timeIdx < expectedTimesamples; timeIdx += 1) {
			reader->meta->jonesMatrices[timeIdx] = calloc(expectedBeamlets * 8, sizeof(float));
		}
	// If we returned more time samples than last time, reallocate the array
	} else if (expectedTimesamples > reader->calibration->calibrationStepsGenerated) {
		// Reallocate the data
		if ((tmpPtr = realloc(reader->meta->jonesMatrices, expectedTimesamples* sizeof(float *))) == NULL) {
			fprintf(stderr, "WARNING %s: Realloc failed, rebuilding calibration array from scratch.\n", __func__);
			// Free the old data
			for (int timeIdx = 0; timeIdx < reader->calibration->calibrationStepsGenerated; timeIdx += 1) {
				FREE_NOT_NULL(reader->meta->jonesMatrices[timeIdx]);
			}
			FREE_NOT_NULL(reader->meta->jonesMatrices);
			// Recursive call
			return _lofar_udp_calibration_shmData_process(reader, sharedData, expectedBeamlets, expectedTimesamples);
		}

		reader->meta->jonesMatrices = tmpPtr;
		for (int timeIdx = reader->calibration->calibrationStepsGenerated; timeIdx < expectedTimesamples; timeIdx += 1) {
			reader->meta->jonesMatrices[timeIdx] = calloc(expectedBeamlets * 8, sizeof(float));
		}
	// If there were less time samples, free the remaining steps and re-use the array
	} else if (expectedTimesamples < reader->calibration->calibrationStepsGenerated) {
		// Free the extra time steps
		for (int timeIdx = reader->calibration->calibrationStepsGenerated; timeIdx < expectedTimesamples; timeIdx += 1) {
			FREE_NOT_NULL(reader->meta->jonesMatrices[timeIdx]);
		}
		if ((tmpPtr = realloc(reader->meta->jonesMatrices, expectedTimesamples * sizeof(float *))) == NULL) {
			fprintf(stderr, "ERROR: Failed to shrink Jones matrix array, exiting.");
			for (int timeIdx = 0; timeIdx < expectedTimesamples; timeIdx += 1) {
				FREE_NOT_NULL(reader->meta->jonesMatrices[timeIdx]);
			}
			FREE_NOT_NULL(reader->meta->jonesMatrices);
			return 1;
		}
		reader->meta->jonesMatrices = tmpPtr;

	}

	// Extract the data
	float *headPointer = &((float *) sharedData->shmPtr)[2];
	const int64_t samplesPerStep = expectedBeamlets * JONESMATSIZE;
	for (int32_t ts = 0; ts < expectedTimesamples; ts++) {
		// Copy a full sample at a time
		if (memcpy(reader->meta->jonesMatrices[ts], headPointer, samplesPerStep * sizeof(float)) != reader->meta->jonesMatrices[ts]) {
			fprintf(stderr, "ERROR: Failed to copy calibration data for sample %d, exiting.\n", ts);
			for (int timeIdx = 0; timeIdx < expectedTimesamples; timeIdx += 1) {
				FREE_NOT_NULL(reader->meta->jonesMatrices[timeIdx]);
			}
			FREE_NOT_NULL(reader->meta->jonesMatrices);
			return -1;
		}
		// Update to the new head of the array
		headPointer = &(headPointer[samplesPerStep]);
	}

	return 0;
}

/**
 * @brief Cleanup a shm struct, closing open shared memory buffers
 *
 * @param sharedData shm struct
 */
void _lofar_udp_calibration_shmData_cleanup(shmData *sharedData) {
	if (sharedData != NULL) {
		// Unmap the /dev/shm file
		if (sharedData->shmPtr != NULL) {
			munmap(sharedData->shmPtr, sharedData->shmSize);
			sharedData->shmPtr = NULL;
		}

		// Close the /dev/shm file
		if (sharedData->shmFd != -1) {
			close(sharedData->shmFd);
			sharedData->shmFd = -1;
		}

		// Unlink (destroy) the /dev/shm file
		if (strnlen(sharedData->shmName, 31)) {
			shm_unlink(sharedData->shmName);
		}
	}
}


/**
 * @brief      Generate the inverted Jones matrix to calibrate the observed data
 *
 * @param      reader  The lofar_udp_reader
 *
 * @return     int: 0: Success, -1: Failure
 */
int32_t _lofar_udp_reader_calibration_generate_data(lofar_udp_reader *reader) {
	int32_t returnVal;

	if ((returnVal = _lofar_udp_calibration_sanity_checks(reader))) {
		return returnVal;
	}

	char* const jonesGeneratorName = "dreamBeamJonesGenerator.py";
	char jonesGenerator[DEF_STR_LEN] = "";

	if ((returnVal = _lofar_udp_calibration_find_script(jonesGeneratorName, jonesGenerator))) {
		return returnVal;
	}

	// Calculate the parameters needed to meet the configuration requirements
	float integrationTime = (float) (reader->packetsPerIteration * UDPNTIMESLICE) *
	                        (float) (clock200MHzSampleTime * reader->meta->clockBit +
	                                 clock160MHzSampleTime * (1 - reader->meta->clockBit));
	int32_t numTimesteps = (reader->calibration->calibrationDuration / integrationTime) + 1;
	int64_t dataShmSize = (numTimesteps * reader->meta->totalProcBeamlets * JONESMATSIZE + 2) * sizeof(float); // Data + 2 validation ints

	shmData sharedData = {
		.shmName = "/dreambeamJonesCalSHM",
		.shmSize = dataShmSize,

		.shmFd = -1,
		.shmPtr = NULL,
	};

	if ((returnVal = _lofar_udp_calibration_shmData_setup(&sharedData))) {
		_lofar_udp_calibration_shmData_cleanup(&sharedData);
		return returnVal;
	}


	if ((returnVal = _lofar_udp_calibration_spawn_python_script(reader, jonesGenerator, numTimesteps, integrationTime, &sharedData)) != 0) {
		_lofar_udp_calibration_shmData_cleanup(&sharedData);
		return returnVal;
	}

	if ((returnVal = _lofar_udp_calibration_shmData_process(reader, &sharedData, reader->meta->totalProcBeamlets, numTimesteps)) > 0) {
		_lofar_udp_calibration_shmData_cleanup(&sharedData);
		return returnVal;
	}

	_lofar_udp_calibration_shmData_cleanup(&sharedData);

	// Update the calibration state
	reader->meta->calibrationStep = -1; // We will bump this on the next usage so that it starts at 0
	reader->calibration->calibrationStepsGenerated = numTimesteps;
	VERBOSE(printf("%s: Exit calibration.\n", __func__););
	return returnVal;
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
