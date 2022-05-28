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
#include "printerset.h"
#include "utf8.h"

static int inih_handler(void *, const char *, const char *, const char *);
static int parse_value_to_binary(const char *, char *, int);

bool load_translation_file(char *filename, struct Inifile **trfile) {
	FILE *file = fopen(filename, "r");
	if (file == NULL)
		return raise_errno_perror(errno, "fopen", "Error opening translation file.");

	if (LOG_DEBUG)
		PRINT_MSG("Parsing translation file '%s':", filename);

	// `Your hash must be declared as a NULL-initialized pointer to your structure.'
	*trfile = NULL;

	int parse_error = ini_parse_file(file, inih_handler, trfile);

	// If there's a parse error, break the one-time do-while loop and properly close the file
	bool error = true;
	do {
		// Negative return number - can be either:
		// -1  Error opening file - we've already handled this
		// -2  Memory allocation error - only if inih was built with INI_USE_STACK
		if (parse_error < 0) {
			PRINT_ERROR_MSG("inih: ini_malloc: Memory allocation error.");
			break;
		}

		// Positive return number - returned error is a line number
		if (parse_error > 0) {
			PRINT_ERROR_MSG("Error parsing translation file '%s', fault on line %d.",
			                filename, parse_error);
			break;
		}

		if (LOG_INFO) {
			int definition_count = HASH_COUNT(*trfile);
			PRINT_MSG("Loaded translation file '%s' with %d definitions.",
			          filename, definition_count);
		}

		error = false;
	} while (0);

	int tmperr = fclose(file);
	if (raise_perror(tmperr, "close", "Failed to close the translation file."))
		return true;

	return error;
}

/*
 * [section]
 * name = value  (inih library)
 * key  = item   (uthash)
 *
 * `Handler should return nonzero on success, zero on error.'
 */
static int inih_handler(void *user, const char *section, const char *name,
                   const char *value)
{
	struct Inifile **file = (struct Inifile**)user;  // Passed from caller
	struct Inifile *s;                               // Local to this function
	(void)section;

	size_t name_len  = strlen(name);
	size_t value_len = strlen(value);

	if (name_len > MAX_INIFILE_ELEMENT_LENGTH || value_len > MAX_INIFILE_ELEMENT_LENGTH) {
		PRINT_ERROR_MSG("Name or value length exceeds maximum of %zu bytes.",
		                (size_t)MAX_INIFILE_ELEMENT_LENGTH);
		return COPRIS_PARSE_FAILURE;
	}

	if (name_len == 0 || value_len == 0) {
		PRINT_ERROR_MSG("Found an entry with either no name or no value.");
		return COPRIS_PARSE_FAILURE;
	}

	// Check for a duplicate key
	HASH_FIND_STR(*file, name, s);
	if (s != NULL) {
		if (LOG_INFO)
			PRINT_LOCATION(stdout);

		if (LOG_ERROR)
			PRINT_MSG("Definition for '%s' appears more than once in translation file, "
			          "skipping new value.", name);

		return COPRIS_PARSE_DUPLICATE;
	}

	// Key doesn't exist, add it
	s = malloc(sizeof *s);
	if (s == NULL) {
		PRINT_ERROR_MSG("Memory allocation error.");
		return COPRIS_PARSE_FAILURE;
	}

	char parsed_value[MAX_INIFILE_ELEMENT_LENGTH];
	int element_count = parse_value_to_binary(value, parsed_value, (sizeof parsed_value) - 1);

	// Check for a parse error
	if (element_count == -1) {
		free(s);
		return COPRIS_PARSE_FAILURE;
	}

	memcpy(s->in,  name, name_len + 1);
	memcpy(s->out, parsed_value, element_count);
	HASH_ADD_STR(*file, in, s);

	if (LOG_DEBUG) {
		PRINT_LOCATION(stdout);
		printf("parse:  %s (%zu) =>", s->in, name_len);

		for (int i = 0; i < element_count; i++)
			printf(" 0x%X", s->out[i]);

		printf(" (%d)\n", element_count);
	}

	return COPRIS_PARSE_SUCCESS;
}

static int parse_value_to_binary(const char *value, char *parsed_value, int length)
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
		if (*endptr && LOG_DEBUG)
			PRINT_MSG("strtol: remaining: '%s'.", endptr);

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

	return i;
}

void unload_translation_file(struct Inifile **trfile) {
	struct Inifile *definition;
	struct Inifile *tmp;

	if (LOG_DEBUG)
		PRINT_MSG("Unloading translation file.");

	HASH_ITER(hh, *trfile, definition, tmp) {
		HASH_DEL(*trfile, definition);
		free(definition);
	}
}

// Translate input string. Input may contain multibyte characters, translated output
// string won't contain them - every translated character is exactly one byte long.
// If there's a multibyte character in the input that doesn't have a definition,
// it'll get fully copied without any modifications.
char *copris_translate(char *source, int source_len, struct Inifile **trfile) {
	// A duplicate copy of input string
	errno = 0;
	char *tr_source = strdup(source);
	if(tr_source == NULL) {
		raise_errno_perror(errno, "strdup", "Memory allocation error");
		exit(EXIT_FAILURE);
	}

	// Space for a (multibyte) character to be matched with definitions
	char inspected_char[UTF8_MAX_LENGTH + 1];

	// Output string with all the translation
	errno = 0;
	char *tr_string = malloc(source_len + 1);
	if(tr_string == NULL) {
		raise_errno_perror(errno, "strdup", "Memory allocation error");
		exit(EXIT_FAILURE);
	}
	tr_string[0] = '\0';

	struct Inifile *s;

	int src_i  = 0; // Source string iterator
	int insp_i = 0; // Inspected char iterator
	int tr_i   = 0; // Translated string iterator

	while(src_i < source_len) {
		inspected_char[insp_i] = tr_source[src_i];

		// Check for a multibyte character
		if(insp_i < UTF8_MAX_LENGTH && UTF8_IS_MULTIBYTE(inspected_char[insp_i])) {
			// Multibyte char found, load its next byte
			insp_i++;
			src_i++;
			continue;
		} else {
			// Either:
			// - end of multibyte character
			// - character in question was only single-byte
			inspected_char[insp_i + 1] = '\0';
		}

		// Match the loaded char with a definition, if it exists
		HASH_FIND_STR(*trfile, inspected_char, s);
		if(s != NULL) {
			// Definition found
			tr_string[tr_i] = s->out;
			tr_i++;
		} else {
			// Definition not found
			if(insp_i > 0) {
				// Copy multibyte character
				strcat(tr_string, inspected_char);

				tr_i += insp_i + 1;
				//tr_i += strlen(inspected_char);
			} else {
				// Copy single character
				tr_string[tr_i++] = tr_source[src_i];
			}
		}
		insp_i = 0;
		src_i++;
	}

	tr_string[tr_i] = '\0';

	free(tr_source);

	return tr_string;
}
