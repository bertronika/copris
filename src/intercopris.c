/*
 * Interactive COPRIS
 *
 * This is a tool that provides a readline-based command line interpreter
 * with the same parsing mechanism as COPRIS' printer feature files.
 *
 * It is intended to be used for experimenting with printers by
 * providing a straight-forward interface between human-readable
 * numbers and raw data, understood by printers.
 *
 * Copyright (C) 2023 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files 'main.c' and 'COPYING' for more details.
 */

// Getopt requires #define _XOPEN_SOURCE, but readline's pkgconf file already defines it

#define IC_HISTORY_FILE  "./intercopris_history"
#define HISTORY_LIST_LEN 20

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <uthash.h>
#include <utstring.h>

#include "../src/Copris.h"
#include "../src/parse_value.h"
#include "../src/writer.h"
#include "../src/feature.h"
#include "../src/main-helpers.h"

#define ESC_BOLD "\x1B[1m"
#define ESC_NORM "\x1B[0m"

// TODO: Add a -v switch?
#ifdef DEBUG
int verbosity = 3;
#else
int verbosity = 1;
#endif

static char **completer(const char *text, int start, int end);
static char *complete_possible_commands(const char *text, int state);
static int load_feature_file(const char *filename, struct Inifile **features);
static void hex_dump(const char *string, int length, bool mixed);
static void print_help(const char *argv0);

// List of possible commands, suggested by Readline's tab completion
char *possible_commands[100];
int comp_cmd_count = 0;

// Helper for adding commands to the above array. It recycles
// 'append_file_name' from COPRIS' argument parsing.
#define ADD_RL_COMMAND(cmd)                                         \
	do {                                                            \
		if (comp_cmd_count >= (int)(sizeof possible_commands))      \
			exit(EXIT_FAILURE);                                     \
		append_file_name(cmd, possible_commands, comp_cmd_count++); \
	} while (0)

