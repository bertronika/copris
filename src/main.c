/*
 * main.c
 *
 * COPRIS - a converting printer server
 * Copyright (C) 2020-2021 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
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
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <utstring.h> /* Dynamic strings */

#include "debug.h"
#include "config.h"
#include "Copris.h"

#include "server.h"
#include "translate.h"
#include "printerset.h"
#include "writer.h"

// Expand parameter to a literal string
#define _STRINGIFY(str) #str
#define STRINGIFY(str) _STRINGIFY(str)

/*
 * Verbosity levels:
 * 0  silent/only fatal
 * 1  error
 * 2  info
 * 3  debug
 */
int verbosity = 1;

void copris_help(char *copris_location) {
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
	       "  -V, --version          Show program version and included printer\n"
	       "                         feature sets\n"
	       "\n"
	       "To read from stdin, omit the port argument. To echo data to stdout\n"
	       "(console/terminal), omit the output file.\n"
	       "\n"
	       "Warning notes will be printed if COPRIS assumes it is not invoked\n"
	       "correctly, but never when the quiet argument is present.\n",
	       copris_location);

	exit(EXIT_SUCCESS);
}

void copris_version() {
#ifdef DEBUG
	printf("COPRIS version %s-%s\n", COPRIS_VER, STRINGIFY(DEBUG));
#else
	printf("COPRIS version %s\n", COPRIS_VER);
#endif
	printf("(C) 2020-21 Nejc Bertoncelj <nejc at bertoncelj.eu.org>\n\n"
	       "Compiled options:\n"
		   "  Buffer size:          %4d bytes\n\n",
	       BUFSIZE);

	exit(EXIT_SUCCESS);
}

int parse_arguments(int argc, char **argv, struct Attribs *attrib) {
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
				fprintf(stderr, "Unrecognised characters in port number (%s). ", parserr);
				if (*parserr == '-')
					fprintf(stderr, "Perhaps you forgot to specify the number?\n");
				else
					fprintf(stderr, "\n");

				return 1;
			}

			// If user specifies a negative port number, it overflows the unsigned long.
			// To prevent displaying a big number, display the entered string instead.
			if (temp_portno > 65535 || temp_portno < 1) {
				fprintf(stderr, "Port number %s out of reasonable range.\n", optarg);
				return 1;
			}

			attrib->portno = (unsigned int)temp_portno;
			break;
		case 'd':
			attrib->daemon = 1;
			break;
		case 't':
			if (*optarg == '-') {
				fprintf(stderr, "Unrecognised characters in translation file name (%s). "
				                "Perhaps you forgot to specify the file?\n", optarg);
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
				fprintf(stderr, "Translation file's name is too long.\n");
				return 1;
			}

			attrib->trfile = optarg;
			attrib->copris_flags |= HAS_TRFILE;
			break;
		case 'r':
#ifndef WITHOUT_CMARK
			if (*optarg == '-') {
				fprintf(stderr, "Unrecognised character in printer set name (%s). "
				                "Perhaps you forgot to specify the set?\n", optarg);
				return 1;
			}

			if (strlen(optarg) <= PRSET_LEN) {
				attrib->prsetname = optarg;
				attrib->copris_flags |= HAS_PRSET;
			} else {
				// Excessive length already makes it wrong
				fprintf(stderr, "Selected printer feature set does not exist (%s).\n", optarg);
				return 1;
			}
#else
			fprintf(stderr, "Option `-r|--printerset' isn't available in this "
			                "release of COPRIS\n"
			                "(printer feature sets were omitted with `-DWITHOUT_CMARK' "
			                "at build time).\n");
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
				fprintf(stderr, "Unrecognised characters in limit number (%s). ", parserr);
				if (*parserr == '-')
					fprintf(stderr, "Perhaps you forgot to specify the number?\n");

				return 1;
			}

			if (temp_limit > INT_MAX) {
				fprintf(stderr, "Limit number %s out of range. Maximum possible "
				                "value is %d (bytes).\n", optarg, INT_MAX);
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
				fprintf(stderr, "Port number is missing.\n");
			else if (optopt == 't')
				fprintf(stderr, "Translation file is missing.\n");
			else if (optopt == 'r')
				fprintf(stderr, "Printer set is missing.\n");
			else if (optopt == 'l')
				fprintf(stderr, "Limit number is missing.\n");
			else
				fprintf(stderr, "Option '-%c' is missing an argument.\n", optopt);
			return 1;
		case '?':
			if (optopt == 0)
				fprintf(stderr, "Option '%s' not recognised.\n", argv[optind - 1]);
			else
				fprintf(stderr, "Option '-%c' not recognised.\n", optopt);
			return 1;
		default:
			fprintf(stderr, "Getopt returned an unknown character code 0x%x.\n", c);
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
			fprintf(stderr, "Destination filename is too long.\n");
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
		if (LOG_ERROR){
			if (LOG_INFO)
				LOG_LOCATION();
			else
				printf("Note: ");

			printf("Only the first destination file name will be used.\n");
		}
	}

	return 0;
}

