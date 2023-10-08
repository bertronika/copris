/*
 * Create a system socket on port number 'portno' and set a file descriptor,
 * passed by 'parentfd'.
 * Return 0 on success.
 */
int copris_socket_listen(int *parentfd, unsigned int portno);

/*
 * Accept incoming connections from a socket, whose endpoint is passed by a file
 * descriptor 'parentfd'. Read and process incoming text to 'copris_text' using
 * program's global attributes 'attrib'.
 * Return 0 on success.
 */
int copris_handle_socket(UT_string *copris_text, int *parentfd, struct Attribs *attrib);

/*
 * Send 'message', prepended by 'copris: ', to socket, pointed to by 'childfd'.
 * Return a (always) positive number on success, negative on error.
 */
int send_to_socket(int childfd, const char *message);