int main(int argc, char **argv)
{
	int c;
	int error;
	bool use_history_file = true;
	const char *feature_file = NULL;
	struct Inifile *features = NULL;

	while ((c = getopt(argc, argv, "f:hn")) != -1) {
		switch (c) {
		case 'f':
			feature_file = optarg;
			error = load_feature_file(feature_file, &features);
			if (error)
				return EXIT_FAILURE;

			break;
		case 'h':
			print_help(argv[0]);
			return 0;
		case 'n':
			use_history_file = false;
			break;
		default:
			print_help(argv[0]);
			return EXIT_FAILURE;
		}
	}

	char *output_device = argv[optind]; // Either a string or NULL

	// Set up the prompt
	UT_string *ic_prompt;
	utstring_new(ic_prompt);
	utstring_printf(ic_prompt, "%s > ", (output_device) ? output_device : "stdout");

	// Set up the output buffer
	UT_string *output_text;
	utstring_new(output_text);

	// Print usage instructions
	puts(" " ESC_BOLD "Welcome to Interactive COPRIS\n" ESC_NORM
	     " Enter commands in hexadecimal, decimal or octal notation, as you would\n"
	     " in a COPRIS feature file. To print characters as text instead, start your\n"
	     " line with '"
			ESC_BOLD "t" ESC_NORM
			" ' or '"
			ESC_BOLD "text" ESC_NORM
			" '. Prefix any comments with '"
			ESC_BOLD "#" ESC_NORM
			"' or '"
			ESC_BOLD ";" ESC_NORM
			"'.\n"
		 "\n"
		 " Use '"
			ESC_BOLD "q" ESC_NORM
			"', '"
			ESC_BOLD "quit" ESC_NORM
			"' or "
			ESC_BOLD "Ctrl-D" ESC_NORM
			" to quit.\n"
		 " Enter '"
			ESC_BOLD "h" ESC_NORM
			"' or '"
			ESC_BOLD "hex" ESC_NORM
			"' to echo parsed hexadecimal output to terminal.");

	if (use_history_file)
		puts(" Enter '" ESC_BOLD "l" ESC_NORM"' or '" ESC_BOLD "last" ESC_NORM
		     "' to review most recently used commands. Comments,\n"
		     " prefixed with a number sign, will be saved to history.");

	if (features)
		puts("\n Enter '" ESC_BOLD "d" ESC_NORM "' or '" ESC_BOLD "dump" ESC_NORM
		     "' for a listing of loaded printer feature commands,\n"
		     " and '" ESC_BOLD "r" ESC_NORM "' or '" ESC_BOLD "reload" ESC_NORM
		     "' to reevaluate the printer feature file.");

	puts("\n Use the " ESC_BOLD "TAB" ESC_NORM
	     " key to complete a partially entered command, or to\n"
	     " list all possible ones.");

	if (output_device == NULL)
		puts("\n No output device provided; echoing input text to terminal.");

	// Init Readline
	using_history();
	rl_attempted_completion_function = completer;
	ADD_RL_COMMAND("dump");
	ADD_RL_COMMAND("exit");
	ADD_RL_COMMAND("hex");
	ADD_RL_COMMAND("last");
	ADD_RL_COMMAND("reload");
	ADD_RL_COMMAND("text");
	ADD_RL_COMMAND("quit");

	// TODO: create a history file automatically?
	if (use_history_file)
		read_history(IC_HISTORY_FILE);

	char *input = NULL;
	char *input_ptr;
	int command_count = 0;
	bool print_hex = false;
	bool previous_cmd_had_error = false;

	for (;;) {
		if (input)
			free(input);

		input = readline(utstring_body(ic_prompt));
		input_ptr = input;

		if (previous_cmd_had_error) {
			// Remove erroneous command after the user had hopefully corrected it
			HIST_ENTRY *cmd = remove_history(history_length - 1);
			assert(cmd);
			free(cmd->line);
			free(cmd);

			previous_cmd_had_error = false;
			command_count--;
		}

		// Exit on Ctrl-D
		if (!input) {
			puts("");
			break;
		}

		// Trim leading whitespace
		while (isspace(*input_ptr))
			input_ptr++;

		// Ignore blank lines and semicolon-prefixed comments
		if (*input_ptr == '\0' || *input_ptr == ';')
			continue;

		if (strncasecmp(input_ptr, "quit", 4) == 0 || strcasecmp(input_ptr, "q") == 0 ||
		    strncasecmp(input_ptr, "exit", 4) == 0)
			break;

		if (strncasecmp(input_ptr, "hex", 3) == 0 || strcasecmp(input_ptr, "h") == 0) {
			print_hex = !print_hex;
			printf("Readable hexadecimal output %s.\n",
			       (print_hex) ? "enabled" : "disabled");
			continue;
		}

		if (strncasecmp(input_ptr, "dump", 4) == 0 || strcasecmp(input_ptr, "d") == 0) {
			puts("Loaded printer feature commands:");

			struct Inifile *s;
			for (s = features; s != NULL; s = s->hh.next) {
				if (*s->out != '\0') {
					printf("%16s = ", s->in);
					for (int i = 0; s->out[i] != '\0'; i++)
						printf("0x%X ", s->out[i] & 0xFF);
					puts("");
				}
			}
			continue;
		}

		if (strncasecmp(input_ptr, "reload", 6) == 0 || strcasecmp(input_ptr, "r") == 0) {
			unload_printer_feature_commands(&features);
			free_filenames(possible_commands, comp_cmd_count);
			comp_cmd_count = 0;
			if (load_feature_file(feature_file, &features) == 0) {
				continue;
			} else {
				features = NULL;
				break;
			}
		}

		if (strncasecmp(input_ptr, "last", 4) == 0 || strcasecmp(input_ptr, "l") == 0) {
			puts("Most recently used commands:");
			stifle_history(HISTORY_LIST_LEN); // Limit number of shown history entries
			HIST_ENTRY **cmd_history = history_list();

			if (cmd_history) {
				for (int i = 0; cmd_history[i]; i++) {
					printf("%2d: %s\n", i + history_base, cmd_history[i]->line);
				}
			}
			unstifle_history();
			continue;
		}

		bool text_entry = false;
		if (strncasecmp(input_ptr, "text", 4) == 0 || strncasecmp(input_ptr, "t", 1) == 0) {
			if (input_ptr[1] == ' ') {
				input_ptr += 2;  // Skip cmd and leading whitespace
			} else if (input_ptr[4] == ' ') {
				input_ptr += 5;  // Ditto
			} else {
				puts("This command is not understood. Be sure there's a space between it "
				     "and the text.");
				continue;
			}
			text_entry = true;
		}

		// User's command valid, add it to history
		add_history(input);

		// Number-sign-prefixed comments go to history as well
		if (*input_ptr == '#')
			continue;

		command_count++;

		// Parse command line, interpret variables, if any
		utstring_clear(output_text);

		int element_count = 0;
		if (!text_entry) {
			element_count = parse_all_to_commands(input_ptr, strlen(input_ptr),
			                                      output_text, &features);

			// Check for failure. If found, remember the erroneous command only
			// until the next one is entered.
			if (element_count == -1) {
				previous_cmd_had_error = true;
				continue;
			}
		} else {
			utstring_printf(output_text, "%s\n", input_ptr);
			element_count = utstring_len(output_text);
		}

		if (print_hex) // Prefix 'hex: '
			hex_dump(utstring_body(output_text), element_count, false);

		if (output_device) {
			copris_write_file(output_device, output_text);
		} else {       // Prefix 'cmd: '
			hex_dump(utstring_body(output_text), element_count, true);
		}
	}

	// Save history
	if (use_history_file)
		append_history(command_count, IC_HISTORY_FILE);

	// Clean up
	if (features)
		unload_printer_feature_commands(&features);

	free(input);
	free_filenames(possible_commands, comp_cmd_count);
	utstring_free(output_text);
	utstring_free(ic_prompt);

	return 0;
}

