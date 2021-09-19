#ifndef SERVER_H
#define SERVER_H

int copris_socket_listen(int *parentfd, int portno);
int copris_read_socket(int *parentfd, struct Attribs *attrib);
int copris_read_stdin(struct Attribs *attrib);
int copris_process(unsigned char *buffer, int buffer_size, struct Attribs *attrib);

#endif /* SERVER_H */
