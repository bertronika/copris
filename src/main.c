/*
 * copris.c
 * Parses arguments, trfile and printerset selections and runs the server
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
 * 
 * - https://azrael.digipen.edu/~mmead/www/Courses/CS180/getopt.html
 */

#define COPRIS_VER "0.9"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>

#include "common.h"
#include "config.h"
#include "debug.h"
#include "copris.h"
#include "server.h"
#include "translate.h"
#include "printerset.h"

#define list_prsets() for(int p = 0; printerset[p][0][0] != '\0'; p++)\
	printf("%s  ", printerset[p][0]);

/* 
 * Verbosity levels:
 * 0  silent/fatal
 * 1  error
 * 2  info
 * 3  debug
 */
int verbosity = 1;

int main(int argc, char **argv) {
	int opt;               // Character, read by getopt
	char *parserr;         // String to integer conversion error
	int terminate = 0;     // 1 -> free pointers, terminate
	                       // 2 -> free pointers, display help (3 -> version)
	int daemon    = 0;     // Is the daemon option set?
	int is_stdin  = 0;     // Don't open a socket if true

	int parentfd  = 0;     // Parent file descriptor to hold a socket
	int portno    = -1;    // Listening port of this server

	int limitnum     = 0;  // Limit received number of bytes
	int limit_cutoff = 0;  // Cut text off at limit instead of discarding it

	struct attrib trfile, prset, destination;
	trfile      = (struct attrib) { 0 };  // Translation file (user-supplied)
	prset       = trfile;                 // Printer set (compiled in)
	destination = trfile;                 // Destination filename

	/* man 3 getopt_long */
	static struct option long_options[] = {
		{"port",          1, NULL, 'p'},
		{"daemon",        0, NULL, 'd'},
		{"trfile",        1, NULL, 't'},
		{"printer",       1, NULL, 'r'},
		{"limit",         1, NULL, 'l'},
		{"cutoff-limit",  0, NULL, 'D'},
		{"verbose",       2, NULL, 'v'},
		{"quiet",         0, NULL, 'q'},
		{"help",          0, NULL, 'h'},
		{"version",       0, NULL, 'V'},
		{NULL,            0, NULL,   0} /* end-of-list */
	};
	
	// int argc, char* argv[], char* optstring, struct* long_options, int* longindex
	while((opt = getopt_long(argc, argv, ":p:dt:r:l:vqhV", long_options, NULL)) != -1 &&
		  !terminate) {
		switch(opt) {
		case 'p':
			portno = strtol(optarg, &parserr, 10);
			if(*parserr) {
				fprintf(stderr, "Unrecognised characters in port number (%s). "
				                "Exiting...\n", parserr);
				terminate = 1;
				break;
			}

			if(portno > 65535 || portno < 1) {
				fprintf(stderr, "Port number out of range. "
				                "Exiting...\n");
				terminate = 1;
				break;
			}
			break;
		case 'd':
			daemon = 1;
			break;
		case 't':
			if(strlen(optarg) <= FNAME_LEN) {
				terminate = store_argument(optarg, &trfile);
			} else {
				fprintf(stderr, "Trfile filename too long (%s). "
				                "Exiting...\n", optarg);
				terminate = 1;
			}
			break;
		case 'r':
			if(strlen(optarg) <= PRSET_LEN) {
				terminate = store_argument(optarg, &prset);
			} else {
				// Excessive length already makes it wrong
				fprintf(stderr, "Selected printer feature set does not exist (%s). "
				                "Exiting...\n", optarg);
				terminate = 1;
			}
			break;
		case 'l':
			limitnum = strtol(optarg, &parserr, 10);
			if(*parserr) {
				fprintf(stderr, "Unrecognised characters in limit number (%s). "
				                "Exiting...\n", parserr);
				terminate = 1;
				break;
			}

			if(limitnum > 4096 || limitnum < 0) {
				fprintf(stderr, "Limit number out of range. "
				                "Exiting...\n");
				terminate = 1;
				break;
			}
			break;
		case 'D':
			limit_cutoff = 1;
			break;
		case 'v':
			if(verbosity < 3)
				verbosity++;
			break;
		case 'q':
			verbosity = 0;
			break;
		case 'h':
			terminate = 2;
			break;
		case 'V':
			terminate = 3;
			break;
		case ':':
			if(optopt == 'p')
				fprintf(stderr, "Port number is missing. "
				                "Exiting...\n");
			else if(optopt == 't')
				fprintf(stderr, "Translation file is missing. "
				                "Exiting...\n");
			else if(optopt == 'r')
				fprintf(stderr, "Printer set is missing. "
				                "Exiting...\n");
			else if(optopt == 'l')
				fprintf(stderr, "Limit number is missing. "
				                "Exiting...\n");
			else
				fprintf(stderr, "Option '-%c' is missing an argument. "
				                "Exiting...\n", optopt);
			terminate = 1;
			break;
		case '?':
			if(optopt == 0)
				fprintf(stderr, "Option '%s' not recognised. "
				                "Exiting...\n", argv[optind - 1]);
			else
				fprintf(stderr, "Option '-%c' not recognised. "
				                "Exiting...\n" , optopt);
			terminate = 1;
			break;
		default:
			fprintf(stderr, "Undefined problem while parsing options. "
			                "Exiting... \n");
			terminate = 10;
			break;
		}
	}
	
	// Only one destination argument is accepted, others are discarded
	if(!terminate && argv[optind]) {
		if(strlen(argv[optind]) <= FNAME_LEN) {
			terminate = store_argument(argv[optind], &destination);
		} else {
			fprintf(stderr, "Destination filename too long (%s). " 
			                "Exiting...\n", argv[optind]);
			terminate = 1;
		}

		if(access(destination.text, W_OK) == -1) {
			log_perr(-1, "access", "Unable to write to output file/printer. Does "
			                       "it exist, with appropriate permissions?");

			terminate = 1;
		}
	}
	
	if(!terminate && prset.exists) {
		prset.exists = 0;
		for(int p = 0; printerset[p][0][0] != '\0'; p++) {
			if(strcmp(prset.text, printerset[p][0]) == 0) {
				prset.exists = p + 1;
				break;
			}
		}

		if(prset.exists < 1) {
			fprintf(stderr, "Selected printer feature set does not exist "
			                "(%s). ", prset.text);
			if(verbosity) {
				fprintf(stderr, "Exiting...\n");
				terminate = 1;
			} else {
				fprintf(stderr, "Disabling it.\n");
			}
		}

		free(prset.text);
	}

	// Check and load translation definitions
	if(!terminate && trfile.exists && copris_trfile(trfile.text)) {
		trfile.exists = 0; // trfile is free'd in copris_trfile
		if(verbosity) {
			// Error in trfile. We are not quiet, so notify and exit
			fprintf(stderr, "Exiting...\n");
			terminate = 1;
		} else {
			// Error in trfile. We are quiet, so disable, notify and continue
			fprintf(stderr, "Disabling translation.\n");
		}
	}

	// If an erroneous argument is passed after allocations, free them
	if(terminate) {
		if(trfile.exists)
			free(trfile.text);

		if(prset.exists)
			free(prset.text);

		if(destination.exists)
			free(destination.text);

		if(terminate == 2) {
			copris_help(argv[0]);
			return 0;
		} else if(terminate == 3) {
			copris_version();
			return 0;
		}

		return terminate;
	}

	if(argc < 2 && log_err()) {
		if(log_info())
			log_date();
		else
			printf("Note: ");

		printf("Without any arguments, COPRIS won't do much. "
               "Try using the '--help' option.\n");
	}

	if(portno < 1)
		is_stdin = 1;

	if(log_info()) {
		log_date();
		printf("Verbosity level set to %d.\n", verbosity);
	}

	if(daemon && is_stdin) {
		daemon = 0;
		if(log_err()){
			if(log_info())
				log_date();
			else
				printf("Note: ");
			
			printf("Daemon mode not available while reading from stdin.\n");
		}
	}

	if(daemon && log_debug()) {
		log_date();
		printf("Daemon mode enabled.\n");
	}
	
	if(prset.exists && log_info()) {
		log_date();
		printf("Selected printer feature set %s.\n",
		       printerset[prset.exists - 1][0]);
	}
	
	if(limitnum > 0 && log_debug()) {
		log_date();
		printf("Limiting received number of bytes to %d B.\n", limitnum);
	}
	
	if(log_debug() && !is_stdin) {
		log_date();
		printf("Server listening port set to %d.\n", portno);
	}
	
	if(log_info()) {
		log_date();
		printf("Data stream will be sent to ");
		if(destination.exists)
			printf("%s.\n", destination.text);
		else
			printf("stdout.\n");
	}
	
	// Open socket and listen
	if(!is_stdin)
		if((terminate = copris_listen(&parentfd, portno)))
			return terminate;

	do {
		if(!is_stdin) {
			// Read from socket
			terminate = copris_read(&parentfd, daemon, limitnum, limit_cutoff,
			                        &trfile, prset.exists, &destination);
		} else {
			// Read from stdin
			terminate = copris_stdin(&trfile, prset.exists, &destination);
		}
	} while(daemon && !terminate);

	if(destination.exists)
		free(destination.text);

	if(terminate)
		return terminate;

	if(log_debug() && !is_stdin) {
		log_date();
		printf("Daemon mode is not set, exiting.\n");
	}

	return 0;
}

int store_argument(char *optarg, struct attrib *attribute) {
	attribute->text = malloc(strlen(optarg) + 1);

	if(!attribute->text) {
		log_perr(-1, "malloc", "Memory allocation error.");
		return 1;
	} else {
		attribute->exists = 1;
		strcpy(attribute->text, optarg);
	}

	return 0;
}

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
}

void copris_version() {
	printf("COPRIS version %s%s\n"
	       "(C) 2020-21 Nejc Bertoncelj <nejc at bertoncelj.eu.org>\n\n"
	       "Compiled options:\n"
	       "  Buffer size:          %4d bytes\n"
	       "  Max. filename length: %4d characters\n",
	       COPRIS_VER, COPRIS_RELEASE, BUFSIZE, FNAME_LEN);
	printf("Included printer feature sets:\n  ");
	list_prsets();
	printf("\n");
}
