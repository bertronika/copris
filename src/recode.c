/*
 * Encoding file handling and text recoding
 *
 * Terminology note: encoding files consist of definitions.
 *
 * Copyright (C) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files 'main.c' and 'COPYING' for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <ini.h>      /* inih library - .ini file parser  */
#include <uthash.h>   /* uthash library - hash table      */
#include <utstring.h> /* uthash library - dynamic strings */

#include "Copris.h"
#include "debug.h"
#include "recode.h"
#include "utf8.h"
#include "parse_value.h"

static int inih_handler(void *, const char *, const char *, const char *);

bool error_known = false;

int load_encoding_file(const char *filename, struct Inifile **encoding)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		PRINT_SYSTEM_ERROR("fopen", "Failed to open encoding file '%s'.", filename);
		return -1;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Parsing encoding file '%s':", filename);

	int parse_error = ini_parse_file(file, inih_handler, encoding);

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
		PRINT_ERROR_MSG("'%s': (first) fault on line %d.", filename, parse_error);

		if (!error_known) // Error unknown, perhaps it is a INI section problem
			PRINT_ERROR_MSG("Have you used a '[' character for a name? Escape it "
			                "with a backslash.");
		goto close_file;
	}

	int definition_count = HASH_COUNT(*encoding);

	if (LOG_INFO)
		PRINT_MSG("Loaded %d definitions from encoding file '%s'.", definition_count, filename);

	if (definition_count < 1)
		PRINT_NOTE("Your encoding file appears to be empty.");

	error = 0;

	// Why parentheses? warning: a label can only be part of a statement and a declaration
	//                  is not a statement [-Wpedantic]
	close_file: {
		int tmperr = fclose(file);
		if (tmperr != 0) {
			PRINT_SYSTEM_ERROR("fclose", "Failed to close encoding file '%s'.", filename);
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
// 'Handler should return nonzero on success, zero on error.'
#define COPRIS_PARSE_FAILURE   0
#define COPRIS_PARSE_SUCCESS   1
#define COPRIS_PARSE_DUPLICATE 2

static int inih_handler(void *user, const char *section, const char *name, const char *value)
{
	(void)section;

	size_t name_len  = strlen(name);
	size_t value_len = strlen(value);

	if (name_len == 0 || value_len == 0) {
		PRINT_ERROR_MSG("Found an entry with either no name or no value.");
		error_known = true;
		return COPRIS_PARSE_FAILURE;
	}

	if (value_len > MAX_INIFILE_ELEMENT_LENGTH) {
		PRINT_ERROR_MSG("'%s': value length exceeds maximum of %zu bytes.", value,
		                (size_t)MAX_INIFILE_ELEMENT_LENGTH);
		error_known = true;
		return COPRIS_PARSE_FAILURE;
	}

	// Inih cannot parse escaped equals signs and will split the definition at the first
	// sign, leaving only '\' as the name. Detect that and tell user that '\e' can be
	// used instead.
	if (name[0] == '\\' && value[0] == '=') {
		PRINT_ERROR_MSG("An escaped equals sign was detected. Since COPRIS cannot parse it "
		                "properly, replace it with '\\e' in the encoding file.");
		error_known = true;
		return COPRIS_PARSE_FAILURE;
	}

	size_t codepoint_count = utf8_count_codepoints(name, 2);
	bool equals_sign = false;
	if (codepoint_count > 1) {
		// Multiple characters found, do they consist from a backslash and a single character?
		if (name[0] != '\\' || codepoint_count > 2) {
			PRINT_ERROR_MSG("'%s': name has more than one character.", name);
			error_known = true;
			return COPRIS_PARSE_FAILURE;
		}

		// Detect '\e', our escape character that represents the equals sign
		if (name[1] == 'e' || name[1] == 'E')
			equals_sign = true;

		// A character in name was escaped; omit the backslash from the string
		name++;
		name_len--;
	}

	struct Inifile **file = (struct Inifile**)user;  // Passed from caller
	struct Inifile *s;                               // Local to this function

	// Check if this 'name' already exists
	HASH_FIND_STR(*file, name, s);
	bool name_overwritten = (s != NULL);

	// Add a new key if it doesn't already exist
	if (!name_overwritten) {
		s = malloc(sizeof *s);
		CHECK_MALLOC(s);

		if (!equals_sign) {
			memcpy(s->in, name, name_len + 1);
		} else {
			s->in[0] = '=';
			s->in[1] = '\0';
		}
		HASH_ADD_STR(*file, in, s);
	}

	int element_count = 0;

	// Parse value if it wasn't explicitly specified to be empty
	if (*value != '@') {
		char parsed_value[MAX_INIFILE_ELEMENT_LENGTH];
		element_count = parse_number_string(value, parsed_value, (sizeof parsed_value) - 1);

		// Check for a parse error
		if (element_count == -1) {
			PRINT_ERROR_MSG("Failure while processing value for '%s'.", name);
			free(s);
			error_known = true;
			return COPRIS_PARSE_FAILURE;
		}

		memcpy(s->out, parsed_value, element_count + 1);
	} else {
		*s->out = '\0';
	}

	if (LOG_DEBUG) {
		PRINT_LOCATION(stdout);

		if (element_count == 0) {
			printf(" %1s (%zu) => (empty)", s->in, name_len);
		} else {
			printf(" %1s (%zu) =>", s->in, name_len);
			for (int i = 0; i < element_count; i++)
				printf(" 0x%X", (unsigned int)(s->out[i] & 0xFF));
		}

		if (name_overwritten)
			printf(" (overwriting old value)");

		printf("\n");
	}

	return COPRIS_PARSE_SUCCESS;
}

void unload_encoding_definitions(struct Inifile **encoding)
{
	struct Inifile *definition;
	struct Inifile *tmp;
	int count = 0;

	HASH_ITER(hh, *encoding, definition, tmp) {
		HASH_DEL(*encoding, definition);
		free(definition);
		count++;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Unloaded encoding definitions (count = %d).", count);
}

int recode_text(UT_string *copris_text, struct Inifile **encoding)
{
	UT_string *recoded_text;
	utstring_new(recoded_text);

	int error = 0;
	const char *original = utstring_body(copris_text);

	for (size_t i = 0; i < utstring_len(copris_text);) {
		char input_char[UTF8_MAX_LENGTH + 1];
		size_t input_len;
		char output_char[MAX_INIFILE_ELEMENT_LENGTH];
		size_t output_len;
		struct Inifile *s;

		if (UTF8_IS_MULTIBYTE(original[i])) {
			input_len = utf8_codepoint_length(original[i]);
			memcpy(input_char, &original[i], input_len);
		} else {
			input_len = 1;
			input_char[0] = original[i];
		}
		input_char[input_len] = '\0';

		HASH_FIND_STR(*encoding, input_char, s);
		if (s) {
			// Definition found
			output_len = strlen(s->out);
			memcpy(output_char, s->out, output_len);
		} else {
			// Definition not found, copy original
			output_len = input_len;
			memcpy(output_char, input_char, output_len);
			if (input_len > 1) {
				error = 1; // Warn user if multi-byte characters are really wanted
			}
		}

		utstring_bincpy(recoded_text, output_char, output_len);
		i += input_len;
	}

	utstring_clear(copris_text);
	utstring_concat(copris_text, recoded_text);
	utstring_free(recoded_text);

	return error;
}
