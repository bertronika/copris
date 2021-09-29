#ifndef SERVER_H
#define SERVER_H

int copris_socket_listen(int *parentfd, unsigned int portno);
int copris_read_socket(int *parentfd, struct Attribs *attrib);
int copris_read_stdin(struct Attribs *attrib);
int copris_process(unsigned char *text, int text_length, struct Attribs *attrib);

#endif /* SERVER_H */
