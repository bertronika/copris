/*
 * User feature command parser
 *
 * Copyright (C) 2024 Nejc Bertoncelj <bertronika at mailo.com>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files 'main.c' and 'COPYING' for more details.
 */

#define _GNU_SOURCE // For strcasestr(3)

#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

#include <uthash.h>   /* uthash library - hash table      */
#include <utstring.h> /* uthash library - dynamic strings */

#include "Copris.h"
#include "debug.h"
#include "parse_value.h"
#include "user_command.h"
#include "tokeniser.h"

modeline_t parse_modeline(UT_string *copris_text)
{
	char *text = utstring_body(copris_text);

	if (utstring_len(copris_text) < 6 || (
	    utstring_len(copris_text) >= 6 && strncasecmp(text, "COPRIS", 6) != 0))
		return NO_MODELINE;

	if (utstring_len(copris_text) == 6 || (
	    utstring_len(copris_text) > 6 && text[6] == '\n'))
		return ML_EMPTY;

	modeline_t modeline = ML_UNKNOWN;
	text += 6; // Skip initial 'COPRIS'

	// Allow singular and plural command(s)
	if (strcasestr(text, "ENABLE-COMMAND") ||
	    strcasestr(text, "ENABLE-CMD"))
		modeline |= ML_ENABLE_CMD;

	if (strcasestr(text, "DISABLE-MARKDOWN") ||
	    strcasestr(text, "DISABLE-MD"))
		modeline |= ML_DISABLE_MD;

	return modeline;
}

void apply_modeline(UT_string *copris_text, modeline_t modeline)
{
	switch(modeline) {
	case NO_MODELINE:
		if (LOG_INFO)
			PRINT_MSG("No 'COPRIS <cmd>' modeline found, not parsing any variables.");

		return;
	case ML_EMPTY:
		if (LOG_ERROR)
			PRINT_MSG("Modeline is empty, ignoring it.");

		break;
	case ML_UNKNOWN:
		PRINT_ERROR_MSG("Modeline has unknown commands, ignoring it.");
		break;
	default:
		if (LOG_DEBUG)
			PRINT_MSG("Found valid modeline.");
		break;
	}

	// Get the text without a modeline
	const char *text = utstring_body(copris_text);
	char *text_without_ml = strchr(text, '\n');

	// Assume there's no further data if no newline is found
	if (text_without_ml == NULL) {
		utstring_clear(copris_text);
		if (LOG_INFO)
			PRINT_NOTE("There's no data after the modeline.");

		return;
	}

	// Skip '\n', get new text's length
	text_without_ml += 1;

	int ml_length = text_without_ml - text;
	assert(ml_length > 0);

	size_t text_without_ml_length = utstring_len(copris_text) - ml_length;

	// Create a copy of text without the modeline
	UT_string *temp_text;
	utstring_new(temp_text);
	utstring_bincpy(temp_text, text_without_ml, text_without_ml_length);

	utstring_clear(copris_text);
	utstring_concat(copris_text, temp_text);

	utstring_free(temp_text);
}

static void parse_extracted_variable(UT_string *text, struct Inifile **features,
                                     char *variable, size_t variable_len);
static int ispunct_custom(int c);

void parse_variables(UT_string *copris_text, struct Inifile **features)
{
	UT_string *output_text;
	utstring_new(output_text);

	UT_string *inner_token;
	utstring_new(inner_token);

	binary_token_t token_nl_storage, token_storage;
	binary_token_t token_nl = binary_tokeniser(copris_text, '\n', &token_nl_storage, true);
	while (token_nl.data != NULL) {
		// printf("str1: '%.*s' (%zu) last: %d\n", (int)token_nl.length, token_nl.data, token_nl.length, token_nl.last);
		utstring_bincpy(inner_token, token_nl.data, token_nl.length);
		binary_token_t token = binary_tokeniser(inner_token, ' ', &token_storage, true);

		while (token.data != NULL) {
			if (token.data[0] == USER_CMD_SYMBOL) {
				// Terminate the token to make it a string (replacing the white space separator),
				// except the last one, which doesn't have any separator at the end.
				if (!token_nl.last)
					token.data[--token.length] = '\0';

				// printf("cmd : '%s' (%zu) last/nl: %d/%d\n", token.data, token.length, token.last, token_nl.last);
				parse_extracted_variable(output_text, features, token.data, token.length);
			} else {
				// printf("str2: '%.*s' (%zu) last: %d\n", (int)token.length, token.data, token.length, token.last);
				utstring_bincpy(output_text, token.data, token.length);
			}
			token = binary_tokeniser(inner_token, ' ', &token_storage, false);
		}
		token_nl = binary_tokeniser(copris_text, '\n', &token_nl_storage, false);
		utstring_clear(inner_token);
	}

	utstring_free(inner_token);

	utstring_clear(copris_text);
	utstring_concat(copris_text, output_text);
	utstring_free(output_text);
}

