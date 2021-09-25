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
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>

#include "debug.h"
#include "config.h"
#include "Copris.h"

#include "server.h"
#include "translate.h"
#include "printerset.h"

#define list_prsets() for(int p = 0; printerset[p][0][0] != '\0'; p++)\
	printf("%s  ", printerset[p][0]);

#ifndef DEBUG
#	define COPRIS_RELEASE ""
#else
#	define COPRIS_RELEASE ("-" REL)
#endif

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
	       "  -r, --printer PRSET    Printer feature set\n"
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
	printf("COPRIS version %s%s\n"
	       "(C) 2020-21 Nejc Bertoncelj <nejc at bertoncelj.eu.org>\n\n"
	       "Compiled options:\n"
	       "  Buffer size:          %4d bytes\n",
	       COPRIS_VER, COPRIS_RELEASE, BUFSIZE);
	printf("Included printer feature sets:\n  ");
	list_prsets();
	printf("\n");

	exit(EXIT_SUCCESS);
}

int parse_arguments(int argc, char **argv, struct Attribs *attrib) {
	int c;          // Current Getopt argument
	char *parserr;  // String to integer conversion error
	int ferror;     // Error code of a file operation

	unsigned long temp_long;  // A temporary long integer
	long max_path_len;

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

	// Putting a colon in front of the options disables the built-in error reporting
	// of getopt_long(3) and allows us to specify more appropriate errors (ie. `Printer
	// set is missing.' instead of `option requires an argument -- 'r')
	while((c = getopt_long(argc, argv, ":p:dt:r:l:vqhV", long_options, NULL)) != -1) {
		switch(c) {
		case 'p':
			// Reset errno to distinguish between success/failure after call
			errno = 0;
			temp_long = strtoul(optarg, &parserr, 10);

			if(raise_errno_perror(errno, "strtoul", "Error parsing port number."))
				return 1;

			if(*parserr) {
				fprintf(stderr, "Unrecognised characters in port number (%s). ", parserr);
				if(*parserr == '-')
					fprintf(stderr, "Perhaps you forgot to specify the number?\n");

				return 1;
			}

			// If user specifies a negative port number, it overflows the unsigned long.
			// To prevent displaying a big number, display the entered string instead.
			if(temp_long > 65535 || temp_long < 1) {
				fprintf(stderr, "Port number %s out of reasonable range.\n", optarg);
				return 1;
			}

			attrib->portno = (unsigned int)temp_long;
			break;
		case 'd':
			attrib->daemon = 1;
			break;
		case 't':
			if(*optarg == '-') {
				fprintf(stderr, "Unrecognised character found in translation file name (-). "
				                "Perhaps you forgot to specify the file?\n");
				return 1;
			}

			// Get the maximum path name length on the filesystem where
			// the trfile resides.
			errno = 0;
			max_path_len = pathconf(optarg, _PC_PATH_MAX);

			if(raise_errno_perror(errno, "pathconf", "Error querying translation file."))
				return 1;

			// TODO: is this check necessary after checking pathconf for errors?
			if(strlen(optarg) >= (size_t)max_path_len) {
				fprintf(stderr, "Translation file's name is too long.\n");
				return 1;
			}

			attrib->trfile = optarg;
			attrib->copris_flags |= HAS_TRFILE;
			break;
		case 'r':
			if(*optarg == '-') {
				fprintf(stderr, "Unrecognised character found in printer set name (-). "
				                "Perhaps you forgot to specify the set?\n");
				return 1;
			}

			if(strlen(optarg) <= PRSET_LEN) {
				attrib->prsetname = optarg;
				attrib->copris_flags |= HAS_PRSET;
			} else {
				// Excessive length already makes it wrong
				fprintf(stderr, "Selected printer feature set does not exist (%s).\n", optarg);
				return 1;
			}
			break;
		case 'l':
			// Reset errno to distinguish between success/failure after call
			errno = 0;
			temp_long = strtoul(optarg, &parserr, 10);

			if(raise_errno_perror(errno, "strtoul", "Error parsing limit number."))
				return 1;

			if(*parserr) {
				fprintf(stderr, "Unrecognised characters in limit number (%s). ", parserr);
				if(*parserr == '-')
					fprintf(stderr, "Perhaps you forgot to specify the number?\n");

				return 1;
			}

			if(temp_long > INT_MAX) {
				fprintf(stderr, "Limit number %s out of range. Maximum possible "
				                "value is %d (bytes).\n", optarg, INT_MAX);
				return 1;
			}

			attrib->limitnum = (int)temp_long;
			break;
		case 'D':
			attrib->limit_cutoff = 1;
			break;
		case 'v':
			if(verbosity != 0 && verbosity < 3)
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
			if(optopt == 'p')
				fprintf(stderr, "Port number is missing.\n");
			else if(optopt == 't')
				fprintf(stderr, "Translation file is missing.\n");
			else if(optopt == 'r')
				fprintf(stderr, "Printer set is missing.\n");
			else if(optopt == 'l')
				fprintf(stderr, "Limit number is missing.\n");
			else
				fprintf(stderr, "Option '-%c' is missing an argument.\n", optopt);
			return 1;
		case '?':
			if(optopt == 0)
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
	if(argv[optind]) {
		// Get the maximum path name length on the filesystem where
		// the output file resides.
		errno = 0;
		max_path_len = pathconf(argv[optind], _PC_PATH_MAX);

		if(raise_errno_perror(errno, "pathconf", "Error querying your chosen destination."))
			return 1;

		// TODO: is this check necessary after checking pathconf for errors?
		if(strlen(argv[optind]) >= (size_t)max_path_len) {
			fprintf(stderr, "Destination filename is too long.\n");
			return 1;
		}

		ferror = access(argv[optind], W_OK);
		if(raise_perror(ferror, "access", "Unable to write to output file/printer. Does "
		                                  "it exist, with appropriate permissions?"))
			return 1;

		attrib->destination = argv[optind];
		attrib->copris_flags |= HAS_DESTINATION;
	}

	// Check for multiple destination arguments
	if((attrib->copris_flags & HAS_DESTINATION) && argv[++optind]) {
		if(log_error()){
			if(log_info())
				log_date();
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

	attrib.portno       = 0;  // 0 -> input from stdin, >0 -> actual port number
	attrib.prset        = -1;
	attrib.daemon       = 0;
	attrib.limitnum     = 0;
	attrib.limit_cutoff = 0;
	attrib.copris_flags = 0x00;

	int is_stdin  = 0;     // Determine if text is coming via the standard input
	int parentfd  = 0;     // Parent file descriptor to hold a socket
	int error     = 0;

	// Parsing arguments
	error = parse_arguments(argc, argv, &attrib);
	if(error)
		return EXIT_FAILURE;

	if(argc < 2 && log_error()) {
		if(log_info())
			log_date();
		else
			printf("Note: ");

		printf("COPRIS won't do much without any arguments. "
               "Try using the '--help' option.\n");
	}

	if(attrib.portno == 0)
		is_stdin = 1;

	if(log_info()) {
		log_date();
		printf("Verbosity level set to %d.\n", verbosity);
	}

	if(attrib.daemon && is_stdin) {
		attrib.daemon = 0;
		if(log_error()){
			if(log_info())
				log_date();
			else
				printf("Note: ");
			
			printf("Daemon mode not available while reading from stdin.\n");
		}
	}

	if(attrib.daemon && log_debug()) {
		log_date();
		printf("Daemon mode enabled.\n");
	}

	// Parsing the selected printer feature sets
	if(attrib.copris_flags & HAS_PRSET) {
		for(int p = 0; printerset[p][0][0] != '\0'; p++) {
			if(strcmp(attrib.prsetname, printerset[p][0]) == 0) {
				//prset.exists = p + 1;
				attrib.prset = p;
				break;
			}
		}

		// Missing printer sets are not a fatal error in quiet mode
		if(attrib.prset == -1) {
			fprintf(stderr, "Selected printer feature set does not exist "
			                "(%s). ", attrib.prsetname);
			if(verbosity) {
				return EXIT_FAILURE;
			} else {
				fprintf(stderr, "Disabling it.\n");
				attrib.copris_flags &= ~HAS_PRSET;
			}
		}
	}

	// Parsing and loading translation definitions
	if(attrib.copris_flags & HAS_TRFILE) {
		error = copris_loadtrfile(attrib.trfile);
		if(error) {
			// Missing translation files are as well not a fatal error when --quiet
			if(verbosity) {
				return EXIT_FAILURE;
			} else {
				fprintf(stderr, "Disabling translation.\n");
				attrib.copris_flags &= ~HAS_TRFILE;
			}
		}
	}
	
	if((attrib.copris_flags & HAS_PRSET) && log_info()) {
		log_date();
		printf("Selected printer feature set %s.\n",
		       printerset[attrib.prset][0]);
	}
	
	if(attrib.limitnum > 0 && log_debug()) {
		log_date();
		printf("Limiting incoming data to %d bytes.\n", attrib.limitnum);
	}
	
	if(log_debug() && !is_stdin) {
		log_date();
		printf("Server is listening to port %u.\n", attrib.portno);
	}
	
	if(log_info()) {
		log_date();
		printf("Data stream will be sent to ");
		if(attrib.copris_flags & HAS_DESTINATION)
			printf("%s.\n", attrib.destination);
		else
			printf("stdout.\n");
	}
	
	// Open socket and listen
	if(!is_stdin) {
		error = copris_socket_listen(&parentfd, attrib.portno);
		if(error)
			goto exit_on_error;
	}

	do {
		if(is_stdin) {
			error = copris_read_stdin(&attrib);
		} else {
			error = copris_read_socket(&parentfd, &attrib);
		}
	} while(attrib.daemon && !error);

	exit_on_error:
	if(error)
		return EXIT_FAILURE;

	if(log_debug() && !is_stdin) {
		log_date();
		printf("Not running as a daemon, exiting.\n");
	}

	return 0;
}
