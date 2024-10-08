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

static void parse_extracted_variable(UT_string *text, struct Inifile **features,
                                     UT_string *variable);

void parse_variables(UT_string *copris_text, struct Inifile **features)
{
	UT_string *temp_text, *variable_name;
	utstring_new(temp_text);
	utstring_new(variable_name);

	char *s = utstring_body(copris_text);
	size_t l = utstring_len(copris_text);

	while (l > 0) {
		char *tok = memchr(s, VAR_SYMBOL, l);
		size_t tok_offset = tok ? (size_t)(tok - s) : l;

		// Main separator (VAR_SYMBOL) is located when token offset is zero.
		if (tok_offset > 0) {
			// No separator yet, copy text to output
			utstring_bincpy(temp_text, s, tok_offset);
			s += tok_offset;
			l -= tok_offset;
			continue;
		}

		if (s == tok) {
			// Separator located, treat token like a command
			const char *tok_end = strpbrk(s, VAR_SEPARATORS);
			size_t tok_len = tok_end ? (size_t)(tok_end - s) : l;
			int skip_char = 0;

			if (tok[tok_len - 1] == VAR_SYMBOL) {
				// $COMMAND$ - discard last two characters (e.g. omit \n)
				tok[tok_len - 1] = '\0';  // null one

				if (tok_end != NULL)      // discard the other
					skip_char = 1;

			} else if (tok_end != NULL && tok[tok_len] == VAR_TERMINATOR) {
				// $VARIABLE; - discard last character (join with following text)
				skip_char = 1;

			} else if (tok_end != NULL && tok[1] == VAR_COMMENT) {
				// $#VARIABLE - comment, discard last character
				skip_char = 1;
			}

			// printf("tok %2zu: '%.*s', end:%d, skip_char:%d\n", tok_len, (int)tok_len, tok, tok_end == NULL, skip_char);
			utstring_bincpy(variable_name, tok, tok_len);
			parse_extracted_variable(temp_text, features, variable_name);
			utstring_clear(variable_name);

			s += tok_len + skip_char;
			l -= tok_len + skip_char;
		}
	}
	utstring_free(variable_name);

	utstring_clear(copris_text);
	utstring_concat(copris_text, temp_text);
	utstring_free(temp_text);
}

static void parse_extracted_variable(UT_string *text, struct Inifile **features,
                                     UT_string *variable)
{
	// Skip the command symbol
	char *variable_name = utstring_body(variable) + 1;
	size_t variable_len = utstring_len(variable) - 1;

	// Comment variable
	if (*variable_name == VAR_COMMENT)
		return;

	// Escaped variable symbol
	if (*variable_name == VAR_SYMBOL) {
		utstring_bincpy(text, variable_name, variable_len);
		return;
	}

	// Number variable
	if (isdigit(*variable_name)) {
		char parsed_number[2];
		// int element_count = parse_values(variable_name, &parsed_number, 1);
		int element_count = parse_values(variable_name, parsed_number, 2);

		if (element_count != -1) {
			utstring_bincpy(text, parsed_number, element_count);
		} else {
			if (LOG_ERROR)
				PRINT_MSG("Variable '%s' was skipped.", utstring_body(variable));
		}
		return;
	}

	// Command variable
	// Prepare a valid string for lookup
	char variable_lookup[MAX_INIFILE_ELEMENT_LENGTH];
	size_t variable_lookup_len = snprintf(variable_lookup, MAX_INIFILE_ELEMENT_LENGTH,
	                                      "C_%s", variable_name);

	if (variable_lookup_len >= MAX_INIFILE_ELEMENT_LENGTH) {
		if (LOG_ERROR) {
			PRINT_MSG("Found command notation in following line, but it is "
			          "too long to be parsed.");
			PRINT_MSG(" %.*s...", MAX_INIFILE_ELEMENT_LENGTH, utstring_body(variable));
		}

		return;
	}

	struct Inifile *s = NULL;
	HASH_FIND_STR(*features, variable_lookup, s);

	if (!s) {
		utstring_bincpy(text, utstring_body(variable), utstring_len(variable));
		if (LOG_ERROR)
			PRINT_MSG("Found variable '%s', but the command is not defined.",
			          utstring_body(variable));

		return;
	}

	if (LOG_INFO)
		PRINT_MSG("Found variable '%s'.", utstring_body(variable));

	if (s->out_len > 0)
		utstring_bincpy(text, s->out, s->out_len);

	return;
}
