extern const int BUFSIZE;

int copris_listen(int *parentfd, int portno);
int copris_read(int *parentfd, char *destination, int daemon, int trfile, int printerset,
				int limitnum, int limit_cutoff);