static char **completer(const char *text, int start, int end)
{
	(void)start;
	(void)end;
	// Don't attempt filename completion after exhausting possible commands
	// rl_attempted_completion_over = 1;
	return rl_completion_matches(text, complete_possible_commands);
}

static char *complete_possible_commands(const char *text, int state)
{
	static int cmd_index, cmd_len;
	char *cmd_name;

	if (!state) {
		cmd_index = 0;
		cmd_len = strlen(text);
	}

	while ((cmd_name = possible_commands[cmd_index++])) {
		if (strncmp(cmd_name, text, cmd_len) == 0)
			return strdup(cmd_name);
	}

	return NULL;
}

static int load_feature_file(const char *filename, struct Inifile **features)
{
	*features = NULL;
	int error = initialise_commands(features);
	if (error)
		return error;

	error = load_printer_feature_file(filename, features);
	if (error)
		return error;

	struct Inifile *s;
	for (s = *features; s != NULL; s = s->hh.next) {
		if (*s->out != '\0') // Load only non-empty commands
			ADD_RL_COMMAND(s->in);
	}

	return error;
}


static void hex_dump(const char *string, int length, bool mixed)
{
	(mixed) ? fputs(" cmd: ", stdout) : fputs(" hex: ", stdout);

	for (int i = 0; i < length; i++) {
		if (mixed && isgraph(string[i])) {
			putchar(string[i]);
			putchar(' ');
		} else {
			printf("0x%02X ", string[i] & 0xFF);
		}
	}
	puts("");
}

static void print_help(const char *argv0)
{
	printf("Usage: %s [-f FILE] [-hn] [output file]\n"
	       "\n"
	       "  -f FILE   Read commands from printer feature FILE to be used\n"
	       "            in the command line\n"
	       "  -h        Print this help message\n"
	       "  -n        Do not use the history file\n"
	       "\n"
	       "Point the last argument to your printer. If omitted, text will\n"
	       "be echoed to the terminal.\n"
	       "\n"
	       "To have Intercopris remember entered commands, create a file\n"
	       "in current directory named '" IC_HISTORY_FILE "'.\n",
	       argv0);
}
