#ifndef LOFAR_UDP_CALIBRATION_H
#define LOFAR_UDP_CALIBRATION_H

#include "lofar_udp_metadata.h"

#include <spawn.h>
#include <wait.h>
#include <fcntl.h>
#include <semaphore.h>

extern char **environ; // NOLINT(readability-redundant-declaration)

typedef struct {
	// Initialisers
	char shmName[32];
	int64_t shmSize;

	// Working variables
	int32_t shmFd;
	void *shmPtr;
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
