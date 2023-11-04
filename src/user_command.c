/*
 * User feature command parser
 *
 * Copyright (C) 2023 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files 'main.c' and 'COPYING' for more details.
 */

#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <assert.h>

#include <utstring.h> /* uthash library - dynamic strings  */

#include "Copris.h"
#include "config.h"
#include "debug.h"
#include "parse_value.h"
#include "user_command.h"

static user_action_t substitute_with_command(UT_string *copris_text, size_t *text_pos,
                                             const char *parsed_cmd, int original_cmd_len,
                                             struct Inifile **features);
static int ispunct_custom(int c);

user_action_t parse_user_commands(UT_string *copris_text, struct Inifile **features)
{
	const char *text = utstring_body(copris_text);
	user_action_t retval = NO_ACTION;

	// First, we must explicitly check if the text should be parsed for user commands.
	// Deciding factor is inclusion of one of the following commands at the beginning
	// of the received text:
	if (strncasecmp(text, "$ENABLE_COMMANDS", 16) != 0 &&
	    strncasecmp(text, "$ENABLE_CMD", 11) != 0 &&
	    strncasecmp(text, "$CMD", 4) != 0) {
		if (LOG_DEBUG)
			PRINT_MSG("No '$ENABLE_COMMANDS' found, not parsing user commands.");

		return DISABLE_COMMANDS;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Searching for user feature commands.");

	for (size_t i = 0; text[i] != '\0';) {
		// Detect USER_CMD_SYMBOL in text, if other characters follow it
		if (text[i] == USER_CMD_SYMBOL && text[i + 1] != '\0' && !isspace(text[i + 1])) {
			// String pointer for moving locally through the command
			const char *text_tmp = &text[i];

			// Length of a possible USER_CMD_SYMBOL-prefixed command (up to a whitespace char)
			size_t possible_cmd_len = 0;

			while (!isspace(*text_tmp)) {
				// If there's a punctuation character, stop parsing the command
				if (possible_cmd_len > 0 && ispunct_custom(*text_tmp))
					break;

				// Whitespace character not yet found, increase possible command length
				// and move to the next character
				possible_cmd_len++;
				text_tmp++;

				if (possible_cmd_len >= MAX_INIFILE_ELEMENT_LENGTH - 2) {
					// (subtract 2 for 'C_' prefix)
					if (LOG_ERROR) {
						PRINT_MSG("Found command notation in following line, but it is "
						          "too long to be parsed.");
						PRINT_MSG(" %.*s", MAX_INIFILE_ELEMENT_LENGTH, text);
					}
					return ERROR;
				}
			}

			// Prepare a string for a possible command
			char possible_cmd[MAX_INIFILE_ELEMENT_LENGTH] = "C_";
			char *possible_cmd_ptr = possible_cmd + 2; // Account for 'C_' prefix

			// + 1 character on string and - 1 on its length to omit USER_CMD_SYMBOL
			assert(possible_cmd_len > 0);
			memcpy(possible_cmd_ptr, &text[i + 1], possible_cmd_len - 1);

			// Try to substitute command text with an actual command
			retval = substitute_with_command(copris_text, &i,
			                                 possible_cmd, possible_cmd_len,
			                                 features);
		} else {
			// Increment only if no command has been detected.
			// The iterator is updated manually on a command
			// substitution (previous text + command length).
			i++;
		}
	}

	return retval;
}

static user_action_t substitute_with_command(UT_string *copris_text, size_t *text_pos,
                                             const char *parsed_cmd, int original_cmd_len,
                                             struct Inifile **features)
{
	user_action_t retval = NO_ACTION;
	struct Inifile *s;

	// Check for special commands
	if (strcasecmp(parsed_cmd, "C_DISABLE_MARKDOWN") == 0) {
		retval = DISABLE_MARKDOWN;
	} else if (strcasecmp(parsed_cmd, "C_DISABLE_COMMANDS") == 0) {
		retval = DISABLE_COMMANDS;
	} else if (parsed_cmd[2] == '#') {
		retval = SKIP_CMD;
	} else if (strcasecmp(parsed_cmd, "C_ENABLE_COMMANDS") == 0 ||
	           strcasecmp(parsed_cmd, "C_ENABLE_CMD") == 0 ||
	           strcasecmp(parsed_cmd, "C_CMD") == 0) {
		retval = SKIP_CMD;
	} else {
		// Check if the command exists
		HASH_FIND_STR(*features, parsed_cmd, s);
		if (!s && !isdigit(parsed_cmd[2])) {
			// If not, skip it and increment the text iterator
			*text_pos += original_cmd_len;
			if (LOG_ERROR)
				PRINT_MSG("Found command notation '$%s', but the command is not defined.",
						  parsed_cmd + 2);
			return ERROR;
		}
	}

	if (LOG_INFO) {
		if (retval == DISABLE_MARKDOWN || retval == DISABLE_COMMANDS) {
			PRINT_MSG("Found special command $%s.", parsed_cmd + 2);
		} else if (retval == NO_ACTION) {
			PRINT_MSG("Found $%s.", parsed_cmd + 2);
		}
	}

	// Split copris_text into two parts - before and after the command.
	// Strip up to one blank space before and after the variable.
	UT_string *text_before_cmd;
	utstring_new(text_before_cmd);

	int leading_whitespace_before = 0; // Leading whitespace before the command

	if (*text_pos > 0 && isspace(copris_text->d[*text_pos - 1]))
		leading_whitespace_before = 1;

	// Copy all text up to the command. Omit up to one space before it.
	utstring_bincpy(text_before_cmd,
	                utstring_body(copris_text),
	                *text_pos - leading_whitespace_before);

	UT_string *text_after_cmd;
	utstring_new(text_after_cmd);

	// Copy all text after the command. Omit up to one space around it.
	const char *text_after_cmd_body = utstring_body(copris_text)
	                                + utstring_len(text_before_cmd)
	                                + original_cmd_len + leading_whitespace_before;

	size_t text_after_cmd_len = utstring_len(copris_text)
	                          - utstring_len(text_before_cmd)
	                          - original_cmd_len;

	int leading_whitespace_after  = 0; // leading/
	int trailing_whitespace_after = 0; // trailing whitespace after the command

	if (!leading_whitespace_before && isspace(text_after_cmd_body[0]))
		leading_whitespace_after = 1;

	if (isspace(text_after_cmd_body[text_after_cmd_len - 1]))
		trailing_whitespace_after = 1;

	assert(text_after_cmd_body + leading_whitespace_after != NULL);
	utstring_bincpy(text_after_cmd,
	                text_after_cmd_body
	              + leading_whitespace_after,
	                text_after_cmd_len
	              - trailing_whitespace_after);

	// Assemble new copris_text in the following order:
	//  - text_before_cmd
	//  - cmd               (command value which can be empty)
	//  - text_after_cmd
	utstring_clear(copris_text);
	utstring_concat(copris_text, text_before_cmd);

	size_t cmd_len = 0;

	// If a special command was found, omit it from the output.
	// If a command was found, but is empty, also omit it.
	if (retval == NO_ACTION) {
		if (s != NULL && *s->out != '\0') {
			// Add the command
			cmd_len = strlen(s->out);
			utstring_bincpy(copris_text, s->out, cmd_len);
		} else {
			// Parse the supposed number and add it
			char parsed_value[MAX_INIFILE_ELEMENT_LENGTH];
			int element_count = parse_number_string(parsed_cmd + 2,
			                                        parsed_value, (sizeof parsed_value) - 1);
			if (element_count != -1) {
				utstring_bincpy(copris_text, parsed_value, element_count);
			} else {
				if (LOG_ERROR)
					PRINT_MSG("Number notation '$%s' was skipped.",
							  parsed_cmd + 2);
				retval = ERROR;
			}
		}
	}

	utstring_concat(copris_text, text_after_cmd);

	// copris_text, passed into this function, was modified. Notify
	// parse_user_commands() of the new length, consisting of the length
	// of text before the command and the length of the inserted command.
	*text_pos = utstring_len(text_before_cmd) + cmd_len;

	utstring_free(text_before_cmd);
	utstring_free(text_after_cmd);

	return retval;
}

static int ispunct_custom(int c)
{
	switch (c) { // Omitted: $ _ { } #
	case '!':
	case '"':
	case '%':
	case '&':
	case '\'':
	case '(':
	case ')':
	case '*':
	case '+':
	case ',':
	case '-':
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
	case '|':
	case '~':
		return 1;
	}

	return 0;
}
