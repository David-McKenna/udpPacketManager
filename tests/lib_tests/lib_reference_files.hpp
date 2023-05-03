//
// Created by suddenly on 31/03/23.
//

#ifndef LIB_REFERENCE_FILES_HPP
#define LIB_REFERENCE_FILES_HPP

#include "lofar_udp_general.h"
#include <stdint.h>
#include <string>
#include <vector>

const int32_t numPorts = 4;
const std::vector<std::string> inputLocations {
	"",
	"referenceFiles/testCase1/udp_1613portnum.ucc1.2022-06-29T01:30:00.000",
	"referenceFiles/testCase2/udp_1613portnum.ucc1.2022-06-29T01:30:00.000.zst",
	"referenceFiles/testCase3/udp_1613portnum.ucc1.2022-06-29T01:30:00.000",
	"referenceFiles/testCase4/udp_1613portnum.ucc1.2022-06-29T01:30:00.000",
	"referenceFiles/testCase5/udp_1613portnum.ucc1.2022-06-29T01:30:00.000",
	"referenceFiles/testCase6/udp_1613portnum.ucc1.2022-06-29T01:30:00.000",
	"referenceFiles/testCase7/udp_1613portnum.ucc1.2022-06-29T01:30:00.000",
	"referenceFiles/testCase8/udp_1613portnum.ucc1.2022-06-29T01:30:00.000",
	"referenceFiles/testCase9/udp_1613portnum.ucc1.2022-06-29T01:30:00.000",
};
const int32_t numTests = (int32_t) inputLocations.size();
const std::vector<int32_t> flaggedTests = {0, 5, 6};


const std::vector<std::string> metadataLocation {
	"", // 0
	"referenceFiles/metadata_3.h", // 1
	"referenceFiles/metadata_5.h", // 2
	"referenceFiles/metadata_6.h", // 3
	"referenceFiles/metadata_7.h", // 4
	"referenceFiles/metadata_357.h", // 5
	"referenceFiles/metadata.yml", // 6
	"referenceFiles/metadata_sbb_blt_mismatch.h", // 7
	"referenceFiles/metadata_no_sbb.h", // 8
	"referenceFiles/metadata_no_conf_data.h.yml", // 9
};
const int32_t numMeta = (int32_t) metadataLocation.size();


const std::vector<processMode_t> processingModes {
	TEST_INVALID_MODE,

	UNSET_MODE, PACKET_FULL_COPY, PACKET_NOHDR_COPY, PACKET_SPLIT_POL, BEAMLET_MAJOR_FULL,
	BEAMLET_MAJOR_SPLIT_POL, BEAMLET_MAJOR_FULL_REV, BEAMLET_MAJOR_SPLIT_POL_REV, TIME_MAJOR_FULL,
	TIME_MAJOR_SPLIT_POL, TIME_MAJOR_ANT_POL, TIME_MAJOR_ANT_POL_FLOAT, TEST_INVALID_MODE,

	STOKES_I, STOKES_Q, STOKES_U, STOKES_V, STOKES_IQUV, STOKES_IV, STOKES_I_DS2, STOKES_Q_DS2,
	STOKES_U_DS2, STOKES_V_DS2, STOKES_IQUV_DS2, STOKES_IV_DS2, STOKES_I_DS4, STOKES_Q_DS4, STOKES_U_DS4,
	STOKES_V_DS4, STOKES_IQUV_DS4, STOKES_IV_DS4, STOKES_I_DS8, STOKES_Q_DS8, STOKES_U_DS8, STOKES_V_DS8,
	STOKES_IQUV_DS8, STOKES_IV_DS8, STOKES_I_DS16, STOKES_Q_DS16, STOKES_U_DS16, STOKES_V_DS16, STOKES_IQUV_DS16,
	STOKES_IV_DS16,

	STOKES_I_REV, STOKES_Q_REV, STOKES_U_REV, STOKES_V_REV, STOKES_IQUV_REV, STOKES_IV_REV, STOKES_I_DS2_REV,
	STOKES_Q_DS2_REV, STOKES_U_DS2_REV, STOKES_V_DS2_REV, STOKES_IQUV_DS2_REV, STOKES_IV_DS2_REV, STOKES_I_DS4_REV,
	STOKES_Q_DS4_REV, STOKES_U_DS4_REV, STOKES_V_DS4_REV, STOKES_IQUV_DS4_REV, STOKES_IV_DS4_REV, STOKES_I_DS8_REV,
	STOKES_Q_DS8_REV, STOKES_U_DS8_REV, STOKES_V_DS8_REV, STOKES_IQUV_DS8_REV, STOKES_IV_DS8_REV, STOKES_I_DS16_REV,
	STOKES_Q_DS16_REV, STOKES_U_DS16_REV, STOKES_V_DS16_REV, STOKES_IQUV_DS16_REV, STOKES_IV_DS16_REV,

	STOKES_I_TIME, STOKES_Q_TIME, STOKES_U_TIME, STOKES_V_TIME, STOKES_IQUV_TIME, STOKES_IV_TIME, STOKES_I_DS2_TIME,
	STOKES_Q_DS2_TIME, STOKES_U_DS2_TIME, STOKES_V_DS2_TIME, STOKES_IQUV_DS2_TIME, STOKES_IV_DS2_TIME,
	STOKES_I_DS4_TIME, STOKES_Q_DS4_TIME, STOKES_U_DS4_TIME, STOKES_V_DS4_TIME, STOKES_IQUV_DS4_TIME,
	STOKES_IV_DS4_TIME, STOKES_I_DS8_TIME, STOKES_Q_DS8_TIME, STOKES_U_DS8_TIME, STOKES_V_DS8_TIME,
	STOKES_IQUV_DS8_TIME, STOKES_IV_DS8_TIME, STOKES_I_DS16_TIME, STOKES_Q_DS16_TIME, STOKES_U_DS16_TIME,
	STOKES_V_DS16_TIME, STOKES_IQUV_DS16_TIME, STOKES_IV_DS16_TIME,

};

const int8_t invalidBitMode = 111;
const std::vector<int8_t> bitModes { 4, 8, 16, invalidBitMode };
const calibrate_t invalidCalibration = (calibrate_t) 33;
const std::vector<calibrate_t> calibrationModes { NO_CALIBRATION, GENERATE_JONES, APPLY_CALIBRATION, invalidCalibration };


#endif //LIB_REFERENCE_FILES_HPP
