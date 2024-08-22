/*
 * String-parsing functions
 *
 * Copyright (C) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files 'main.c' and 'COPYING' for more details.
 */

// For 'strndup' in ISO C
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include <uthash.h>   /* uthash library - hash table       */
#include <utstring.h> /* uthash library - dynamic strings  */

#include "Copris.h"
#include "debug.h"
#include "parse_value.h"

int parse_all_to_commands(const char *value, size_t value_len,
                          UT_string *parsed_value, struct Inifile **features)
{
	char *value_copy = strndup(value, value_len + 1);
	CHECK_MALLOC(value_copy);

	int element_count = 0;
	// int variable_count = 0;

	char *token = strtok(value_copy, " ");
	while (token != NULL) {
		if (!isdigit(token[0])) {
			// Value is a variable
			if (token[0] != 'C' && token[0] != 'F') {
				PRINT_ERROR_MSG("Variables must be prefixed with either 'C_' or 'F_', and "
				                "'%s' is with neither of them.", token);
				break;
			}

			struct Inifile *s;
			HASH_FIND_STR(*features, token, s);
			if (s == NULL) {
				PRINT_ERROR_MSG("Internal variable '%s' does not exist. If it is a custom "
				                "command, make sure it has the 'C_' prefix.", token);
				break;
			}

			if (s->out_len == 0) {
				PRINT_ERROR_MSG("Variable '%s' does not (yet) exist. Custom command "
				                "should be specified after it.", token);
				break;
			}

			// variable_count++;
			utstring_bincpy(parsed_value, s->out, s->out_len);
			element_count += s->out_len;
		} else {
			// Value is a number
			char parsed_token[MAX_INIFILE_ELEMENT_LENGTH];
			int new_value_len = parse_number_string(token, parsed_token,
			                                        (sizeof parsed_token) - 1);

			if (new_value_len == -1)
				break;

			utstring_bincpy(parsed_value, parsed_token, new_value_len);
			element_count += new_value_len;
		}
		token = strtok(NULL, " ");
	}

	free(value_copy);

	if (token != NULL)
		return -1;

	//PRINT_MSG("%d variable(s), %d element(s).", variable_count, element_count);
	return element_count;
}

int parse_number_string(const char *value, char *parsed_value, int parsed_value_len)
{
	const char *valuepos = value;
	char *endptr;   // Remaining text to be converted
	int i = 0;      // Parsed value iterator

	while (*valuepos) {
		long temp_value = strtol(valuepos, &endptr, 0);

		if (temp_value == LONG_MIN || temp_value == LONG_MAX) {
			PRINT_SYSTEM_ERROR("strtol", "Error parsing number.");
			return -1;
		}

		// Check if no conversion has been done
		if (valuepos == endptr) {
			PRINT_ERROR_MSG("Found unrecognised character(s) '%s'. Make sure your values "
			                "consist only of space-separated decimal, hexadecimal or octal "
			                "numbers.", valuepos);

			return -1;
		}

		if ((unsigned long)temp_value > 255) { // 8 bytes
			if (strcmp(valuepos, value) == 0)
				PRINT_ERROR_MSG("Value '%s' is out of bounds.", valuepos);
			else
				PRINT_ERROR_MSG("Value '%s', part of '%s', is out of bounds.", valuepos, value);
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
