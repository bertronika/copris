/*
 * Text filters
 *
 * Copyright (C) 2023 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <ctype.h>
#include <utstring.h> /* uthash library - dynamic strings */
#include <assert.h>

#include "debug.h"
#include "utstring_cut.h"
#include "filters.h"

void filter_non_ascii(UT_string *copris_text)
{
	char *text = utstring_body(copris_text);
	size_t orig_len = utstring_len(copris_text);
	char *p = text; // Text pointer
	size_t i = 0;   // Filtered string length

	while (*text) {
		// If an ASCII printable character or a white-space character, copy in-place
		if (isgraph(*text) || isspace(*text)) {
			*p = *text;
			p++;
			i++;
		}
		text++;
	}

	// Terminate string on the new position (always shorter or equal to the original length)
	utstring_cut(copris_text, i);

	assert(i <= orig_len);

	if (LOG_DEBUG)
		PRINT_MSG("Non-ASCII filter: number of bytes removed: %zu", orig_len - i);
}
