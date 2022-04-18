/*
 * server.c
 * Stream socket server with trfile and prset hooks
 * 
 * Copyright (C) 2020-2021 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <assert.h>

#include "Copris.h"
#include "config.h"
#include "debug.h"
#include "server.h"
#include "writer.h"
#include "translate.h"
#include "printerset.h"
#include "utf8.h"

const struct Stats STATS_INIT = {
	0, 0, false, 0
};
static int read_from_socket(int childfd, struct Stats *stats, struct Attribs *attrib);
static int read_from_stdin(struct Stats *stats, struct Attribs *attrib);

int copris_socket_listen(int *parentfd, unsigned int portno) {
	int fderror;  // Return value of a socket operation

	/*
	 * Create a system socket using the following:
	 *   AF_INET      IPv4
	 *   SOCK_STREAM  TCP protocol
	 *   IPPROTO_IP   IP protocol
	 */
	fderror = (*parentfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP));
	if (raise_perror(fderror, "socket", "Failed to create socket endpoint."))
		return 1;

	if (LOG_DEBUG)
		LOG_STRING("Socket endpoint created.");

	/*
	 * A hack from tcpserver.c:87
	 * setsockopt: Handy debugging trick that lets us rerun the server
	 * immediately after we kill it; otherwise we have to wait about 20 secs.
	 * Eliminates "ERROR on binding: Address already in use" error.
	 */
	int optval = 1;
	setsockopt(*parentfd, SOL_SOCKET, SO_REUSEADDR,
	           (const void *)&optval, sizeof(int));

// 	memset((char *)&serveraddr, '\0', sizeof(serveraddr)); // TODO: is this necessary?

	struct sockaddr_in serveraddr; // Server's own address
	serveraddr.sin_family      = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port        = htons((unsigned short)portno);
	
	// Associate the parent socket with a port
	fderror = bind(*parentfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (raise_perror(fderror, "bind", "Failed to bind socket to address. "
	                                  "Non-root users should set it >1023."))
		return 1;

	if (LOG_DEBUG)
		LOG_STRING("Socket bound to address.");

	// Make the parent socket passive - accept incoming connections.
	// Limit number of connections to the value of BACKLOG.
	fderror = listen(*parentfd, BACKLOG);
	if (raise_perror(fderror, "listen", "Failed to make socket passive."))
		return 1;

	if (LOG_INFO) {
		LOG_LOCATION();
		if (LOG_DEBUG) {
			printf("Socket made passive. ");
		}
		printf("Now we listen...\n");
	}

	return 0;
}

int copris_handle_socket(int *parentfd, struct Attribs *attrib, struct Trfile **trfile) {
	int fderror;                    // Return value of a socket operation
	struct sockaddr_in clientaddr;  // Client's address
	socklen_t clientlen;            // (Byte) size of client's address (sockaddr)
	clientlen = sizeof(clientaddr);

	// Wait for a connection request, accept it and pass it on as a child socket
	int childfd = accept(*parentfd, (struct sockaddr *)&clientaddr, &clientlen);
	if (raise_perror(childfd, "accept", "Failed to accept the connection."))
		return 1;

	if (LOG_DEBUG)
		LOG_STRING("Connection to socket accepted.");

	// Prevent more than one connection if not a daemon
	if (!attrib->daemon) {
		fderror = close(*parentfd);
		if (raise_perror(fderror, "close", "Failed to close the parent connection."))
			return 1;
	}

	// Get host info (IP, hostname) of the client (TODO gai_strerror)
	char host_info[NI_MAXHOST];
	fderror = getnameinfo((struct sockaddr *)&clientaddr, sizeof(clientaddr),
	                      host_info, sizeof(host_info), NULL, 0, 0);
// 	if(fderror != 0) {
// 		log_perr(-1, "getnameinfo", "Failed getting hostname from address.");
// 	}

	// Convert client's address from network byte order to a dotted-decimal form
	char *host_address = inet_ntoa(clientaddr.sin_addr);
	if (host_address == NULL) {
		fprintf(stderr, "inet_ntoa: Failed converting host's address to dotted decimal.\n");
		host_address = "<addr unknown>";
	}

	if (LOG_INFO)
		LOG_LOCATION();

	if (LOG_ERROR) {
		printf("Inbound connection from %s (%s).\n", host_info, host_address);

		// Print a Beginning-Of-Stream marker if output isn't a file
		if (!(attrib->copris_flags & HAS_DESTINATION))
			printf("; BOS\n");
	}

	// Read text from socket and process it
	struct Stats stats = STATS_INIT;
	int read_error = read_from_socket(childfd, &stats, attrib);
	if (read_error)
		return read_error;

	// Close the current connection
	fderror = close(childfd);
	if (raise_perror(fderror, "close", "Failed to close the child connection."))
		return 1;

	// Print a End-Of-Stream marker if output isn't a file
	if (LOG_ERROR && !(attrib->copris_flags & HAS_DESTINATION))
		printf("; EOS\n");

	if (LOG_INFO)
		LOG_LOCATION();

	if (LOG_ERROR) {
		printf("End of stream, received %zu byte(s) in %d chunk(s)",
			   stats.sum, stats.chunks);

		if (stats.size_limit_active) {
			printf(", %zu byte(s) %s.\n", stats.discarded,
			       (attrib->copris_flags & MUST_CUTOFF) ? "cut off" : "discarded");
		} else {
			printf(".\n");
		}
	}

	if (LOG_INFO) {
		LOG_LOCATION();
		printf("Connection from %s (%s) closed.\n", host_info, host_address);
	}

	return 0;
}

