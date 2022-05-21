#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "utf8.h"

/*
 * Count the number of "characters" in string, where any "character" can be
 * from 1 to 4 bytes long.
 */
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

/*
 * Determine the byte length of a (multibyte) character (by analysing its first byte).
 */
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

/*
 * Check for incomplete multibyte characters in input string. If found, terminate the string
 * instead of letting them (and any following text) through.
 */
bool utf8_terminate_incomplete_buffer(char *str, size_t len)
{
	size_t check_start = 0;

	if (len > 3)
		check_start = len - 3;

	for (size_t i = check_start; i < len; i++) {
		size_t needed_bytes = utf8_codepoint_length(str[i]);

		if (i + needed_bytes > len) {
			str[i] = '\0';
			return true;
		}
	}

	return false;
}
