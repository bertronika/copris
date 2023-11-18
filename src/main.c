/*
 * COPRIS - a converting printer server
 * Copyright (C) 2020-2023 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>

#include <utstring.h> /* uthash library - dynamic strings */

#include "debug.h"
#include "Copris.h"

#include "socket_io.h"
#include "stream_io.h"
#include "recode.h"
#include "feature.h"
#include "markdown.h"
#include "main-helpers.h"
#include "user_command.h"

/*
 * Verbosity levels:
 * 0  silent/only fatal
 * 1  error
 * 2  info
 * 3  debug
 */
int verbosity = 1;

static void copris_help(const char *argv0) {
	printf("Usage: %s [arguments] [printer or output file]\n"
	       "\n"
	       "  -p, --port PORT         Run as a network server on port number PORT\n"
	       "  -e, --encoding FILE     Recode received text with encoding FILE\n"
	       "  -E, --ENCODING FILE     Same as -e, but don't stop if encoding FILE doesn't\n"
	       "                          catch every multi-byte character\n"
	       "  -f, --feature FILE      Process Markdown in received text and use session\n"
	       "                          commands according to printer feature FILE\n"
	       "  -c, --parse-commands    If using '--feature', recognise printer feature\n"
	       "                          command invocations in received text (prefixed by %c)\n"
	       "      --dump-commands     Show all possible printer feature commands\n"
	       "  -d, --daemon            Do not exit after the first network connection\n"
	       "  -l, --limit LIMIT       Discard the whole chunk of text, received from the\n"
	       "                          network, when it surpasses LIMIT number of bytes\n"
	       "      --cutoff-limit      If using '--limit', cut text off at exactly LIMIT\n"
	       "                          number of bytes instead of discarding the whole chunk\n"
	       "\n"
	       "  -v, --verbose           Display diagnostic messages (can be used twice)\n"
	       "  -q, --quiet             Supress all unnecessary messages, except warnings and\n"
	       "                          fatal errors\n"
	       "  -h, --help              Show this argument summary\n"
	       "  -V, --version           Show program version, author and build-time options\n"
	       "\n"
	       "To read from stdin, omit the port argument. To echo data\n"
	       "to stdout (console/terminal), omit the output file.\n"
	       "\n"
	       "Notes will be shown if COPRIS assumes it is not invoked\n"
	       "correctly, but never when the quiet argument is present.\n"
	       "\n"
	       "If --parse-commands is enabled, received text should begin with either\n"
	       "$ENABLE_COMMANDS, $ENABLE_CMD or $CMD.\n",
	       argv0, USER_CMD_SYMBOL);

	exit(EXIT_SUCCESS);
}

static void copris_version(void) {
	printf("COPRIS version %s\n"
	       "(C) 2020-23 Nejc Bertoncelj <nejc at bertoncelj.eu.org>\n\n"
	       "Build-time options\n"
	       "  Text buffer size:                     %4d bytes\n"
	       "  Maximum .ini file element length:     %4d bytes\n"
	       "  Maximum number of each encoding and\n"
	       "  feature files that can be loaded:     %4d\n"
	       "  Symbol for invoking feature commands:  '%c'\n"
	       "\n",
	       VERSION, BUFSIZE, MAX_INIFILE_ELEMENT_LENGTH, NUM_OF_INPUT_FILES, USER_CMD_SYMBOL);

	exit(EXIT_SUCCESS);
}

