#include <stdio.h>
#include <stdint.h>

#include "utf8.h"

size_t utf8_count_codepoints(const char *s, size_t n) {
	size_t count = 0;

	while(*s && count <= n) {
		// Count every character, except Unicode continuation bytes
		if(!UTF8_IS_CONTINUATION(*s++))
			count++;
	}

	return count;
}

size_t utf8_codepoint_length(const char s) {
	if((s & 0xF8) == 0xF0) {
		return 4;
	} else if((s & 0xF0) == 0xE0) {
		return 3;
	} else if((s & 0xE0) == 0xC0) {
		return 2;
	} else
		return 1;
}

/*
 * Return number of remaining bytes to be read into the input string for a complete multibyte
 * character, if one is found incomplete at the end
 */
size_t utf8_calculate_needed_bytes(const char *str, size_t len)
{
	// At most 3 characters can be missing from the string, as an UTF-8 encoded character
	// can be up to 4 bytes long
	for (size_t i = len - 3; i < len; i++) {
		// Subtract 1; we are interested in multibyte characters
		size_t needed_bytes = utf8_codepoint_length(str[i]) - 1;

		if (i + needed_bytes > len)
			return needed_bytes;
	}

	return 0;
}
