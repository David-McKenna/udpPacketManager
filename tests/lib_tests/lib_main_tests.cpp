#include "gtest/gtest.h"
#include "lofar_udp_general.h"


TEST(LibSignalTests, SignalHandler) {
	//int lofar_udp_prepare_signal_handler()
	EXPECT_EQ(0, lofar_udp_prepare_signal_handler());

	//void lofar_udp_signal_handler(int signalnum)
	EXPECT_NO_THROW(lofar_udp_signal_handler(SIGPIPE));
	EXPECT_DEATH(lofar_udp_signal_handler(SIGKILL), ".*");

}