static int read_from_socket(int childfd, struct Stats *stats, struct Attribs *attrib) {
	int fderror;            // Return value of a socket operation
	char buf[BUFSIZE + 3];  // Inbound message buffer

	// Read the data sent by the client into the buffer
	// A terminating null byte is _not_ stored at the end!
	while ((fderror = read(childfd, buf, BUFSIZE - 1)) > 0) {
		buf[fderror] = '\0';
		stats->chunks++;

		// Check if additional bytes need to be read (to complete a multibyte character)
		size_t needed_bytes = utf8_calculate_needed_bytes(buf, fderror);
		size_t additional   = 0;

		if (needed_bytes > 0) {
			char buf2[4]; // Maximum of 3 remaining bytes + ending null
			assert(needed_bytes < 4);

			additional = read(childfd, buf2, needed_bytes);
			buf2[needed_bytes] = '\0';
			stats->chunks++;

			// If a read error occures
			if (raise_perror(additional, "read", "Error reading from socket."))
				return 1;

			// Concatenate both buffers to a maximum of BUFSIZE + 2 bytes (leaving space
			// for null termination)
			assert(strlen(buf) + strlen(buf2) < BUFSIZE + 3);
			strcat(buf, buf2);
		}

		stats->sum += fderror + additional;

		// Check if length of received text went over the limit (if limit is active)
		if (attrib->limitnum && stats->sum > attrib->limitnum) {
			const char limit_message[] = "Send size limit exceeded, terminating connection.\n";
			int tmperr = write(childfd, limit_message, (sizeof limit_message) - 1);
			raise_perror(tmperr, "write", "Error sending termination text to socket.");
			stats->size_limit_active = true;
		}

		// Terminate before processing excessive data - discard whole chunk
		if (stats->size_limit_active && !(attrib->copris_flags & MUST_CUTOFF)) {
			stats->discarded = fderror + additional;
			if (log_error())
				printf("Client exceeded send size limit (%zu B/%zu B), discarding remaining "
				       "data and terminating connection.\n", stats->sum, attrib->limitnum);

			break;
		}

		// Terminate after processing excessive data - cut off buffer at limit
		if (stats->size_limit_active && attrib->copris_flags & MUST_CUTOFF) {
			if (stats->sum == fderror + additional) {
				// First buffer read already bigger than limit
				buf[attrib->limitnum] = '\0';
			} else {
				buf[attrib->limitnum - (stats->sum - fderror - additional)] = '\0';
			}
		}

// 		copris_process(buf, fderror + additional, attrib, trfile); TODO
		copris_process(buf, fderror + additional, attrib, NULL);

		if (stats->size_limit_active && attrib->copris_flags & MUST_CUTOFF) {
			stats->discarded = stats->sum - attrib->limitnum;
			if (LOG_ERROR)
				printf("\nClient exceeded send size limit (%zu B/%zu B), cutting off and "
				       "terminating connection.\n", stats->sum, attrib->limitnum);

			break;
		}
	} /* end of socket reading loop */

	if (raise_perror(fderror, "read", "Error reading from socket."))
		return 1;

	return 0;
}

int copris_handle_stdin(struct Attribs *attrib, struct Trfile **trfile) {
	if (LOG_INFO) {
		LOG_STRING("Trying to read from stdin...");
	}

	errno = 0;
	if (log_error() && isatty(STDIN_FILENO)) {
		raise_errno_perror(errno, "isatty", "Error determining input terminal.");
		if (LOG_INFO) {
			LOG_LOCATION();
		} else {
			printf("Note: ");
		}

		printf("You are in text input mode (reading from "
		       "stdin). To stop reading, press Ctrl+D\n");
	}

	// Print a Beginning-Of-Stream marker if output isn't a file
	if(LOG_ERROR && !(attrib->copris_flags & HAS_DESTINATION))
		printf("; BOS\n");

	// Read text from standard input and process it
	struct Stats stats = STATS_INIT;
	int read_error = read_from_stdin(&stats, attrib);
	if (read_error)
		return read_error;

	// Print a End-Of-Stream marker if output isn't a file
	if(LOG_ERROR && !(attrib->copris_flags & HAS_DESTINATION))
		printf("; EOS\n");

	if(LOG_ERROR)
		printf("End of stream, received %zu byte(s) in %u chunk(s).\n", stats.sum, stats.chunks);

	return 0;
}

