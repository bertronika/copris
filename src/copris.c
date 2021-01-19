/*
 * copris.c
 * Parses arguments, trfile and printerset selections and runs the server
 * 
 * COPRIS - a converting printer server
 * (c) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 * 
 * Licensed under the MIT/Expat license.
 * 
 * - https://azrael.digipen.edu/~mmead/www/Courses/CS180/getopt.html
 */

#define COPRIS_VER "0.9"

#ifndef REL
#define COPRIS_RELEASE ""
#else
#define COPRIS_RELEASE ("-" REL)
#endif

#define FNAME_LEN 36

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

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
 * 1  err
 * 2  info
 * 3  debug
 */
int verbosity = 1;

int main(int argc, char **argv) {
	int parentfd = 0;      // Parent file descriptor to hold a socket
	int daemon   = 0;      // Is the daemon option set?
	int portno   = -1;     // Listening port of this server
	int limitnum = 0;      // Limit received number of bytes
	int limit_cutoff = 0;  // Cut text off at limit instead of discarding it
	int is_stdin = 0;
	int is_prset = 0;
	int opt;               // Character, read by getopt
	char *parserr;         // String to integer conversion error
	char trfile[FNAME_LEN + 1]      = { 0 }; // Input translation file
	char prsetinput[PRSET_LEN + 1]  = { 0 }; // Input printer set
	char destination[FNAME_LEN + 1] = { 0 }; // Output filename

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
	while((opt = getopt_long(argc, argv, ":p:dt:r:l:vqhV", long_options, NULL)) != -1) {
		switch(opt) {
			case 'p':
				portno = strtol(optarg, &parserr, 10);
				if(*parserr) {
					fprintf(stderr, "Unrecognised characters in port number (%s). " 
					                "Exiting...\n", parserr);
					return 1;
				}
				
				if(portno > 65535 || portno < 1) {
					fprintf(stderr, "Port number out of range. " 
					                "Exiting...\n");
					return 1;
				}
				break;
			case 'd':
				daemon = 1;
				break;
			case 't':
				if(strlen(optarg) <= FNAME_LEN) {
					strcpy(trfile, optarg);
				} else {
					fprintf(stderr, "Trfile filename too long (%s). "
					                "Exiting...\n", optarg);
					return 1;
				}
				break;
			case 'r':
				if(strlen(optarg) <= PRSET_LEN) {
					strcpy(prsetinput, optarg);
					is_prset = 1;
				} else {
					// Excessive length already makes it wrong
					fprintf(stderr, "Selected printer feature set does not exist (%s). "
					                "Exiting...\n", optarg);
					return 1;
				}
				break;
			case 'l':
				limitnum = strtol(optarg, &parserr, 10);
				if(*parserr) {
					fprintf(stderr, "Unrecognised characters in limit number (%s). " 
					                "Exiting...\n", parserr);
					return 1;
				}
				
				if(limitnum > 4096 || limitnum < 0) {
					fprintf(stderr, "Limit number out of range. " 
					                "Exiting...\n");
					return 1;
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
				copris_help();
				return 0;
			case 'V':
				copris_version();
				return 0;
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
				return 1;
			case '?':
				if(optopt == 0)
					fprintf(stderr, "Option '%s' not recognised. "
					                "Exiting...\n", argv[optind - 1]);
				else
					fprintf(stderr, "Option '-%c' not recognised. " 
					                "Exiting...\n" , optopt);
				return 1;
			default:
				fprintf(stderr, "Undefined problem while parsing options. "
				                "Exiting... \n");
				return 2;
		}
	}
	
	if(argc < 2) {
		printf("Without any arguments, COPRIS won't do much. Try using "
               "using the '--help' option.\n");
	}
	
	if(portno < 1)
		is_stdin = 1;
	
	// If the last argument is present, copy it to destination[]
	// Only one argument is accepted, others are discarded
	if(argv[optind]) {
		if(strlen(argv[optind]) <= FNAME_LEN) {
			strcpy(destination, argv[optind]);
		} else {
			fprintf(stderr, "Destination filename too long (%s). " 
			                "Exiting...\n", argv[optind]);
			return 1;
		}
	}
	
	// We are writing to a file if destination is not NULL.
	if(destination[0]) {
		// Are we able to write to the output file?
		if(access(destination, W_OK) == -1)
			log_perr(-1, "access", "Unable to write to output file/printer. " 
			                       "Does it exist?");
	}
	
	// Check if printer set exists and set it to its index + 1
	if(is_prset) {
		for(int p = 0; printerset[p][0][0] != '\0'; p++) {
			if(strcmp(prsetinput, printerset[p][0]) == 0) {
				prsetinput[0] = p + 1;
				is_prset = 2;
				
				break;
			}
		}
		
		if(is_prset != 2){
			fprintf(stderr, "Selected printer feature set does not exist. " 
			                "Exiting...\n");
			return 1;
		}
	}
	
	// Read the translation file. The function populates global variables *input
	// and *replacement, defined in translate.c
	// copris_trfile returns 1 if a failure is detected
	if(trfile[0] && copris_trfile(trfile)) {
		// These two are malloc'd by copris_trfile()
		free(input);
		free(replacement);
		if(verbosity) {
			// Error in trfile. We are not quiet, so notify and exit
			fprintf(stderr, "Exiting...\n");
			return 1;
		} else {
			// Error in trfile. We are quiet, so notify and continue
			fprintf(stderr, "Continuing without trfile.\n");
		}
	}
	
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
	
	if(is_prset && log_info()) {
		log_date();
		printf("Selected printer feature set %s.\n", 
		       printerset[(int)prsetinput[0] - 1][0]);
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
		if(destination[0])
			printf("%s.\n", destination);
		else
			printf("stdout.\n");
	}
	
	// Open socket and listen
	if(!is_stdin)
		copris_listen(&parentfd, portno);
	
	do {
		if(!is_stdin) {
			// Accept incoming connections, process data and send it out
			copris_read(&parentfd, destination, daemon, trfile[0], prsetinput[0],
			            limitnum, limit_cutoff);
		} else {
			if(copris_stdin(destination, trfile[0], prsetinput[0]))
				return 1;
		}
	} while(daemon);
	
	if(log_debug() && !is_stdin) {
		log_date();
		printf("Daemon mode is not set, exiting.\n");
	}
	
	return 0;
}

void copris_help() {
	printf("Usage: copris [arguments] <printer location>\n\n"
	       "  -p, --port NUMBER      Listening port\n"
	       "  -d, --daemon           Run as a daemon\n"
	       "  -t, --trfile TRFILE    Character translation file\n"
	       "  -r, --printer PRSET    Printer feature set\n"
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
	       "Printer location can either be an actual printer address, such\n"
	       "as /dev/ttyS0, or a file. If left empty, output is printed to stdout.\n\n"
		   "Data can also be read from stdin. If this is desired, omit the port\n"
		   "argument, as incoming connections are prefered over stdin.\n"
	       );
}

void copris_version() {
	printf("COPRIS version %s%s\n", COPRIS_VER, COPRIS_RELEASE);
	printf("(C) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>\n\n");
	printf("Compiled options:\n");
	printf("  Buffer size:          %4d bytes\n", BUFSIZE);
	printf("  Max. filename length: %4d characters\n", FNAME_LEN);
	printf("Included printer feature sets:\n  ");
	list_prsets();
	printf("\n");
}
