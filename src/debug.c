/*
 * debug.c
 * Debugging and logging interfaces
 * 
 * Part of COPRIS - a converting printer server
 * (c) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef REL
#include <unistd.h> /* getpid() */
#include <time.h>
#endif

#include "debug.h"

/* 
 * Fatal errors, which go to STDERR. Arguments:
 *  errnum	function return code
 *  perr	system error message (errno)
 *  mesg	custom error message
 */
void log_perr(int errnum, char *perr, char *mesg) {
	if(errnum < 0) {
		fprintf(stderr, "%s\n", mesg);
		perror(perr);
		exit(1);
	}
}

/* Verbosity levels:
 * 0  silent/fatal
 * 1  err         (default)
 * 2  info
 * 3  debug
 */

int log_err() {
	return (verbosity > 0) ? 1 : 0;
}

int log_info() {
	return (verbosity > 1) ? 1 : 0;
}

int log_debug() {
	return (verbosity > 2) ? 1 : 0;
}

void log_date() {
#ifdef REL
	time_t t = time(NULL);
	struct tm cur_time = *localtime(&t);
	printf("%02d.%02d.%d %02d:%02d:%02d copris[%d]: ",
		cur_time.tm_mday, cur_time.tm_mon + 1, cur_time.tm_year + 1900,
		cur_time.tm_hour, cur_time.tm_min, cur_time.tm_sec, getpid());
#endif
}
