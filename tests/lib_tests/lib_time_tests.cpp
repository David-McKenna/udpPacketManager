#include "gtest/gtest.h"
#include "lofar_udp_reader.h"
#include "lofar_udp_time.h"

// Random inputs through rand()
#include <cstdlib>

// CEP_HDR_SRC_OFFSET = 1
// CEP_HDR_TIME_OFFSET= 8
// CEP_HDR_SEQ_OFFSET = 12
// UDPHDRLEN = 16
const size_t sourceBytesOffset = 1;
const size_t timeBytesOffset = 8;
const size_t seqBytesOffset = 12;
const size_t hdrLen = 16;
struct dummyHeader {
	int8_t data[hdrLen] = {0,};

	void setTime(uint32_t time) {
		*((uint32_t*) &(data[timeBytesOffset])) = time;
	}

	void incrementTime() {
		*((uint32_t*) &(data[timeBytesOffset])) += 1;
	}

	void setSequence(int32_t seq) {
		*((int32_t*) &(data[seqBytesOffset])) = seq;
	}

	void setClock(int use200MHz) {
		(*((lofar_source_bytes*) &(data[sourceBytesOffset]))).clockBit = use200MHz;
	}
};

TEST(LibTimeTests, DeriveToPackets) {
	//long lofar_udp_time_get_packet_from_isot(const char *inputTime, const uint32_t clock200MHz);
	{
		SCOPED_TRACE("lofar_udp_time_get_packet_from_isot");

		ASSERT_EQ(-1, lofar_udp_time_get_packet_from_isot(nullptr, 0));
		EXPECT_EQ(-1, lofar_udp_time_get_packet_from_isot("This is not a time string", 0));

		const size_t numTestCases = 4;
		// https://random.limited/date-time-generator/
		const std::string inputDates[numTestCases] = {
			"2017-07-04T08:32:08",
			"2014-03-28T03:22:54",
			"2031-01-29T21:32:58",
			"2022-03-04T17:02:05",
		};
		const long resultTimes[numTestCases][2] = {
			{ 18300257910156 ,  14640206328125 },
			{ 17040734545898 ,  13632587636718 },
			{ 23528915747070 ,  18823132597656 },
			{ 20097818908691 ,  16078255126953 },
		};

		for (size_t i = 0; i < numTestCases; i++) {
			EXPECT_EQ(resultTimes[i][0], lofar_udp_time_get_packet_from_isot(inputDates[i].c_str(), 1));
			EXPECT_EQ(resultTimes[i][1], lofar_udp_time_get_packet_from_isot(inputDates[i].c_str(), 0));
		}

		// Expected Failure
		//EXPECT_EQ(1, lofar_udp_time_get_packet_from_isot("2022-02-29T01:01:01", 0)); // -- apparently this is a legal date for strptime / timegm?
		EXPECT_EQ(-1, lofar_udp_time_get_packet_from_isot("2022-02-02T26:01:01", 0));
		EXPECT_EQ(-1, lofar_udp_time_get_packet_from_isot("2222-02-02T26:01:01", 0));
		EXPECT_EQ(-1, lofar_udp_time_get_packet_from_isot("2022-02-02T22:22:22.22", 0));
	}

	//long lofar_udp_time_get_packets_from_seconds(double seconds, const uint32_t clock200MHz);
	{
		SCOPED_TRACE("lofar_udp_time_get_packets_from_seconds");

		const double packetsPerSecond200MHz = 1. / (5.12e-6) / 16; // (1. / 200e6) * 1024
		const double packetsPerSecond160MHz = 1. / (6.4e-6) / 16;  // (1. / 160e6) * 1024

		const double lowerLimit = 0.1;
		const double upperLimit = 3600 * 7 * 4;

		const size_t numTests = 4;
		for (size_t i = 0; i < numTests; i++) {
			double sample = lowerLimit + ((double) rand() / RAND_MAX) * (upperLimit - lowerLimit);
			EXPECT_EQ((long) (sample * packetsPerSecond200MHz), lofar_udp_time_get_packets_from_seconds(sample, 1));
			EXPECT_EQ((long) (sample * packetsPerSecond160MHz), lofar_udp_time_get_packets_from_seconds(sample, 0));
		}

	}
}

