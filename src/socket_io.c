/*
 * Stream socket (TCP) server for receiving text over the network
 *
 * Copyright (C) 2020-2023 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files 'main.c' and 'COPYING' for more details.
 */

// For 'getnameinfo' and 'memccpy' in ISO C
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <assert.h>

#include <utstring.h> /* uthash library - dynamic strings */

#include "Copris.h"
#include "debug.h"
#include "socket_io.h"
#include "utf8.h"
#include "utstring_cut.h"

static int read_from_socket(UT_string *copris_text, int childfd,
                             struct Stats *stats, struct Attribs *attrib);
static void apply_byte_limit(UT_string *copris_text, int childfd,
                             struct Stats *stats, struct Attribs *attrib);

/*
 * Round backlog to a power of 2.
 * https://stackoverflow.com/a/5111841
 */
#define BACKLOG 2

int copris_socket_listen(int *parentfd, unsigned int portno)
{
	/*
	 * Create a system socket using the following:
	 *   AF_INET      IPv4
	 *   SOCK_STREAM  TCP protocol
	 *   IPPROTO_IP   IP protocol
	 */
	*parentfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (*parentfd == -1) {
		PRINT_SYSTEM_ERROR("socket", "Failed to create socket endpoint.");
		return -1;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Socket endpoint created.");

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
	int tmperr = bind(*parentfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (tmperr != 0) {
		PRINT_SYSTEM_ERROR("bind", "Failed to bind socket to address. "
		                           "Non-root users should set it >1023.");
		return -1;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Socket bound to address.");

	// Make the parent socket passive - accept incoming connections.
	// Limit number of connections to the value of BACKLOG.
	tmperr = listen(*parentfd, BACKLOG);
	if (tmperr != 0) {
		PRINT_SYSTEM_ERROR("listen", "Failed to make socket passive.");
		return -1;
	}

	if (LOG_INFO) {
		PRINT_LOCATION(stdout);

		if (LOG_DEBUG)
			printf("Socket made passive. ");

		puts("Now we listen...");
	}

	return 0;
}

int copris_handle_socket(UT_string *copris_text, int *parentfd, int *childfd,
                         struct Attribs *attrib)
{
	struct sockaddr_in clientaddr;  // Client's address
	socklen_t clientlen;            // (Byte) size of client's address (sockaddr)
	clientlen = sizeof(clientaddr);
	int tmperr;

	// Wait for a connection request, accept it and pass it on as a child socket
	*childfd = accept(*parentfd, (struct sockaddr *)&clientaddr, &clientlen);
	if (*childfd == -1) {
		PRINT_SYSTEM_ERROR("accept", "Failed to accept the connection.");
		return -1;
	}

	if (LOG_DEBUG)
		PRINT_MSG("Connection to socket accepted.");

	// Prevent more than one connection if not a daemon
	if (!attrib->daemon) {
		tmperr = close_socket(*parentfd, "parent");

		if (tmperr != 0)
			return tmperr;
	}

	// Get host info (IP, hostname) of the client (TODO gai_strerror)
	char host_info[256];
	tmperr = getnameinfo((struct sockaddr *)&clientaddr, sizeof(clientaddr),
	                      host_info, sizeof(host_info), NULL, 0, 0);
	if (tmperr != 0) {
		PRINT_SYSTEM_ERROR("getnameinfo", "Failed getting hostname from address.");
		memccpy(host_info, "name unknown", '\0', 256);
	}

	// Convert client's address from network byte order to a dotted-decimal form
	char *host_address = inet_ntoa(clientaddr.sin_addr);
	char addr_unknown[] = "<address unknown>";
	if (host_address == NULL) {
		PRINT_ERROR_MSG("inet_ntoa: Failed converting host's address to dotted decimal.\n");
		host_address = addr_unknown;
	}

	if (LOG_ERROR) {
		if (LOG_INFO)
			PRINT_LOCATION(stdout);

		printf("Inbound connection from %s (%s).\n", host_info, host_address);
	}

	// Read text from socket and process it
	struct Stats stats = STATS_INIT;
	int read_error = read_from_socket(copris_text, *childfd, &stats, attrib);
	if (read_error)
		return -1;

	if (LOG_ERROR) {
		if (LOG_INFO)
			PRINT_LOCATION(stdout);

		printf("End of stream, received %zu byte(s) in %d chunk(s)",
		       stats.sum, stats.chunks);

		if (stats.size_limit_active) {
			printf(", %zu byte(s) %s.\n", stats.discarded,
			       (attrib->copris_flags & MUST_CUTOFF) ? "cut off" : "discarded");
		} else {
			printf(".\n");
		}
	}

	if (LOG_INFO)
		PRINT_MSG("Connection from %s (%s) closed.", host_info, host_address);

	return 0;
}

int close_socket(int fd, const char *socket_type)
{
	if (LOG_DEBUG)
		PRINT_MSG("Closing %s socket.", socket_type);

	int error = close(fd);
	if (error != 0) {
		PRINT_SYSTEM_ERROR("close", "Failed to close %s socket.", socket_type);
		return -1;
	}

	return 0;
}

int send_to_socket(int childfd, const char *message)
{
	UT_string *full_message;
	utstring_new(full_message);
	utstring_printf(full_message, "copris: %s\n", message);

	ssize_t buffer_length = write(childfd, utstring_body(full_message),
	                              utstring_len(full_message) - 1);

	if (buffer_length == -1)
		PRINT_SYSTEM_ERROR("write", "Error sending text to socket.");

	utstring_free(full_message);

	return buffer_length;
}

static int read_from_socket(UT_string *copris_text, int childfd,
                            struct Stats *stats, struct Attribs *attrib)
{
	char buffer[BUFSIZE];  // Inbound message buffer
	ssize_t buffer_length; // Return value of a socket operation - number of
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

	if (buffer_length == -1) {
		PRINT_SYSTEM_ERROR("read", "Error reading from socket.");
		return -1;
	}

	return 0;
}

static void apply_byte_limit(UT_string *copris_text, int childfd,
                             struct Stats *stats, struct Attribs *attrib)
{
	const char limit_message[] = "You have sent too much text. Terminating connection.\n";
	send_to_socket(childfd, limit_message);

	stats->size_limit_active = true;

	if (!(attrib->copris_flags & MUST_CUTOFF)) {
		// Discard whole chunk of text, if over the limit
		stats->discarded = stats->sum;
		utstring_clear(copris_text);

		if (LOG_ERROR) {
			if (LOG_INFO)
				PRINT_LOCATION(stdout);

			printf("Client exceeded send size limit (%zu B/%zu B), discarding remaining "
			       "text and terminating connection.\n", stats->sum, attrib->limitnum);
		}

	} else {
		// Cut off text at limit and remove any possible remains of multibyte characters,
		// possibly split at the limit
		char *text = utstring_body(copris_text);

		stats->discarded = stats->sum - attrib->limitnum;
		utstring_cut(copris_text, attrib->limitnum);
		assert(strlen(text) == attrib->limitnum);

		int terminated = utf8_terminate_incomplete_buffer(text, utstring_len(copris_text));

		if (LOG_ERROR) {
			if (LOG_INFO)
				PRINT_LOCATION(stdout);

			printf("Client exceeded send size limit (%zu B/%zu B), cutting off text and "
			       "terminating connection.\n", stats->sum, attrib->limitnum);
		}

		if (terminated && LOG_DEBUG)
			PRINT_MSG("Additional multibyte characters were omitted from the output.");
	}
}
