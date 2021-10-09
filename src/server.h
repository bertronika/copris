#ifndef SERVER_H
#define SERVER_H

int copris_socket_listen(int *parentfd, unsigned int portno);
int copris_read_socket(int *parentfd, struct Attribs *attrib, struct Trfile **trfile);
int copris_read_stdin(struct Attribs *attrib, struct Trfile **trfile);
int copris_process(char *stream, int stream_length, struct Attribs *attrib, struct Trfile **trfile);

#endif /* SERVER_H */