static int parse_arguments(int argc, char **argv, struct Attribs *attrib) {
	static struct option long_options[] = {
		{"port",             required_argument, NULL, 'p'},
		{"encoding",         required_argument, NULL, 'e'},
		{"ENCODING",         required_argument, NULL, 'E'},
		{"feature",          required_argument, NULL, 'f'},
		{"parse-commands",   no_argument,       NULL, 'c'},
		{"dump-commands",    no_argument,       NULL, ','},
		{"daemon",           no_argument,       NULL, 'd'},
		{"limit",            required_argument, NULL, 'l'},
		{"cutoff-limit",     no_argument,       NULL, '.'},
		{"verbose",          no_argument,       NULL, 'v'},
		{"quiet",            no_argument,       NULL, 'q'},
		{"help",             no_argument,       NULL, 'h'},
		{"version",          no_argument,       NULL, 'V'},
		{NULL,               0,                 NULL, 0  }
	};

	// Variables common to multiple switch cases
	int c;              // Current Getopt argument
	char *parse_error;  // Filled in by strtoul
	long max_path_len;  // For filenames

	// Putting a colon in front of the options disables the built-in error reporting
	// of getopt_long(3) and allows us to specify more appropriate errors (ie. 'You must
	// specify a printer feature file.' instead of 'option requires an argument -- 'r')
	while ((c = getopt_long(argc, argv, ":p:e:E:f:cdl:vqhV", long_options, NULL)) != -1) {
		switch (c) {
		case 'p': {
			unsigned long temp_portno = strtoul(optarg, &parse_error, 10);

			if (temp_portno == ULONG_MAX) {
				PRINT_SYSTEM_ERROR("strtoul", "Error parsing port number.");
				return 1;
			}

			if (*parse_error) {
				PRINT_ERROR_MSG("Unrecognised characters in port number (%s).", parse_error);
				if (*parse_error == '-')
					PRINT_ERROR_MSG("Perhaps you forgot to specify the number?");

				return 1;
			}

			// If user specifies a negative port number, it overflows the unsigned long.
			// To prevent displaying a big number, display the entered string instead.
			if (temp_portno > 65535 || temp_portno < 1) {
				PRINT_ERROR_MSG("Port number %s out of reasonable range.", optarg);
				return 1;
			}

			attrib->portno = (unsigned int)temp_portno;
			break;
		}
		case 'e':
		case 'E': {
			if (*optarg == '-') {
				PRINT_ERROR_MSG("Unrecognised characters in encoding file name (%s). "
				                "Perhaps you forgot to specify the file?", optarg);
				return 1;
			}

			// Get the maximum path name length on the filesystem where
			// the file resides.
			errno = 0; /* pathconf() needs errno to be reset */
			max_path_len = pathconf(optarg, _PC_PATH_MAX);

			if (max_path_len == -1) {
				PRINT_SYSTEM_ERROR("pathconf", "Error querying encoding file '%s'.", optarg);
				return 1;
			}

			// TODO: is this check necessary after checking pathconf for errors?
// 			if (strlen(optarg) >= max_path_len) {
// 				PRINT_ERROR_MSG("Translation file's name is too long.");
// 				return 1;
// 			}

			if (attrib->encoding_file_count == NUM_OF_INPUT_FILES) {
				PRINT_ERROR_MSG("Too many encoding files were provided. Either "
				                "combine some of them or recompile COPRIS with a "
				                "bigger NUM_OF_INPUT_FILES parameter.");
				return 1;
			}

			append_file_name(optarg, attrib->encoding_files, attrib->encoding_file_count);
			attrib->encoding_file_count++;
			attrib->copris_flags |= HAS_ENCODING;

			if (c == 'E')
				attrib->copris_flags |= ENCODING_NO_STOP;

			break;
		}
		case 'f': {
			if (*optarg == '-') {
				PRINT_ERROR_MSG("Unrecognised character in printer feature file name (%s). "
				                "Perhaps you forgot to specify the file?", optarg);
				return 1;
			}

			errno = 0; /* pathconf() needs errno to be reset */
			max_path_len = pathconf(optarg, _PC_PATH_MAX);

			if (max_path_len == -1) {
				PRINT_SYSTEM_ERROR("pathconf", "Error querying printer feature file '%s'.", optarg);
				return 1;
			}

			// TODO: is this check necessary after checking pathconf for errors?
// 			if (strlen(optarg) >= max_path_len) {
// 				PRINT_ERROR_MSG("Printer set file's name is too long.");
// 				return 1;
// 			}

			if (attrib->feature_file_count == NUM_OF_INPUT_FILES) {
				PRINT_ERROR_MSG("Too many printer feature files were provided. Either "
				                "combine some of them or recompile COPRIS with a "
				                "bigger NUM_OF_INPUT_FILES parameter.");
				return 1;
			}

			append_file_name(optarg, attrib->feature_files, attrib->feature_file_count);
			attrib->feature_file_count++;
			attrib->copris_flags |= HAS_FEATURES;
			break;
		}
		case 'c':
			attrib->copris_flags |= USER_COMMANDS;
			break;
		case ',': {
			struct Inifile *features = NULL;
			exit(dump_printer_feature_commands(&features));
		}
		case 'd':
			attrib->daemon = true;
			break;
		case 'l': {
			unsigned long temp_limit = strtoul(optarg, &parse_error, 10);

			if (temp_limit == ULONG_MAX) {
				PRINT_SYSTEM_ERROR("strtoul", "Error parsing limit number.");
				return 1;
			}

			if (*parse_error) {
				PRINT_ERROR_MSG("Unrecognised characters in limit number (%s).", parse_error);
					if (*parse_error == '-')
						PRINT_ERROR_MSG("Perhaps you forgot to specify the number?");

				return 1;
			}

			if (temp_limit > INT_MAX) {
				PRINT_ERROR_MSG("Limit number %s out of range. Maximum possible "
				                "value is %d (bytes).", optarg, INT_MAX);
				return 1;
			}

			attrib->limitnum = (size_t)temp_limit;
			break;
		}
		case '.':
			attrib->copris_flags |= MUST_CUTOFF;
			break;
		case 'v':
			if (verbosity != 0 && verbosity < 3)
				verbosity++;
			break;
		case 'q':
			verbosity = 0;
			break;
		case 'h':
			copris_help(argv[0]);
			break;
		case 'V':
			copris_version();
			break;
		case ':':
			if (optopt == 'p')
				PRINT_ERROR_MSG("You must specify a port number.");
			else if (optopt == 'e')
				PRINT_ERROR_MSG("You must specify an encoding file.");
			else if (optopt == 'f')
				PRINT_ERROR_MSG("You must specify a printer feature file.");
			else if (optopt == 'l')
				PRINT_ERROR_MSG("You must specify a limit number.");
			else
				PRINT_ERROR_MSG("Option '-%c' is missing an argument.", optopt);
			return 1;
		case '?':
			if (optopt == 0)
				PRINT_ERROR_MSG("Option '%s' not recognised.", argv[optind - 1]);
			else
				PRINT_ERROR_MSG("Option '-%c' not recognised.", optopt);
			return 1;
		default:
			PRINT_ERROR_MSG("Getopt returned an unknown character code 0x%X.", c);
			return 2;
		}
	} /* end of getopt */

	// Check if there's no last argument - output file name
	if (argv[optind] == NULL)
		goto no_output_file;

	// Parse the output file name. Note that only the first argument is accepted.
	// If it equals '-', assume standard output.
	if (*argv[optind] == '-') {
		PRINT_NOTE("Found '-' as the output file name, redirecting text to standard output.\n"
		           "COPRIS does not use '-' to denote reading from standard input. To do that, "
		           "simply omit the last argument.");
	} else {
		errno = 0; /* pathconf() needs errno to be reset */
		max_path_len = pathconf(argv[optind], _PC_PATH_MAX);

		if (max_path_len == -1) {
			PRINT_SYSTEM_ERROR("pathconf", "Error querying output file '%s'.", argv[optind]);
			return 1;
		}

		// TODO: is this check necessary after checking pathconf for errors?
// 		if (strlen(argv[optind]) >= max_path_len) {
// 			PRINT_ERROR_MSG("Destination filename is too long.");
// 			return 1;
// 		}

		int tmperr = access(argv[optind], W_OK);
		if (tmperr != 0) {
			PRINT_SYSTEM_ERROR("access", "Unable to write to output file. Does it "
			                             "exist, with appropriate permissions?");
			return 1;
		}

		attrib->output_file = argv[optind];
		attrib->copris_flags |= HAS_OUTPUT_FILE;

		if (argv[++optind] != NULL)
			PRINT_NOTE("Multiple output file names detected; only the first one "
			           "will be used.");
	}

	no_output_file:
	return 0;
}

