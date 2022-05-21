/*
 * Stream socket (TCP) server for receiving text over the network
 *
 * Copyright (C) 2020-2022 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

// An addition to utstring.h that terminates the string at given index
#define utstring_cut(s,n)  \
    do {                   \
        (s)->i=(n);        \
        (s)->d[(n)]='\0';  \
    } while (0)

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

#include <utstring.h> /* uthash library - dynamic strings */

#include "Copris.h"
#include "config.h"
#include "debug.h"
#include "read_socket.h"

static bool read_from_socket(UT_string *copris_text, int childfd,
                             struct Stats *stats, struct Attribs *attrib);
static void apply_byte_limit(UT_string *copris_text, int childfd,
                             struct Stats *stats, struct Attribs *attrib);

bool copris_socket_listen(int *parentfd, unsigned int portno) {
	int fderror;  // Return value of a socket operation

	/*
	 * Create a system socket using the following:
	 *   AF_INET      IPv4
	 *   SOCK_STREAM  TCP protocol
	 *   IPPROTO_IP   IP protocol
	 */
	fderror = (*parentfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP));
	if (raise_perror(fderror, "socket", "Failed to create socket endpoint."))
		return true;

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
		return true;

	if (LOG_DEBUG)
		LOG_STRING("Socket bound to address.");

	// Make the parent socket passive - accept incoming connections.
	// Limit number of connections to the value of BACKLOG.
	fderror = listen(*parentfd, BACKLOG);
	if (raise_perror(fderror, "listen", "Failed to make socket passive."))
		return true;

	if (LOG_INFO) {
		LOG_LOCATION();
		if (LOG_DEBUG) {
			printf("Socket made passive. ");
		}
		printf("Now we listen...\n");
	}

	// No error reported.
	return false;
}

bool copris_handle_socket(UT_string *copris_text, int *parentfd, struct Attribs *attrib) {
	int fderror;                    // Return value of a socket operation
	struct sockaddr_in clientaddr;  // Client's address
	socklen_t clientlen;            // (Byte) size of client's address (sockaddr)
	clientlen = sizeof(clientaddr);

	// Wait for a connection request, accept it and pass it on as a child socket
	int childfd = accept(*parentfd, (struct sockaddr *)&clientaddr, &clientlen);
	if (raise_perror(childfd, "accept", "Failed to accept the connection."))
		return true;

	if (LOG_DEBUG)
		LOG_STRING("Connection to socket accepted.");

	// Prevent more than one connection if not a daemon
	if (!attrib->daemon) {
		fderror = close(*parentfd);
		if (raise_perror(fderror, "close", "Failed to close the parent connection."))
			return true;
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
	char addr_unknown[] = "<address unknown>";
	if (host_address == NULL) {
		fprintf(stderr, "inet_ntoa: Failed converting host's address to dotted decimal.\n");
		host_address = addr_unknown;
	}

	if (LOG_INFO)
		LOG_LOCATION();

	if (LOG_ERROR)
		printf("Inbound connection from %s (%s).\n", host_info, host_address);

	// Read text from socket and process it
	struct Stats stats = STATS_INIT;
	bool read_error = read_from_socket(copris_text, childfd, &stats, attrib);
	if (read_error)
		return true;

	// Close the current connection
	fderror = close(childfd);
	if (raise_perror(fderror, "close", "Failed to close the child connection."))
		return true;

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

	// No error reported.
	return false;
}

static bool read_from_socket(UT_string *copris_text, int childfd,
                             struct Stats *stats, struct Attribs *attrib) {
	char buffer[BUFSIZE];  // Inbound message buffer
	int buffer_length;     // Return value of a socket operation - number of
	                       // read bytes if successful

	// read() returns number of read bytes, or -1 on error (and sets errno), and puts
	// _no_ termination at the end of the buffer.
	while ((buffer_length = read(childfd, buffer, BUFSIZE)) > 0) {
		//buffer[buffer_length] = '\0';

		// Note that the ending null byte is omitted from the count. This isn't
		// a problem, since utstring_bincpy() terminates its internal string
		// after appending to it.
		utstring_bincpy(copris_text, buffer, buffer_length);

		stats->chunks++;
		stats->sum += buffer_length;

		// Check if length of received text went over the limit (if limit is active)
		if (attrib->limitnum && stats->sum > attrib->limitnum) {
			apply_byte_limit(copris_text, childfd, stats, attrib);
			break;
		}
	}

	if (raise_perror(buffer_length, "read", "Error reading from socket."))
		return true;

	return false;
}

static void apply_byte_limit(UT_string *copris_text, int childfd,
                             struct Stats *stats, struct Attribs *attrib) {
	const char limit_message[] = "You have sent too much text. Terminating connection.\n";

	int werror = write(childfd, limit_message, (sizeof limit_message) - 1);
	raise_perror(werror, "write", "Error sending termination text to socket.");

	stats->size_limit_active = true;

	// Terminate before processing excessive text - discard whole chunk
	if (!(attrib->copris_flags & MUST_CUTOFF)) {
		stats->discarded = stats->sum;
		utstring_clear(copris_text);

		if (LOG_ERROR)
			printf("Client exceeded send size limit (%zu B/%zu B), discarding remaining "
			       "text and terminating connection.\n", stats->sum, attrib->limitnum);

	// Terminate after processing excessive text - cut off buffer at limit
	} else {
		stats->discarded = stats->sum - attrib->limitnum;
		utstring_cut(copris_text, attrib->limitnum);
		assert(strlen(utstring_body(copris_text)) == attrib->limitnum);

		if (LOG_ERROR)
			printf("Client exceeded send size limit (%zu B/%zu B), cutting off text and "
			       "terminating connection.\n", stats->sum, attrib->limitnum);
	}
}
