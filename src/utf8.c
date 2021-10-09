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

int utf8_codepoint_length(const char s) {
	if((s & 0xF8) == 0xF0) {
		return 4;
	} else if((s & 0xF0) == 0xE0) {
		return 3;
	} else if((s & 0xE0) == 0xC0) {
		return 2;
	} else
		return 1;
}
