/* Header for the server.c stream socket server */

typedef struct server {
	// File descriptor of the parent socket,
	// waiting for clients.
	int parentfd;
} server_t;

extern const int BUFSIZE;
extern const int BACKLOG;

int copris_listen(server_t *server, int portno);
int copris_read(server_t *server, char *destination, int trfile_set);
