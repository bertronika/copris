/*
 * COPRIS - a converting printer server
 * Copyright (C) 2020-2022 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
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

#define COPRIS_VER "0.9"

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
#include "config.h"
#include "Copris.h"

#include "read_socket.h"
#include "read_stdin.h"
#include "translate.h"
#include "printerset.h"
#include "writer.h"

// Expand parameter to a literal string
#define _STRINGIFY(str) #str
#define STRINGIFY(str) _STRINGIFY(str)

#ifndef WITHOUT_CMARK
#   define MARKDOWN_SUPPORT "yes"
#else
#   define MARKDOWN_SUPPORT "no"
#endif

/*
 * Verbosity levels:
 * 0  silent/only fatal
 * 1  error
 * 2  info
 * 3  debug
 */
int verbosity = 1;

static void copris_help(const char *copris_location) {
	printf("Usage: %s [arguments] [printer or output file]\n\n"
	       "  -p, --port NUMBER      Listening port\n"
	       "  -t, --trfile TRFILE    Character translation file\n"
#ifndef WITHOUT_CMARK
	       "  -r, --printer PRSET    Printer feature set\n"
#endif
	       "  -d, --daemon           Run as a daemon\n"
	       "  -l, --limit NUMBER     Limit number of received bytes\n"
	       "      --cutoff-limit     Cut text off at limit instead of\n"
	       "                         discarding the whole chunk\n"
	       "\n"
	       "  -v, --verbose          Be verbose (-vv more)\n"
	       "  -q, --quiet            Display nothing except fatal errors (to stderr)\n"
	       "  -h, --help             Show this help\n"
	       "  -V, --version          Show program version and build-time options\n"
	       "\n"
	       "To read from stdin, omit the port argument. To echo data to stdout\n"
	       "(console/terminal), omit the output file.\n"
	       "\n"
	       "Warning notes will be printed if COPRIS assumes it is not invoked\n"
	       "correctly, but never when the quiet argument is present.\n",
	       copris_location);

	exit(EXIT_SUCCESS);
}

static void copris_version() {
#ifdef DEBUG
	printf("COPRIS version %s-%s\n", COPRIS_VER, STRINGIFY(DEBUG));
#else
	printf("COPRIS version %s\n", COPRIS_VER);
#endif
	printf("(C) 2020-22 Nejc Bertoncelj <nejc at bertoncelj.eu.org>\n\n"
	       "Build-time options\n"
	       "  Text buffer size:                 %5d bytes\n"
	       "  Maximum .ini file element length: %5d bytes\n"
	       "  Markdown support:                 %5s\n"
	       "\n",
	       BUFSIZE, MAX_INIFILE_ELEMENT_LENGTH, MARKDOWN_SUPPORT);

	exit(EXIT_SUCCESS);
}

