#include "gtest/gtest.h"
#include <unordered_map> // hasher
#include "lofar_udp_metadata.h"
#include "lofar_udp_structs.h"
#include "lofar_udp_reader.h"
#include "lib_reference_files.hpp"

TEST(LibMetadataTests, HelperFunctions) {
	SCOPED_TRACE("lofar_udp_metadata_helper_functions");

	{
		// metadata_t lofar_udp_metadata_parse_type_output(const char optargc[]);
		// metadata_t lofar_udp_metadata_string_to_meta(const char input[]);
		SCOPED_TRACE("lofar_udp_metadata_parse_type_output");
		for (auto const [val, keys] : std::map<std::string, std::pair<metadata_t, metadata_t>>{
			{"", {NO_META, NO_META}},
			{"no_hints_here", {NO_META, NO_META}},
			{"GUPPI:test", {GUPPI, GUPPI}},
			{"DADA:test", {DADA, DADA}},
			{"THISISANINPUT.fil", {SIGPROC, NO_META}},
			{"SIGPROC:notafil", {SIGPROC, SIGPROC}},
			{"thisisa.h5", {HDF5_META, NO_META}},
			{"thisisa.hdf5", {HDF5_META, NO_META}},
			{"HDF5:thisisnotahdf", {HDF5_META, HDF5_META}}
			}) {
			EXPECT_EQ(keys.first, lofar_udp_metadata_parse_type_output(val.c_str()));
			std::cout << val << ", " << keys.first << ", " << keys.second << std::endl;
			EXPECT_EQ(keys.second, lofar_udp_metadata_string_to_meta(val.c_str()));
		}
	}

	{
		// TODO: Validate station numbers as well as the type check
		// int lofar_udp_metadata_get_station_name(int stationID, char *stationCode);
		SCOPED_TRACE(_lofar_udp_metadata_get_station_name);
		std::map<int32_t, std::string> stations;
		std::vector<int32_t> OoOCoreStations = {
			121, 141, 142, 161, 181
		};
		for (int stationID : std::set<int>{
			1,2,3,4,5,6,7, 11, 13, 17, 21, 24, 26, 28, 30, 31, 32, 101, 103,
			121,
			141, 142,
			161,
			181,
			106,
			125, 128, 130,
			145, 146, 147, 150,
			166, 167, 169,
			183, 188, 189,
			201, 202, 203, 204, 205,
			210,
			206,
			207,
			208,
			211, 212, 213,
			214,
			215,
			901,
			902}) {

			if (stationID <= 103 || std::find(OoOCoreStations.begin(), OoOCoreStations.end(), stationID) != OoOCoreStations.end()) {
				stations.insert(std::make_pair(stationID, std::string{"CS"}));
			} else if (stationID < 200) {
				stations.insert(std::make_pair(stationID, std::string{"RS"}));
			} else if (stationID < 206 || stationID == 210) {
				stations.insert(std::make_pair(stationID, std::string{"DE"}));
			} else if (stationID == 206) {
				stations.insert(std::make_pair(stationID, std::string{"FR"}));
			} else if (stationID == 207) {
				stations.insert(std::make_pair(stationID, std::string{"SE"}));
			} else if (stationID == 208 || stationID == 902) {
				stations.insert(std::make_pair(stationID, std::string{"UK"}));
			} else if (stationID >= 211 && stationID < 214) {
				stations.insert(std::make_pair(stationID, std::string{"PL"}));
			} else if (stationID == 214) {
				stations.insert(std::make_pair(stationID, std::string{"IE"}));
			} else if (stationID == 215) {
				stations.insert(std::make_pair(stationID, std::string{"LV"}));
			} else if (stationID == 901) {
				stations.insert(std::make_pair(stationID, std::string{"FI"}));
			} else {
				printf("%d\n", stationID);
				ASSERT_TRUE(0);
			}

		}

		char test[META_STR_LEN];
		for (auto [stationID, name] : stations) {
			EXPECT_EQ(0, _lofar_udp_metadata_get_station_name(stationID, test));
			EXPECT_EQ(1, strstr(test, name.c_str()) != NULL);
		}
		EXPECT_EQ(1, _lofar_udp_metadata_get_station_name(0, test));

	}

	{
		SCOPED_TRACE("set_func_checks");

		// int _isEmpty(const char *string);
		EXPECT_EQ(1, _isEmpty(NULL));
		EXPECT_EQ(1, _isEmpty("\0Hello"));
		EXPECT_EQ(0, _isEmpty("Hello"));

		// int _intNotSet(int input);
		EXPECT_EQ(1, _intNotSet(-1));
		EXPECT_EQ(0, _intNotSet(0));

		// int _longNotSet(long input);
		EXPECT_EQ(1, _longNotSet(-1));
		EXPECT_EQ(0, _longNotSet(0));

		// int _floatNotSet(double input, int exception);
		// int _doubleNotSet(double input, int exception);
		EXPECT_EQ(1, _floatNotSet(-1.0f, 0));
		EXPECT_EQ(0, _floatNotSet(-1.0f, 1));
		EXPECT_EQ(1, _floatNotSet(0.0f, 1));
		EXPECT_EQ(0, _floatNotSet(0.0f, 0));
		EXPECT_EQ(1, _doubleNotSet(-1.0, 0));
		EXPECT_EQ(0, _doubleNotSet(-1.0, 1));
		EXPECT_EQ(1, _doubleNotSet(0.0, 1));
		EXPECT_EQ(0, _doubleNotSet(0.0, 0));
	}


	lofar_udp_metadata *metadata = lofar_udp_metadata_alloc();

	// int lofar_udp_metdata_set_default(lofar_udp_metadata *metadata)
	const int32_t flagIdx = 64;
	static_assert(flagIdx < MAX_NUM_PORTS * UDPMAXBEAM);
	for (metadata_t mode : std::vector<metadata_t>{NO_META, GUPPI, DADA, SIGPROC, HDF5_META}) {
		metadata->type = mode;
		metadata->subbands[flagIdx] = 32;
		EXPECT_EQ(0, _lofar_udp_metdata_setup_BASE(metadata));

		if (metadata->type != NO_META && metadata->type != HDF5_META) {
			EXPECT_NE(nullptr, metadata->headerBuffer);
		}

		for (size_t idx = 0; idx < MAX_NUM_PORTS * UDPMAXBEAM; idx++) EXPECT_EQ(-1, metadata->subbands[idx]);

		for (int32_t port = 0; port < MAX_NUM_PORTS; port++) {
			EXPECT_EQ(0, strnlen(metadata->rawfile[port], META_STR_LEN));
			EXPECT_EQ(0, strnlen(metadata->upm_outputfmt[port], META_STR_LEN));
		}

		EXPECT_EQ(1, metadata->resolution);
		EXPECT_EQ("Linear", std::string{metadata->basis});
		EXPECT_NE(0, strnlen(metadata->hostname, META_STR_LEN));
		EXPECT_EQ("LOFAR-RSP", std::string{metadata->receiver});
		EXPECT_EQ(-1, metadata->output_file_number);
		EXPECT_EQ(0, metadata->obs_offset);
		EXPECT_EQ(0, metadata->obs_overlap);
		EXPECT_EQ("Unknown", std::string{metadata->mode});
		EXPECT_EQ(0, metadata->upm_processed_packets);
		EXPECT_EQ(0, metadata->upm_dropped_packets);
		EXPECT_EQ(0, metadata->upm_last_dropped_packets);

	}

	metadata->output.sigproc = sigproc_hdr_alloc(32);
	metadata->output.guppi = guppi_hdr_alloc();


	{
		SCOPED_TRACE("_lofar_udp_metadata_handle_external_factors");
		// int lofar_udp_metadata_handle_external_factors(lofar_udp_metadata *metadata, const metadata_config *config);
		metadata_config *config = lofar_udp_metadata_config_alloc();
		lofar_udp_metadata *reference = (lofar_udp_metadata *) calloc(1, sizeof(lofar_udp_metadata));
		assert(reference != nullptr);
		*reference = *metadata;

		reference->tsamp_raw = 5.12e-5;
		reference->subband_bw = 100.0 / 512;
		reference->nsubband = 488;
		reference->bw = reference->subband_bw * reference->nsubband;
		reference->freq_raw = 150.0;
		reference->ftop = reference->freq_raw - reference->bw * 0.5;
		reference->fbottom = reference->freq_raw + reference->bw * 0.5;

		config->externalChannelisation = 0;
		config->externalDownsampling = 0;

		//assert(memcpy(metadata, reference, sizeof(lofar_udp_metadata)) == metadata);
		*metadata = *reference;

		EXPECT_EQ(-1, _lofar_udp_metadata_handle_external_factors(metadata, nullptr));
		EXPECT_EQ(-1, _lofar_udp_metadata_handle_external_factors(nullptr, config));

		EXPECT_EQ(0, _lofar_udp_metadata_handle_external_factors(metadata, config));
		EXPECT_DOUBLE_EQ(reference->tsamp_raw, metadata->tsamp);
		EXPECT_DOUBLE_EQ(reference->subband_bw, metadata->channel_bw);

		// (config->externalDownsampling > 1)
		config->externalDownsampling = 8;
		EXPECT_EQ(0, _lofar_udp_metadata_handle_external_factors(metadata, config));
		EXPECT_DOUBLE_EQ(reference->tsamp_raw * config->externalDownsampling, metadata->tsamp);
		EXPECT_DOUBLE_EQ(reference->subband_bw, metadata->channel_bw);

		// (config->externalChannelisation > 1)
		config->externalDownsampling = 8;
		config->externalChannelisation = 8;
		EXPECT_EQ(0, _lofar_udp_metadata_handle_external_factors(metadata, config));
		EXPECT_DOUBLE_EQ(reference->tsamp_raw * config->externalDownsampling * config->externalChannelisation, metadata->tsamp);
		EXPECT_DOUBLE_EQ(reference->subband_bw / config->externalChannelisation, metadata->channel_bw);
		EXPECT_TRUE(reference->freq_raw != metadata->freq);
		EXPECT_TRUE(reference->ftop != metadata->ftop);
		EXPECT_DOUBLE_EQ(reference->ftop + metadata->channel_bw / 2, metadata->ftop);
		EXPECT_TRUE(reference->fbottom != metadata->fbottom);
		EXPECT_DOUBLE_EQ(reference->fbottom + metadata->channel_bw / 2, metadata->fbottom);


		config->externalDownsampling = 9;
		config->externalChannelisation = 9;
		EXPECT_EQ(0, _lofar_udp_metadata_handle_external_factors(metadata, config));
		EXPECT_DOUBLE_EQ(reference->tsamp_raw * config->externalDownsampling * config->externalChannelisation, metadata->tsamp);
		EXPECT_DOUBLE_EQ(reference->subband_bw / config->externalChannelisation, metadata->channel_bw);
		EXPECT_DOUBLE_EQ(reference->freq_raw, metadata->freq);
		EXPECT_DOUBLE_EQ(reference->ftop, metadata->ftop);
		EXPECT_DOUBLE_EQ(reference->fbottom, metadata->fbottom);

		config->externalDownsampling = 3;
		config->externalChannelisation = 3;
		metadata->bw *= -1;
		metadata->subband_bw *= -1;
		EXPECT_EQ(0, _lofar_udp_metadata_handle_external_factors(metadata, config));
		EXPECT_DOUBLE_EQ(reference->tsamp_raw * config->externalDownsampling * config->externalChannelisation, metadata->tsamp);
		EXPECT_DOUBLE_EQ(-1 * reference->subband_bw / config->externalChannelisation, metadata->channel_bw);
		EXPECT_DOUBLE_EQ(reference->freq_raw, metadata->freq);
		EXPECT_DOUBLE_EQ(reference->fbottom, metadata->ftop);
		EXPECT_DOUBLE_EQ(reference->ftop, metadata->fbottom);


		free(reference);
		free(config);

	}

	{

		// int lofar_udp_metadata_processing_mode_metadata(lofar_udp_metadata *metadata);
		SCOPED_TRACE("lofar_udp_metadata_processing_mode_metadata");
		EXPECT_EQ(-1, _lofar_udp_metadata_processing_mode_metadata(nullptr));
	}

	lofar_udp_metadata_cleanup(metadata);
};

