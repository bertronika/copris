extern const int BUFSIZE;
extern const int BACKLOG;

int copris_listen(int *parentfd, int portno);
int copris_read(int *parentfd, char *destination, int trfile, int printerset);
