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

#include "debug.h"

int raise_perror(int return_value, char *function_name, char *message) {
	if(return_value < 0) {
		fprintf(stderr, "%s\n", message);
		perror(function_name);
		return 1;
	}

	return 0;
}

int raise_errno_perror(int received_errno, char *function_name, char *message) {
	if(received_errno != 0)
		return raise_perror(-1, function_name, message);

	return 0;
}