TEST(LibMetadataTests, InternalParsers) {
	{
		SCOPED_TRACE("_lofar_udp_metadata_get_clockmode");
		// int lofar_udp_metadata_get_clockmode(int input);
		for (auto [clock, modes] : std::map<int, std::vector<int>>{
			{200, {1, 2, 3, 4, 5, 7, 10, 30, 110, 210}},
			{160, {6, 160}},
			{-1, {99}},
		}) {
			for (int mode : modes) {
				EXPECT_EQ(clock, _lofar_udp_metadata_get_clockmode(mode));
			}
		}

		SCOPED_TRACE("_lofar_udp_metadata_get_rcumode");
		// int lofar_udp_metadata_get_rcumode(int input);
		for (auto [rcu, modes] : std::map<int, std::vector<int>>{
			{-1, {-1, -10, 0, 99, 101, 120}},
			{1, {1}},
			{2, {2}},
			{3, {3, 10}},
			{4, {4, 30}},
			{5, {5, 110}},
			{6, {6, 160}},
			{7, {7, 210}},
		}) {
			for (int mode: modes) {
				EXPECT_EQ(rcu, _lofar_udp_metadata_get_rcumode(mode));
			}
		}
	}

	{
		SCOPED_TRACE("_lofar_udp_metadata_get_beamlets");
		// int lofar_udp_metadata_get_beamlets(int bitmode);
		for (int32_t bitMode: bitModes) {
			if (bitMode != invalidBitMode) {
				EXPECT_EQ(61 * 16 / bitMode, _lofar_udp_metadata_get_beamlets(bitMode));
			} else {
				EXPECT_EQ(-1, _lofar_udp_metadata_get_beamlets(bitMode));
			}
		}
	}

	{
		SCOPED_TRACE("_lofar_udp_metadata_get_tsv");
		// int lofar_udp_metadata_get_tsv(const char *inputStr, const char *keyword, char *result);
		char testStr[3][META_STR_LEN] = { "TESTKEY", "TESTVAL", "" };
		char testStrCombined[META_STR_LEN];
		if (snprintf(testStrCombined, META_STR_LEN - 1, "%s\t%s", testStr[0], testStr[1]) < 0) {
			FAIL();
		}


		EXPECT_EQ(0, _lofar_udp_metadata_get_tsv(testStrCombined, testStr[0], testStr[2]));
		EXPECT_EQ(std::string{testStr[1]}, std::string{testStr[2]});
		EXPECT_EQ(-1, _lofar_udp_metadata_get_tsv(testStr[0], testStr[0], testStr[2]));
		EXPECT_EQ(-2, _lofar_udp_metadata_get_tsv(testStrCombined, testStr[1], testStr[2]));

	}

	{
		// int lofar_udp_metadata_parse_csv(const char *inputStr, int *values, int *data);


		lofar_udp_metadata *meta = lofar_udp_metadata_alloc();
		// int lofar_udp_metadata_parse_subbands(lofar_udp_metadata *metadata, const char *inputLine, int *results);

		{
			SCOPED_TRACE("_lofar_udp_metadata_parse_subbands");
			EXPECT_EQ(-1, _lofar_udp_metadata_parse_subbands(nullptr, nullptr, nullptr));
		}

		// switch (results[0])  subbandOffset = 0, 512, 1024 mode dependant

		// (results[0] < 1) -> -1 results[0] == rcuMode

		// (inputPtr == NULL) -> 1 (no space in beamctl command)

		// (subbandCount = lofar_udp_metadata_parse_csv(workingPtr + strlen("--subbands="), subbands, &(results[1])) -> -1

		// ((beamletCount = lofar_udp_metadata_parse_csv(workingPtr + strlen("--beamlets="), beamlets, &(results[5]))) < 0) -> -1

		// (subbandCount != beamletCount || subbandCount == 0) -> -1

		// metadata->subbands[beamlets[i]] = subbandOffset + subbands[i]; -> verifiable


		SCOPED_TRACE("_lofar_udp_metadata_parse_pointing");
		// int lofar_udp_metadata_parse_pointing(lofar_udp_metadata *metadata, const char inputStr[], int digi);
		std::map<std::string, std::tuple<int, double, double, std::string, std::string, std::string>> inputs {
			std::make_pair("0,0,'J2000'", std::make_tuple(0, 0., 0., "00:00:00.0000", "00:00:00.0000", "J2000")),
			std::make_pair("asdfjasdjfakjsdfj';#[];[]];[2.0, 1.0, \"SUN\"]dfljasldjfa", std::make_tuple(1, 2.0, 1.0, "07:38:21.9742", "57:17:44.8062", "SUN")),
		};
		for (auto [key, tup] : inputs) {
			int digi = std::get<0>(tup);
			EXPECT_EQ(0, _lofar_udp_metadata_parse_pointing(meta, key.c_str(), digi));
			if (digi) {
				EXPECT_EQ(meta->ra_rad, std::get<1>(tup));
				EXPECT_EQ(meta->dec_rad, std::get<2>(tup));
				EXPECT_EQ(std::get<3>(tup), std::string{meta->ra});
				EXPECT_EQ(std::get<4>(tup), std::string{meta->dec});
			} else {
				EXPECT_EQ(meta->ra_rad_analog, std::get<1>(tup));
				EXPECT_EQ(meta->dec_rad_analog, std::get<2>(tup));
				EXPECT_EQ(std::get<3>(tup), std::string{meta->ra_analog});
				EXPECT_EQ(std::get<4>(tup), std::string{meta->dec_analog});
			}
			EXPECT_EQ(std::get<5>(tup), std::string{meta->coord_basis});

			meta->coord_basis[0] = '\0';
		}
		meta->coord_basis[0] = 'S';
		EXPECT_EQ(-4, _lofar_udp_metadata_parse_pointing(meta, "0,0,'J2000'", 1));
		EXPECT_EQ(-1, _lofar_udp_metadata_parse_pointing(meta, "1", 1));



		// int lofar_udp_metadata_parse_rcumode(lofar_udp_metadata *metadata, const char *inputStr, int *beamctlData);


		lofar_udp_metadata_cleanup(meta);
	}

};

