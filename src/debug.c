/*
 * debug.c
 * Debugging and logging interfaces
 * 
 * Part of COPRIS - a converting printer server
 * (c) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 */

#define COPRIS_VER "0.9"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

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
	time_t t = time(NULL);
	struct tm cur_time = *localtime(&t);
	printf("%02d.%02d.%d %02d:%02d:%02d copris[%d]: ",
		cur_time.tm_mday, cur_time.tm_mon + 1, cur_time.tm_year + 1900,
		cur_time.tm_hour, cur_time.tm_min, cur_time.tm_sec, getpid());
}


void copris_help() {
	printf("Usage: copris [-p PORT] (optional arguments) <printer location>\n\n");
	printf("  -p, --port     Port to listen to\n");
	printf("  -d, --daemon   Run continuously, as a daemon\n");
	printf("  -t, --trfile   (optional) character translation file\n");
	printf("\n");
	printf("  -v, --verbose  Be verbose (-vv more, -vvv even more)\n");
	printf("  -q, --quiet    Display nothing except fatal errors (stderr)\n");
	printf("  -h, --help     Show this help\n");
	printf("  -V, --version  Show program version and included printer \n");
	printf("                 feature sets\n");
	printf("\n");
	printf("Printer location can either be an actual printer address, such\n");
	printf("as /dev/ttyS0, or a file. If left empty, output is printed to STDOUT.\n");
	printf("\n");
}

void copris_version() {
	printf("COPRIS version %s\n", COPRIS_VER);
	printf("(C) 2020 Nejc Bertoncelj <nejc@bertoncelj.eu.org>\n\n");
// 	printf("Available/compiled options: -TRFILE -PRINTERDEF -IPV6\n\n");
	printf("Included printer feature sets:\n");
	printf("none\n");
	printf("\n");
}
