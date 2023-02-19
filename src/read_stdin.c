/*
 * Functions for reading text from the standard input
 *
 * Copyright (C) 2020-2023 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <unistd.h>

#include <utstring.h> /* uthash library - dynamic strings */

#include "Copris.h"
#include "config.h"
#include "debug.h"
#include "read_stdin.h"

static size_t read_from_stdin(UT_string *, struct Stats *);

int copris_handle_stdin(UT_string *copris_text) {
	if (LOG_INFO)
		PRINT_MSG("Trying to read from stdin...");

	// Check if Copris is invoked standalone, outside of a pipe. That is usually
	// unwanted, since the user has specified reading from stdin, and the only
	// remaining way to enter text is to type it in interactively.
	if (isatty(STDIN_FILENO))
		PRINT_NOTE("You are in text input mode (reading from " /* LCOV_EXCL_LINE */
		           "stdin). To stop reading, press Ctrl+D.");

	// Read text from standard input, print a note if only EOF has been received
	struct Stats stats = STATS_INIT;
	size_t text_length = read_from_stdin(copris_text, &stats);

	if (text_length == 0)
		PRINT_NOTE("No text has been read!");

	if (LOG_ERROR)
		PRINT_MSG("Received %zu byte(s) in %d chunk(s) from stdin.", stats.sum, stats.chunks);

	// Return -1 if no text has been read
	return (text_length) ? 0 : -1;
}

static size_t read_from_stdin(UT_string *copris_text, struct Stats *stats) {
	char buffer[BUFSIZE];
	size_t buffer_length = 0;

	// fgets() stops on error or at EOF (triggered with Ctrl-D),
	// reads up to BUFSIZE - 1 bytes of text and terminates it.
	while (fgets(buffer, BUFSIZE, stdin) != NULL) {
		buffer_length = strlen(buffer);

		// Note that strlen() doesn't count the ending null byte. This isn't
		// a problem, since utstring_bincpy() terminates its internal string
		// after appending to it.
		utstring_bincpy(copris_text, buffer, buffer_length);

		stats->chunks++;
		stats->sum += buffer_length; // TODO - possible overflow?
	}

	return buffer_length;
}
