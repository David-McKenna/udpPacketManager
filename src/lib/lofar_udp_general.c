#include "lofar_udp_general.h"

void lofar_udp_signal_handler(int signalnum) {
	switch (signalnum) {
		case SIGPIPE:
			fprintf(stderr, "WARNING: SIGPIPE sent, reader/writer may have been pre-maturely closed. Attempting to continue.\n");
			break;

		case SIGFPE:
			fprintf(stderr, "WARNING: Floating point exception occurred. Attempting to continue.\n");
			break;
	}
}

int lofar_udp_prepare_signal_handler() {
	int handledSignals[] = {
		SIGPIPE,
		SIGFPE,
	};

	int numSignals = sizeof(handledSignals) / sizeof(handledSignals[0]);

	sig_t returnedFunc;
	for (int i = 0; i < numSignals; i++) {
		if ((returnedFunc = signal(handledSignals[i], lofar_udp_signal_handler)) != NULL) {
			fprintf(stderr, "WARNING: While attempting to set a catch for signal %d (i=%d), another function was returned indicating it was previously registered. Continue with caution.", handledSignals[i], i);
		}
	}

	return 0;
}

