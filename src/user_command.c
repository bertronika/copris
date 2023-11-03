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
#include "user_command.h"

static user_action_t substitute_with_command(UT_string *copris_text, size_t *text_pos,
                                             const char *parsed_cmd, int original_cmd_len,
                                             struct Inifile **features);

user_action_t parse_user_commands(UT_string *copris_text, struct Inifile **features)
{
	const char *text = utstring_body(copris_text);
	user_action_t retval = NO_ACTION;

	if (LOG_DEBUG)
		PRINT_MSG("Searching for user feature commands.");

	for (size_t i = 0; text[i] != '\0';) {
		// Detect USER_CMD_SYMBOL in text, but not if it is standalone
		if (text[i] == USER_CMD_SYMBOL && text[i + 1] != '\0' && !isspace(text[i + 1])) {
			// Prepare a string for a possible command
			char possible_cmd[MAX_INIFILE_ELEMENT_LENGTH] = "C_";
			char *possible_cmd_ptr = possible_cmd + 2; // Account for 'C_' prefix

			// Length of a possible USER_CMD_SYMBOL-prefixed command (up to a whitespace char)
			size_t possible_cmd_len = 0;

			// String pointer for moving locally through the command
			const char *text_tmp = &text[i];

			while (!isspace(*text_tmp)) {
				// If there's a punctuation character, also stop parsing the command
				if (possible_cmd_len > 0 && ispunct(*text_tmp))
					break;

				// Whitespace character not yet found, increase possible command length
				// and move to the next character
				possible_cmd_len++;
				text_tmp++;

				// Subtract 2 for 'C_' prefix
				if (possible_cmd_len >= MAX_INIFILE_ELEMENT_LENGTH - 2) {
					if (LOG_ERROR) {
						PRINT_MSG("Found command notation in following line, but it is "
						          "too long to be parsed.");
						PRINT_MSG(" %.*s", MAX_INIFILE_ELEMENT_LENGTH, text);
					}
					return -1;
				}
			}

			// + 1 on string and - 1 on its length to omit USER_CMD_SYMBOL
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
	if (strcasecmp(parsed_cmd, "C_NO_MARKDOWN") == 0) {
		retval = DISABLE_MARKDOWN;
	} else if (strcasecmp(parsed_cmd, "C_NO_COMMANDS") == 0) {
		retval = DISABLE_COMMANDS;
	} else {
		// Check if the command exists
		HASH_FIND_STR(*features, parsed_cmd, s);
		if (!s) {
			// If not, skip it and increment the text iterator
			*text_pos += original_cmd_len;
			if (LOG_ERROR)
				PRINT_MSG("Found command notation '$%s', but the command is not defined.",
						  parsed_cmd + 2);
			return ERROR;
		}
	}

	if (LOG_INFO) {
		if (retval != NO_ACTION) {
			PRINT_MSG("Found special command $%s.", parsed_cmd + 2);
		} else {
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
	char *text_after_cmd_body = utstring_body(copris_text)
	                          + utstring_len(text_before_cmd)
	                          + original_cmd_len + leading_whitespace_before;

	size_t text_after_cmd_len = utstring_len(copris_text)
	                          - utstring_len(text_before_cmd)
	                          - original_cmd_len;

	int leading_whitespace_after  = 0;
	int trailing_whitespace_after = 0;

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
	if (retval == NO_ACTION && *s->out != '\0') {
		cmd_len = strlen(s->out);
		utstring_bincpy(copris_text, s->out, cmd_len);
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
