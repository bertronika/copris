/*
 * Modeline and variable parser
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
#include "parse_vars.h"
#include "parse_value.h"

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

	// Allow singular and plural VARIABLE(S)
	if (strcasestr(text, "ENABLE-VARIABLE") ||
	    strcasestr(text, "ENABLE-VAR"))
		modeline |= ML_ENABLE_VAR;

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

static int parse_extracted_variable(UT_string *text, struct Inifile **features,
                                    UT_string *variable);

void parse_variables(UT_string *copris_text, struct Inifile **features)
{
	UT_string *temp_text, *variable_name;
	utstring_new(temp_text);
	utstring_new(variable_name);

	char *s = utstring_body(copris_text);
	size_t l = utstring_len(copris_text);

	while (l > 0) {
		// Find the next symbol denoting a variable
		char *tok = memchr(s, VAR_SYMBOL, l);
		size_t tok_offset = tok ? (size_t)(tok - s) : l;

		// Main separator (VAR_SYMBOL) is located when token offset is zero,
		// else, it's ordinary text or data
		if (tok_offset > 0) {
			// No separator yet, copy text to output
			utstring_bincpy(temp_text, s, tok_offset);
			s += tok_offset;
			l -= tok_offset;
			continue;
		}

		if (s == tok) {
			// Separator located, treat token like a command until the end of line
			const char *tok_end = strchr(s, '\n');
			size_t tok_len = tok_end ? (size_t)(tok_end - s) : l;
			int skip_newline = 0;

			if (tok_end != NULL && tok[1] == VAR_COMMENT) {
				// Line begins with $# - interpret as comment, discard its new line
				skip_newline = 1;
				goto skip_parse;
			}

			// printf("tok %2zu: '%.*s', end:%d, skip_newline:%d\n", tok_len, (int)tok_len, tok, tok_end == NULL, skip_newline);

			// Parse contents of the variable
			utstring_bincpy(variable_name, tok, tok_len);
			int error = parse_extracted_variable(temp_text, features, variable_name);
			utstring_clear(variable_name);

			// If parse fails, nothing, except the newline, gets copied to output.
			// Thus, remove it.
			if (error)
				skip_newline = 1;

			skip_parse:
			s += tok_len + skip_newline;
			l -= tok_len + skip_newline;
		}
	}
	utstring_free(variable_name);

	utstring_clear(copris_text);
	utstring_concat(copris_text, temp_text);
	utstring_free(temp_text);
}

static int parse_extracted_variable(UT_string *text, struct Inifile **features,
                                    UT_string *variable)
{
	// Skip the command symbol
	char *variable_name = utstring_body(variable) + 1;
	size_t variable_len = utstring_len(variable) - 1;
	// TODO len>0?

	// Escaped variable symbol
	if (*variable_name == VAR_SYMBOL) {
		utstring_bincpy(text, variable_name, variable_len);
		return 0;
	}

	int element_count = parse_values_with_variables(variable_name, variable_len, text, features);

	if (element_count == -1) {
		if (LOG_ERROR)
			PRINT_MSG("Variable '%s' was skipped.", utstring_body(variable));

		return -1;
	}

	// if (!s) {
	// 	utstring_bincpy(text, utstring_body(variable), utstring_len(variable));
	// 	if (LOG_ERROR)
	// 		PRINT_MSG("Found variable '%s', but the command is not defined.",
	// 		          utstring_body(variable));

	// 	return;
	// }

	if (LOG_INFO)
		PRINT_MSG("Found variable '%s'.", utstring_body(variable));

	return 0;
}
