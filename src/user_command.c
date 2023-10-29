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

#include <utstring.h> /* uthash library - dynamic strings  */

#include "Copris.h"
#include "config.h"
#include "debug.h"
#include "user_command.h"

static int substitute_with_command(UT_string *copris_text, int text_pos,
                                   const char *parsed_cmd, int original_cmd_len,
                                   struct Inifile **features);

int parse_user_commands(UT_string *copris_text, struct Inifile **features)
{
	const char *text = utstring_body(copris_text);

	if (LOG_DEBUG)
		PRINT_MSG("Searching for user feature commands.");

	for (size_t i = 0; text[i] != '\0'; i++) {
		if (text[i] == USER_CMD_SYMBOL) {
			// Prepare a string for a possible command
			char possible_cmd[MAX_INIFILE_ELEMENT_LENGTH] = "C_";
			char *possible_cmd_ptr = possible_cmd + 2; // Account for 'C_' prefix

			// Length of a possible command (up to a whitespace char)
			size_t possible_cmd_len = 0;

			// String pointer for moving locally through the command
			const char *text_tmp = &text[i];

			while (!isspace(*text_tmp)) {
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

			// + 1 to omit leading USER_CMD_SYMBOL, -1 to omit trailing whitespace
			memcpy(possible_cmd_ptr, &text[i + 1], possible_cmd_len - 1);

			// Try to substitute command text with an actual command
			substitute_with_command(copris_text, i, possible_cmd, possible_cmd_len, features);
		}
	}

	return 0;
}

static int substitute_with_command(UT_string *copris_text, int text_pos,
                                   const char *parsed_cmd, int original_cmd_len,
                                   struct Inifile **features)
{
	// Check if the command exists
	struct Inifile *s;
	HASH_FIND_STR(*features, parsed_cmd, s);
	if (!s) {
		if (LOG_ERROR)
			PRINT_MSG("Found command notation '$%s', but the command is not defined.",
					  parsed_cmd + 2);
		return -1;
	}

	if (LOG_INFO)
		PRINT_MSG("Found $%s.", parsed_cmd + 2);

	// Split copris_text into two parts, add command value (which can be empty) in between:
	//  - text_before_cmd
	//  - cmd
	//  - text_after_cmd
	UT_string *text_before_cmd;
	utstring_new(text_before_cmd);
	utstring_bincpy(text_before_cmd, utstring_body(copris_text), text_pos);

	UT_string *text_after_cmd;
	utstring_new(text_after_cmd);
	utstring_bincpy(text_after_cmd,
	                utstring_body(copris_text) + utstring_len(text_before_cmd) + original_cmd_len,
	                utstring_len(copris_text) - utstring_len(text_before_cmd) - original_cmd_len);

	utstring_clear(copris_text);

	utstring_concat(copris_text, text_before_cmd);
	if (*s->out != '\0')
		utstring_bincpy(copris_text, s->out, strlen(s->out));
	utstring_concat(copris_text, text_after_cmd);

	utstring_free(text_before_cmd);
	utstring_free(text_after_cmd);

	return 0;
}
