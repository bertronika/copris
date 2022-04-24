/*
 * Functions for reading text from the standard input
 *
 * Copyright (C) 2020-2022 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <utstring.h>

#include "Copris.h"
#include "config.h"
#include "debug.h"

static bool read_from_stdin(UT_string *copris_text, struct Stats *stats);

bool copris_handle_stdin(UT_string *copris_text) {
	if (LOG_INFO)
		LOG_STRING("Trying to read from stdin...");

	// Check if Copris is invoked standalone, outside of a pipe. That is usually
	// unwanted, since the user has specified reading from stdin, and the only
	// remaining way to enter text is to type it in interactively.
	errno = 0;
	if (LOG_ERROR && isatty(STDIN_FILENO)) {
		raise_errno_perror(errno, "isatty", "Error determining input terminal.");
		if (LOG_INFO)
			LOG_LOCATION();
		else
			printf("Note: ");

		printf("You are in text input mode (reading from "
		       "stdin). To stop reading, press Ctrl+D.\n");
	}

	// Read text from standard input, print a note if only EOF has been received
	struct Stats stats = STATS_INIT;
	bool no_text_read = read_from_stdin(copris_text, &stats);

	if (no_text_read)
		LOG_STRING("Note: no text has been read.");

	if(LOG_ERROR)
		printf("End of stream, received %zu byte(s) in %u chunk(s).\n", stats.sum, stats.chunks);

	// Return true if no text has been read
	return no_text_read;
}

static bool read_from_stdin(UT_string *copris_text, struct Stats *stats) {
	char buffer[BUFSIZE];
	size_t buffer_length = 0;

	// fgets() terminates on error or at EOF (triggered with Ctrl-D),
	// reads up to BUFSIZE bytes of text and terminates it.
	while (fgets(buffer, BUFSIZE, stdin) != NULL) {
		buffer_length = strlen(buffer);

		// Note that strlen() doesn't count the ending null byte. This isn't
		// a problem, since utstring_bincpy() terminates its internal string
		// after appending to it.
		utstring_bincpy(copris_text, buffer, buffer_length);

		stats->chunks++;
		stats->sum += buffer_length; // TODO - possible overflow?
	}

	// Return true if no text has been read
	return (buffer_length == 0);
}