int main(int argc, char **argv) {
	// Run-time options (program attributes)
	struct Attribs attrib;

	// Encoding and printer features hash structures
	// 'Your hash must be declared as a NULL-initialized pointer to your structure.'
	struct Inifile *encoding = NULL, *features = NULL;

	attrib.portno       = 0;  // If 0, read from stdin
	attrib.daemon       = false;
	attrib.limitnum     = 0;
	attrib.copris_flags = 0x00;

	attrib.encoding_file_count = 0;
	attrib.feature_file_count  = 0;

	// Parse command line arguments
	int error = parse_arguments(argc, argv, &attrib);
	if (error)
		return error;

	if (argc < 2)
		PRINT_NOTE("COPRIS won't do much without any arguments. "
		           "Try using the '--help' option.");

	if (LOG_INFO)
		PRINT_MSG("Verbosity level set to %d.", verbosity);

	if (LOG_DEBUG)
		PRINT_MSG("COPRIS started with PID %d.", getpid());

	// If no port number was specified by the user, assume input from stdin
	bool is_stdin = false;
	if (attrib.portno == 0)
		is_stdin = true;

	if (attrib.limitnum && is_stdin)
		PRINT_NOTE("Limit number cannot be used while reading from stdin, continuing without the "
		           "limit feature.");

	// Disable daemon mode if input is coming from stdin
	if (attrib.daemon && is_stdin) {
		attrib.daemon = false;
		PRINT_NOTE("Daemon mode not available while reading from stdin, continuing with "
		           "daemon mode disabled.");
	}

	if (attrib.daemon && LOG_DEBUG)
		PRINT_MSG("Daemon mode enabled.");

	// Load an encoding file
	if (attrib.copris_flags & HAS_ENCODING) {
		for (int i = 0; i < attrib.encoding_file_count; i++) {
			error = load_encoding_file(attrib.encoding_files[i], &encoding);
			if (error) {
				if (verbosity)
					return EXIT_FAILURE;

				// Missing encoding files are not a fatal error when --quiet
				unload_encoding_definitions(&encoding);
				attrib.copris_flags &= ~HAS_ENCODING;
				PRINT_ERROR_MSG("Continuing without character recoding.");
			}

			if ((attrib.copris_flags & ENCODING_NO_STOP) && LOG_INFO)
				PRINT_MSG("Forcing recoding even in case of missing encoding definitions.");
		}
	}

	// Load a printer feature file
	if (attrib.copris_flags & HAS_FEATURES) {
		error = initialise_commands(&features);
		if (error)
			return EXIT_FAILURE;

		for (int i = 0; i < attrib.feature_file_count; i++) {
			error = load_printer_feature_file(attrib.feature_files[i], &features);
			if (error) {
				if (verbosity)
					return EXIT_FAILURE;

				// Missing printer feature files are as well not a fatal error in quiet mode
				unload_printer_feature_commands(&features);
				attrib.copris_flags &= ~HAS_FEATURES;
				PRINT_ERROR_MSG("Continuing without printer features.");
			}
		}
	}

	if (attrib.copris_flags & USER_COMMANDS && !(attrib.copris_flags & HAS_FEATURES)) {
	    attrib.copris_flags &= ~(USER_COMMANDS);
		PRINT_NOTE("User feature commands cannot be parsed if there's no printer feature "
		           "file loaded.");
	}

	if (attrib.limitnum > 0 && LOG_DEBUG)
		PRINT_MSG("Limiting incoming data to %zu bytes.", attrib.limitnum);
	
	if (!is_stdin && LOG_DEBUG)
		PRINT_MSG("Server is listening to port %u.", attrib.portno);

	if (LOG_INFO) {
		PRINT_LOCATION(stdout);
		printf("Data stream will be sent to ");
		if (attrib.copris_flags & HAS_OUTPUT_FILE)
			printf("%s.\n", attrib.output_file);
		else
			printf("stdout.\n");
	}

	// Open socket and listen if not reading from stdin
	int parentfd = 0;
	int childfd = 0;
	if (!is_stdin) {
		error = copris_socket_listen(&parentfd, attrib.portno);
		if (error)
			return EXIT_FAILURE;
	}

	// Create a string for the input text, passed between functions
	UT_string *copris_text;
	utstring_new(copris_text);

	// Prepend the startup session command
	if (attrib.copris_flags & HAS_FEATURES) {
		int num_of_chars = apply_session_commands(copris_text, &features, SESSION_STARTUP);

		if (num_of_chars > 0) {
			write_to_output(copris_text, &attrib);
			utstring_clear(copris_text);
		} else if (num_of_chars < 0) {
			return EXIT_FAILURE; // Negative return value - an error
		}
	}

	// Run the main program loop
	do {
		// Stage 1: Read input text
		if (is_stdin) {
			copris_handle_stdin(copris_text);
		} else {
			error = copris_handle_socket(copris_text, &parentfd, &childfd, &attrib);
			if (error)
				return EXIT_FAILURE;
		}

		if (utstring_len(copris_text) == 0)
			continue; // Do not attempt to write/display nothing

		// Stage 2: Handle Markdown and session commands with a printer feature file
		if (attrib.copris_flags & HAS_FEATURES) {
			parse_action_t action = NO_ACTION;
			// Parse user commands in text
			if (attrib.copris_flags & USER_COMMANDS)
				action = parse_user_commands(copris_text, &features);

			if (action != DISABLE_MARKDOWN)
				parse_markdown(copris_text, &features);

			apply_session_commands(copris_text, &features, SESSION_PRINT);
		}

		// Stage 3: Recode text with an encoding file
		if (attrib.copris_flags & HAS_ENCODING) {
			error = recode_text(copris_text, &encoding);

			// Terminate on error only if user hasn't forced recoding with '-E'
			// TODO: telling the remote user to use -E isn't helpful
			if (error && !(attrib.copris_flags & ENCODING_NO_STOP)) {
				const char error_msg[] =
				        "One or more multi-byte characters, not handled by "
				        "encoding file(s), were received. If this is the intended "
				        "behaviour, specify the file(s) with option -E/--ENCODING "
				        "instead.";

				if (verbosity) {
					if (!is_stdin)
						send_to_socket(childfd, error_msg);

					PRINT_MSG("%s", error_msg);
					return EXIT_FAILURE;
				}

				PRINT_NOTE(error_msg);
			}
		}

		// Stage 4: Write text to the output destination
		write_to_output(copris_text, &attrib);

		// Current session's text has been processed, clear it for a new read
		utstring_clear(copris_text);

		// Close the current session's socket
		if (!is_stdin) {
			error = close_socket(childfd, "child");
			if (error)
				return EXIT_FAILURE;
		}

	} while (attrib.daemon); /* end of main program loop */

	// Append the shutdown session command
	if (attrib.copris_flags & HAS_FEATURES) {
		int num_of_chars = apply_session_commands(copris_text, &features, SESSION_SHUTDOWN);

		if (num_of_chars > 0) {
			write_to_output(copris_text, &attrib);
		} else if (num_of_chars < 0) {
			return EXIT_FAILURE; // Negative return value - an error
		}

		unload_printer_feature_commands(&features);
		free_filenames(attrib.feature_files, attrib.feature_file_count);
	}

	if (attrib.copris_flags & HAS_ENCODING) {
		unload_encoding_definitions(&encoding);
		free_filenames(attrib.encoding_files, attrib.encoding_file_count);
	}

	// Close the global parent socket
	if (!is_stdin && attrib.daemon) {
		error = close_socket(parentfd, "parent");
		if (error)
			return EXIT_FAILURE;
	}

	utstring_free(copris_text);

	if (!is_stdin && LOG_DEBUG)
		PRINT_MSG("Not running as a daemon, exiting.");

	return 0;
}
