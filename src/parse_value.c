/*
 * Parse a string with multiple space-separated numbers into an array
 *
 * Copyright (C) 2020-2022 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include "debug.h"
#include "parse_value.h"

int parse_value_to_binary(const char *value, char *parsed_value, int length)
{
	const char *valuepos = value;
	char *endptr;   // Remaining text to be converted
	int i = 0;      // Parsed value iterator

	while (*valuepos) {
		errno = 0;
		long temp_value = strtol(valuepos, &endptr, 0);

		if (raise_errno_perror(errno, "strtol", "Error parsing number."))
			return -1;

		// Check if no conversion has been done
		if (valuepos == endptr) {
			if (LOG_DEBUG)
				PRINT_MSG("Found unrecognised characters.");

			return -1;
		}

		// Check if characters are still remaining
		//if (*endptr && LOG_DEBUG)
			//PRINT_MSG("strtol: remaining: '%s'.", endptr);

		// Check if value fits
		if (temp_value < CHAR_MIN || temp_value > CHAR_MAX) {
			PRINT_ERROR_MSG("'%s': value out of bounds.", value);
			return -1;
		}

		parsed_value[i++] = temp_value;
		valuepos = endptr;
		assert(i <= length);
	}
	parsed_value[i] = '\0';

	// Return number of parsed bytes
	return i;
}