int main(int argc, char **argv) {
	// The main attributes struct which holds most of the run-time options
	struct Attribs attrib;

	// Translation file hash structure
	struct Trfile *trfile;

	// Printer set hash structure
	struct Prset *prset;

	// Input text, passed between functions
	UT_string *copris_text;

	attrib.portno       = 0;  // 0 -> input from stdin, >0 -> actual port number
	attrib.prset        = -1;
	attrib.daemon       = 0;
	attrib.limitnum     = 0;
	attrib.copris_flags = 0x00;

	int is_stdin = 0;     // Determine if text is coming via the standard input
	int parentfd = 0;     // Parent file descriptor to hold a socket

	// Parsing arguments
	int error = parse_arguments(argc, argv, &attrib);
	if (error)
		return EXIT_FAILURE;

	if (LOG_DEBUG) {
		LOG_LOCATION();
		printf("COPRIS started with PID %d\n", getpid());
	}

	if (argc < 2 && LOG_ERROR) {
		if (LOG_INFO)
			LOG_LOCATION();
		else
			printf("Note: ");

		printf("COPRIS won't do much without any arguments. "
               "Try using the '--help' option.\n");
	}

	if (attrib.portno == 0)
		is_stdin = 1;

	if (LOG_INFO) {
		LOG_LOCATION();
		printf("Verbosity level set to %d.\n", verbosity);
	}

	if (attrib.daemon && is_stdin) {
		attrib.daemon = 0;
		if (LOG_ERROR){
			if (LOG_INFO)
				LOG_LOCATION();
			else
				printf("Note: ");
			
			printf("Daemon mode not available while reading from stdin.\n");
		}
	}

	if (attrib.daemon && LOG_DEBUG)
		LOG_STRING("Daemon mode enabled.");

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
		error = copris_loadtrfile(attrib.trfile, &trfile);
		if (error) {
			// Missing translation files are as well not a fatal error when --quiet
			if (verbosity) {
				return EXIT_FAILURE;
			} else {
				fprintf(stderr, "Disabling translation.\n");
				attrib.copris_flags &= ~HAS_TRFILE;
			}
		}
	}
	
	if (attrib.limitnum > 0 && LOG_DEBUG) {
		LOG_LOCATION();
		printf("Limiting incoming data to %zu bytes.\n", attrib.limitnum);
	}
	
	if (LOG_DEBUG && !is_stdin) {
		LOG_LOCATION();
		printf("Server is listening to port %u.\n", attrib.portno);
	}
	
	if (LOG_INFO) {
		LOG_LOCATION();
		printf("Data stream will be sent to ");
		if (attrib.copris_flags & HAS_DESTINATION)
			printf("%s.\n", attrib.destination);
		else
			printf("stdout.\n");
	}
	
	// Open socket and listen
	if (!is_stdin) {
		error = copris_socket_listen(&parentfd, attrib.portno);
		if (error)
			goto exit_on_error;
	}

	// Allocate initial space input text
	utstring_new(copris_text);

	do {
		if (is_stdin) {
			copris_handle_stdin(copris_text, &attrib);
		} else {
			error = copris_handle_socket(copris_text, &parentfd, &attrib);
			if (error)
				break;
		}

		if (attrib.copris_flags & HAS_DESTINATION) {
			copris_write_file(attrib.destination, utstring_body(copris_text));
		} else {
			// Print Beginning-/End-Of-Stream markers if output isn't a file
			if (LOG_ERROR)
				puts("; BOS");

			fputs(utstring_body(copris_text), stdout);

			if (LOG_ERROR)
				puts("; EOS");
		}

	} while (attrib.daemon);

	utstring_free(copris_text);

	exit_on_error:
// 	if (attrib.copris_flags & HAS_TRFILE) {
// 		if (LOG_DEBUG) {
// 			LOG_STRING("Unloading translation file.");
// 		}
// 		copris_unload_trfile(&trfile);
// 	}

	if (error)
		return EXIT_FAILURE;

	if (LOG_DEBUG && !is_stdin) {
		LOG_STRING("Not running as a daemon, exiting.");
	}

	return 0;
}
