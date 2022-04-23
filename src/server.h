#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>  /* size_t */
#include <stdbool.h> /* bool */

struct Stats {
	int chunks;             // Number of read chunks
	size_t sum;             // Sum of all read (received) bytes
	bool size_limit_active;
	size_t discarded;       // Discarded number of bytes, if limit is set
};

int copris_socket_listen(int *parentfd, unsigned int portno);
int copris_handle_socket(int *parentfd, struct Attribs *attrib);
int copris_handle_stdin(struct Attribs *attrib);
int copris_process(char *stream, int stream_length, struct Attribs *attrib);

#endif /* SERVER_H */
