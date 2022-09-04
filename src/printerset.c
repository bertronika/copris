/*
 * Printer feature set file handling and text formatting
 *
 * Copyright (C) 2020-2022 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include <ini.h>      /* inih library - .ini file parser   */
#include <uthash.h>   /* uthash library - hash table       */
#include <utstring.h> /* uthash library - dynamic strings  */

#include "Copris.h"
#include "config.h"
#include "debug.h"
#include "printerset.h"
#include "printer_commands.h"
#include "utf8.h"
#include "parse_value.h"

static int initialise_commands(struct Inifile **);
static int inih_handler(void *, const char *, const char *, const char *);
static int validate_definition_pairs(const char *, struct Inifile **);

int load_printer_set_file(const char *filename, struct Inifile **prset)
{
	int error = initialise_commands(prset);
	if (error)
		return error;

	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		PRINT_SYSTEM_ERROR("fopen", "Failed to open printer feature set file.");
		return -1;
	}

	if (LOG_DEBUG)
		PRINT_MSG("`%s': parsing printer feature set file:", filename);

	int parse_error = ini_parse_file(file, inih_handler, prset);

	// If there's a parse error, properly close the file before exiting
	error = -1;

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
		int definition_count = 0; // These are user-specified definitions, not all available

		struct Inifile *s;
		for (s = *prset; s != NULL; s = s->hh.next) {
			if (*s->out != '\0')
				definition_count++;
		}

		PRINT_MSG("`%s': loaded %d printer feature set definitions.",
		          filename, definition_count);
	}

	error = 0;

	// Why parentheses? warning: a label can only be part of a statement and a declaration
	//                  is not a statement [-Wpedantic]
	close_file: {
		int tmperr = fclose(file);
		if (tmperr != 0) {
			PRINT_SYSTEM_ERROR("close", "Failed to close printer feature set file.");
			return -1;
		}
	}

	if (!error)
		error = validate_definition_pairs(filename, prset);

	return error;
}

