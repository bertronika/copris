/*
 * debug.c
 * Debugging and logging interfaces
 * 
 * Copyright (C) 2020-2021 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG
#include <unistd.h> /* getpid() */
#include <time.h>
#endif

#include "debug.h"

/* 
 * Fatal errors that go to stderr. Arguments:
 *  retnum  function return number
 *  errnum  system error number (errno)
 *  mesg    custom error message
 */
int log_perr(int retnum, char *errnum, char *message) {
	if(retnum < 0) {
		fprintf(stderr, "%s\n", message);
		perror(errnum);
		return 1;
	}

	return 0;
}

/*
 * Verbosity levels:
 * 0  silent/fatal
 * 1  error       (default)
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
#ifdef DEBUG
	time_t t = time(NULL);
	struct tm cur_time = *localtime(&t);
	printf("%02d.%02d.%d %02d:%02d:%02d copris[%d]: ",
		cur_time.tm_mday, cur_time.tm_mon + 1, cur_time.tm_year + 1900,
		cur_time.tm_hour, cur_time.tm_min, cur_time.tm_sec, getpid());
#endif
}
