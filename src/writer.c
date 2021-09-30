/*
 * writer.c
 * Data output interface
 * 
 * Copyright (C) 2020-2021 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "debug.h"
#include "writer.h"

int copris_write_file(char *dest, char *data) {
	FILE *file_ptr;  // File stream pointer
	int buf_num;     // Number of bytes in buffer
	int buf_written; // Number of written bytes to file
	int ferror;      // Error code of a file operation
	int fatal = 0;   // Fatal error, close the file if possible
	
	// Count the buffer
	//for(buf_num = 0; data[buf_num] != '\0'; buf_num++);
	buf_num = strlen(data);
	
	file_ptr = fopen(dest, "a"); // rEAD, wRITE, aPPEND, man 3 fopen
	if(file_ptr == NULL)
		return raise_errno_perror(errno, "fopen", "Failed to open output file.");
		
	if(log_debug()) {
		log_date();
		printf("Output file opened.\n");
	}
	
	buf_written = fwrite(data, sizeof(data[0]), buf_num, file_ptr);
	if(buf_written < buf_num) {
		fprintf(stderr, "fwrite: Failure while appending to output file; "
		                "not enough bytes transferred.\n");
		fatal = 1;
// 		return 1;
	} else if(log_debug()) {
		log_date();
		printf("Finished appending %d B.\n", buf_written);
	}

	ferror = fclose(file_ptr);
	if(raise_perror(ferror, "fclose", "Failed to close the output file."))
		return 1;

	if(log_debug()) {
		log_date();
		printf("Output file closed.\n");
	}
	
	return fatal;
}
