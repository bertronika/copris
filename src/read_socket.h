#ifndef SOCKET_H
#define SOCKET_H

#include <stdbool.h>  /* bool      */
#include <utstring.h> /* UT_string */

/*
 * Round backlog to a power of 2.
 * https://stackoverflow.com/a/5111841
 */
#define BACKLOG 2

bool copris_socket_listen(int *parentfd, unsigned int portno);
bool copris_handle_socket(UT_string *copris_text, int *parentfd, struct Attribs *attrib);

#endif /* SOCKET_H */