TEST(LibTimeTests, DerivedFromPackets) {


	//double lofar_udp_time_get_packet_time(const int8_t *inputData);
	{
		SCOPED_TRACE("lofar_udp_time_get_packet_time");

		dummyHeader header;

		//
		// The horrible (double) randomInt syntax used here is unfortunately
		//  intentional; FP64 epsilon is too large in a number of cases to
		//  correctly match the addition of the sequences to the time otherwise
		//

		const size_t numTests = 4;
		for (size_t i = 0; i < numTests; i++) {
			int32_t randomInt = rand();
			double random = (double) randomInt;
			header.setTime(randomInt);
			header.setClock(0);
			header.setSequence(0);
			EXPECT_DOUBLE_EQ(random, lofar_udp_time_get_packet_time(&(header.data[0])));
			header.setSequence(1);
			EXPECT_DOUBLE_EQ(randomInt + (1024. * 1. / 160e6), lofar_udp_time_get_packet_time(header.data));
			header.setSequence(156250);
			EXPECT_DOUBLE_EQ(randomInt + 1., lofar_udp_time_get_packet_time(header.data));

			header.setClock(1);
			header.setSequence(0);
			EXPECT_DOUBLE_EQ(random, lofar_udp_time_get_packet_time(header.data));
			header.setSequence(1);
			EXPECT_DOUBLE_EQ(random + (1024. * 1. / 200e6), lofar_udp_time_get_packet_time(header.data));
			header.setSequence(195312);
			EXPECT_DOUBLE_EQ(random + (1. - 5.12e-6 / 2), lofar_udp_time_get_packet_time(header.data));
			header.setSequence(195313);
			EXPECT_DOUBLE_EQ(random + (1. + 5.12e-6 / 2), lofar_udp_time_get_packet_time(header.data));
		}

	}

	//double lofar_udp_time_get_packet_time_mjd(const int8_t *inputData);
	{
		SCOPED_TRACE("lofar_udp_time_get_packet_time_mjd");
		dummyHeader header;


		const size_t numTests = 4;
		for (size_t i = 0; i < numTests; i++) {
			int32_t randomInt = rand();
			double random = ((double) randomInt / 86400.) + 40587.; // MJD conversion
			header.setTime(randomInt);
			header.setClock(0);
			header.setSequence(0);
			EXPECT_DOUBLE_EQ(random, lofar_udp_time_get_packet_time_mjd(&(header.data[0])));
			header.setSequence(1);
			EXPECT_DOUBLE_EQ(random + (1024. * 1. / 160e6) / 86400., lofar_udp_time_get_packet_time_mjd(header.data));
			header.setSequence(156250);
			EXPECT_DOUBLE_EQ(random + 1. / 86400., lofar_udp_time_get_packet_time_mjd(header.data));

			header.setClock(1);
			header.setSequence(0);
			EXPECT_DOUBLE_EQ(random, lofar_udp_time_get_packet_time_mjd(header.data));
			header.setSequence(1);
			EXPECT_DOUBLE_EQ(random + (1024. * 1. / 200e6) / 86400., lofar_udp_time_get_packet_time_mjd(header.data));
			header.setSequence(195312);
			EXPECT_DOUBLE_EQ(random + (1. - 5.12e-6 / 2) / 86400., lofar_udp_time_get_packet_time_mjd(header.data));
			header.setSequence(195313);
			EXPECT_DOUBLE_EQ(random + (1. + 5.12e-6 / 2) / 86400., lofar_udp_time_get_packet_time_mjd(header.data));
		}

	}



	// _get_packet_numer is a data wrapper to _beamformed_packno
	//inline long lofar_udp_time_get_packet_number(const int8_t *inputData);
	//inline long lofar_udp_time_beamformed_packno(uint32_t timestamp, uint32_t sequence, uint32_t clock200MHz);

	// _get_next is the same result we're feeding in to the next packet in our loops (well, it's actually simplified
	//  from the actual value we could provide here, but we're looping in the same manner, might as well check it here
	//  instead
	//inline uint32_t lofar_udp_time_get_next_packet_sequence(const int8_t *inputData);
	{
		SCOPED_TRACE("lofar_udp_time_get_packet_number");
		SCOPED_TRACE("lofar_udp_time_beamformed_packno");
		SCOPED_TRACE("lofar_udp_time_get_next_packet_sequence");

		dummyHeader header160M;
		dummyHeader header200M;
		header160M.setClock(0);
		header200M.setClock(1);

		const int32_t baseTime = 1600000000;
		const size_t numSeconds = 34; // Should see 200MHz rollover every 32 seconds of packets
		int32_t sequence = 0;
		//int8_t increment = 0;

		header160M.setTime(baseTime);
		header200M.setTime(baseTime);
		header160M.setSequence(sequence);
		header200M.setSequence(sequence);

		// clock160MHzSteps = 156250
		// cloc200MhzSteps = 195312.5
		float max160MHzSteps = 156250;
		float max200MHzSteps = 195312.5;

		int32_t currStep = 0;
		int64_t currPacket = 15625000000000;
		while (currStep < max160MHzSteps * numSeconds / 16) {
			EXPECT_EQ(currPacket, lofar_udp_time_get_packet_number(header160M.data));
			sequence += 16;
			EXPECT_EQ(sequence, lofar_udp_time_get_next_packet_sequence(header160M.data));
			if (sequence > (int32_t) max160MHzSteps) {
				sequence = sequence % (int32_t) max160MHzSteps;
				header160M.incrementTime();
			}
			header160M.setSequence(sequence);
			currPacket += 1;
			currStep += 1;
		}

		float sequenceFrac = 0;
		currStep = 0;
		currPacket = 19531250000000;
		while (currStep < max200MHzSteps * numSeconds / 16) {
			EXPECT_EQ(currPacket, lofar_udp_time_get_packet_number(header200M.data));
			sequenceFrac += 16.0f;
			EXPECT_EQ((int32_t) (sequenceFrac), lofar_udp_time_get_next_packet_sequence(header200M.data));
			if ((int32_t) sequenceFrac > (int32_t) max200MHzSteps) {
				sequenceFrac = (sequenceFrac - max200MHzSteps);
				header200M.incrementTime();
			}
			header200M.setSequence((int32_t) sequenceFrac);
			currPacket += 1;
			currStep += 1;
		}
	}
}

