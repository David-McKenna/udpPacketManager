#include "lofar_udp_general.h"


void lofar_udp_signal_handler(int signalnum) {
	switch (signalnum) {
		case SIGPIPE:
			fprintf(stderr, "WARNING: SIGPIPE sent, reader/writer may have been pre-maturely closed. Attempting to continue.\n");
			break;
		/*
		case SIGFPE:
			fprintf(stderr, "WARNING: Floating point exception occurred. Attempting to continue.\n");
			break;
		*/
		default:

			fprintf(stderr, "ERROR %s: Signal %d sent unexpectedly, exiting.\n", __func__, signalnum);
			exit(EXIT_FAILURE);
	}
}


int lofar_udp_prepare_signal_handler() {
	int handledSignals[] = {
		SIGPIPE,
		//SIGFPE,
	};

	int numSignals = sizeof(handledSignals) / sizeof(handledSignals[0]);

	struct sigaction regSignal;
	regSignal.sa_handler = lofar_udp_signal_handler;
	for (int i = 0; i < numSignals; i++) {
		sigemptyset(&regSignal.sa_mask);

		if (sigaction(handledSignals[i], &regSignal, NULL) == -1) {
			fprintf(stderr, "WARNING: While attempting to set a catch for signal %d (i=%d), an error was raised (%d, %s). Continue with caution.", handledSignals[i], i, errno, strerror(errno));
		}
	}

	return 0;
}

int32_t internal_strtoi(char *str, char **endPtr) {
	long intermediate = strtol(str, endPtr, 10);

	if (intermediate < INT32_MIN || intermediate > INT32_MAX) {
		*(endPtr) = str;
		errno = ERANGE;
		return INT32_MAX;
	}

	return (int32_t) intermediate;
}
