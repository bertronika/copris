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
#include <sys/socket.h>

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

int __real_accept(int sockfd, struct sockaddr *addr, socklen_t addrlen);
int __wrap_accept(int sockfd, struct sockaddr *addr, socklen_t addrlen)
{
	(void)sockfd;
	(void)addr;
	(void)addrlen;

	return 7;
}

int __real_close(int fd);
int __wrap_close(int fd)
{
	(void)fd;

	return 0;
}

int __real_getnameinfo(void);
int __wrap_getnameinfo(void)
{
	return 0;
}

static char inet_buffer[18]; /* copied straight from glibc/inet/inet_ntoa.c */

char *__real_inet_ntoa(void);
char *__wrap_inet_ntoa(void)
{
	memccpy(inet_buffer, "127.0.0.1", '\0', 18);

	return inet_buffer;
}

ssize_t __real_read(int fd, void *buffer, size_t count);
ssize_t __wrap_read(int fd, void *buffer, size_t count)
{
	(void)fd;

	char *read_data = mock_type(char *);

	if (read_data == NULL)
		return 0;

	size_t data_len = mock_type(size_t);

	// Check if the test value fits into the buffer
	assert_in_range(data_len, 0, count);

	memcpy(buffer, read_data, data_len);

	return data_len;
}