TEST(LibMetadataTests, ParserGlue) {
	// int lofar_udp_metadata_parse_beamctl(lofar_udp_metadata *metadata, const char *inputLine, int *rcuMode);
	// int lofar_udp_metadata_update_frequencies(lofar_udp_metadata *metadata, int *subbandData);
	// int lofar_udp_metadata_count_csv(const char *inputStr);


	// int lofar_udp_metadata_parse_input_file(lofar_udp_metadata *metadata, const char inputFile[]);
	// int lofar_udp_metadata_parse_normal_file(lofar_udp_metadata *metadata, FILE *input, int *beamctlData);
	// int lofar_udp_metadata_parse_yaml_file(lofar_udp_metadata *metadata, FILE *input, int *beamctlData);
	// int lofar_udp_metadata_parse_reader(lofar_udp_metadata *metadata, const lofar_udp_reader *reader);
	{
		SCOPED_TRACE("lofar_udp_metadata_parse_reader");
		lofar_udp_metadata *metadata = lofar_udp_metadata_alloc();
		lofar_udp_obs_meta *meta = lofar_udp_obs_meta_alloc();
		lofar_udp_reader *reader = lofar_udp_reader_alloc(meta);

		EXPECT_EQ(-1, _lofar_udp_metadata_parse_reader(nullptr, nullptr));

	}

};

