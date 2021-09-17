/*
 * server.c
 * Stream socket server with trfile and prset hooks
 * 
 * Copyright (C) 2020-2021 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 * 
 * - https://beej.us/guide/bgnet/html/
 * - https://ops.tips/blog/a-tcp-server-in-c/
 * - https://www.binarytides.com/socket-programming-c-linux-tutorial/
 * - tcpserver.c from https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "Copris.h"
#include "config.h"
#include "debug.h"
#include "server.h"
#include "writer.h"
#include "translate.h"
#include "printerset.h"

/*
 * Round backlog to a power of 2.
 * https://stackoverflow.com/a/5111841
 */
#define BACKLOG 2

int copris_listen(int *parentfd, int portno) {
	int fderr; // Error code of a socket operation
	struct sockaddr_in serveraddr; // Server's address

	/*
	 * Create a system socket using the following:
	 *   AF_INET      IPv4
	 *   SOCK_STREAM  TCP protocol
	 *   IPPROTO_IP   IP protocol
	 */
	fderr = (*parentfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP));
	if(log_perr(fderr, "socket", "Failed to create socket endpoint."))
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
	fderr = bind(*parentfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if(log_perr(fderr, "bind", "Failed to bind socket to address. "
	                           "Non-root users should set it >1023."))
		return 1;

	if(log_debug()) {
		log_date();
		printf("Socket bound to address.\n");
	}

	// Make the parent socket passive - accept incoming connections.
	// Also limit the number of connections to BACKLOG
	fderr = listen(*parentfd, BACKLOG);
	if(log_perr(fderr, "listen", "Failed to make socket passive."))
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

int copris_read(int *parentfd, int daemon, int limitnum, int limit_cutoff,
                struct attrib *trfile, int printerset, struct attrib *destination) {
	int fderr;             // Error code of a socket operation
	int childfd;           // Child socket, which processes one client at a time
	int bytenum   = 0;     // Received/sent message (byte) size
	int discarded = 0;     // Discarded number of bytes, if limit is set
	struct sockaddr_in clientaddr;  // Client's address
	socklen_t clientlen;            // (Byte) size of client's address (sockaddr)
	char *hostaddrp;                // Host address string
	char host[NI_MAXHOST];          // Host info (IP, hostname). NI_MAXHOST is built in
	unsigned char buf[BUFSIZE + 1]; // Inbound message buffer
	char limit_message[] = "Send size limit exceeded, terminating connection.\n";

	// Get the struct size
	clientlen = sizeof(clientaddr);

	// Wait for a connection request and accept it
	childfd = accept(*parentfd, (struct sockaddr *)&clientaddr, &clientlen);
	if(log_perr(childfd, "accept", "Failed to accept the connection."))
		return 1;

	if(log_info()) {
		log_date();
		printf("Connection to socket accepted.\n");
	}

	// Prevent more than one connection if not a daemon
	if(!daemon) {
		fderr = close(*parentfd);
		if(log_perr(fderr, "close", "Failed to close the parent connection."))
			return 1;
	}

	// Get the hostname of the client
	fderr = getnameinfo((struct sockaddr *)&clientaddr, sizeof(clientaddr), 
	                    host, sizeof(host), NULL, 0, 0);
	if(fderr != 0){
		fprintf(stderr, "getnameinfo: Failed getting hostname from address.\n");
	}

	// Convert client's address from network byte order to a dotted-decimal form
	hostaddrp = inet_ntoa(clientaddr.sin_addr);
	if(hostaddrp == NULL) {
		fprintf(stderr, "inet_ntoa: Failed converting host's address.\n");
	}

	if(log_info())
		log_date();

	if(log_err()) {
		printf("Inbound connection from %s (%s).\n", host, hostaddrp);
		if(!destination->exists)
			printf("; BOS\n");
	}

	// Empty out the inbound buffer
	memset(buf, '\0', BUFSIZE + 1);

	// Read the data sent by the client into the buffer
	while((fderr = read(childfd, buf, BUFSIZE)) > 0) {
		bytenum += fderr; // Append read bytes to the total byte counter
		if(limitnum && bytenum > limitnum) {
			if(limit_cutoff) {
				buf[limitnum] = '\0';
				discarded = bytenum - limitnum;
				limit_cutoff = 2;

				fderr = write(childfd, limit_message, strlen(limit_message));
				log_perr(fderr, "write", "Error sending termination text to socket.");
			} else {
				if(log_err())
					printf("Client exceeded send size limit (%d B/%d B), discarding and "
					       "terminating connection.\n", bytenum, limitnum);

				discarded = fderr;

				fderr = write(childfd, limit_message, strlen(limit_message));
				log_perr(fderr, "write", "Error sending termination text to socket.");

				break;
			}
		}

		copris_send(buf, fderr, &trfile, printerset, &destination);

//		memset(buf, '\0', BUFSIZE + 1); // Clear the buffer for next read. TODO ???
		if(limit_cutoff == 2) {
			if(log_err())
				printf("\nClient exceeded send size limit (%d B/%d B), cutting off and "
				       "terminating connection.\n", bytenum, limitnum);
			break;
		}
	}
	if(log_perr(fderr, "read", "Error reading from socket."))
		return 1;

	// Close the current connection 
	fderr = close(childfd);
	if(log_perr(fderr, "close", "Failed to close the child connection."))
		return 1;

	if(log_err() && !destination->exists)
		printf("; EOS\n");

	if(log_info())
		log_date();

	if(log_err()) {
		printf("End of stream, received %d B in %d chunk(s)", 
			   bytenum, (bytenum && bytenum < BUFSIZE) ? 1 : bytenum / BUFSIZE);

		if(discarded) {
			printf(", %d B %s.\n", discarded,
			      (limit_cutoff == 2) ? "cut off" : "discarded");
		} else {
			printf(".\n");
		}
	}

	if(log_info()) {
		log_date();
		printf("Connection from %s (%s) closed.\n", host, hostaddrp);
	}

	return 0;
}

int copris_stdin(struct attrib *trfile, int printerset, struct attrib *destination) {
	int bytenum = 0; // Nr. of read bytes
	char buf[BUFSIZE + 1]; // Inbound message buffer

//	memset(buf, '\0', BUFSIZE + 1); TODO ???

	if(log_info()) {
		log_date();
		printf("Trying to read from stdin...\n");
	}

	if(isatty(STDIN_FILENO) && log_err()) {
		if(log_info())
				log_date();
			else
				printf("Note: ");

		printf("You are in text input mode (reading from "
		       "stdin). To stop reading, press Ctrl+D\n");
	}

	if(log_err() && !destination->exists)
		printf("; BOS\n");

	while(fgets(buf, BUFSIZE, stdin) != NULL) {
		copris_send((unsigned char *)buf, strlen(buf),
		            &trfile, printerset, &destination);
		bytenum += strlen(buf);
	}

	if(log_err() && !destination->exists)
		printf("; EOS\n");

	if(log_err()) {
		printf("End of stream, received %d B in %d chunk(s).\n",
		       bytenum, (bytenum && bytenum < BUFSIZE) ? 1 : bytenum / BUFSIZE);
	}

	return 0;
}

int copris_send(unsigned char *buffer, int buffer_size,
                struct attrib **trfile, int printerset, struct attrib **destination) {
	unsigned char to_print[INSTRUC_LEN * BUFSIZE + 1]; // Final, converted stream

	int z;
	// Only for prset/trfile magic
	for(z = 0; z <= BUFSIZE; z++) {
		to_print[z] = buffer[z];
	}
	to_print[z] = '\0';

	if(printerset) {
		copris_printerset(buffer, buffer_size, to_print, printerset);
		if((*trfile)->exists) {
			copris_translate(to_print, buffer_size, to_print);
		}
	} else if((*trfile)->exists) {
		copris_translate(buffer, buffer_size, to_print);
	}

	// Destination can be either stdout or a file
	if((*destination)->exists) {
		// Write to the output file/printer
		copris_write((*destination)->text, to_print);
	} else {
		// Write to stdout
		printf("%s", to_print);
	}

	return 0;
}