static int parse_arguments(int argc, char **argv, struct Attribs *attrib) {
	static struct option long_options[] = {
		{"port",          required_argument, NULL, 'p'},
		{"daemon",        no_argument,       NULL, 'd'},
		{"trfile",        required_argument, NULL, 't'},
		{"printer",       required_argument, NULL, 'r'},
		{"limit",         required_argument, NULL, 'l'},
		{"cutoff-limit",  no_argument,       NULL, 'D'},
		{"verbose",       no_argument,       NULL, 'v'},
		{"quiet",         no_argument,       NULL, 'q'},
		{"help",          no_argument,       NULL, 'h'},
		{"version",       no_argument,       NULL, 'V'},
		{NULL,            0,                 NULL, 0  }
	};

	int c;  // Current Getopt argument
	/*
	 * Putting a colon in front of the options disables the built-in error reporting
	 * of getopt_long(3) and allows us to specify more appropriate errors (ie. `Printer
	 * set is missing.' instead of `option requires an argument -- 'r')
	 */
	while ((c = getopt_long(argc, argv, ":p:dt:r:l:vqhV", long_options, NULL)) != -1) {
		char *parserr;  // String to integer conversion error

		switch (c) {
		case 'p':
			// Reset errno to distinguish between success/failure after call
			errno = 0;
			unsigned long temp_portno = strtoul(optarg, &parserr, 10);

			if (raise_errno_perror(errno, "strtoul", "Error parsing port number."))
				return 1;

			if (*parserr) {
				if (*parserr == '-')
					PRINT_ERROR_MSG("Unrecognised characters in port number (%s). "
					                "Perhaps you forgot to specify the number?", parserr);
				else
					PRINT_ERROR_MSG("Unrecognised characters in port number (%s).", parserr);

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
		case 'd':
			attrib->daemon = 1;
			break;
		case 't':
			if (*optarg == '-') {
				PRINT_ERROR_MSG("Unrecognised characters in translation file name (%s). "
				                "Perhaps you forgot to specify the file?", optarg);
				return 1;
			}

			// Get the maximum path name length on the filesystem where
			// the trfile resides.
			errno = 0;
			size_t max_path_len = (size_t)pathconf(optarg, _PC_PATH_MAX);

			if (raise_errno_perror(errno, "pathconf", "Error querying translation file."))
				return 1;

			// TODO: is this check necessary after checking pathconf for errors?
			if (strlen(optarg) >= max_path_len) {
				PRINT_ERROR_MSG("Translation file's name is too long.");
				return 1;
			}

			attrib->trfile = optarg;
			attrib->copris_flags |= HAS_TRFILE;
			break;
		case 'r':
#ifndef WITHOUT_CMARK
			if (*optarg == '-') {
				PRINT_ERROR_MSG("Unrecognised character in printer set name (%s). "
				                "Perhaps you forgot to specify the set?", optarg);
				return 1;
			}

			if (strlen(optarg) <= PRSET_LEN) {
				attrib->prsetname = optarg;
				attrib->copris_flags |= HAS_PRSET;
			} else {
				// Excessive length already makes it wrong
				PRINT_ERROR_MSG("Selected printer feature set does not exist (%s).", optarg);
				return 1;
			}
#else
			PRINT_ERROR_MSG("Option `-r|--printerset' isn't available in this "
			                "release of COPRIS -- "
			                "printer feature sets were omitted with `-DWITHOUT_CMARK' "
			                "at build time.");
			return 1;
#endif
			break;
		case 'l':
			// Reset errno to distinguish between success/failure after call
			errno = 0;
			unsigned long temp_limit = strtoul(optarg, &parserr, 10);

			if (raise_errno_perror(errno, "strtoul", "Error parsing limit number."))
				return 1;

			if (*parserr) {
				if (*parserr == '-')
					PRINT_ERROR_MSG("Unrecognised characters in limit number (%s). "
					                "Perhaps you forgot to specify the number?", parserr);
				else
					PRINT_ERROR_MSG("Unrecognised characters in limit number (%s).", parserr);

				return 1;
			}

			if (temp_limit > INT_MAX) {
				PRINT_ERROR_MSG("Limit number %s out of range. Maximum possible "
				                "value is %d (bytes).", optarg, INT_MAX);
				return 1;
			}

			attrib->limitnum = (size_t)temp_limit;
			break;
		case 'D':
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
				PRINT_ERROR_MSG("Port number is missing.");
			else if (optopt == 't')
				PRINT_ERROR_MSG("Translation file is missing.");
			else if (optopt == 'r')
				PRINT_ERROR_MSG("Printer set is missing.");
			else if (optopt == 'l')
				PRINT_ERROR_MSG("Limit number is missing.");
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

	// Parse the last argument, destination (or lack thereof).
	// Note that only the first argument is accepted.
	if (argv[optind]) {
		// Get the maximum path name length on the filesystem where
		// the output file resides.
		errno = 0;
		size_t max_path_len = (size_t)pathconf(argv[optind], _PC_PATH_MAX);

		if (raise_errno_perror(errno, "pathconf", "Error querying your chosen destination."))
			return 1;

		// TODO: is this check necessary after checking pathconf for errors?
		if (strlen(argv[optind]) >= max_path_len) {
			PRINT_ERROR_MSG("Destination filename is too long.");
			return 1;
		}

		int ferror = access(argv[optind], W_OK);
		if (raise_perror(ferror, "access", "Unable to write to output file/printer. Does "
		                                   "it exist, with appropriate permissions?"))
			return 1;

		attrib->destination = argv[optind];
		attrib->copris_flags |= HAS_DESTINATION;
	}

	// Check for multiple destination arguments
	if ((attrib->copris_flags & HAS_DESTINATION) && argv[++optind]) {
		if (LOG_ERROR)
			PRINT_NOTE("Only the first destination file name will be used.");
	}

	return 0;
}

int main(int argc, char **argv) {
	// Run-time options (program attributes)
	struct Attribs attrib;

	// Translation file and printer set hash structures
	struct Inifile *trfile, *prset;

	attrib.portno       = 0;  // If 0, read from stdin
	attrib.prset        = -1;
	attrib.daemon       = 0;
	attrib.limitnum     = 0;
	attrib.copris_flags = 0x00;

	// Parse command line arguments
	int error = parse_arguments(argc, argv, &attrib);
	if (error)
		return error;

	if (LOG_DEBUG)
		PRINT_MSG("COPRIS started with PID %d.", getpid());

	if (argc < 2 && LOG_ERROR)
		PRINT_NOTE("COPRIS won't do much without any arguments. "
                   "Try using the '--help' option.");

	if (LOG_INFO)
		PRINT_MSG("Verbosity level set to %d.", verbosity);

	// If no port number was specified by the user, assume input from stdin
	bool is_stdin = false;
	if (attrib.portno == 0)
		is_stdin = true;

	if (attrib.limitnum && is_stdin && LOG_ERROR)
		PRINT_NOTE("Limit number not used while reading from stdin.");

	// Disable daemon mode if input is coming from stdin
	if (attrib.daemon && is_stdin) {
		attrib.daemon = 0;
		if (LOG_ERROR)
			PRINT_NOTE("Daemon mode not available while reading from stdin.");
	}

	if (attrib.daemon && LOG_DEBUG)
		PRINT_MSG("Daemon mode enabled.");

#ifndef WITHOUT_CMARK
	// Parsing and loading printer feature definitions
	if (attrib.copris_flags & HAS_PRSET) {
		copris_initprset(&prset);
// 		error = copris_loadprset(attrib.prsetfile, &prset);
// 		if(error) {
// 			// Missing printer sets are not a fatal error in quiet mode
// 			if(verbosity) {
// 				return EXIT_FAILURE;
// 			} else {
// 				fprintf(stderr, "Disabling printer feature set.\n");
// 				attrib.copris_flags &= ~HAS_PRSET;
// 			}
// 		}
	}
#else
	(void)prset;
#endif

	// Parsing and loading translation definitions
	if (attrib.copris_flags & HAS_TRFILE) {
		error = load_translation_file(attrib.trfile, &trfile);
		if (error) {
			// Missing translation files are as well not a fatal error when --quiet
			if (verbosity) {
				unload_translation_file(&trfile);
				return EXIT_FAILURE;
			} else {
				PRINT_ERROR_MSG("Disabling translation.");
				attrib.copris_flags &= ~HAS_TRFILE;
			}
		}
	}
	
	if (attrib.limitnum > 0 && LOG_DEBUG)
		PRINT_MSG("Limiting incoming data to %zu bytes.", attrib.limitnum);
	
	if (!is_stdin && LOG_DEBUG)
		PRINT_MSG("Server is listening to port %u.", attrib.portno);
	
	if (LOG_INFO) {
		PRINT_LOCATION(stdout);
		printf("Data stream will be sent to ");
		if (attrib.copris_flags & HAS_DESTINATION)
			printf("%s.\n", attrib.destination);
		else
			printf("stdout.\n");
	}

	// Open socket and listen if not reading from stdin
	int parentfd = 0;
	if (!is_stdin) {
		error = copris_socket_listen(&parentfd, attrib.portno);
		if (error)
			return EXIT_FAILURE;
	}

	// Create a string for the input text, passed between functions
	UT_string *copris_text;
	utstring_new(copris_text);

	// Run the main program loop
	do {
		// Stage 1: Read input text
		if (is_stdin) {
			copris_handle_stdin(copris_text);
		} else {
			error = copris_handle_socket(copris_text, &parentfd, &attrib);
			if (error)
				return EXIT_FAILURE;
		}

		// Stage 2: Translate selected characters in text with a translation file

		// Stage 3: Normalise text

		// Stage 4: Handle Markdown in text with a printer set file

		// Stage 5: Write text to the output destination
		size_t text_length = utstring_len(copris_text);
		if (text_length == 0)
			continue; // Do not attempt to write/display nothing

		if (attrib.copris_flags & HAS_DESTINATION) {
			copris_write_file(attrib.destination, copris_text);
		} else {
			// Print Beginning-/End-Of-Stream markers if output isn't a file
			if (LOG_ERROR)
				puts("; BOS");

			fputs(utstring_body(copris_text), stdout);

			if (LOG_ERROR) {
				// Print a new line if one's missing in the final text
				char *processed_text = utstring_body(copris_text);
				if(processed_text[text_length - 1] != '\n')
					puts("");

				puts("; EOS");
			}
		}

		// Current session's text has been processed, clear it for a new read
		utstring_clear(copris_text);

	} while (attrib.daemon); /* end of main program loop */

	if (attrib.copris_flags & HAS_TRFILE)
		unload_translation_file(&trfile);

	utstring_free(copris_text);

	if (!is_stdin && LOG_DEBUG)
		PRINT_MSG("Not running as a daemon, exiting.");

	return 0;
}