TEST(LibMetadataTests, StructHandlers) {
	lofar_udp_metadata *metadata = lofar_udp_metadata_alloc();
	lofar_udp_obs_meta *meta = lofar_udp_obs_meta_alloc();
	lofar_udp_reader *reader = lofar_udp_reader_alloc(meta);
	metadata_config *config = lofar_udp_metadata_config_alloc();
	{
		SCOPED_TRACE("lofar_udp_metadata_setup");
		// int lofar_udp_metadata_setup(lofar_udp_metadata *metadata, const lofar_udp_reader *reader, const metadata_config *config);

		EXPECT_EQ(-1, lofar_udp_metadata_setup(nullptr, reader, config));
		metadata->type = NO_META;
		EXPECT_EQ(-1, lofar_udp_metadata_setup(metadata, reader, config));
		metadata->type = DADA;
		//config->metadataLocation[0] == '\0'; // continues
		EXPECT_EQ(-1, lofar_udp_metadata_setup(metadata, nullptr, nullptr)); // reader-> nullptr continues, config -> nullptr -> -1
		EXPECT_EQ(0, lofar_udp_metadata_setup(metadata, reader, config));

		metadata->type = GUPPI;
		EXPECT_EQ(0, lofar_udp_metadata_setup(metadata, reader, config));
		metadata->type = SIGPROC;
		EXPECT_EQ(0, lofar_udp_metadata_setup(metadata, reader, config));

		// (config != NULL && strnlen(config->metadataLocation, DEF_STR_LEN)) ->
		//      (lofar_udp_metadata_parse_input_file(metadata, config->metadataLocation) < 0)

		// lofar_udp_metadata_parse_input_file(lofar_udp_metadata *metadata, const char *inputFile)
		// int lofar_udp_metadata_update_frequencies(lofar_udp_metadata *metadata, int *subbandData)
		// We can't do these check via the setup function as we already ensure it is not the case
		metadata->upm_rcuclock = -1;
		EXPECT_EQ(-1, _lofar_udp_metadata_update_frequencies(nullptr, nullptr));
		EXPECT_EQ(-1, _lofar_udp_metadata_parse_csv(nullptr, nullptr, nullptr, 0));
		EXPECT_EQ(-1, _lofar_udp_metadata_parse_input_file(metadata, ""));
		EXPECT_EQ(-1, _lofar_udp_metadata_parse_input_file(metadata, "this_is_not_a_real_file"));
		EXPECT_EQ(0, _lofar_udp_metadata_parse_input_file(metadata, metadataLocation[6].c_str()));
		int32_t hold = metadata->nsubband;
		metadata->nsubband = -1;
		int tmpSubbands[9] = {};
		EXPECT_EQ(-1, _lofar_udp_metadata_update_frequencies(metadata, tmpSubbands));
		metadata->nsubband = hold;


		strncpy(config->metadataLocation, metadataLocation[3].c_str(), DEF_STR_LEN);
		EXPECT_EQ(-1, lofar_udp_metadata_setup(metadata, reader, config));

		double refTopFreq = 11.5 * 100 / 512;
		double refBottomFreq = 499.5 * 100 / 512;
		double refFreq = (refTopFreq + refBottomFreq) / 2;
		double refBw = 488.0 * 100.0 / 512.0;
		strncpy(config->metadataLocation, metadataLocation[2].c_str(), DEF_STR_LEN);
		EXPECT_EQ(-1, lofar_udp_metadata_setup(metadata, reader, config));
		metadata->upm_rcuclock = -1;
		EXPECT_EQ(-1, lofar_udp_metadata_setup(metadata, reader, config));
		metadata->subband_bw = 0.0;
		EXPECT_EQ(-1, lofar_udp_metadata_setup(metadata, reader, config));

		metadata->upm_beamctl[0] = '\0';
		EXPECT_EQ(0, lofar_udp_metadata_setup(metadata, nullptr, config));
		EXPECT_EQ(std::string("DUMMY"), std::string(metadata->obs_id));
		EXPECT_EQ(std::string("DMCKENNA"), std::string(metadata->observer));
		EXPECT_EQ(std::string("J2107+2606"), std::string(metadata->source));
		EXPECT_EQ(std::string("beamctl --antennaset=HBA_JOINED --rcus=0:191 --band=110_190  --beamlets='0,1,2,3,4:487' --subbands=\"12:499\" --anadir=5.530512067257031,0.45553093477052004,J2000 --digdir=5.530512067257031,0.45553093477052004,J2000 &\t"), std::string(metadata->upm_beamctl));
		EXPECT_EQ(0, metadata->upm_lowerbeam);
		EXPECT_EQ(488, metadata->nsubband);
		EXPECT_EQ(487, metadata->upm_upperbeam);
		EXPECT_EQ(200, metadata->upm_rcuclock);
		EXPECT_EQ(std::string(UPM_VERSION), std::string(metadata->upm_version));
		EXPECT_DOUBLE_EQ(refBottomFreq + 100, metadata->fbottom);
		EXPECT_DOUBLE_EQ(refFreq + 100, metadata->freq_raw);
		EXPECT_DOUBLE_EQ(refTopFreq + 100, metadata->ftop);
		EXPECT_DOUBLE_EQ(refBw, metadata->bw);


		*(metadata) = lofar_udp_metadata_default;
		strncpy(config->metadataLocation, metadataLocation[1].c_str(), DEF_STR_LEN);
		metadata->type = DADA;
		EXPECT_EQ(0, lofar_udp_metadata_setup(metadata, nullptr, config));
		EXPECT_DOUBLE_EQ(refBottomFreq, metadata->fbottom);
		EXPECT_DOUBLE_EQ(refFreq, metadata->freq_raw);
		EXPECT_DOUBLE_EQ(refTopFreq, metadata->ftop);
		EXPECT_DOUBLE_EQ(refBw, metadata->bw);

		*(metadata) = lofar_udp_metadata_default;
		strncpy(config->metadataLocation, metadataLocation[3].c_str(), DEF_STR_LEN);
		metadata->type = DADA;
		EXPECT_EQ(0, lofar_udp_metadata_setup(metadata, nullptr, config));
		EXPECT_DOUBLE_EQ(160.0 + 499.5 * 80 / 512.0, metadata->fbottom);
		EXPECT_DOUBLE_EQ(160.0 + (11.5 + 499.5) * 0.5 * 80.0 / 512.0, metadata->freq_raw);
		EXPECT_DOUBLE_EQ(160.0 + 11.5 * 80 / 512.0, metadata->ftop);
		EXPECT_DOUBLE_EQ(488.0 * 80.0 / 512.0, metadata->bw);

		*(metadata) = lofar_udp_metadata_default;
		strncpy(config->metadataLocation, metadataLocation[4].c_str(), DEF_STR_LEN);
		metadata->type = DADA;
		EXPECT_EQ(0, lofar_udp_metadata_setup(metadata, nullptr, config));
		EXPECT_DOUBLE_EQ(refBottomFreq + 200, metadata->fbottom);
		EXPECT_DOUBLE_EQ(refFreq + 200, metadata->freq_raw);
		EXPECT_DOUBLE_EQ(refTopFreq + 200, metadata->ftop);
		EXPECT_DOUBLE_EQ(refBw, metadata->bw);

		*(metadata) = lofar_udp_metadata_default;
		strncpy(config->metadataLocation, metadataLocation[5].c_str(), DEF_STR_LEN);
		metadata->type = DADA;
		EXPECT_EQ(0, lofar_udp_metadata_setup(metadata, nullptr, config));
		EXPECT_EQ(std::string("beamctl --antennaset=HBA_JOINED --rcus=0:31,96:127 --rcumode=3 --beamlets=0:199 --subbands=54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,98,100,102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,136,138,140,142,144,146,148,150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,182,184,186,188,190,192,194,196,198,200,202,204,206,208,210,212,214,216,218,220,222,224,226,228,230,232,234,236,238,240,242,244,246,248,250,252,254,256,258,260,262,264,266,268,270,272,274,276,278,280,282,284,286,288,290,292,294,296,298,300,302,304,306,308,310,312,314,316,318,320,322,324,326,328,330,332,334,336,338,340,342,344,346,348,350,352,354,356,358,360,362,364,366,368,370,372,374,376,378,380,382,384,386,388,390,392,394,396,398,400,402,404,406,408,410,412,414,416,418,420,422,424,426,428,430,432,434,436,438,440,442,444,446,448,450,452 --anadir=0.0,0.0,SUN --digdir=0.0,0.0,SUN &\tbeamctl --antennaset=HBA_JOINED --rcus=32:63,128:159 --rcumode=5  --beamlets=200:399 --subbands=54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,98,100,102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,136,138,140,142,144,146,148,150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,182,184,186,188,190,192,194,196,198,200,202,204,206,208,210,212,214,216,218,220,222,224,226,228,230,232,234,236,238,240,242,244,246,248,250,252,254,256,258,260,262,264,266,268,270,272,274,276,278,280,282,284,286,288,290,292,294,296,298,300,302,304,306,308,310,312,314,316,318,320,322,324,326,328,330,332,334,336,338,340,342,344,346,348,350,352,354,356,358,360,362,364,366,368,370,372,374,376,378,380,382,384,386,388,390,392,394,396,398,400,402,404,406,408,410,412,414,416,418,420,422,424,426,428,430,432,434,436,438,440,442,444,446,448,450,452 --anadir=0.0,0.0,SUN --digdir=0.0,0.0,SUN &\tbeamctl --antennaset=HBA_JOINED --rcus=64:95,160:191 --rcumode=7  --beamlets=400:487 --subbands=54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,98,100,102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,136,138,140,142,144,146,148,150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,182,184,186,188,190,192,194,196,198,200,202,204,206,208,210,212,214,216,218,220,222,224,226,228 --anadir=0.0,0.0,SUN --digdir=0.0,0.0,SUN &\t"), metadata->upm_beamctl);
		EXPECT_DOUBLE_EQ(228.5 * 100.0 / 512.0 + 200.0, metadata->fbottom);
		EXPECT_DOUBLE_EQ(122.51857069672131, metadata->freq_raw);
		EXPECT_DOUBLE_EQ(53.5 * 100.0 / 512.0, metadata->ftop);
		EXPECT_DOUBLE_EQ((228.5 - 53.5) * 100.0 / 512.0 + 200.0, metadata->bw);

		std::cout << "Mark\n";
		*(metadata) = lofar_udp_metadata_default;
		strncpy(config->metadataLocation, metadataLocation[7].c_str(), DEF_STR_LEN);
		metadata->type = DADA;
		EXPECT_EQ(-1, lofar_udp_metadata_setup(metadata, nullptr, config));

		*(metadata) = lofar_udp_metadata_default;
		strncpy(config->metadataLocation, metadataLocation[8].c_str(), DEF_STR_LEN);
		metadata->type = DADA;
		EXPECT_EQ(-1, lofar_udp_metadata_setup(metadata, nullptr, config));


		*(metadata) = lofar_udp_metadata_default;
		strncpy(config->metadataLocation, metadataLocation[9].c_str(), DEF_STR_LEN);
		metadata->type = DADA;
		EXPECT_EQ(-1, lofar_udp_metadata_setup(metadata, nullptr, config));



		*(metadata) = lofar_udp_metadata_default;
		strncpy(config->metadataLocation, metadataLocation[1].c_str(), DEF_STR_LEN);
		metadata->type = DADA;
		reader->meta->clockBit = 1;
		reader->meta->inputBitMode = 8;
		reader->input->basePort = 0;
		reader->input->readerType = NORMAL;
		reader->meta->replayDroppedPackets = 1;
		reader->packetsPerIteration = 128;
		reader->meta->packetOutputLength[0] = 7824;
		reader->meta->processingMode = STOKES_I_REV;
		reader->meta->calibrateData = APPLY_CALIBRATION;
		reader->input->numInputs = 4;
		reader->meta->numOutputs = 1;
		reader->input->offsetPortCount = 0;
		reader->meta->baseBeamlets[0] = 0;
		reader->input->numInputs = 4;
		reader->meta->upperBeamlets[reader->input->numInputs - 1] = 488;
		reader->meta->totalProcBeamlets = 488;
		reader->meta->inputData[0] = (int8_t*) calloc(16, sizeof(int8_t));
		*((uint32_t *) &(reader->meta->inputData[CEP_HDR_TIME_OFFSET])) = (uint32_t) rand();
		for (int i = 0; i < reader->input->numInputs; i++) {
			std::string tmp = std::to_string(i);
			strncpy(reader->input->inputLocations[i], tmp.c_str(), tmp.length());
		}

		EXPECT_EQ(0, lofar_udp_metadata_setup(metadata, reader, config));


	}

	int8_t packetHeader[UDPHDRLEN] = { 3, -128,    1,  -86,  -64,   26,  122,   16,   32,   33,   45,  100,   34,  104,    0,    0 };
	const double refMjdStart = 60039.305557135289;
	const int64_t testIter = 64;
	const int32_t numTestInput = 2;
	const std::string daq("Wed Apr  5 07:20:00 2023");

	{
		SCOPED_TRACE("_lofar_udp_metadata_update_BASE");
		// int lofar_udp_metadata_update_BASE(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, int newObs);
		lofar_udp_reader_cleanup(reader);
		meta = lofar_udp_obs_meta_alloc();
		reader = lofar_udp_reader_alloc(meta);
		lofar_udp_metadata_cleanup(metadata);
		metadata = lofar_udp_metadata_alloc();
		reader->meta->inputData[0] = packetHeader;

		reader->meta->packetsPerIteration = testIter;
		reader->meta->packetOutputLength[0] = testIter;
		metadata->upm_processed_packets = 0;
		metadata->upm_dropped_packets = 0;
		metadata->upm_num_inputs = numTestInput;
		for (int i = 0; i < numTestInput; i++) reader->meta->portLastDroppedPackets[i] = testIter;
		EXPECT_EQ(-1, _lofar_udp_metadata_update_BASE(nullptr, nullptr, 1));

		EXPECT_EQ(0, _lofar_udp_metadata_update_BASE(reader, metadata, 0));
		EXPECT_EQ(0, strnlen(metadata->obs_utc_start, META_STR_LEN));
		EXPECT_DOUBLE_EQ(-1.0, metadata->obs_mjd_start);
		EXPECT_EQ(daq, std::string(metadata->upm_daq));
		EXPECT_EQ(testIter, metadata->upm_pack_per_iter);
		EXPECT_EQ(testIter * testIter, metadata->upm_blocksize);
		EXPECT_EQ(numTestInput * testIter, metadata->upm_processed_packets);
		EXPECT_EQ(numTestInput * testIter, metadata->upm_last_dropped_packets);
		EXPECT_EQ(numTestInput * testIter, metadata->upm_dropped_packets);

		EXPECT_EQ(0, _lofar_udp_metadata_update_BASE(reader, metadata, 1));
		EXPECT_EQ(std::string("2023-04-05T07:20:00.136489"), std::string(metadata->obs_utc_start));
		EXPECT_DOUBLE_EQ(refMjdStart, metadata->obs_mjd_start);
		EXPECT_EQ(numTestInput * testIter, metadata->upm_processed_packets);
		EXPECT_EQ(numTestInput * testIter, metadata->upm_dropped_packets);
		EXPECT_EQ(daq, std::string(metadata->upm_daq));
		EXPECT_EQ(testIter, metadata->upm_pack_per_iter);
		EXPECT_EQ(testIter * testIter, metadata->upm_blocksize);
		EXPECT_EQ(numTestInput * testIter, metadata->upm_processed_packets);
		EXPECT_EQ(numTestInput * testIter, metadata->upm_last_dropped_packets);
		EXPECT_EQ(numTestInput * testIter, metadata->upm_dropped_packets);

	}

	{
		SCOPED_TRACE("_lofar_udp_metadata_update_GUPPI");
		guppi_hdr *hold;
		if (metadata->output.guppi == NULL) {
			hold = guppi_hdr_alloc();
		} else {
			hold = metadata->output.guppi;
			metadata->output.guppi = nullptr;
		}
		EXPECT_EQ(-1, _lofar_udp_metadata_update_GUPPI(metadata, 1));
		metadata->output.guppi = hold;
		metadata->upm_pack_per_iter = 0;
		EXPECT_EQ(-1, _lofar_udp_metadata_update_GUPPI(metadata, 1));
		metadata->upm_pack_per_iter = testIter;
		EXPECT_EQ(50000, metadata->output.guppi->stt_imjd);
		EXPECT_EQ(0, metadata->output.guppi->stt_smjd);
		EXPECT_EQ(0, metadata->output.guppi->stt_offs);
		EXPECT_EQ(0, metadata->output.guppi->pktidx);
		EXPECT_EQ(0, _lofar_udp_metadata_update_GUPPI(metadata, 1));
		EXPECT_EQ(metadata->upm_blocksize, metadata->output.guppi->blocsize);
		EXPECT_EQ(daq, std::string(metadata->output.guppi->daqpulse));
		EXPECT_EQ(numTestInput * testIter / testIter / numTestInput, metadata->output.guppi->dropblk);
		EXPECT_DOUBLE_EQ(1.0, metadata->output.guppi->droptot);
		EXPECT_EQ(0, metadata->output.guppi->pktidx);
		EXPECT_EQ((int) refMjdStart, metadata->output.guppi->stt_imjd);
		EXPECT_EQ((int) ((refMjdStart - (int) refMjdStart) * 86400), metadata->output.guppi->stt_smjd);
		EXPECT_EQ(((refMjdStart  - (int) refMjdStart) * 86400) - (int) ((refMjdStart - (int) refMjdStart) * 86400), metadata->output.guppi->stt_offs);
		EXPECT_EQ(0, metadata->output.guppi->pktidx);
	}

	{
		SCOPED_TRACE("_lofar_udp_metadata_update_SIGPROC");
		sigproc_hdr *hold;
		if (metadata->output.sigproc == NULL) {
			hold = sigproc_hdr_alloc(0);
		} else {
			hold = metadata->output.sigproc;
			metadata->output.sigproc = nullptr;
		}
		// int lofar_udp_metadata_update_SIGPROC(lofar_udp_metadata *metadata, int newObs);
		EXPECT_EQ(-1, _lofar_udp_metadata_update_SIGPROC(nullptr, 0));
		EXPECT_EQ(-1, _lofar_udp_metadata_update_SIGPROC(metadata, 0));
		metadata->output.sigproc = hold;
		EXPECT_EQ(0, _lofar_udp_metadata_update_SIGPROC(metadata, 0));
		EXPECT_EQ(-1, metadata->output.sigproc->tstart);
		EXPECT_EQ(0, _lofar_udp_metadata_update_SIGPROC(metadata, 1));
		EXPECT_EQ(refMjdStart, metadata->output.sigproc->tstart);

	}
	// int lofar_udp_metadata_update_HDF5(lofar_udp_metadata *metadata, int newObs);

	{
		SCOPED_TRACE("lofar_udp_metadata_update");
		// int lofar_udp_metadata_update(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, int newObs);
		metadata->type = DADA;
		EXPECT_EQ(-1, lofar_udp_metadata_update(nullptr, metadata, 0));
		EXPECT_EQ(0, lofar_udp_metadata_update(reader, nullptr, 0));
		for (metadata_t meta : std::vector<metadata_t> {DADA, SIGPROC, GUPPI, (metadata_t) 111}) {
			metadata->type = meta;
			int32_t refIdx = metadata->output_file_number;
			EXPECT_EQ(meta == (metadata_t) 111 ? -1 : 0, lofar_udp_metadata_update(reader, metadata, 0));
			EXPECT_EQ(refIdx, metadata->output_file_number);
			EXPECT_EQ(meta == (metadata_t) 111 ? -1 : 0, lofar_udp_metadata_update(reader, metadata, 1));
			EXPECT_EQ(refIdx + 1, metadata->output_file_number);
		}
	}

	lofar_udp_metadata_cleanup(metadata);
	lofar_udp_reader_cleanup(reader);
	free(config);
};