TEST(LibTimeTests, DerivedFromReader) {

	// void lofar_udp_time_get_current_isot(const lofar_udp_reader *reader, char *stringBuff, int strlen)
	// void lofar_udp_time_get_daq(const lofar_udp_reader *reader, char *stringBuff, int strlen);
	// reader -> startTime = lofar_udp_time_get_packet_time(reader->meta->inputData[0]);
	{
		SCOPED_TRACE("lofar_udp_time_get_current_isot");
		SCOPED_TRACE("lofar_udp_time_get_daq");

				dummyHeader header;
		header.setClock(1);
		lofar_udp_obs_meta *meta = (lofar_udp_obs_meta *) calloc(1, sizeof(lofar_udp_obs_meta));
		lofar_udp_reader *reader = _lofar_udp_reader_alloc(meta);
		reader->meta->inputData[0] = &(header.data[0]);

		const int64_t outputStrLen = 64;
		char output[outputStrLen] = "";

		lofar_udp_time_get_daq(nullptr, output, 0);
		EXPECT_EQ(0, strlen(output));
		ARR_INIT(output, outputStrLen, '\0');
		lofar_udp_time_get_daq(reader, output, 0);
		EXPECT_EQ(0, strlen(output));
		ARR_INIT(output, outputStrLen, '\0');

		lofar_udp_time_get_current_isot(nullptr, output, 0);
		EXPECT_EQ(0, strlen(output));
		ARR_INIT(output, outputStrLen, '\0');
		lofar_udp_time_get_current_isot(reader, output, 0);
		EXPECT_EQ(0, strlen(output));
		ARR_INIT(output, outputStrLen, '\0');

		struct timeStrPair {
			int32_t time;
			int32_t seq;
			std::string outputisot;
			std::string outputdaq;
		};

		const size_t numTestCases = 4;
		// Outputs are not perfect due to limits of double precision in the epoch basis
		timeStrPair inputOutputPairs[numTestCases] = {
			{ 1499157128 ,  0, "2017-07-04T08:32:08.000000" , "Tue Jul  4 08:32:08 2017" },
			{ 1395976974 ,  1, "2014-03-28T03:22:54.000005" , "Fri Mar 28 03:22:54 2014" },
			{ 1927488778 ,  195312, "2031-01-29T21:32:58.999997" , "Wed Jan 29 21:32:58 2031" },
			{ 1646413325 ,  195313, "2022-03-04T17:02:06.000003" , "Fri Mar  4 17:02:06 2022" }
		};

		for (auto testCase : inputOutputPairs) {
			header.setTime(testCase.time);
			header.setSequence(testCase.seq);
			lofar_udp_time_get_current_isot(reader, output, outputStrLen);
			EXPECT_STREQ(testCase.outputisot.c_str(), output);

			lofar_udp_time_get_daq(reader, output, outputStrLen);
			EXPECT_STREQ(testCase.outputdaq.c_str(), output);
		}


		lofar_udp_reader_cleanup(reader);
	}


}

TEST(LibTimeTests, ValidateConstValuesForGCC7) {
	ASSERT_EQ(clockStepsDelta, clock200MHzSteps - clock160MHzSteps);
	ASSERT_EQ(clockSampleTimeDelta, clock200MHzSampleTime - clock160MHzSampleTime);
	ASSERT_EQ(clockPacketRateDelta,clock200MHzPacketRate - clock160MHzPacketRate);
}

TEST(LibTimeTests, ValidateConstValues) {
	ASSERT_LT(0, clockStepsDelta);
	ASSERT_GT(0, clockSampleTimeDelta);
	ASSERT_LT(0, clockPacketRateDelta);
}