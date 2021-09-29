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
