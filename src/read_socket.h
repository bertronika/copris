#ifndef SOCKET_H
#define SOCKET_H

#include <stdbool.h>  /* bool      */
#include <utstring.h> /* UT_string */

bool copris_socket_listen(int *parentfd, unsigned int portno);
bool copris_handle_socket(UT_string *copris_text, int *parentfd, struct Attribs *attrib);

#endif /* SOCKET_H */
