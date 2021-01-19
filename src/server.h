extern const int BUFSIZE;

int copris_listen(int *parentfd, int portno);
int copris_read(int *parentfd, char *destination, int daemon, int trfile, int printerset,
                int limitnum, int limit_cutoff);
int copris_stdin(char *destination, int trfile, int printerset);
int copris_send(unsigned char *buffer, int buffer_size, char *destination,
                int printerset, int trfile);
