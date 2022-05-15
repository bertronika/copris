/*
 * Wrapper functions for emulating system calls.
 * These are wrapped via a linker option, passed through by the compiler
 * (eg. -Wl,--wrap=fgets). The cmocka framework is then used to simulate
 * their behaviour.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>
#include <string.h>

#include "../src/config.h"

char *__real_fgets(char *buffer, int buffer_size, FILE *stream);
char *__wrap_fgets(char *buffer, int buffer_size, FILE *stream)
{
	(void)stream;

	char *read_data = mock_type(char *);

	if (read_data == NULL)
		return NULL;

	// Check if the test value fits into the buffer
	assert_in_range(strlen(read_data), 0, buffer_size - 1);

	memccpy(buffer, read_data, '\0', buffer_size);

	return buffer;
}

int __real_isatty(int fd);
int __wrap_isatty(int fd) {
	(void)fd;

	// Pretend we're always running non-interactively
	return 0;
}
