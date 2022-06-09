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
#include "debug.h"
#include "printerset.h"
#include "printer_commands.h"
#include "utf8.h"
#include "parse_value.h"

static int inih_handler(void *, const char *, const char *, const char *);
static int initialise_commands(struct Inifile **);
static void render_node(cmark_node *, cmark_event_type, struct Inifile **, UT_string *);
static void insert_code(const char *, struct Inifile **, UT_string *);

int load_printer_set_file(char *filename, struct Inifile **prset)
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

	// If there's a parse error, break the one-time do-while loop and properly close the file
	error = -1;
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
			PRINT_ERROR_MSG("Error parsing printer set file '%s', fault on line %d.",
			                filename, parse_error);
			break;
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
	} while (0);

	int tmperr = fclose(file);
	if (tmperr != 0) {
		PRINT_SYSTEM_ERROR("close", "Failed to close the printer set file.");
		return -1;
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

	if (value_len > MAX_INIFILE_ELEMENT_LENGTH) {
		PRINT_ERROR_MSG("'%s': value length exceeds maximum of %zu bytes.", value,
		                (size_t)MAX_INIFILE_ELEMENT_LENGTH);
		return COPRIS_PARSE_FAILURE;
	}

	if (name_len == 0 || value_len == 0) {
		PRINT_ERROR_MSG("Found an entry with either no name or no value.");
		return COPRIS_PARSE_FAILURE;
	}

	struct Inifile **file = (struct Inifile**)user;  // Passed from caller
	struct Inifile *s;                               // Local to this function

	// Check if this 'name' doesn't exist
	HASH_FIND_STR(*file, name, s);
	if (s == NULL) {
		PRINT_ERROR_MSG("Name '%s' doesn't belong to any command. Append '-vv' to the command "
		                "line to see the available commands.", name);
		return COPRIS_PARSE_FAILURE;
	}

	if (*s->out != '\0') {
		if (LOG_ERROR) {
			if (LOG_INFO)
				PRINT_LOCATION(stdout);

			PRINT_MSG("Definition for '%s' appears more than once in translation file, "
			          "skipping new value.", name);
		}

		return COPRIS_PARSE_DUPLICATE;
	}

	char parsed_value[MAX_INIFILE_ELEMENT_LENGTH];
	int element_count = parse_value_to_binary(value, parsed_value, (sizeof parsed_value) - 1);

	// Check for a parse error
	if (element_count == -1) {
		free(s);
		return COPRIS_PARSE_FAILURE;
	}

	memcpy(s->out, parsed_value, element_count + 1);

	if (LOG_DEBUG) {
		PRINT_LOCATION(stdout);
		printf(" %s => 0x", s->in);
		for (int i = 0; i < element_count; i++)
			printf("%X ", s->out[i]);

		printf("\n");
	}

	return COPRIS_PARSE_SUCCESS;
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
		if(s != NULL)
			continue;

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

	if(LOG_DEBUG) {
		PRINT_MSG("Dump of available printer set definitions:");
		for (s = *prset; s != NULL; s = s->hh.next)
			PRINT_MSG(" %s", s->in);

		PRINT_MSG("Initialised %d empty printer commands.", command_count);
	}

	return 0;
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

		char heading_code[9]; // "C_Hn_OFF"

		if (entering) {
			INSERT_TEXT("\n");
			sprintf(heading_code, "C_H%1d_ON", heading_level);
		} else
			sprintf(heading_code, "C_H%1d_OFF", heading_level);

		insert_code(heading_code, prset, text);

		if (!entering)
			INSERT_TEXT("\n");

		break;
	}
	case CMARK_NODE_STRONG:
		insert_code((entering ? "C_BOLD_ON" : "C_BOLD_OFF"), prset, text);
		break;

	case CMARK_NODE_EMPH:
		insert_code((entering ? "C_ITALIC_ON" : "C_ITALIC_OFF"), prset, text);
		break;

	case CMARK_NODE_CODE_BLOCK:
	case CMARK_NODE_CODE:
	case CMARK_NODE_TEXT: {
		const char *node_literal = cmark_node_get_literal(node);
		assert(node_literal != NULL);

		size_t node_len = strlen(node_literal);

		if (node_type == CMARK_NODE_CODE)
			insert_code("C_CODE_ON", prset, text);
		else if (node_type == CMARK_NODE_CODE_BLOCK) {
			INSERT_TEXT("\n");
			insert_code("C_CODE_BLOCK_ON", prset, text);
		}

		utstring_bincpy(text, node_literal, node_len);

		if (node_type == CMARK_NODE_CODE)
			insert_code("C_CODE_OFF", prset, text);
		else if (node_type == CMARK_NODE_CODE_BLOCK)
			insert_code("C_CODE_BLOCK_OFF", prset, text);

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

		if (list_type == CMARK_BULLET_LIST)
			INSERT_TEXT("\n> ");
		else if (list_type == CMARK_ORDERED_LIST)
			utstring_printf(text, "\n%d. ", ordered_list_index++);

		break;

	case CMARK_NODE_LINK:
		if (entering) {
			const char *node_url = cmark_node_get_url(node);
			assert(node_url != NULL);

			size_t node_len = strlen(node_url);

			insert_code("C_LINK_ON", prset, text);
			utstring_bincpy(text, node_url, node_len);
			insert_code("C_LINK_OFF", prset, text);
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

static void insert_code(const char *code, struct Inifile **prset, UT_string *text)
{
	struct Inifile *s;
	HASH_FIND_STR(*prset, code, s);

	assert(s != NULL);

	if (*s->out == '\0')
		return;

	size_t code_len = strlen(s->out);
	utstring_bincpy(text, s->out, code_len);
}
