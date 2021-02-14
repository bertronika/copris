/*
 * writer.c
 * Data output interface
 * 
 * Part of COPRIS - a converting printer server
 * (c) 2020-21 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "writer.h"

int copris_write(char *dest, unsigned char *data) {
	FILE *file_ptr;  // File stream pointer
	int buf_num;     // Number of bytes in buffer
	int buf_written; // Number of written bytes to file
	int ferr;        // Error code of a file operation
	
	// Count the buffer
	//for(buf_num = 0; data[buf_num] != '\0'; buf_num++);
	buf_num = strlen((char *)data);
	
	file_ptr = fopen(dest, "a"); // rEAD, wRITE, aPPEND, man 3 fopen
	
	if(file_ptr == NULL)
		if(log_perr(-1, "fopen", "Failed to open output file for writing/appending."))
			return 1;
		
	if(log_debug()) {
		log_date();
		printf("Output file opened.\n");
	}
	
	buf_written = fwrite(data, sizeof(data[0]), buf_num, file_ptr);
	if(log_debug()) {
		log_date();
		printf("Finished writing %d B.\n", buf_written);
	}

	if(buf_written < buf_num) {
		fprintf(stderr, "fwrite: Failure while writing to output file; " 
		                "not enough bytes transferred.\n");
		return 1;
	}
	
	ferr = fclose(file_ptr);
	if(log_perr(ferr, "close", "Failed to close the output file after writing."))
		return 1;

	if(log_debug()) {
		log_date();
		printf("Output file closed.\n");
	}
	
	return 0;
}