// Initialise the prset struct with predefined names and empty strings as values.
static int initialise_commands(struct Inifile **prset)
{
	// `Your hash must be declared as a NULL-initialized pointer to your structure.'
	*prset = NULL;
	struct Inifile *s;
	int command_count = 0;

	for (int i = 0; printer_commands[i] != NULL; i++) {
		// Check for a duplicate name. Shouldn't happen with the hardcoded table, but
		// better be safe. Better check this with a unit test (TODO).
		HASH_FIND_STR(*prset, printer_commands[i], s);
		assert(s == NULL);

		// Insert the (unique) name
		s = malloc(sizeof *s);
		if (s == NULL) {
			PRINT_ERROR_MSG("Memory allocation error.");
			return -1;
		}

		// Each name gets an empty value, to be filled later from the configuration file
		memccpy(s->in, printer_commands[i], '\0', MAX_INIFILE_ELEMENT_LENGTH);
		*s->out = '\0';
		HASH_ADD_STR(*prset, in, s);

		command_count++;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Initialised %d empty printer commands. Use `--dump-commands' for "
		          "their listing.", command_count);

	return 0;
}

/*
 * [section]
 * name = value  (inih library)
 * key  = item   (uthash)
 */

// `Handler should return nonzero on success, zero on error.'
#define COPRIS_PARSE_FAILURE   0
#define COPRIS_PARSE_SUCCESS   1

static int inih_handler(void *user, const char *section, const char *name, const char *value)
{
	(void)section;

	size_t name_len  = strlen(name);
	size_t value_len = strlen(value);

	if (name_len == 0 || value_len == 0) {
		PRINT_ERROR_MSG("Found an entry with either no name or no value. If you want to "
		                "define a command without any value, use `@' in place of the value.");
		return COPRIS_PARSE_FAILURE;
	}

	if (value_len > MAX_INIFILE_ELEMENT_LENGTH) {
		PRINT_ERROR_MSG("`%s': value length exceeds maximum of %zu bytes.", value,
		                (size_t)MAX_INIFILE_ELEMENT_LENGTH);
		return COPRIS_PARSE_FAILURE;
	}

	struct Inifile **file = (struct Inifile**)user;  // Passed from caller
	struct Inifile *s;                               // Local to this function

	// Check if this 'name' doesn't exist
	HASH_FIND_STR(*file, name, s);
	if (s == NULL) {
		PRINT_ERROR_MSG("Name `%s' doesn't belong to any command. Run COPRIS with "
		                "`--dump-commands' to see the available commands.", name);
		return COPRIS_PARSE_FAILURE;
	}

	// Warn if definition was already set, but only if command isn't meant to be redefined
	bool definition_overwriten = (*s->out != '\0');

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
		*s->out = '@';
	}

	if (LOG_DEBUG) {
		PRINT_LOCATION(stdout);
		printf(" %s =>", s->in);

		if (element_count == 0)
			printf(" (empty)");
		else
			for (int i = 0; i < element_count; i++)
				printf(" 0x%X", s->out[i]);

		if (definition_overwriten)
			printf(" (overwriting old value)");

		printf("\n");
	}

	return COPRIS_PARSE_SUCCESS;
}

// Check if part of a definition pair is missing (every *_ON has a *_OFF and vice-versa)
static int validate_definition_pairs(const char *filename, struct Inifile **prset)
{
	struct Inifile *s;

	if (LOG_DEBUG)
		PRINT_MSG("`%s': validating definitions.", filename);

	for (int i = 0; printer_commands[i] != NULL; i++) {
		// Only pick commands with a pair (prefixed with F_)
		if (printer_commands[i][0] != 'F')
			continue;

		size_t command_len = strlen(printer_commands[i]);

		// Check which state the command defines
		bool is_on = (printer_commands[i][command_len - 2] == 'O' &&
		              printer_commands[i][command_len - 1] == 'N');

		// Check if the command was user-defined in the printer set file
		HASH_FIND_STR(*prset, printer_commands[i], s);
		assert(s != NULL);
		if (*s->out == '\0')
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

		HASH_FIND_STR(*prset, command_pair, s);
		assert(s != NULL);
		// No value, meaning no pair was found
		if (*s->out == '\0') {
			PRINT_ERROR_MSG("`%s': command `%s' is missing its pair definition `%s'. "
			                "Either add one or define it as empty using `@' as the value.",
			                filename, printer_commands[i], command_pair);

			return -1;
		// '@' found, meaning the command is empty on purpose
		} else if (*s->out == '@') {
			*s->out = '\0';
		}
	}

	if (LOG_DEBUG)
		PRINT_MSG("`%s': no definition pairs are missing.", filename);

	return 0;
}

int dump_printer_set_commands(struct Inifile **prset)
{
	int error = initialise_commands(prset);
	if (error)
		return error;

	struct Inifile *s;
	char code_prefix = '0';

	printf("# Printer feature set commands listing; generated by COPRIS %s\n\n", VERSION);

	for (s = *prset; s != NULL; s = s->hh.next) {
		if (*s->in != code_prefix) {
			code_prefix = *s->in;
			switch (code_prefix) {
			case 'C':
				puts("# Non-printable commands");
				break;
			case 'F':
				puts("\n# Formatting commands; both parts of a pair must be defined");
				break;
			}
		}

		size_t command_len = strlen(s->in);

		if (s->in[command_len - 2] == 'O' && s->in[command_len - 1] == 'N') {
			// *_ON commands with extra padding
			printf("; %s  = \n", s->in);

		} else {
			// all others
			printf("; %s = \n", s->in);
		}
	}

	puts("");

	return 0;
}

void unload_printer_set_file(const char *filename, struct Inifile **prset)
{
	struct Inifile *definition;
	struct Inifile *tmp;
	int count = 0;

	HASH_ITER(hh, *prset, definition, tmp) {
		HASH_DEL(*prset, definition);
		free(definition);
		count++;
	}

	if (LOG_DEBUG)
		PRINT_MSG("`%s': unloaded printer feature set file (count = %d).", filename, count);
}
