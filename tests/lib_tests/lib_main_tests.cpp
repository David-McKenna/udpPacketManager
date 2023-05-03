#include "gtest/gtest.h"
#include "lofar_udp_general.h"

TEST(LibGenTests, interalStrtoi) {
	// int32_t internal_strtoi(char *str, char **endPtr)

	const int32_t bufferLen = 64;
	char *endPtr, *buffer = (char*) calloc(bufferLen, sizeof(char));

	snprintf(buffer, bufferLen, "%ld", ((int64_t) INT32_MAX) * 2);
	EXPECT_EQ(INT32_MAX, internal_strtoi(buffer, &endPtr));
	EXPECT_EQ(endPtr, buffer);
	snprintf(buffer, bufferLen, "%ld", ((int64_t) INT32_MIN) * 2);
	EXPECT_EQ(INT32_MAX, internal_strtoi(buffer, &endPtr));
	EXPECT_EQ(endPtr, buffer);

	char *expectedEndPtr = buffer + snprintf(buffer, bufferLen, "%d", INT32_MAX - 1);
	EXPECT_EQ(INT32_MAX - 1, internal_strtoi(buffer, &endPtr));
	EXPECT_EQ(expectedEndPtr, endPtr);

	expectedEndPtr = buffer + snprintf(buffer, bufferLen, "%d", INT32_MIN + 1);
	EXPECT_EQ(INT32_MIN + 1, internal_strtoi(buffer, &endPtr));
	EXPECT_EQ(expectedEndPtr, endPtr);

	free(buffer);

}

TEST(LibGenTests, interalStrtoc) {
	// int8_t internal_strtoc(char *str, char **endPtr)

	const int32_t bufferLen = 64;
	char *endPtr, *buffer = (char*) calloc(bufferLen, sizeof(char));

	snprintf(buffer, bufferLen, "%ld", ((int64_t) INT8_MAX) * 2);
	EXPECT_EQ(INT8_MAX, internal_strtoc(buffer, &endPtr));
	EXPECT_EQ(endPtr, buffer);
	snprintf(buffer, bufferLen, "%ld", ((int64_t) INT8_MIN) * 2);
	EXPECT_EQ(INT8_MAX, internal_strtoc(buffer, &endPtr));
	EXPECT_EQ(endPtr, buffer);

	char *expectedEndPtr = buffer + snprintf(buffer, bufferLen, "%d", INT8_MAX - 1);
	EXPECT_EQ(INT8_MAX - 1, internal_strtoc(buffer, &endPtr));
	EXPECT_EQ(expectedEndPtr, endPtr);

	expectedEndPtr = buffer + snprintf(buffer, bufferLen, "%d", INT8_MIN + 1);
	EXPECT_EQ(INT8_MIN + 1, internal_strtoc(buffer, &endPtr));
	EXPECT_EQ(expectedEndPtr, endPtr);

	free(buffer);

}

TEST(LibSignalTests, SignalSetup) {
	//int _lofar_udp_prepare_signal_handler()
	EXPECT_EQ(0, _lofar_udp_prepare_signal_handler());
}

TEST(LibSignalTests, SignalPipe) {
	//void _lofar_udp_signal_handler(int signalnum)
	ASSERT_NO_THROW(_lofar_udp_signal_handler(SIGPIPE));
}

//#define NO_EXIT_TESTS
#ifndef NO_EXIT_TESTS
TEST(LibSignalTests, SignalSegv) {
	//void _lofar_udp_signal_handler(int signalnum)
	ASSERT_EXIT(_lofar_udp_signal_handler(SIGSEGV), ::testing::ExitedWithCode(SIGSEGV), "");
}

// Non-defined signals, these should never be registered, but we want to make sure that we exit by default
TEST(LibSignalTests, SignalKill) {
	ASSERT_EXIT(_lofar_udp_signal_handler(SIGKILL), ::testing::ExitedWithCode(SIGKILL), ".*Signal " + std::to_string(SIGKILL) + " .*sent unexpectedly.*");
}

TEST(LibSignalTests, SignalAlarm) {
	ASSERT_EXIT(_lofar_udp_signal_handler(SIGALRM), ::testing::ExitedWithCode(SIGALRM), ".*Signal " + std::to_string(SIGALRM) + " .*sent unexpectedly.*");
}
#endif