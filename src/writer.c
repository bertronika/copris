/*
 * File writing interface
 *
 * Copyright (C) 2020-2022 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <utstring.h> /* uthash library - dynamic strings */

#include "debug.h"
#include "writer.h"

bool copris_write_file(const char *dest, UT_string *copris_text) {
	bool error = false;
	size_t text_length = utstring_len(copris_text);
	
	// Open destination file, set for (a)ppending text to it
	FILE *file_ptr = fopen(dest, "a");
	if (file_ptr == NULL)
		return raise_errno_perror(errno, "fopen", "Failed to open output file.");
		
	if (LOG_DEBUG)
		LOG_STRING("Output file opened.");

	size_t written_text_length = fwrite(utstring_body(copris_text), 1, text_length, file_ptr);

	if (written_text_length < text_length) {
		fprintf(stderr, "fwrite: Failure while appending to output file; "
		                "not enough bytes transferred.\n");
		error = true;
	} else if (LOG_INFO) {
		LOG_LOCATION();
		printf("Appended %zu byte(s) to %s.\n", written_text_length, dest);
	}

	// Flush streams to file and close it
	int tmperr = fclose(file_ptr);
	if (raise_perror(tmperr, "fclose", "Failed to close the output file."))
		return true;

	if (LOG_DEBUG)
		LOG_STRING("Output file closed.");
	
	return error;
}
