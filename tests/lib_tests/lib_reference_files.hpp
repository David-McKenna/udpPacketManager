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
const int32_t numTests = inputLocations.size();

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
const int32_t numMeta = metadataLocation.size();


const std::vector<int32_t> processingModes {
	(int32_t) TEST_INVALID_MODE,
	0, 1, 2, 10, 11, 20, 21, 30, 31, 32, 35, 100, 110, 120, 130, 150, 160,
	101, 102, 103, 104, 111, 112, 113, 114, 121, 122, 123, 124, 131, 132,
	133, 134, 151, 152, 153, 154, 161, 162, 163, 164, 200, 210, 220, 230, 250, 260,
	201, 202, 203, 204, 211, 212, 213, 214, 221, 222, 223, 224, 231, 232,
	233, 234, 251, 252, 253, 254, 261, 262, 263, 264, 300, 310, 320, 330, 350, 360,
	301, 302, 303, 304, 311, 312, 313, 314, 321, 322, 323, 324, 331, 332,
	333, 334, 351, 352, 353, 354, 361, 362, 363, 364,

};

const int32_t invalidBitMode = 111;
const std::vector<int32_t> bitModes { 4, 8, 16, invalidBitMode };
const calibrate_t invalidCalibration = (calibrate_t) 33;
const std::vector<calibrate_t> calibrationModes { NO_CALIBRATION, GENERATE_JONES, APPLY_CALIBRATION, (calibrate_t) invalidCalibration };


#endif //LIB_REFERENCE_FILES_HPP