static int read_from_stdin(struct Stats *stats, struct Attribs *attrib) {
	char buf[BUFSIZE + 3]; // Inbound message buffer

	/*
	 * Read data from the standard input into the buffer. Exit on error or at EOF.
	 * fgets() stores a terminating null byte at the end of the buffer.
	 * Space for 3 additional bytes is reserved in the buffer for a multibyte character,
	 * which, when encoded in UTF-8, needs at most 4 bytes. If we detect such character
	 * at the buffer's end, we request an additional read to make the character complete.
	*/
	while (fgets(buf, BUFSIZE, stdin) != NULL) {
		stats->chunks++;

		size_t buffer_length = strlen(buf);
		size_t needed_bytes  = utf8_calculate_needed_bytes(buf, buffer_length);

		if (needed_bytes > 0) {
			char buf2[4]; // Maximum of 3 remaining bytes + ending null
			assert(needed_bytes < 4);

			char *error = fgets(buf2, needed_bytes + 1, stdin);
			stats->chunks++;

			// If a read error occures
			if (error == NULL)
				return 1;

			// Concatenate both buffers to a maximum of BUFSIZE + 2 bytes (leaving space
			// for null termination)
			assert(strlen(buf) + strlen(buf2) < BUFSIZE + 3);
			strcat(buf, buf2);
		}

// 		copris_process(buf, buffer_length + needed_bytes + 1, attrib, trfile); TODO
		copris_process(buf, buffer_length + needed_bytes + 1, attrib, NULL);
		stats->sum += buffer_length + needed_bytes; // TODO when it overflows...
	}

	return 0;
}

int copris_process(char *stream, int stream_length, struct Attribs *attrib, struct Trfile **trfile) {
// 	if(attrib->copris_flags & HAS_TRFILE) {
		// copris_translate() makes a copy of final_stream and returns its
		// newly allocated position - which should be free'd.
// 		final_stream = copris_translate(final_stream, stream_length, trfile);
// 	}

// 	if(attrib->copris_flags & HAS_PRSET) {
// 		copris_printerset();
// 	}

	(void)stream_length;
	(void)attrib;
	(void)trfile;

	// Destination can be either a file/printer or stdout
	if(attrib->copris_flags & HAS_DESTINATION) {
		copris_write_file(attrib->destination, stream);
	} else {
// 		fputs(stream, stdout);
// 		fflush(stdout);
		printf("OUT: \"%s\"\n", stream);
	}

// 	if(attrib->copris_flags & (HAS_TRFILE/*|HAS_PRSET*/))
// 		free(final_stream);

	return 0;
}
int copris_process1(char *stream, int stream_length, struct Attribs *attrib, struct Trfile **trfile) {
	char *final_stream = stream;

	// To avoid splitting multibyte characters:
	// If the buffer has been filled to the limit, check codepoint length of the
	// last 3 characters. If their length exceeds buffer size, stash them into a
	// hold buffer, replace them with null bytes and return the number of missing
	// characters, which should be additionally read from the input stream.
	// After the missing bytes are read, an extra buffer is assembled from the hold
	// buffer and the new stream, containing missing bytes.
	static char hold_buffer[UTF8_MAX_LENGTH + 1];
	static int is_on_hold = 0;
	char extra_buffer[UTF8_MAX_LENGTH + 1];

	if(is_on_hold) {
		extra_buffer[0] = '\0';

		// Merge the two buffers
		strcpy(extra_buffer, hold_buffer);
		strcat(extra_buffer, stream);

		final_stream = extra_buffer;
		is_on_hold = 0;
	}

	// strlen() omits ending null byte
	if(stream_length + 1 == BUFSIZE) {
		int hold_i = 0;

		for(int source_i = 4; source_i > 1; source_i--) {
			// How many bytes are needed for a complete codepoint?
			int remaining = utf8_codepoint_length(stream[BUFSIZE - source_i]);

			if(remaining == 1)
				continue;

			// More than the buffer can hold, stash them for a subsequent read
			if(remaining >= source_i) {
				hold_buffer[hold_i] = stream[BUFSIZE - source_i];
				hold_i++;

				stream[BUFSIZE - source_i] = '\0';
				is_on_hold = remaining;
			}
		}
		hold_buffer[hold_i] = '\0';
	}

	if(attrib->copris_flags & HAS_TRFILE) {
		// copris_translate() makes a copy of final_stream and returns its
		// newly allocated position - which should be free'd.
		final_stream = copris_translate(final_stream, stream_length, trfile);
	}

	if(attrib->copris_flags & HAS_PRSET) {
// 		copris_printerset();
	}

	// Destination can be either a file/printer or stdout
	if(attrib->copris_flags & HAS_DESTINATION) {
		copris_write_file(attrib->destination, final_stream);
	} else {
		fputs(final_stream, stdout);
	}

	if(attrib->copris_flags & (HAS_TRFILE/*|HAS_PRSET*/))
		free(final_stream);

	return is_on_hold;
}
