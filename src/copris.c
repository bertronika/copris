/*
 * copris.c
 * Main program. Contains an argument parser and the essential COPRIS program structure.
 * 
 * COPRIS - a converting printer server
 * (c) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 * 
 * - https://azrael.digipen.edu/~mmead/www/Courses/CS180/getopt.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include "debug.h"
#include "server.h"
#include "writer.h"
#include "translate.h"

/* 
 * Verbosity levels:
 * 0  silent/fatal
 * 1  err
 * 2  info
 * 3  debug
 */
int verbosity = 1;

int main(int argc, char **argv)
{
	int portno = -1;    // Listening port of this server
	int daemon = 0;     // Is the daemon option set?
	int trfile_set = 0; // Is the translation file option set?
	int opt;            // Char read by getopt
	server_t server      = { 0 }; // File descriptor struct initialisation
	char destination[32] = "0";   // Which device to output final data to
	char trfile[32]      = "0";   // Specified translation file

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
		{"verbose", 1, NULL, 'v'},
		{"quiet",   0, NULL, 'q'},
		{"help",    0, NULL, 'h'},
		{"version", 0, NULL, 'V'},
		{NULL,      0, NULL,   0} /* end-of-list */
	};
	
	// int argc, char* argv[], char* optstring, struct* long_options, int* longindex
// 	while((opt = getopt_long(argc, argv, ":p:dt:vqhV", long_options, &longindex)) != -1) {
	while((opt = getopt_long(argc, argv, ":p:dt:vqhV", long_options, NULL)) != -1) {
		switch(opt) {
			case 'p':
				portno = atoi(optarg);
				break;
			case 'd':
				daemon = 1;
				break;
			case 't':
				trfile_set = 1;
				strcpy(trfile, optarg);
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
	
	// If the last argument is a file, copy it to destination[], otherwise to stdout
	if(argv[optind] != NULL) {
	// 		destination = argv[optind];
// 			printf("velikost argv = %ld\n", sizeof(destination));
			
		if(strlen(argv[optind]) >= sizeof(destination)) {
			fprintf(stderr, "Destination filename too long (%s). " 
			                "Exiting...\n", argv[optind]);
			exit(1);
		} else {
			strcpy(destination, argv[optind]);
		}
		
		//char *destination;
		//destination = argv[optind];
		
	}
	
// 	if(optind < argc) {
// 		printf("Non-option args: ");
// 		while (optind < argc)
// 			printf("%s ", argv[optind++]);
// 		printf("\n");
// 	} else {
// 		
// 	}

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
	
	// Is destination not stdout?
	if(destination[0] != '0') {
		// Are we able to write to the output file?
		if(access(destination, W_OK) == -1)
			log_perr(-1, "access", "Unable to write to output file/printer. " 
			                       "Does it exist?");
	}
	
	// Where does the output go?
	if(log_info()) {
		log_date();
		printf("Data stream will be sent to ");
		if(destination[0] != '0')
			printf("%s.\n", destination);
		else
			printf("stdout.\n");
	}
	
	// Read the translation file
	if(trfile[0] != '0') {
		copris_trfile(trfile);
	}
	
	// Open socket and listen
	copris_listen(&server, portno);
	
	do {
		// Accept incoming connections
	    copris_read(&server, destination, trfile_set);
	} while(daemon);
	
	if(log_info()) {
		log_date();
		printf("Daemon mode is not set, exiting...\n");
	}
	
	return 0;
}
