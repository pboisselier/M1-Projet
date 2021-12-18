/*
 * Simple example utilizing the PCF8563 module on the ARPI600
 *
 * (c) Pierre Boisselier
 * 
 * Compile:
 *      cc -I/path/to/include/folder pcf8563.c -std=c11 -Wall -o pcf8563 
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpi600/pcf8563.h>
#include <time.h>
#include <errno.h>

int main(int argc, char **argv)
{
	time_t tm;
	if (pcf8563_init() != 0)
		perror("init pcf8563");
	if (pcf8563_read_time(&tm) != 0)
		perror("read time");
	printf("Is RTC running: %s\n", pcf8563_is_running(NULL) ? "yes" : "no");

	printf("RTC Time: %s\n", asctime(gmtime(&tm)));

	//	struct tm *tms = gmtime(&tm);
	//	tms->tm_mday += 1;
	//	tm = mktime(tms);

	time(&tm);
	if (pcf8563_set_time(&tm) != 0)
		perror("unable to se time");

	if (pcf8563_close() != 0)
		perror("unable to close pcf device");

	return EXIT_SUCCESS;
}