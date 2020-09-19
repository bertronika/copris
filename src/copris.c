/*
 * copris.c
 * Main program. Contains an argument parser and the essential COPRIS program structure.
 * 
 * COPRIS - a converting printer server
 * (c) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 * 
 * - https://azrael.digipen.edu/~mmead/www/Courses/CS180/getopt.html
 */

#define COPRIS_VER "0.9"

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


/* 
 * Verbosity levels:
 * 0  silent/fatal
 * 1  err
 * 2  info
 * 3  debug
 */
int verbosity = 1;

int main(int argc, char **argv) {
	int parentfd   = 0;  // Parent file descriptor to hold a socket
	int portno     = -1; // Listening port of this server
	int daemon     = 0;  // Is the daemon option set?
// 	int trfile_set = 0;  // Is the translation file option set?
// 	int prfile_set = 0;  // Is the printer feature set selected?
	int opt;             // Character, read by getopt
	char trfile[FNAME_LEN + 1]      = { 0 }; // Input translation file
	char printerset[PRSET_LEN + 1]  = { 0 }; // Input translation file
	char destination[FNAME_LEN + 1] = { 0 }; // Output filename

	// Bail if there is not even a port specified.
	if(argc < 2) {
		fprintf(stderr, "Missing arguments. The '--help' option will help you " 
		                "pick them.\n");
		return 1;
	}

	/* man 3 getopt_long */
	static struct option long_options[] = {
		{"port",    1, NULL, 'p'},
		{"daemon",  0, NULL, 'd'},
		{"trfile",  1, NULL, 't'},
		{"printer", 1, NULL, 'r'},
		{"verbose", 1, NULL, 'v'},
		{"quiet",   0, NULL, 'q'},
		{"help",    0, NULL, 'h'},
		{"version", 0, NULL, 'V'},
		{NULL,      0, NULL,   0} /* end-of-list */
	};
	
	// int argc, char* argv[], char* optstring, struct* long_options, int* longindex
	while((opt = getopt_long(argc, argv, ":p:dt:r:vqhV", long_options, NULL)) != -1) {
		switch(opt) {
			case 'p':
				portno = atoi(optarg);
				break;
			case 'd':
				daemon = 1;
				break;
			case 't':
// 				trfile_set = 1;
				if(strlen(optarg) <= FNAME_LEN) {
					strcpy(trfile, optarg);
				} else {
					fprintf(stderr, "Trfile filename too long (%s). "
					                "Exiting...\n", optarg);
					exit(1);
				}
				break;
			case 'r':
// 				prfile_set = 1;
				if(strlen(optarg) <= PRSET_LEN) {
					strcpy(printerset, optarg);
				} else {
					// Excessive length already makes it wrong
					fprintf(stderr, "Wrong printer feature set (%s). "
					                "Exiting...\n", optarg);
					exit(1);
				}
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
					fprintf(stderr, "You need to specify the port number.\n");
				else if(optopt == 't')
					fprintf(stderr, "You need to specify a translation file.\n");
				else
					fprintf(stderr, "Option '-%c' is missing an argument.\n", optopt);
				return 1;
			case '?':
				if(optopt == 0)
					fprintf(stderr, "Option '%s' not recognised.\n", argv[optind - 1]);
				else
					fprintf(stderr, "Error parsing option '-%c'\n", optopt);
				return 1;
			default:
				fprintf(stderr, "Undefined problem while parsing options\n");
				return 2;
		}
	}
	
	// If the last argument is present, copy it to destination[]
	// Only one argument is accepted, others are discarded
	if(argv[optind]) {
		if(strlen(argv[optind]) <= FNAME_LEN) {
			strcpy(destination, argv[optind]);
		} else {
			fprintf(stderr, "Destination filename too long (%s). " 
			                "Exiting...\n", argv[optind]);
			exit(1);
		}
	}
	
// 	if(optind < argc) {
// 		printf("Non-option args: ");
// 		while (optind < argc)
// 			printf("%s ", argv[optind++]);
// 		printf("\n");

	if(portno < 1) {
		fprintf(stderr, "Port number is missing. Set it with the '-p' option. " 
		                "Exiting...\n");
		return 1;
	}
	
	if(log_info()) {
		log_date();
		printf("Verbosity level set to %d.\n", verbosity);
	}
	
	if(daemon && log_debug()) {
		log_date();
		printf("Daemon mode enabled.\n");
	}
	
	if(log_debug()) {
		log_date();
		printf("Server listening port set to %d.\n", portno);
	}
	
	// We are writing to a file if destination is not NULL.
	if(destination[0]) {
		// Are we able to write to the output file?
		if(access(destination, W_OK) == -1)
			log_perr(-1, "access", "Unable to write to output file/printer. " 
			                       "Does it exist?");
	}
	
	// Where does the output go?
	if(log_info()) {
		log_date();
		printf("Data stream will be sent to ");
		if(destination[0])
			printf("%s.\n", destination);
		else
			printf("stdout.\n");
	}
	
	// Read the translation file. The function populates global variables *input
	// and *replacement, defined in translate.c
	if(trfile[0]) {
		copris_trfile(trfile);
	}
	
	// Open socket and listen
	copris_listen(&parentfd, portno);
	
	do {
		// Accept incoming connections
// 	    copris_read(&parentfd, destination, trfile[0], printerset[0]);
	    copris_read(&parentfd, destination, trfile[0], 1);
	} while(daemon);
	
	if(log_debug()) {
		log_date();
		printf("Daemon mode is not set, exiting...\n");
	}
	
	return 0;
}

void copris_help() {
	printf("Usage: copris [-p PORT] (optional arguments) <printer location>\n\n");
	printf("  -p, --port     Port to listen to\n");
	printf("  -d, --daemon   Run continuously, as a daemon\n");
	printf("  -t, --trfile   (optional) character translation file\n");
	printf("  -r, --printer  (optional) printer feature set\n");
	printf("\n");
	printf("  -v, --verbose  Be verbose (-vv more\n");
	printf("  -q, --quiet    Display nothing except fatal errors (stderr)\n");
	printf("  -h, --help     Show this help\n");
	printf("  -V, --version  Show program version and included printer \n");
	printf("                 feature sets\n");
	printf("\n");
	printf("Printer location can either be an actual printer address, such\n");
	printf("as /dev/ttyS0, or a file. If left empty, output is printed to stdout.\n");
	printf("\n");
}

void copris_version() {
	printf("COPRIS version %s\n", COPRIS_VER);
	printf("(C) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>\n\n");
	printf("Compiled options:\n");
	printf("  Buffer size:          %d B\n", BUFSIZE);
	printf("  Connection backlog:   %d connection(s)\n", BACKLOG);
	printf("  Max. filename length: %d characters\n", FNAME_LEN);
	printf("Included printer feature sets:\n  ");
	if(prsets[0]) {
		for(int p = 0; prsets[p][0] != '\0'; p++) {
			printf("%s  ", prsets[p]);
		}
	} else {
		printf("(none)");
	}
	printf("\n");
}
