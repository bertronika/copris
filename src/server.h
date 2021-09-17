#ifndef SERVER_H
#define SERVER_H

int copris_listen(int *parentfd, int portno);
int copris_read(int *parentfd, int daemon, int limitnum, int limit_cutoff,
                struct attrib *trfile, int printerset, struct attrib *destination);
int copris_stdin(struct attrib *trfile, int printerset, struct attrib *destination);
int copris_send(unsigned char *buffer, int buffer_size,
                struct attrib **trfile, int printerset, struct attrib **destination);

#endif /* SERVER_H */