TEST(LibMetadataTests, InternalBufferBuilders) {
	std::hash<std::string> hasher;
	char headerBuffer[DEF_HDR_LEN];
	char testKey[META_STR_LEN] = "MYKEY";
	char testVal[META_STR_LEN] = "THISISMYVAL";
	assert(strnlen(testVal, META_STR_LEN) > 8); // Will be used to test GUPPI failures
	int testInt = rand();
	long testLong = ((long) testInt) * ((long) testInt);
	float testFloat = (float) testLong / (float) rand();
	double testDouble = (double) testLong / (double) rand();

	{
		SCOPED_TRACE("guppi_header_builders");
		// int writeStr_GUPPI(char *headerBuffer, const char *key, const char *val)
		headerBuffer[0] = '\0';
		EXPECT_EQ(headerBuffer + 80, _writeStr_GUPPI(headerBuffer, testKey, testVal));
		std::string expectedOutput = "MYKEY   = 'THISISMYVAL'";
		expectedOutput.insert(expectedOutput.length(), 80 - expectedOutput.length(), ' ');
		std::cout << headerBuffer << "|" << std::endl;
		std::cout << expectedOutput << "|" << std::endl;
		EXPECT_EQ(0, strcmp(expectedOutput.c_str(), headerBuffer));

		headerBuffer[0] = '\0';
		EXPECT_EQ(nullptr, _writeStr_GUPPI(headerBuffer, testVal, testVal));
		headerBuffer[0] = '\0';
		EXPECT_EQ(nullptr, _writeStr_GUPPI(headerBuffer, testKey, expectedOutput.c_str()));

		// int writeInt_GUPPI(char *headerBuffer, char *key, int val)
		headerBuffer[0] = '\0';
		EXPECT_EQ(headerBuffer + 80, _writeInt_GUPPI(headerBuffer, testKey, testInt));
		expectedOutput = "MYKEY   = " + std::to_string(testInt);
		expectedOutput.insert(expectedOutput.length(), 80 - expectedOutput.length(), ' ');
		std::cout << headerBuffer << "|" << std::endl;
		std::cout << expectedOutput << "|" << std::endl;
		EXPECT_EQ(0, strcmp(expectedOutput.c_str(), headerBuffer));

		headerBuffer[0] = '\0';
		EXPECT_EQ(nullptr, _writeInt_GUPPI(headerBuffer, testVal, testInt));

		// int writeLong_GUPPI(char *headerBuffer, char *key, long val)
		headerBuffer[0] = '\0';
		EXPECT_EQ(headerBuffer + 80, _writeLong_GUPPI(headerBuffer, testKey, testLong));
		expectedOutput = "MYKEY   = " + std::to_string(testLong);
		expectedOutput.insert(expectedOutput.length(), 80 - expectedOutput.length(), ' ');
		std::cout << headerBuffer << "|" << std::endl;
		std::cout << expectedOutput << "|" << std::endl;
		EXPECT_EQ(0, strcmp(expectedOutput.c_str(), headerBuffer));

		headerBuffer[0] = '\0';
		EXPECT_EQ(nullptr, _writeLong_GUPPI(headerBuffer, testVal, testLong));

		// __attribute__((unused)) int writeFloat_GUPPI(char *headerBuffer, char *key, float val)
		headerBuffer[0] = '\0';
		EXPECT_EQ(headerBuffer + 80, _writeFloat_GUPPI(headerBuffer, testKey, testFloat));
		//expectedOutput = "MYKEY   = " + std::format("{:.17f}", testFloat);
		snprintf(headerBuffer + 128, 80, "%.12f", testFloat);
		expectedOutput = "MYKEY   = ";
		expectedOutput.append(headerBuffer + 128);
		expectedOutput.insert(expectedOutput.length(), 80 - expectedOutput.length(), ' ');
		std::cout << headerBuffer << "|" << std::endl;
		std::cout << expectedOutput << "|" << std::endl;
		EXPECT_EQ(0, strcmp(expectedOutput.c_str(), headerBuffer));

		headerBuffer[0] = '\0';
		EXPECT_EQ(nullptr, _writeFloat_GUPPI(headerBuffer, testVal, testFloat));

		// int writeDouble_GUPPI(char *headerBuffer, char *key, double val)
		headerBuffer[0] = '\0';
		EXPECT_EQ(headerBuffer + 80, _writeDouble_GUPPI(headerBuffer, testKey, testDouble));
		//expectedOutput = "MYKEY   = " + std::format("{:.17lf}", testDouble);
		snprintf(headerBuffer + 128, 80, "%.17f", testDouble);
		expectedOutput = "MYKEY   = ";
		expectedOutput.append(headerBuffer + 128);
		expectedOutput.insert(expectedOutput.length(), 80 - expectedOutput.length(), ' ');
		std::cout << headerBuffer << "|" << std::endl;
		std::cout << expectedOutput << "|" << std::endl;
		EXPECT_EQ(0, strcmp(expectedOutput.c_str(), headerBuffer));

		headerBuffer[0] = '\0';
		EXPECT_EQ(nullptr, _writeDouble_GUPPI(headerBuffer, testVal, testDouble));


		// int lofar_udp_metadata_write_GUPPI(const guppi_hdr *hdr, char *headerBuffer, size_t headerLength)
		guppi_hdr *hdr = guppi_hdr_alloc();
		std::string testBuffer(DEF_HDR_LEN, '\0');
		(testBuffer.data())[0] = 'a';
		EXPECT_EQ(-1, _lofar_udp_metadata_write_GUPPI(hdr, (int8_t *) testBuffer.data(), (ASCII_HDR_MEMBS - 1) * 80));
		EXPECT_EQ(2720, _lofar_udp_metadata_write_GUPPI(hdr, (int8_t *) testBuffer.data(), DEF_HDR_LEN));
		EXPECT_EQ(5001145786552019502l, hasher(testBuffer));

		free(hdr);
	}

	{
		SCOPED_TRACE("sigproc_header_builders");
		// char* _writeKey_SIGPROC(char *buffer, const char *name);
		ARR_INIT(headerBuffer, DEF_HDR_LEN, '\0');
		char expectedOutput[META_STR_LEN];// = {5, 0, 0, 0, 77, 89, 75, 69, 89, 0};
		ARR_INIT(expectedOutput, META_STR_LEN, '\0');

		int32_t outputLength = strnlen(testKey, META_STR_LEN);
		memcpy(expectedOutput, &outputLength, sizeof(int32_t));
		memcpy(expectedOutput + sizeof(int32_t), testKey, strlen(testKey) * sizeof(char));
		outputLength += sizeof(int32_t);
		EXPECT_EQ(headerBuffer + outputLength, _writeKey_SIGPROC(headerBuffer, testKey));

		EXPECT_EQ(0, strncmp(headerBuffer, expectedOutput, outputLength));

		EXPECT_EQ(nullptr, _writeKey_SIGPROC(nullptr, nullptr));
		EXPECT_EQ(nullptr, _writeKey_SIGPROC(headerBuffer, ""));

		// char* _writeStr_SIGPROC(char *buffer, const char *name, const char *value);
		outputLength = strnlen(testKey, META_STR_LEN);
		memcpy(expectedOutput, &outputLength, sizeof(int32_t));
		memcpy(expectedOutput + sizeof(int32_t), testKey, strlen(testKey) * sizeof(char));
		outputLength += sizeof(int32_t);
		outputLength = strnlen(testVal, META_STR_LEN);
		memcpy(expectedOutput + sizeof(int32_t) + strnlen(testKey, META_STR_LEN), &outputLength, sizeof(int32_t));
		memcpy(expectedOutput + 2 * sizeof(int32_t) + strnlen(testKey, META_STR_LEN), testVal, strlen(testVal) * sizeof(char));
		outputLength += 2 * sizeof(int32_t) + strnlen(testKey, META_STR_LEN);
		EXPECT_EQ(headerBuffer + outputLength, _writeStr_SIGPROC(headerBuffer, testKey, testVal));

		EXPECT_EQ(0, strncmp(headerBuffer, expectedOutput, outputLength));

		EXPECT_EQ(nullptr, _writeStr_SIGPROC(nullptr, nullptr, nullptr));
		EXPECT_EQ(headerBuffer, _writeStr_SIGPROC(headerBuffer, "", ""));

		// char* _writeInt_SIGPROC(char *buffer, const char *name, int32_t value);
		int32_t testInt = 0xBAAAAAAD;
		outputLength = strnlen(testKey, META_STR_LEN);
		memcpy(expectedOutput, &outputLength, sizeof(int32_t));
		memcpy(expectedOutput + sizeof(int32_t), testKey, strlen(testKey) * sizeof(char));
		outputLength += sizeof(int32_t);
		memcpy(expectedOutput + sizeof(int32_t), &testInt, sizeof(int32_t));
		outputLength += sizeof(int32_t);
		EXPECT_EQ(headerBuffer + outputLength, _writeInt_SIGPROC(headerBuffer, testKey, testInt));

		EXPECT_EQ(0, strncmp(headerBuffer, expectedOutput, outputLength));

		EXPECT_EQ(nullptr, _writeInt_SIGPROC(nullptr, nullptr, 0));
		EXPECT_EQ(headerBuffer, _writeInt_SIGPROC(headerBuffer, "", 0));

		// char* _writeDouble_SIGPROC(char *buffer, const char *name, double value, int exception);
		double testDbl = 19.16;
		outputLength = strnlen(testKey, META_STR_LEN);
		memcpy(expectedOutput, &outputLength, sizeof(int32_t));
		memcpy(expectedOutput + sizeof(int32_t), testKey, strlen(testKey) * sizeof(char));
		outputLength += sizeof(int32_t);
		memcpy(expectedOutput + sizeof(int32_t), &testDbl, sizeof(double));
		outputLength += sizeof(double);
		EXPECT_EQ(headerBuffer + outputLength, _writeDouble_SIGPROC(headerBuffer, testKey, testDbl, 0));

		EXPECT_EQ(0, strncmp(headerBuffer, expectedOutput, outputLength));

		EXPECT_EQ(nullptr, _writeDouble_SIGPROC(nullptr, nullptr, 0, 0));
		EXPECT_EQ(headerBuffer, _writeDouble_SIGPROC(headerBuffer, "", 0.0, 0));
		EXPECT_EQ(headerBuffer, _writeDouble_SIGPROC(headerBuffer, testKey, 0.0, 1));
		EXPECT_NE(headerBuffer, _writeDouble_SIGPROC(headerBuffer, testKey, 0.1, 1));

		// __attribute__((unused)) char* _writeLong_SIGPROC(char *buffer, const char *name, int64_t value); // Not in spec
		int64_t testLng = 0xBAAAAAAD;
		outputLength = strnlen(testKey, META_STR_LEN);
		memcpy(expectedOutput, &outputLength, sizeof(int32_t));
		memcpy(expectedOutput + sizeof(int32_t), testKey, strlen(testKey) * sizeof(char));
		outputLength += sizeof(int32_t);
		memcpy(expectedOutput + sizeof(int32_t), &testLng, sizeof(int64_t));
		outputLength += sizeof(int64_t);
		EXPECT_EQ(headerBuffer + outputLength, _writeLong_SIGPROC(headerBuffer, testKey, testLng));

		EXPECT_EQ(0, strncmp(headerBuffer, expectedOutput, outputLength));

		EXPECT_EQ(nullptr, _writeLong_SIGPROC(nullptr, nullptr, 0));
		EXPECT_EQ(headerBuffer, _writeLong_SIGPROC(headerBuffer, "", 0));

		// __attribute__((unused)) char* _writeFloat_SIGPROC(char *buffer, const char *name, float value, int exception); // Not in spec
		float testFlt = 19.16f;
		outputLength = strnlen(testKey, META_STR_LEN);
		memcpy(expectedOutput, &outputLength, sizeof(int32_t));
		memcpy(expectedOutput + sizeof(int32_t), testKey, strlen(testKey) * sizeof(char));
		outputLength += sizeof(int32_t);
		memcpy(expectedOutput + sizeof(int32_t), &testDbl, sizeof(float));
		outputLength += sizeof(float);
		EXPECT_EQ(headerBuffer + outputLength, _writeFloat_SIGPROC(headerBuffer, testKey, testFlt, 0));

		EXPECT_EQ(0, strncmp(headerBuffer, expectedOutput, outputLength));

		EXPECT_EQ(nullptr, _writeFloat_SIGPROC(nullptr, nullptr, 0.0f, 0));
		EXPECT_EQ(headerBuffer, _writeFloat_SIGPROC(headerBuffer, "", 0.0f, 0));
		EXPECT_EQ(headerBuffer, _writeFloat_SIGPROC(headerBuffer, testKey, 0.0f, 1));
		EXPECT_NE(headerBuffer, _writeFloat_SIGPROC(headerBuffer, testKey, 0.1f, 1));


		// int lofar_udp_metadata_write_SIGPROC(const sigproc_hdr *hdr, char *headerBuffer, size_t headerLength);
		sigproc_hdr *hdr = sigproc_hdr_alloc(0);

		snprintf(testKey, META_STR_LEN, "HEADER_START");
		snprintf(testVal, META_STR_LEN, "HEADER_END");
		outputLength = strnlen(testKey, META_STR_LEN);
		memcpy(expectedOutput, &outputLength, sizeof(int32_t));
		memcpy(expectedOutput + sizeof(int32_t), testKey, strlen(testKey) * sizeof(char));
		outputLength += sizeof(int32_t);
		outputLength = strnlen(testVal, META_STR_LEN);
		memcpy(expectedOutput + sizeof(int32_t) + strnlen(testKey, META_STR_LEN), &outputLength, sizeof(int32_t));
		memcpy(expectedOutput + 2 * sizeof(int32_t) + strnlen(testKey, META_STR_LEN), testVal, strlen(testVal) * sizeof(char));
		outputLength += 2 * sizeof(int32_t) + strnlen(testKey, META_STR_LEN);

		EXPECT_EQ(outputLength, _lofar_udp_metadata_write_SIGPROC(hdr, (int8_t*) headerBuffer, DEF_HDR_LEN));

		EXPECT_EQ(0, strncmp(headerBuffer, expectedOutput, outputLength));
		free(hdr);

		sigproc_hdr hdr_raw = {
			.telescope_id = 1916,
			.machine_id = 1916,
			.data_type = 1,
			.rawdatafile = {'\0'},
			.source_name = {'\0'},
			.barycentric = 0,
			.pulsarcentric = 1,
			.az_start = 111111.1,
			.za_start = 222222.1,
			.src_raj = 232323.555,
			.src_decj = 121212.555,
			.tstart = 60000.0,
			.tsamp = 5.12e-6,
			.nbits = 8,
			.nsamples = -1,
			.fch1 = 100.0,
			.foff = -0.1,
			.fchannel = NULL,
			.fchannels = -1,
			.nchans = 12,
			.nifs = 1,
			.refdm = 0.0,
			.period = 1.0,
		};
		snprintf(hdr_raw.source_name, META_STR_LEN, "Hello");
		snprintf(hdr_raw.rawdatafile, META_STR_LEN, "no_input");

		ARR_INIT(headerBuffer, DEF_HDR_LEN, 0);
		EXPECT_EQ(395, _lofar_udp_metadata_write_SIGPROC(&hdr_raw, (int8_t*) headerBuffer, DEF_HDR_LEN));
		EXPECT_EQ(7406572685367788247l, hasher(headerBuffer));

		// SigProc helpers
		//int _sigprocStationID(int telescopeId);
		//__attribute__((unused)) int _sigprocMachineID(const char *machineName); // Currently not implemented

		// double _sigprocStr2Pointing(const char *input);
		int32_t raH = 03, raM = 32, raS = 11, raFrac = 1312312;
		int32_t decD = -03, decM = 32, decS = 11, decFrac = 1231231;
		snprintf(testKey, META_STR_LEN, "%02d:%02d:%02d.%d", raH, raM, raS, raFrac);
		snprintf(testVal, META_STR_LEN, "%02d:%02d:%02d.%d", decD, decM, decS, decFrac);
		auto log10_reduce = [](int n) {
			double temp = static_cast<double>(n);
			while (n != 0) {
				n /= 10;
				temp /= 10.0;
			}
			return temp;
		};

		double ra = raH * 1e4 + raM * 1e2 + raS + log10_reduce(raFrac);
		double dec = decD * 1e4 - decM * 1e2 - decS - log10_reduce(decFrac);

		EXPECT_DOUBLE_EQ(ra, _sigprocStr2Pointing(testKey));
		EXPECT_DOUBLE_EQ(dec, _sigprocStr2Pointing(testVal));

		snprintf(testKey, META_STR_LEN, "MYKEY");
		snprintf(testVal, META_STR_LEN, "THISISMYVAL");

	}

	{
		SCOPED_TRACE("dada_header_builders");
		// int _writeStr_DADA(char *header, const char *key, const char *value);
		ARR_INIT(headerBuffer, DEF_HDR_LEN, '\0');
		EXPECT_EQ(0, _writeStr_DADA(headerBuffer, testKey, testVal));

		std::string expectedOutput = "MYKEY        THISISMYVAL            \n";
		EXPECT_EQ(expectedOutput, std::string(headerBuffer));

		EXPECT_EQ(-1, _writeStr_DADA(headerBuffer, nullptr, testVal));
		EXPECT_EQ(0, _writeStr_DADA(headerBuffer, testKey, ""));
		EXPECT_EQ(expectedOutput, std::string(headerBuffer));
		// int _writeInt_DADA(char *header, const char *key, int32_t value);
		int32_t testInt = 32;
		ARR_INIT(headerBuffer, DEF_HDR_LEN, '\0');
		EXPECT_EQ(0, _writeInt_DADA(headerBuffer, testKey, testInt));

		expectedOutput = "MYKEY        32                     \n";
		EXPECT_EQ(expectedOutput, std::string(headerBuffer));

		EXPECT_EQ(-1, _writeInt_DADA(headerBuffer, nullptr, testInt));
		EXPECT_EQ(0, _writeInt_DADA(headerBuffer, testKey, -1));
		EXPECT_EQ(expectedOutput, std::string(headerBuffer));
		ARR_INIT(headerBuffer, DEF_HDR_LEN, '\0');


		// int _writeLong_DADA(char *header, const char *key, int64_t value);
		int64_t testLng = 3212342567890l;
		EXPECT_EQ(0, _writeLong_DADA(headerBuffer, testKey, testLng));

		expectedOutput = "MYKEY        3212342567890          \n";
		EXPECT_EQ(expectedOutput, std::string(headerBuffer));

		EXPECT_EQ(-1, _writeLong_DADA(headerBuffer, "", testLng));
		EXPECT_EQ(0, _writeLong_DADA(headerBuffer, testKey, -1l));
		EXPECT_EQ(expectedOutput, std::string(headerBuffer));
		ARR_INIT(headerBuffer, DEF_HDR_LEN, '\0');

		// __attribute__((unused)) int _writeFloat_DADA(char *header, const char *key, float value, int exception); // Not in spec
		float testFlt = 1234.0f;
		EXPECT_EQ(0, _writeFloat_DADA(headerBuffer, testKey, testFlt, 0));

		expectedOutput = "MYKEY        1234.000000000000      \n";
		EXPECT_EQ(expectedOutput, std::string(headerBuffer));

		EXPECT_EQ(-1, _writeFloat_DADA(headerBuffer, "", testFlt, 0));
		EXPECT_EQ(0, _writeFloat_DADA(headerBuffer, testKey, -1.0f, 0));
		EXPECT_EQ(expectedOutput, std::string(headerBuffer));
		EXPECT_EQ(0, _writeFloat_DADA(headerBuffer, testKey, 0.0f, 1));
		EXPECT_EQ(expectedOutput, std::string(headerBuffer));
		ARR_INIT(headerBuffer, DEF_HDR_LEN, '\0');

		// int _writeDouble_DADA(char *header, const char *key, double value, int exception);
		double testDbl = 1234567.0;
		EXPECT_EQ(0, _writeDouble_DADA(headerBuffer, testKey, testDbl, 0));

		expectedOutput = "MYKEY        1234567.00000000000000000   \n";
		EXPECT_EQ(expectedOutput, std::string(headerBuffer));

		EXPECT_EQ(-1, _writeDouble_DADA(headerBuffer, "", testDbl, 0));
		EXPECT_EQ(0, _writeDouble_DADA(headerBuffer, testKey, -1.0, 0));
		EXPECT_EQ(expectedOutput, std::string(headerBuffer));
		EXPECT_EQ(0, _writeDouble_DADA(headerBuffer, testKey, 0.0, 1));
		EXPECT_EQ(expectedOutput, std::string(headerBuffer));
		ARR_INIT(headerBuffer, DEF_HDR_LEN, '\0');



		// int lofar_udp_metadata_write_DADA(const lofar_udp_metadata *hdr, char * const headerBuffer, size_t headerLength)
		lofar_udp_metadata *hdr = lofar_udp_metadata_alloc();
		EXPECT_EQ(-1, _lofar_udp_metadata_write_DADA(hdr, (int8_t*) headerBuffer, 1));
		EXPECT_EQ(-1, _lofar_udp_metadata_write_DADA(hdr, (int8_t*) headerBuffer, DEF_HDR_LEN));
		snprintf(hdr->obs_utc_start, META_STR_LEN, "2023-03-03T03:03:03");
		EXPECT_EQ(468, _lofar_udp_metadata_write_DADA(hdr, (int8_t*) headerBuffer, DEF_HDR_LEN));
		printf("%s\n", headerBuffer);
		EXPECT_EQ(5286411601707105073UL, hasher(headerBuffer));
		hdr->upm_num_inputs = 4;
		hdr->output_file_number = 1;
		hdr->upm_num_outputs = 4;
		EXPECT_EQ(547, _lofar_udp_metadata_write_DADA(hdr, (int8_t*) headerBuffer, DEF_HDR_LEN));
		printf("%s\n", headerBuffer);
		EXPECT_EQ(9863917093308632906UL, hasher(headerBuffer));
	}
}

TEST(LibMetadataTests, MetadataWriters) {

	// int lofar_udp_metadata_write_HDF5(const sigproc_hdr *hdr, char *headerBuffer, size_t headerLength);

	// int lofar_udp_metadata_write_buffer(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, char *headerBuffer, size_t headerBufferSize, int newObs);
	// int lofar_udp_metadata_write_buffer_force(const lofar_udp_reader *reader, lofar_udp_metadata *metadata, char *headerBuffer, size_t headerBufferSize, int newObs, int force);
	// int
	//   lofar_udp_metadata_write_file(const lofar_udp_reader *reader, lofar_udp_io_write_config *outConfig, int outp, lofar_udp_metadata *metadata, char *headerBuffer,
	//                              size_t headerBufferSize, int newObs);
	// int lofar_udp_metadata_write_file_force(const lofar_udp_reader *reader, lofar_udp_io_write_config *outConfig, int outp, lofar_udp_metadata *metadata,
	//                                        char *headerBuffer, size_t headerBufferSize, int newObs, int force);
};


/*

// Main public functions


// Internal representations
















*/