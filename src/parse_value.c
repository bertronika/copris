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

#include "debug.h"
#include "parse_value.h"

int parse_value_to_binary(const char *value, char *parsed_value, int length)
{
	const char *valuepos = value;
	char *endptr;   // Remaining text to be converted
	int i = 0;      // Parsed value iterator

	while (*valuepos) {
		long temp_value = strtol(valuepos, &endptr, 0);

		/* LCOV_EXCL_START */
		if (temp_value == LONG_MIN || temp_value == LONG_MAX) {
			PRINT_SYSTEM_ERROR("strtol", "Error parsing number.");
			return -1;
		}
		/* LCOV_EXCL_STOP */

		// Check if no conversion has been done
		if (valuepos == endptr) {
			PRINT_ERROR_MSG("Found an unrecognised character. Make sure your definitions consist "
			                "only of decimal, hexadecimal or octal values, separated by spaces. "
			                "Consult the README or the man page for more information.");

			return -1;
		}

		// Check if characters are still remaining
		//if (*endptr && LOG_DEBUG)
			//PRINT_MSG("strtol: remaining: '%s'.", endptr);

		if (temp_value < CHAR_MIN || temp_value > CHAR_MAX) {
			PRINT_ERROR_MSG("Value '%s' in '%s' is out of bounds.", valuepos, value);
			return -1;
		}

		if (i + 1 == length) {
			PRINT_ERROR_MSG("Definition '%s' is overlong. Shorten it or recompile COPRIS "
			                "with a bigger limit.", value);
			return -1;
		}

		parsed_value[i++] = (char)temp_value;
		valuepos = endptr;
	}
	parsed_value[i] = '\0';

	// Return number of parsed bytes
	return i;
}
