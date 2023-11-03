/*
 * Helper functions for various actions in main()
 *
 * Copyright (C) 2023 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files 'main.c' and 'COPYING' for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <utstring.h> /* uthash library - dynamic strings */

#include "Copris.h"
#include "debug.h"
#include "writer.h"
#include "main-helpers.h"

void append_file_name(char *filename, char **filenames, int count)
{
	size_t filename_len = strlen(filename);

	filenames[count] = malloc(filename_len + 1);
	CHECK_MALLOC(filenames[count]);

	memcpy(filenames[count], filename, filename_len + 1);
}

void free_filenames(char **filenames, int count)
{
	while (count--) {
		free(filenames[count]);
	}
}

void write_to_output(UT_string *copris_text, struct Attribs *attrib)
{
	if (attrib->copris_flags & HAS_OUTPUT_FILE) {
		copris_write_file(attrib->output_file, copris_text);
	} else {
		if (LOG_ERROR)
			puts("; BST"); // Begin-Stream-Transcript

		fputs(utstring_body(copris_text), stdout);

		if (LOG_ERROR)
			puts("; EST"); // End-Stream-Transcript
	}
}
