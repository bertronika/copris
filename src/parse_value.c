/*
 * String-parsing functions
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

#include <uthash.h>   /* uthash library - hash table       */
#include <utstring.h> /* uthash library - dynamic strings  */

#include "Copris.h"
#include "debug.h"
#include "parse_value.h"

int parse_all_to_commands(const char *value, size_t value_len,
                          UT_string *parsed_value, struct Inifile **prset)
{
	char *value_copy = strndup(value, value_len + 1);
	if (value_copy == NULL) {
		PRINT_ERROR_MSG("Memory allocation error.");
		exit(1);
	}

	int element_count = 0;
	int variable_count = 0;
	bool problem = false;

	char *token = strtok(value_copy, " ");
	while (token != NULL && !problem) {
		if (token[0] == 'C' || token[0] == 'F') {
			// Value is a variable
			struct Inifile *s;
			HASH_FIND_STR(*prset, token, s);
			if (s == NULL) {
				PRINT_ERROR_MSG("Internal variable `%s' does not exist. If it is a custom "
				                "command, make sure it has the `C_' prefix.", token);
				problem = true;
			} else if (*s->out == '\0') {
				PRINT_ERROR_MSG("Variable `%s' does not (yet) exist. Custom command "
				                "should be specified after it.", token);
				problem = true;
			} else {
				int new_value_len = (int)strlen(s->out);
				variable_count++;

				utstring_bincpy(parsed_value, s->out, new_value_len);
				element_count += new_value_len;
			}
		} else {
			// Value is a number
			char parsed_token[MAX_INIFILE_ELEMENT_LENGTH];
			int new_value_len = parse_numbers_to_commands(token, parsed_token,
			                                              (sizeof parsed_token) - 1);

			if (new_value_len == -1) {
				problem = true;
			} else {
				utstring_bincpy(parsed_value, parsed_token, new_value_len);
				element_count += new_value_len;
			}
		}
		token = strtok(NULL, " ");
	}

	free(value_copy);

	if (problem)
		return -1;

	//PRINT_MSG("%d variable(s), %d element(s).", variable_count, element_count);
	return element_count;
}

int parse_numbers_to_commands(const char *value, char *parsed_value, int parsed_value_len)
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
			PRINT_ERROR_MSG("Found an unrecognised character. Make sure your values consist "
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

		if (i + 1 == parsed_value_len) {
			PRINT_ERROR_MSG("Value '%s' is overlong. Shorten it or recompile COPRIS "
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
