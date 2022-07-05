/*
 * Translation file handling and text conversion
 *
 * Copyright (C) 2020-2022 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include <ini.h>      /* inih library - .ini file parser  */
#include <uthash.h>   /* uthash library - hash table      */
#include <utstring.h> /* uthash library - dynamic strings */

#include "Copris.h"
#include "debug.h"
#include "translate.h"
#include "utf8.h"
#include "parse_value.h"

static int inih_handler(void *, const char *, const char *, const char *);

int load_translation_file(char *filename, struct Inifile **trfile) {
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		PRINT_SYSTEM_ERROR("fopen", "Failed to open translation file.");
		return -1;
	}

	if (LOG_DEBUG)
		PRINT_MSG("`%s': parsing translation file:", filename);

	// `Your hash must be declared as a NULL-initialized pointer to your structure.'
	*trfile = NULL;

	int parse_error = ini_parse_file(file, inih_handler, trfile);

	// If there's a parse error, properly close the file before exiting
	int error = -1;

	// Negative return number - can be either:
	// -1  Error opening file - we've already handled this
	// -2  Memory allocation error - only if inih was built with INI_USE_STACK
	if (parse_error < 0) {
		PRINT_ERROR_MSG("inih: ini_malloc: Memory allocation error.");
		goto close_file;
	}

	// Positive return number - returned error is a line number
	if (parse_error > 0) {
		PRINT_ERROR_MSG("`%s': (first) fault on line %d.", filename, parse_error);
		goto close_file;
	}

	if (LOG_INFO) {
		int definition_count = HASH_COUNT(*trfile);
		PRINT_MSG("`%s': loaded %d translation file definitions.",
		          filename, definition_count);
	}

	error = 0;

	// Why parentheses? warning: a label can only be part of a statement and a declaration
	//                  is not a statement [-Wpedantic]
	close_file: {
		int tmperr = fclose(file);
		if (tmperr != 0) {
			PRINT_SYSTEM_ERROR("close", "Failed to close translation file.");
			return -1;
		}
	}

	return error;
}

/*
 * [section]
 * name = value  (inih library)
 * key  = item   (uthash)
 */
static int inih_handler(void *user, const char *section, const char *name, const char *value)
{
	(void)section;

	size_t name_len  = strlen(name);
	size_t value_len = strlen(value);

	if (name_len == 0 || value_len == 0) {
		PRINT_ERROR_MSG("Found an entry with either no name or no value.");
		return COPRIS_PARSE_FAILURE;
	}

	if (value_len > MAX_INIFILE_ELEMENT_LENGTH) {
		PRINT_ERROR_MSG("`%s': value length exceeds maximum of %zu bytes.", value,
		                (size_t)MAX_INIFILE_ELEMENT_LENGTH);
		return COPRIS_PARSE_FAILURE;
	}

	if (utf8_count_codepoints(name, 2) > 1) {
		PRINT_ERROR_MSG("`%s': name has more than one character.", name);
		return COPRIS_PARSE_FAILURE;
	}

	struct Inifile **file = (struct Inifile**)user;  // Passed from caller
	struct Inifile *s;                               // Local to this function

	// Check if this 'name' already exists
	HASH_FIND_STR(*file, name, s);
	bool name_overwritten = (s != NULL);

	// Add a new key if it doesn't already exist
	if (!name_overwritten) {
		s = malloc(sizeof *s);
		if (s == NULL) {
			PRINT_ERROR_MSG("Memory allocation error.");
			return COPRIS_PARSE_FAILURE;
		}

		memcpy(s->in, name, name_len + 1);
		HASH_ADD_STR(*file, in, s);
	}

	int element_count = 0;

	// Parse value if it wasn't explicitly specified to be empty
	if (*value != '@') {
		char parsed_value[MAX_INIFILE_ELEMENT_LENGTH];
		element_count = parse_value_to_binary(value, parsed_value, (sizeof parsed_value) - 1);

		// Check for a parse error
		if (element_count == -1) {
			PRINT_ERROR_MSG("Failure while processing value for `%s'.", name);
			free(s);
			return COPRIS_PARSE_FAILURE;
		}

		memcpy(s->out, parsed_value, element_count + 1);
	} else {
		*s->out = '\0';
	}

	if (LOG_DEBUG) {
		PRINT_LOCATION(stdout);
		printf(" %1s (%zu) =>", s->in, name_len);

		if (element_count == 0)
			printf(" (empty)");
		else
			for (int i = 0; i < element_count; i++)
				printf(" 0x%X", s->out[i]);

		if (name_overwritten)
			printf(" (overwriting old value)");

		printf("\n");
	}

	return COPRIS_PARSE_SUCCESS;
}

void unload_translation_file(struct Inifile **trfile) {
	struct Inifile *definition;
	struct Inifile *tmp;
	int count = 0;

	HASH_ITER(hh, *trfile, definition, tmp) {
		HASH_DEL(*trfile, definition);
		free(definition);
		count++;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Unloaded translation file (count = %d).", count);
}

void translate_text(UT_string *copris_text, struct Inifile **trfile)
{
	UT_string *translated_text;
	utstring_new(translated_text);

	char *original = utstring_body(copris_text);

	while (*original) {
		char input_char[UTF8_MAX_LENGTH + 1];
		size_t input_len;
		char output_char[MAX_INIFILE_ELEMENT_LENGTH];
		size_t output_len;
		struct Inifile *s;

		if (UTF8_IS_MULTIBYTE(*original)) {
			input_len = utf8_codepoint_length(*original);
			memcpy(input_char, original, input_len);
		} else {
			input_len = 1;
			input_char[0] = *original;
		}
		input_char[input_len] = '\0';

		HASH_FIND_STR(*trfile, input_char, s);
		if (s) {
			// Definition found
			output_len = strlen(s->out);
			memcpy(output_char, s->out, output_len);
		} else {
			// Definition not found, copy original
			output_len = input_len;
			memcpy(output_char, input_char, output_len);
		}

		utstring_bincpy(translated_text, output_char, output_len);
		original += input_len;
	}

	utstring_clear(copris_text);
	utstring_bincpy(copris_text, utstring_body(translated_text), utstring_len(translated_text));
	utstring_free(translated_text);
}
