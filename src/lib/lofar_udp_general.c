#include "lofar_udp_general.h"


void _lofar_udp_signal_handler_stack(int signalnum) {
	const int32_t maxStack = 32;
	void *backtraceBuffer[maxStack];
	int32_t numPtrs = backtrace(backtraceBuffer, maxStack);
	char **funcNames = backtrace_symbols(backtraceBuffer, numPtrs);
	if (funcNames == NULL) {
		fprintf(stderr, "ERROR: Failed to get stack (%d: %s).\n", errno, strerror(errno));
		exit(~signalnum);
	} else {
		for (int8_t i = 0; i < numPtrs; i++) {
			fprintf(stderr, "\t%s\n", funcNames[i]);
		}
	}
	free(funcNames);

	exit(signalnum);
}

void _lofar_udp_signal_handler(int signalnum) {
	switch (signalnum) {
		case SIGPIPE:
			fprintf(stderr, "WARNING: SIGPIPE sent, reader/writer may have been pre-maturely closed. Attempting to continue.\n");
			break;

		/*
		case SIGFPE:
			fprintf(stderr, "WARNING: Floating point exception occurred. Attempting to continue.\n");
			break;
		*/

		case SIGSEGV:
			fprintf(stderr, "ERROR: Segfault occurred and program will exit.\n");
			_lofar_udp_signal_handler_stack(signalnum);
			__builtin_unreachable();
			break;

		default:

			fprintf(stderr, "ERROR %s: Signal %d sent unexpectedly, exiting.\n", __func__, signalnum);
			_lofar_udp_signal_handler_stack(signalnum);
			__builtin_unreachable();
			break;
	}
}


int _lofar_udp_prepare_signal_handler() {
	int handledSignals[] = {
		SIGPIPE,
		//SIGFPE,
		SIGSEGV,
	};

	int numSignals = sizeof(handledSignals) / sizeof(handledSignals[0]);

	struct sigaction regSignal;
	regSignal.sa_handler = _lofar_udp_signal_handler;
	for (int i = 0; i < numSignals; i++) {
		sigemptyset(&regSignal.sa_mask);

		if (sigaction(handledSignals[i], &regSignal, NULL) == -1) {
			fprintf(stderr, "WARNING: While attempting to set a catch for signal %d (i=%d), an error was raised (%d, %s). Continue with caution.", handledSignals[i], i, errno, strerror(errno));
		}
	}

	return 0;
}

/**
 * @brief strtol, but for integers
 *
 * @param str String to parse
 * @param endPtr Working pointer, will be updated to first invalid character in string
 *
 * @return INT32_MAX: Failure, Other: success
 */
int32_t internal_strtoi(char *str, char **endPtr) {
	long intermediate = strtol(str, endPtr, 10);

	if (intermediate < INT32_MIN || intermediate > INT32_MAX) {
		*(endPtr) = str;
		errno = ERANGE;
		return INT32_MAX;
	}

	return (int32_t) intermediate;
}

/**
 * @brief strtol, but for 8-bit ints
 *
 * @param str String to parse
 * @param endPtr Working pointer, will be updated to first invalid character in string
 *
 * @return INT32_MAX: Failure, Other: success
 */
int8_t internal_strtoc(char *str, char **endPtr) {
	int64_t intermediate = strtol(str, endPtr, 10);

	if (intermediate < INT8_MIN || intermediate > INT8_MAX) {
		*(endPtr) = str;
		errno = ERANGE;
		return INT8_MAX;
	}

	return (int8_t) intermediate;
}


/**
 * Copyright (C) 2023 David McKenna
 * This file is part of udpPacketManager <https://github.com/David-McKenna/udpPacketManager>.
 *
 * udpPacketManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * udpPacketManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with udpPacketManager.  If not, see <http://www.gnu.org/licenses/>.
 **/