static void parse_extracted_variable(UT_string *text, struct Inifile **features,
                              char *variable, size_t variable_len)
{
	// Skip the command symbol
	char *variable_name = variable + 1;
	--variable_len;

	// Comment variable
	if (*variable_name == '#')
		return;

	// Check for possible punctuation at the end of the variable and store it
	char *punct_suffix = NULL;
	size_t punct_len = 0;

	for (size_t i = 0; i < variable_len; i++) {
		if (ispunct_custom(variable_name[i])) {
			punct_suffix = strdup(variable_name + i);
			CHECK_MALLOC(punct_suffix);
			punct_len = variable_len - i;
			variable_name[i] = '\0';
			break;
		}
	}

	// Number variable
	if (isdigit(*variable_name)) {
		char parsed_number[2];
		// int element_count = parse_number_string(variable_name, &parsed_number, 1);
		int element_count = parse_number_string(variable_name, parsed_number, 2);

		if (element_count != -1) {
			utstring_bincpy(text, parsed_number, element_count);
		} else {
			if (LOG_ERROR)
				PRINT_MSG("Variable '%s' was skipped.", variable);
		}
		goto final;
	}

	// Command variable
	// Prepare a valid string for lookup
	char variable_lookup[MAX_INIFILE_ELEMENT_LENGTH];
	size_t variable_lookup_len = snprintf(variable_lookup, MAX_INIFILE_ELEMENT_LENGTH, \
	                                      "C_%s", variable_name);

	if (variable_lookup_len >= MAX_INIFILE_ELEMENT_LENGTH) {
		if (LOG_ERROR) {
			PRINT_MSG("Found command notation in following line, but it is "
			          "too long to be parsed.");
			PRINT_MSG(" %.*s...", MAX_INIFILE_ELEMENT_LENGTH, variable);
		}

		goto final;
	}

	struct Inifile *s = NULL;
	HASH_FIND_STR(*features, variable_lookup, s);

	if (!s) {
		if (LOG_ERROR)
			PRINT_MSG("Found variable '%s', but the command is not defined.", variable);

		goto final;
	}

	if (LOG_INFO)
		PRINT_MSG("Found variable '%s'.", variable);

	if (s->out_len > 0)
		utstring_bincpy(text, s->out, s->out_len);

	final:
	// Append back the found punctuation, if any, including the whitespace separator
	if (punct_suffix) {
		utstring_bincpy(text, punct_suffix, punct_len);
		utstring_bincpy(text, " ", 1);
		free(punct_suffix);
	}
	return;
}

static int ispunct_custom(int c)
{
	// Created using 'make_punct_list()'.
	switch (c) {
	case '!':
	case '"':
	case '#':
	case '%':
	case '&':
	case '\'':
	case '(':
	case ')':
	case '*':
	case '+':
	case ',':
	case '.':
	case '/':
	case ':':
	case ';':
	case '<':
	case '=':
	case '>':
	case '?':
	case '@':
	case '[':
	case '\\':
	case ']':
	case '^':
	case '`':
	case '{':
	case '|':
	case '}':
	case '~':
		return 1;
	}

	return 0;
}

/*
void make_punct_list(void)
{
	unsigned char c[3];
	c[2] = '\0';
	for (unsigned char i = 33; i < 127; i++) {
		if (ispunct(i)) {
			switch (i) {
				case '\'':
				case '\\':
					c[0] = '\\';
					c[1] = i;
					break;
				case '$':
				case '_':
				case '-':
					continue;
				default:
					c[0] = i;
					c[1] = '\0';
					break;
			}

			printf("\tcase '%s':%d\n", c, i);
		}
	}
}
*/
