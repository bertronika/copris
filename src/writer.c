/*
 * Output writing interfaces
 *
 * Copyright (C) 2020-2024 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files 'main.c' and 'COPYING' for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <utstring.h> /* uthash library - dynamic strings */

#include "debug.h"
#include "writer.h"

int copris_write_file(const char *output_file, UT_string *copris_text)
{
	// Open output file, set for (a)ppending text to it
	FILE *file_ptr = fopen(output_file, "a");
	if (file_ptr == NULL) {
		PRINT_SYSTEM_ERROR("fopen", "Failed to open output file '%s'.", output_file);
		return -1;
	}
		
	if (LOG_DEBUG)
		PRINT_MSG("Output file '%s' opened.", output_file);

	int error = 0;
	size_t text_length = utstring_len(copris_text);
	size_t written_text_length = fwrite(utstring_body(copris_text), 1, text_length, file_ptr);

	if (written_text_length < text_length) {
		PRINT_ERROR_MSG("fwrite: Failure while appending to output file; "
		                "not enough bytes transferred.");
		error = -1;
	} else if (LOG_INFO) {
		PRINT_MSG("Appended %zu byte(s) to %s.", written_text_length, output_file);
	}

	// Flush streams to file and close it
	int tmperr = fclose(file_ptr);
	if (tmperr != 0) {
		PRINT_SYSTEM_ERROR("fclose", "Failed to close output file '%s'.", output_file);
		return -1;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Output file '%s' closed.", output_file);
	
	return error;
}

int copris_write_stdout(UT_string *copris_text)
{
	int error = 0;
	size_t text_length = utstring_len(copris_text);

	if (LOG_ERROR)
		puts("; BST"); // Begin-Stream-Transcript

	size_t written_text_length = fwrite(utstring_body(copris_text), 1, text_length, stdout);

	if (LOG_ERROR)
		puts("; EST"); // End-Stream-Transcript

	if (written_text_length < text_length) {
		PRINT_ERROR_MSG("fwrite: Failure while writing to stdout; "
		                "not enough bytes transferred.");
		error = -1;
	} else if (LOG_INFO) {
		PRINT_MSG("Appended %zu byte(s) to stdout.", written_text_length);
	}

	return error;
}
