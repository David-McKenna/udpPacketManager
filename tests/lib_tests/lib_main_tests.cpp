#include "gtest/gtest.h"
#include "lofar_udp_general.h"


TEST(LibSignalTests, SignalSetup) {
	//int _lofar_udp_prepare_signal_handler()
	EXPECT_EQ(0, _lofar_udp_prepare_signal_handler());
}

TEST(LibSignalTests, SignalPipe) {
	//void _lofar_udp_signal_handler(int signalnum)
	ASSERT_NO_THROW(_lofar_udp_signal_handler(SIGPIPE));
}

#define NO_EXIT_TESTS
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