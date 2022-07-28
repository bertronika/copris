/*
 * Helper functions for parsing UTF-8 encoded strings
 *
 * Copyright (C) 2021-2022 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <stdint.h>

#include "utf8.h"

size_t utf8_count_codepoints(const char *s, size_t n)
{
	size_t count = 0;

	while (*s && count <= n) {
		// Count every character, except Unicode continuation bytes
		if (!UTF8_IS_CONTINUATION(*s++))
			count++;
	}

	return count;
}

size_t utf8_codepoint_length(const char s)
{
	if ((s & 0xF8) == 0xF0)
		return 4;
	else if ((s & 0xF0) == 0xE0)
		return 3;
	else if ((s & 0xE0) == 0xC0)
		return 2;

	// Only ASCII characters left
	return 1;
}

int utf8_terminate_incomplete_buffer(char *str, size_t len)
{
	size_t check_start = 0;

	if (len > 3)
		check_start = len - 3;

	for (size_t i = check_start; i < len; i++) {
		size_t needed_bytes = utf8_codepoint_length(str[i]);

		if (i + needed_bytes > len) {
			str[i] = '\0';
			return -1;
		}
	}

	return 0;
}
