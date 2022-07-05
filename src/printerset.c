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
#include <cmark.h>    /* cmark library - CommonMark parser */

#include "Copris.h"
#include "config.h"
#include "debug.h"
#include "printerset.h"
#include "printer_commands.h"
#include "utf8.h"
#include "parse_value.h"

static int initialise_commands(struct Inifile **);
static int initialise_predefined_command(const char *, char *);
static int inih_handler(void *, const char *, const char *, const char *);
static int validate_definition_pairs(const char *, struct Inifile **);
static void render_node(cmark_node *, cmark_event_type, struct Inifile **, UT_string *);
static void insert_code_helper(const char *, struct Inifile **, UT_string *);

int load_printer_set_file(const char *filename, struct Inifile **prset)
{
	int error = initialise_commands(prset);
	if (error)
		return error;

	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		PRINT_SYSTEM_ERROR("fopen", "Error opening printer set file.");
		return -1;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Parsing printer set file '%s':", filename);

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
		PRINT_ERROR_MSG("Fault on line %d of printer feature set file `%s'.",
		                parse_error, filename);
		goto close_file;
	}

	if (LOG_INFO) {
		int definition_count = 0; // These are user-specified definitions, not all available

		struct Inifile *s;
		for (s = *prset; s != NULL; s = s->hh.next) {
			if (*s->out != '\0')
				definition_count++;
		}

		PRINT_MSG("Loaded printer set file '%s' with %d definitions.",
		          filename, definition_count);
	}

	error = 0;

	// Why parentheses? warning: a label can only be part of a statement and a declaration
	//                  is not a statement [-Wpedantic]
	close_file: {
		int tmperr = fclose(file);
		if (tmperr != 0) {
			PRINT_SYSTEM_ERROR("close", "Failed to close the printer set file.");
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

		// Check if command can be predefined
		if (printer_commands[i][0] == 'P') {
			int error = initialise_predefined_command(printer_commands[i], s->out);
			if (error)
				return error;
		} else {
			*s->out = '\0';
		}

		HASH_ADD_STR(*prset, in, s);

		command_count++;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Initialised %d empty printer commands. Use `--dump-commands' for "
		          "their listing.", command_count);

	return 0;
}

// Set predefined commands (ones with the `P_' prefix) to values, specified in config.h.
static int initialise_predefined_command(const char *command, char *value)
{
	char raw_value[MAX_INIFILE_ELEMENT_LENGTH];

	if (strcmp(command, "P_LIST_ITEM") == 0)
		SET_RAW_VALUE(P_LIST_ITEM);

	int element_count = parse_value_to_binary(raw_value, value, strlen(raw_value));

	if (element_count < 1) {
		PRINT_ERROR_MSG("Cannot parse predefined command '%s', set to '%s'. "
		                "Please review 'config.h' and recompile.", command, raw_value);
		return -1;
	}

	assert(value[element_count] == '\0');
	return 0;
}

/*
 * [section]
 * name = value  (inih library)
 * key  = item   (uthash)
 */
// Parse the loaded translation file line by line
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
		PRINT_ERROR_MSG("'%s': value length exceeds maximum of %zu bytes.", value,
		                (size_t)MAX_INIFILE_ELEMENT_LENGTH);
		return COPRIS_PARSE_FAILURE;
	}

	struct Inifile **file = (struct Inifile**)user;  // Passed from caller
	struct Inifile *s;                               // Local to this function

	// Check if this 'name' doesn't exist
	HASH_FIND_STR(*file, name, s);
	if (s == NULL) {
		PRINT_ERROR_MSG("Name '%s' doesn't belong to any command. Run COPRIS with "
		                "`--dump-commands' to see the available commands.", name);
		return COPRIS_PARSE_FAILURE;
	}

	// Warn if definition was already set, but only if command isn't meant to be redefined
	bool definition_overwriten = (*s->out != '\0' && *s->in != 'P');

	int element_count = 0;

	// Parse value if it wasn't explicitly specified to be empty
	if (*value != '@') {
		char parsed_value[MAX_INIFILE_ELEMENT_LENGTH];
		element_count = parse_value_to_binary(value, parsed_value, (sizeof parsed_value) - 1);

		// Check for a parse error
		if (element_count == -1) {
			PRINT_ERROR_MSG("Failure happened while processing %s.", name);
			free(s);
			return COPRIS_PARSE_FAILURE;
		}

		memcpy(s->out, parsed_value, element_count + 1);
	} else {
		*s->out = '\0';
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
		if (*s->out == '\0') {
			PRINT_ERROR_MSG("%s: command %s is missing its pair definition %s.",
			                filename, printer_commands[i], command_pair);

			return -1;
		}
	}

	if (LOG_DEBUG)
		PRINT_MSG("No definition pairs are missing in '%s'.", filename);

	return 0;
}

int dump_printer_set_commands(struct Inifile **prset)
{
	int error = initialise_commands(prset);
	if (error)
		return error;

	struct Inifile *s;
	char code_prefix = '0';

	printf("# Printer set commands listing; generated by COPRIS %s\n\n", VERSION);

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
			case 'P':
				puts("\n# Overridable commands with predefined values shown");
				break;
			}
		}

		size_t command_len = strlen(s->in);

		if (s->in[command_len - 2] == 'O' && s->in[command_len - 1] == 'N') {
			printf("; %s  = \n", s->in);
		} else if (s->in[0] == 'P') {
			printf("; %s =  ; defaults to `0x%X", s->in, s->out[0]);
			for (int i = 1; s->out[i] != '\0'; i++)
				printf(" 0x%X", s->out[i]);
			printf("'\n");
		} else {
			printf("; %s = \n", s->in);
		}
	}

	puts("");

	return 0;
}

