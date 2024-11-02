/*
 * Printer feature file handling and text formatting
 *
 * Terminology note: printer feature files consist of commands.
 *
 * Copyright (C) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files 'main.c' and 'COPYING' for more details.
 */

// For 'memccpy' in ISO C
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

#include <ini.h>      /* inih library - .ini file parser   */
#include <uthash.h>   /* uthash library - hash table       */
#include <utstring.h> /* uthash library - dynamic strings  */

#include "Copris.h"
#include "debug.h"
#include "feature.h"
#include "printer_commands.h"
#include "parse_value.h"

static int inih_handler(void *, const char *, const char *, const char *);
static int validate_command_pairs(const char *, struct Inifile **);

int load_printer_feature_file(const char *filename, struct Inifile **features)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		PRINT_SYSTEM_ERROR("fopen", "Failed to open printer feature file '%s'.", filename);
		return -1;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Parsing printer feature file '%s':", filename);

	int parse_error = ini_parse_file(file, inih_handler, features);

	int error = -1;        // If there's a parse error, properly close the file before exiting
	int command_count = 0; // Defined here as there are goto's below

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
		goto close_file;
	}

	// Count commands that were defined by the user
	struct Inifile *s;
	for (s = *features; s != NULL; s = s->hh.next) {
		if (s->out_len > 0)
			command_count++;
	}

	if (LOG_INFO)
		PRINT_MSG("Loaded %d commands from printer feature file '%s'.", command_count, filename);

	if (command_count < 1)
		PRINT_NOTE("Your printer feature file appears to be empty.");

	error = 0;

	// Why parentheses? warning: a label can only be part of a statement and a declaration
	//                  is not a statement [-Wpedantic]
	close_file: {
		int tmperr = fclose(file);
		if (tmperr != 0) {
			PRINT_SYSTEM_ERROR("fclose", "Failed to close printer feature file '%s'.", filename);
			return -1;
		}
	}

	if (!error && command_count > 0)
		error = validate_command_pairs(filename, features);

	return error;
}

