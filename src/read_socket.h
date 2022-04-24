#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>   /* size_t    */
#include <stdbool.h>  /* bool      */
#include <utstring.h> /* UT_string */

int copris_socket_listen(int *parentfd, unsigned int portno);
bool copris_handle_socket(UT_string *copris_text, int *parentfd, struct Attribs *attrib);
int copris_process(char *stream, int stream_length, struct Attribs *attrib);

#endif /* SERVER_H */
