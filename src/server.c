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
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <ctype.h>

#include "Copris.h"
#include "config.h"
#include "debug.h"
#include "server.h"
#include "writer.h"
#include "translate.h"
#include "printerset.h"
#include "utf8.h"

int copris_socket_listen(int *parentfd, unsigned int portno) {
	int fderror;
	struct sockaddr_in serveraddr; // Server's own address

	/*
	 * Create a system socket using the following:
	 *   AF_INET      IPv4
	 *   SOCK_STREAM  TCP protocol
	 *   IPPROTO_IP   IP protocol
	 */
	fderror = (*parentfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP));
	if(raise_perror(fderror, "socket", "Failed to create socket endpoint."))
		return 1;

	if(log_debug()) {
		log_date();
		printf("Socket endpoint created.\n");
	}

	/* 
	 * A hack from tcpserver.c: (line 87)
	 * setsockopt: Handy debugging trick that lets
	 * us rerun the server immediately after we kill it;
	 * otherwise we have to wait about 20 secs.
	 * Eliminates "ERROR on binding: Address already in use" error.
	 */
	int optval = 1;
	setsockopt(*parentfd, SOL_SOCKET, SO_REUSEADDR,
	           (const void *)&optval, sizeof(int));

// 	memset((char *)&serveraddr, '\0', sizeof(serveraddr)); // TODO: is this necessary?

	serveraddr.sin_family      = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port        = htons((unsigned short)portno);
	
	// Associate the parent socket with a port
	fderror = bind(*parentfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if(raise_perror(fderror, "bind", "Failed to bind socket to address. "
	                                 "Non-root users should set it >1023."))
		return 1;

	if(log_debug()) {
		log_date();
		printf("Socket bound to address.\n");
	}

	// Make the parent socket passive - accept incoming connections.
	// Also limit the number of connections to BACKLOG
	fderror = listen(*parentfd, BACKLOG);
	if(raise_perror(fderror, "listen", "Failed to make socket passive."))
		return 1;

	if(log_info()) {
		log_date();
		if(log_debug()) {
			printf("Socket made passive. ");
		}
		printf("Now we listen...\n");
	}

	return 0;
}

