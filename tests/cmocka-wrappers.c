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

// Tests expect the following buffer size:
#if BUFSIZE != 10
#   error "Buffer size set improperly for unit tests."
#endif

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

size_t __real_fread(void *ptr, size_t size, size_t nmemb, FILE* stream);
size_t __wrap_fread(void *ptr, size_t size, size_t nmemb, FILE* stream)
{
	(void)stream;

	// Should always receive 1 byte at a time
	assert_int_equal(size, 1);

	char *read_data = mock_ptr_type(char *);
	size_t data_size = 0;

	// NULL signals an EOF
	if (read_data == NULL) {
		return 0;
	} else {
		data_size = mock_type(size_t);
	}

	// Check if test value fits into the buffer
	assert_in_range(data_size, 0, nmemb);

	memcpy(ptr, read_data, data_size);

	return data_size;
}

ssize_t __real_read(int fd, void *buffer, size_t count);
ssize_t __wrap_read(int fd, void *buffer, size_t count)
{
	(void)fd;

	char *read_data = mock_ptr_type(char *);
	size_t data_size = 0;

	// NULL signals an EOF
	if (read_data == NULL) {
		return 0;
	} else {
		data_size = mock_type(size_t);
	}

	// Check if the test value fits into the buffer (BUFSIZE)
	assert_in_range(data_size, 0, count);

	memcpy(buffer, read_data, data_size);

	return data_size;
}

FILE *__real_freopen(const char pathname, const char mode, FILE *stream);
FILE *__wrap_freopen(const char pathname, const char mode, FILE *stream)
{
	(void)pathname;
	(void)mode;
	(void)stream;

	FILE *f = stdin; // Placeholder FILE pointer that won't be NULL

	return f;
}

int __real_ferror(FILE *stream);
int __wrap_ferror(FILE *stream)
{
	(void)stream;

	// Pretend there's no error
	return 0;
}

int __real_isatty(int fd);
int __wrap_isatty(int fd)
{
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

int __real_getnameinfo(const struct sockaddr addr, socklen_t addrlen,
                       char *host, socklen_t hostlen, char serv, socklen_t servlen, int flags);
int __wrap_getnameinfo(const struct sockaddr addr, socklen_t addrlen,
                       char *host, socklen_t hostlen, char serv, socklen_t servlen, int flags)
{
	(void)addr;
	(void)addrlen;
	(void)serv;
	(void)servlen;
	(void)flags;

	// TODO Figure out how to fill this buffer
	(void)host;
	(void)hostlen;
	//memccpy(host, "localhost", '\0', hostlen);

	return 0;
}

static char inet_buffer[18]; /* copied straight from glibc/inet/inet_ntoa.c */

char *__real_inet_ntoa(void);
char *__wrap_inet_ntoa(void)
{
	memccpy(inet_buffer, "127.0.0.1", '\0', 18);

	return inet_buffer;
}

ssize_t __real_write(int fd, const void *buf, size_t count);
ssize_t __wrap_write(int fd, const void *buf, size_t count)
{
	(void)fd;
	(void)buf;

	// Pretend everything has been written successfully
	return count;
}

/*
int __real_printf(const char *format, ...);
int __wrap_printf(const char *format, ...)
{
	(void)format;

	return 1;
}

int __real_fprintf(FILE *stream, const char *format, ...);
int __wrap_fprintf(FILE *stream, const char *format, ...)
{
	(void)stream;
	(void)format;

	return 1;
}

int __real_puts(const char *s);
int __wrap_puts(const char *s)
{
	(void)s;

	return 1;
}

int __real_fputs(const char *s, FILE *stream);
int __wrap_fputs(const char *s, FILE *stream)
{
	(void)s;
	(void)stream;

	return 1;
}
*/
