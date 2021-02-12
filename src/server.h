int copris_listen(int *parentfd, int portno);
int copris_read(int *parentfd, int daemon, attrib *destination, attrib *trfile, int printerset,
                int limitnum, int limit_cutoff);
int copris_stdin(attrib *destination, attrib *trfile, int printerset);
int copris_send(unsigned char *buffer, int buffer_size, attrib **destination,
                int printerset, attrib **trfile);