int copris_read_socket(int *parentfd, struct Attribs *attrib, struct Trfile **trfile) {
	int fderror;  // Error code of a socket operation
	int childfd;  // Child socket, which processes one client at a time

	int chunks    = 0;  // Number of read chunks
	int sum       = 0;  // Sum of all read (received) bytes
	int discarded = 0;  // Discarded number of bytes, if limit is set
	int additional;     // Number of requested bytes for a subsequent read

	struct sockaddr_in clientaddr;  // Client's address
	socklen_t clientlen;            // (Byte) size of client's address (sockaddr)
	char *host_address;             // Host address string
	char host_info[NI_MAXHOST];     // Host info (IP, hostname). NI_MAXHOST is built in
	char buf[BUFSIZE];              // Inbound message buffer
	char limit_message[] = "Send size limit exceeded, terminating connection.\n";

	clientlen = sizeof(clientaddr);

	// Wait for a connection request and accept it
	childfd = accept(*parentfd, (struct sockaddr *)&clientaddr, &clientlen);
	if(raise_perror(childfd, "accept", "Failed to accept the connection."))
		return 1;

	if(log_info()) {
		log_date();
		printf("Connection to socket accepted.\n");
	}

	// Prevent more than one connection if not a daemon
	if(!attrib->daemon) {
		fderror = close(*parentfd);
		if(raise_perror(fderror, "close", "Failed to close the parent connection."))
			return 1;
	}

	// Get the hostname of the client TODO gai_strerror
	fderror = getnameinfo((struct sockaddr *)&clientaddr, sizeof(clientaddr),
	                       host_info, sizeof(host_info), NULL, 0, 0);
// 	if(fderror != 0) {
// 		log_perr(-1, "getnameinfo", "Failed getting hostname from address.");
// 	}

	// Convert client's address from network byte order to a dotted-decimal form
	host_address = inet_ntoa(clientaddr.sin_addr);
	if(host_address == NULL) {
		fprintf(stderr, "inet_ntoa: Failed converting host's address to dotted decimal.\n");
		host_address = "<addr unknown>";
	}

	if(log_info())
		log_date();
	if(log_error()) {
		printf("Inbound connection from %s (%s).\n", host_info, host_address);
		if(!(attrib->copris_flags & HAS_DESTINATION))
			printf("; BOS\n");
	}

	// Read the data sent by the client into the buffer
	// A terminating null byte is _not_ stored at the end!
	while((fderror = read(childfd, buf, BUFSIZE - 1)) > 0) {
		chunks++;
		sum += fderror; // Append read bytes to the total byte counter

		// Byte limit handling
		if(attrib->limitnum && sum > attrib->limitnum) {
			fderror = write(childfd, limit_message, strlen(limit_message));
			raise_perror(fderror, "write", "Error sending termination text to socket.");

			// Cut-off mid-text and terminate later
			if(attrib->limit_cutoff) {
				buf[attrib->limitnum] = '\0';
				discarded = sum - attrib->limitnum;
				attrib->limit_cutoff = 2;

			// Discard whole line and terminate
			} else {
				if(log_error())
					printf("Client exceeded send size limit (%d B/%d B), discarding remaining "
					       "data and terminating connection.\n", sum, attrib->limitnum);

				discarded = fderror;
				break;
			}
		} /* end of byte limit handling */

		// Terminate the buffer after reading completed
		buf[fderror] = '\0';

		additional = copris_process(buf, fderror, attrib, trfile);
		if(additional) {
			if(read(childfd, buf, additional - 1) != additional - 1)
				return 1;

			buf[additional - 1] = '\0';

			chunks++;
			sum += additional;

			copris_process(buf, additional, attrib, trfile);
		}

		// Terminate connection if cut-off set
		if(attrib->limit_cutoff == 2) {
			if(log_error())
				printf("\nClient exceeded send size limit (%d B/%d B), cutting off and "
				       "terminating connection.\n", sum, attrib->limitnum);
			break;
		}
	} /* end of socket reading loop */

	if(raise_perror(fderror, "read", "Error reading from socket."))
		return 1;

	// Close the current connection 
	fderror = close(childfd);
	if(raise_perror(fderror, "close", "Failed to close the child connection."))
		return 1;

	if(log_error() && !(attrib->copris_flags & HAS_DESTINATION))
		printf("; EOS\n");

	if(log_info())
		log_date();
	if(log_error()) {
		printf("End of stream, received %d byte(s) in %d chunk(s)",
			   sum, chunks);

		if(discarded) {
			printf(", %d B %s.\n", discarded,
			      (attrib->limit_cutoff == 2) ? "cut off" : "discarded");
		} else {
			printf(".\n");
		}
	}

	if(log_info()) {
		log_date();
		printf("Connection from %s (%s) closed.\n", host_info, host_address);
	}

	return 0;
}

int copris_read_stdin(struct Attribs *attrib, struct Trfile **trfile) {
	int count;       // Number of read bytes in one pass (or two, if subsequent requested)
	int chunks = 0;  // Number of read chunks
	int sum    = 0;  // Sum of all read bytes
	int additional;  // Number of requested bytes for a subsequent read
	char buf[BUFSIZE]; // Inbound message buffer

	if(log_info()) {
		log_date();
		printf("Trying to read from stdin...\n");
	}

	errno = 0;
	if(isatty(STDIN_FILENO) && log_error()) {
		raise_errno_perror(errno, "isatty", "Error determining input terminal");
		if(log_info())
			log_date();
		else
			printf("Note: ");

		printf("You are in text input mode (reading from "
		       "stdin). To stop reading, press Ctrl+D\n");
	}

	if(log_error() && !(attrib->copris_flags & HAS_DESTINATION))
		printf("; BOS\n");

	// Read the data from standard input into the buffer
	// A terminating null byte _is_ stored at the end!
	// If additional bytes are requested, read them.
	while(fgets(buf, BUFSIZE, stdin) != NULL) {
		count = strlen(buf);
		chunks++;

		additional = copris_process(buf, count, attrib, trfile);
		if(additional) {
			if(fgets(buf, additional, stdin) == NULL)
				return 1;

			copris_process(buf, additional, attrib, trfile);
			sum += additional - 1;
			chunks++;
		}

		sum += count; // TODO when it overflows...
	}

	if(log_error() && !(attrib->copris_flags & HAS_DESTINATION))
		printf("; EOS\n");

	if(log_error()) {
		printf("End of stream, received %d byte(s) in %d chunk(s).\n", sum, chunks);
	}

	return 0;
}

int copris_process(char *stream, int stream_length, struct Attribs *attrib, struct Trfile **trfile) {
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

	if(attrib->copris_flags & (HAS_TRFILE|HAS_PRSET))
		free(final_stream);

	return is_on_hold;
}