int initialise_commands(struct Inifile **features)
{
	struct Inifile *s;
	int command_count = 0;

	for (int i = 0; printer_commands[i] != NULL; i++) {
		// Insert the (unique) name
		s = malloc(sizeof *s);
		CHECK_MALLOC(s);

		// Each name gets an empty value, to be filled later from the configuration file
		memccpy(s->in, printer_commands[i], '\0', MAX_INIFILE_ELEMENT_LENGTH);
		s->out_len = 0;
		HASH_ADD_STR(*features, in, s);

		command_count++;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Initialised %d empty printer commands.", command_count);

	return 0;
}

/*
 * [section]
 * name = value  (inih library)  - command
 * key  = item   (uthash)        - command
 */

// 'Handler should return nonzero on success, zero on error.'
#define COPRIS_PARSE_FAILURE 0
#define COPRIS_PARSE_SUCCESS 1

static int inih_handler(void *user, const char *section, const char *name, const char *value)
{
	(void)section;

	size_t name_len  = strlen(name);
	size_t value_len = strlen(value);

	if (name_len == 0 || value_len == 0) {
		PRINT_ERROR_MSG("Found an entry with either no name or no value. If you want to "
		                "define a command without any value, use '@' in place of the value.");
		return COPRIS_PARSE_FAILURE;
	}

	if (name_len >= MAX_INIFILE_ELEMENT_LENGTH) {
		PRINT_ERROR_MSG("'%s': name length exceeds maximum of %zu bytes.", name,
		                (size_t)MAX_INIFILE_ELEMENT_LENGTH);
		return COPRIS_PARSE_FAILURE;
	}

	if (value_len >= MAX_INIFILE_ELEMENT_LENGTH) {
		PRINT_ERROR_MSG("'%s': value length exceeds maximum of %zu bytes.", value,
		                (size_t)MAX_INIFILE_ELEMENT_LENGTH);
		return COPRIS_PARSE_FAILURE;
	}

	if (strcasecmp(name, "C_NO_MARKDOWN") == 0 || strcasecmp(name, "C_NO_COMMANDS") == 0) {
		PRINT_ERROR_MSG("'%s': command name is reserved and cannot be used.", name);
		return COPRIS_PARSE_FAILURE;
	}

	struct Inifile **features = (struct Inifile**)user;
	struct Inifile *s;

	// Check if command name exists. If not, validate its name and add it to the table.
	HASH_FIND_STR(*features, name, s);
	if (s == NULL) {
		if (name[0] != 'C' || name[1] != '_') {
			PRINT_ERROR_MSG("Name '%s' is unknown. If you'd like to define a custom "
			                "command, it must be prefixed with 'C_'.", name);
			return COPRIS_PARSE_FAILURE;
		}

		// Insert the (unique) name
		s = malloc(sizeof *s);
		CHECK_MALLOC(s);

		memcpy(s->in, name, name_len);
		HASH_ADD_STR(*features, in, s);
	}

	// Check if a command was already set
	bool command_overwriten = (s->out_len > 0);

	int element_count = 0;

	// Parse value if it wasn't explicitly specified to be empty
	if (*value != '@') {
		UT_string *parsed_value;
		utstring_new(parsed_value);

		// Resolve variables to numbers and numbers to command values
		element_count = parse_values_with_variables(value, value_len, parsed_value, features);

		if (element_count == -1) {
			PRINT_ERROR_MSG("Failure while processing command '%s'.", name);
			free(s);
			return COPRIS_PARSE_FAILURE;
		}

		memcpy(s->out, utstring_body(parsed_value), element_count + 1);
		s->out_len = element_count;
		utstring_free(parsed_value);
	} else {
		*s->out = '@';
		s->out_len = 1;
	}

	if (LOG_DEBUG) {
		PRINT_LOCATION(stdout);

		if (element_count == 0) {
			printf(" %s = %s (empty)", s->in, value);
		} else {
			printf(" %s = %s =>", s->in, value);
			for (int i = 0; i < element_count; i++)
				printf(" 0x%X", (unsigned int)(s->out[i] & 0xFF));
			printf(" (%d)", element_count);
		}

		if (command_overwriten)
			printf(" (overwriting old value)");

		printf("\n");
	}

	return COPRIS_PARSE_SUCCESS;
}

// Check if part of a command pair is missing (every *_ON has a *_OFF and vice-versa)
static int validate_command_pairs(const char *filename, struct Inifile **features)
{
	struct Inifile *s;

	for (int i = 0; printer_commands[i] != NULL; i++) {
		// Only pick commands with a pair (prefixed with F_)
		if (printer_commands[i][0] != 'F')
			continue;

		size_t command_len = strlen(printer_commands[i]);

		// Check which state the command defines
		bool is_on = (printer_commands[i][command_len - 2] == 'O' &&
		              printer_commands[i][command_len - 1] == 'N');

		// Check if the command was user-defined in the printer feature file
		HASH_FIND_STR(*features, printer_commands[i], s);
		assert(s != NULL);
		if (s->out_len == 0)
			continue; /* Looks like it's not */

		// Get command's pair - swap suffix _ON with _OFF or vice versa
		char command_pair[MAX_INIFILE_ELEMENT_LENGTH];
		memcpy(command_pair, printer_commands[i], command_len);
		assert(command_len + 1 <= MAX_INIFILE_ELEMENT_LENGTH);

		if (is_on) {
			command_pair[command_len - 1] = 'F';
			command_pair[command_len]     = 'F';
			command_pair[command_len + 1] = '\0';
		} else {
			command_pair[command_len - 2] = 'N';
			command_pair[command_len - 1] = '\0';
		}

		HASH_FIND_STR(*features, command_pair, s);
		assert(s != NULL);

		// No pair was found
		if (s->out_len == 0) {
			PRINT_ERROR_MSG("'%s': command '%s' is missing its pair '%s'. Either "
			                "add one, or define it as empty using '@' as the value.",
			                filename, printer_commands[i], command_pair);

			return -1;
		// '@' found, meaning the command is empty on purpose
		} else if (*s->out == '@') {
			s->out_len = 0;
		}
	}

	if (LOG_DEBUG)
		PRINT_MSG("No formatting command pairs are missing.");

	return 0;
}

int dump_printer_feature_commands(struct Inifile **features)
{
	int error = initialise_commands(features);
	if (error)
		return error;

	struct Inifile *s;
	char code_prefix = '0';

	puts("# Printer feature command listing. Generated by COPRIS " VERSION "\n\n"
	     "# Define your custom commands here. You can use them in categories below. Examples:\n"
	     "#  C_UNDERLINE_ON = 0x1B 0x2D 0x31\n"
	     "#  C_RESET_PRINTER = C_MARGIN_3CM C_SIZE_10CPI  ; both must be previously defined\n");

	for (s = *features; s != NULL; s = s->hh.next) {
		if (*s->in != code_prefix) {
			code_prefix = *s->in;
			switch (code_prefix) {
			//case 'C':
				//puts("# Non-printable commands");
				//break;
			case 'F':
				puts("# Text formatting commands; both parts of a pair must be defined.");
				break;
			case 'S':
				puts("\n# Session commands; used before and after printing received text,\n"
				     "# or when COPRIS starts and before it exits.");
				break;
			}
		}

		size_t command_len = strlen(s->in);

		if ((s->in[0] == 'F' && s->in[command_len - 1] == 'N') ||
		    (s->in[0] == 'S' && (s->in[3] == 'F' || s->in[6] == 'T'))) {
			// *_ON/S_AFTER_TEXT/S_AT_STARTUP commands with extra padding
			printf("; %s  = \n", s->in);

		} else {
			// all others
			printf("; %s = \n", s->in);
		}
	}

	puts("");

	return 0;
}

void unload_printer_feature_commands(struct Inifile **features)
{
	struct Inifile *command;
	struct Inifile *tmp;
	int count = 0;

	HASH_ITER(hh, *features, command, tmp) {
		HASH_DEL(*features, command);
		free(command);
		count++;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Unloaded printer feature commands (count = %d).", count);
}

int apply_session_commands(UT_string *copris_text, struct Inifile **features, session_t state)
{
	struct Inifile *s;

	switch(state) {
	case SESSION_PRINT:
		HASH_FIND_STR(*features, "S_AFTER_TEXT", s);
		// S_BEFORE_TEXT is handled later in this function
		break;
	case SESSION_STARTUP:
		HASH_FIND_STR(*features, "S_AT_STARTUP", s);
		break;
	case SESSION_SHUTDOWN:
		HASH_FIND_STR(*features, "S_AT_SHUTDOWN", s);
		break;
	default:
		assert(false);
		// We shouldn't reach this spot, but if assert is disabled and a
		// non-existent state is specified...
		return -1;
	}

	assert(s != NULL);

	int num_of_characters = 0; // Number of additional characters in copris_text

	// Append - either when starting/closing COPRIS, or after received text was printed
	if (s->out_len > 0) {
		if (LOG_INFO)
			PRINT_MSG("Adding session command %s.", s->in);

		// Append AFTER_TEXT to 'copris_text'
		utstring_bincpy(copris_text, s->out, s->out_len);

		num_of_characters += s->out_len;
	}

	if (state != SESSION_PRINT) // Below section only applies for SESSION_PRINT
		return num_of_characters;

	// Prepend before received text
	HASH_FIND_STR(*features, "S_BEFORE_TEXT", s);
	assert(s != NULL);

	if (s->out_len > 0) {
		if (LOG_INFO)
			PRINT_MSG("Adding session command S_BEFORE_TEXT.");

		UT_string *temp_text;
		utstring_new(temp_text);

		// Begin the temporary string with BEFORE_TEXT, append received text
		utstring_bincpy(temp_text, s->out, s->out_len);
		utstring_concat(temp_text, copris_text);

		// Move temporary text to 'copris_text'
		utstring_clear(copris_text);
		utstring_concat(copris_text, temp_text);

		utstring_free(temp_text);

		num_of_characters += s->out_len;
	}

	return num_of_characters;
}
