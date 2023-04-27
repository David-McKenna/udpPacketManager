#ifndef LOFAR_UDP_CALIBRATION_H
#define LOFAR_UDP_CALIBRATION_H

#include "lofar_udp_metadata.h"

#include <spawn.h>
#include <wait.h>
#include <fcntl.h>
#include <semaphore.h>

// Access environment variables for python script path search
extern char **environ; // NOLINT(readability-redundant-declaration)

typedef struct {
	// Initialisers
	char shmName[32]; // /dev/shm/shmName
	int64_t shmSize; // Allocated ptr buffer size

	// Working variables
	int32_t shmFd; // fileno
	void *shmPtr; // Buffer
} shmData;

#ifdef __cplusplus
extern "C" {
#endif

int32_t _lofar_udp_reader_calibration_generate_data(lofar_udp_reader *reader);

int32_t _lofar_udp_calibration_sanity_checks(const lofar_udp_reader *reader);
int32_t _lofar_udp_calibration_find_script(const char jonesGeneratorName[], char outputJonesGenerator[DEF_STR_LEN]);
int32_t _lofar_udp_calibration_shmData_setup(shmData *sharedData);
int32_t _lofar_udp_calibration_spawn_python_script(const lofar_udp_reader *reader, char jonesGenerator[DEF_STR_LEN], int32_t numTimesteps, float integrationTime, shmData *sharedData);
int32_t _lofar_udp_calibration_shmData_process(lofar_udp_reader *reader, shmData *sharedData, int32_t expectedBeamlets, int32_t expectedTimesamples);
void _lofar_udp_calibration_shmData_cleanup(shmData *sharedData);

#ifdef __cplusplus
}
#endif

#endif


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
