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
	if (file_ptr == NULL) {
		PRINT_SYSTEM_ERROR("fopen", "Failed to open output file.");
		return true;
	}
		
	if (LOG_DEBUG)
		PRINT_MSG("Output file opened.");

	size_t written_text_length = fwrite(utstring_body(copris_text), 1, text_length, file_ptr);

	if (written_text_length < text_length) {
		PRINT_ERROR_MSG("fwrite: Failure while appending to output file; "
		                "not enough bytes transferred.");
		error = true;
	} else if (LOG_INFO) {
		PRINT_MSG("Appended %zu byte(s) to %s.", written_text_length, dest);
	}

	// Flush streams to file and close it
	int tmperr = fclose(file_ptr);
	if (tmperr != 0) {
		PRINT_SYSTEM_ERROR("fclose", "Failed to close the output file.");
		return true;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Output file closed.");
	
	return error;
}
