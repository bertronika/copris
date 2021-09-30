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

int copris_read_socket(int *parentfd, struct Attribs *attrib) {
	int fderror;  // Error code of a socket operation
	int childfd;  // Child socket, which processes one client at a time

	int sum       = 0;  // Sum of all read (received) bytes
	int chunks    = 0;  // Number of read chunks
	int discarded = 0;  // Discarded number of bytes, if limit is set

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

// 		copris_send(buf, fderror, &trfile, printerset, &destination);
		copris_process(buf, fderror + 1, attrib);

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

int copris_read_stdin(struct Attribs *attrib) {
	int sum    = 0;  // Sum of all read bytes
	int chunks = 0;  // Number of read chunks
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
	while(fgets(buf, BUFSIZE, stdin) != NULL) {
		int count = strlen(buf);
		chunks++;

		copris_process(buf, count, attrib);
		sum += count; // TODO when it overflows...
	}

	if(log_error() && !(attrib->copris_flags & HAS_DESTINATION))
		printf("; EOS\n");

	if(log_error()) {
		printf("End of stream, received %d byte(s) in %d chunk(s).\n", sum, chunks);
	}

	return 0;
}

int copris_process(char *text, int text_length, struct Attribs *attrib) {
	char *final_stream = text;

	// To avoid splitting multibyte characters:
	// If the buffer has been filled to the limit, check the last 3 characters. If
	// any of them are not ASCII (probably multibyte Unicode), stash them into a
	// secondary buffer. They get replaced with null bytes in the primary buffer.
	// Buffers are then combined and sent as one.
	char combined_buffer[BUFSIZE + 5];

	// strlen() omits null byte at the end
	if(text_length + 1 == BUFSIZE) {
		char on_hold[5];
		int secondary  = 0;
		int is_on_hold = 0;

		for(int primary = 3; primary; primary--) {
			if(!isprint(text[BUFSIZE - 1 - primary])) {
				on_hold[secondary] = text[BUFSIZE - 1 - primary];
				secondary++;
				on_hold[secondary] = '\0';
				text[BUFSIZE - 1 - primary] = '\0';
				is_on_hold = 1;
			}
		}
		on_hold[++secondary] = '\0';

		if(is_on_hold) {
			strcpy(combined_buffer, text);
			strcat(combined_buffer, on_hold);

			final_stream = combined_buffer;
		}
	}

	if(attrib->copris_flags & HAS_PRSET) {
// 		copris_printerset();
	}

	if(attrib->copris_flags & HAS_TRFILE) {
// 		copris_translate();
	}

	// Destination can be either a file/printer or stdout
	if(attrib->copris_flags & HAS_DESTINATION) {
		copris_write_file(attrib->destination, final_stream);
	} else {
		printf("%s", final_stream);
	}

	return 0;
}
