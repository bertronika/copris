/*
 * Binary tokeniser
 *
 * Copyright (C) 2024 Nejc Bertoncelj <bertronika at mailo.com>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files 'main.c' and 'COPYING' for more details.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <utstring.h> /* uthash library - dynamic strings  */

#include "tokeniser.h"

binary_token_t binary_tokeniser(UT_string *text, const char sep,
                                binary_token_t *storage, bool first_time)
{
	char *s = utstring_body(text);

	if (first_time) {
		storage->length = utstring_len(text);
		storage->data = s;
	} else {
		s = storage->data;
	}

	assert(s);

	// Find the first occurence of the separator. Return NULL if there's none.
	const char *separator = memchr(s, sep, storage->length);

	// Calculate the length of the token, including one trailing whitespace (+ 1);
	// except if it is the last one.
	size_t token_len = separator ? (size_t)(separator - s + 1) : storage->length;

	if (token_len == 0)
		return (binary_token_t){ .data = NULL };

	storage->data += token_len;
	storage->length -= token_len;

	return (binary_token_t){ .data = s, .length = token_len, .last = (separator == NULL) };
}
