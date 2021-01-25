#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// XOPEN -> strptime requirement
#define __USE_XOPEN
#include <time.h>

#include "lofar_udp_reader.h"
#include "lofar_udp_misc.h"

#ifndef __LOFAR_CLI_META
#define __LOFAR_CLI_META

// Define prototypes
void helpMessages(void);
void processingModes(void);
long getStartingPacket(char inputTime[], const int clock200MHz);
long getSecondsToPacket(float seconds, const int clock200MHz);
void getStartTimeString(lofar_udp_reader *reader, char stringBuff[]);

// Exit reasons, 0, 1 aren't handled, only defined up to 3
extern const char exitReasons[4][1024];
#endif