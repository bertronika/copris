#ifndef SOCKET_H
#define SOCKET_H

/*
 * Round backlog to a power of 2.
 * https://stackoverflow.com/a/5111841
 */
#define BACKLOG 2

/*
 * Create a system socket on port number `portno' and set a file descriptor,
 * passed by `parentfd'.
 * Return 0 on success.
 */
int copris_socket_listen(int *parentfd, unsigned int portno);

/*
 * Accept incoming connections from a socket, whose endpoint is passed by a file
 * descriptor `parentfd'. Read and process incoming text to `copris_text' using
 * program's global attributes `attrib'.
 * Return 0 on success.
 */
int copris_handle_socket(UT_string *copris_text, int *parentfd, struct Attribs *attrib);

#endif /* SOCKET_H */