void unload_printer_set_file(struct Inifile **prset)
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
		PRINT_MSG("Unloaded printer set file (count = %d).", count);
}

void convert_markdown(UT_string *copris_text, struct Inifile **prset)
{
	// Create a temporary string
	UT_string *converted_text;
	utstring_new(converted_text);

	char *body      = utstring_body(copris_text);
	size_t body_len = utstring_len(copris_text);

	cmark_node *document = cmark_parse_document(body, body_len, CMARK_OPT_DEFAULT);
	cmark_iter *iter = cmark_iter_new(document);
	cmark_event_type ev_type;

	while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
		cmark_node *cur = cmark_iter_get_node(iter);
		render_node(cur, ev_type, prset, converted_text);
	}

	cmark_iter_free(iter);
	cmark_node_free(document);

	// Overwrite input text
	utstring_clear(copris_text);
	utstring_bincpy(copris_text, utstring_body(converted_text), utstring_len(converted_text));
	utstring_free(converted_text);
}

static void render_node(cmark_node *node, cmark_event_type ev_type,
                        struct Inifile **prset, UT_string *text)
{
	cmark_node_type node_type = cmark_node_get_type(node);
	bool entering = (ev_type == CMARK_EVENT_ENTER);

	static cmark_list_type list_type;
	static int ordered_list_index = 1;

// 	printf("node type '%s' (%d)\n", cmark_node_get_type_string(node), entering);

	switch (node_type) {
	case CMARK_NODE_DOCUMENT:
	case CMARK_NODE_HTML_BLOCK:
		break;

	case CMARK_NODE_PARAGRAPH: {
	    cmark_node *parent = cmark_node_parent(node);
	    cmark_node *grandparent = cmark_node_parent(parent);
// 		cmark_node_type g_node_type = cmark_node_get_type(grandparent);

// 	    if (grandparent != NULL && g_node_type == CMARK_NODE_LIST) {
// 		if ((entering && grandparent == NULL) || (!entering && g_node_type == CMARK_NODE_LIST))
		if (entering && grandparent == NULL)
			INSERT_TEXT("\n");

		break;
	}
	case CMARK_NODE_HEADING: {
		int heading_level = cmark_node_get_heading_level(node);
		assert(heading_level > 0 && heading_level <= 6);

		char heading_code[9]; // "F_Hn_OFF"

		if (entering) {
			INSERT_TEXT("\n");
			snprintf(heading_code, 9, "F_H%d_ON", heading_level);
		} else
			snprintf(heading_code, 9, "F_H%d_OFF", heading_level);

		INSERT_CODE(heading_code);

		if (!entering)
			INSERT_TEXT("\n");

		break;
	}
	case CMARK_NODE_STRONG:
		INSERT_CODE((entering ? "F_BOLD_ON" : "F_BOLD_OFF"));
		break;

	case CMARK_NODE_EMPH:
		INSERT_CODE((entering ? "F_ITALIC_ON" : "F_ITALIC_OFF"));
		break;

	case CMARK_NODE_CODE_BLOCK:
	case CMARK_NODE_CODE:
	case CMARK_NODE_TEXT: {
		const char *node_literal = cmark_node_get_literal(node);
		assert(node_literal != NULL);

		size_t node_len = strlen(node_literal);

		if (node_type == CMARK_NODE_CODE)
			INSERT_CODE("F_CODE_ON");
		else if (node_type == CMARK_NODE_CODE_BLOCK) {
			INSERT_TEXT("\n");
			INSERT_CODE("F_CODE_BLOCK_ON");
		}

		utstring_bincpy(text, node_literal, node_len);

		if (node_type == CMARK_NODE_CODE)
			INSERT_CODE("F_CODE_OFF");
		else if (node_type == CMARK_NODE_CODE_BLOCK)
			INSERT_CODE("F_CODE_BLOCK_OFF");

		if (!entering)
			INSERT_TEXT("\n");

		break;
	}
	case CMARK_NODE_LIST:
		if (entering) {
			list_type = cmark_node_get_list_type(node);
		} else {
			list_type = CMARK_NO_LIST;
			ordered_list_index = 1;  // reset index
		}

		if (!entering)
			INSERT_TEXT("\n");

		break;

	case CMARK_NODE_ITEM:
		assert(list_type != CMARK_NO_LIST);
		if (!entering)
			break;

		if (list_type == CMARK_BULLET_LIST) {
			INSERT_TEXT("\n");
			INSERT_CODE("P_LIST_ITEM");
			INSERT_TEXT(" ");
		} else if (list_type == CMARK_ORDERED_LIST) {
			utstring_printf(text, "\n%d. ", ordered_list_index++);
		}

		break;

	case CMARK_NODE_LINK:
		if (entering) {
			const char *node_url = cmark_node_get_url(node);
			assert(node_url != NULL);

			size_t node_len = strlen(node_url);

			INSERT_CODE("F_LINK_ON");
			utstring_bincpy(text, node_url, node_len);
			INSERT_CODE("F_LINK_OFF");
		}

		break;

	case CMARK_NODE_SOFTBREAK:
		INSERT_TEXT("\n");
		break;

	case CMARK_NODE_LINEBREAK:
		INSERT_TEXT("\n\n");
		break;

	case CMARK_NODE_NONE:
		assert(0);
		break;

	default:
		printf("Unhandled node type '%s' (%d)\n", cmark_node_get_type_string(node), entering);
		break;
	}
}

static void insert_code_helper(const char *code, struct Inifile **prset, UT_string *text)
{
	struct Inifile *s;
	HASH_FIND_STR(*prset, code, s);

	assert(s != NULL);

	if (*s->out == '\0')
		return;

	size_t code_len = strlen(s->out);
	utstring_bincpy(text, s->out, code_len);
}
