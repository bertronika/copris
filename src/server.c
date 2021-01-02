/*
 * server.c
 * Stream socket server with trfile and prset hooks
 * 
 * Part of COPRIS - a converting printer server
 * (c) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
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

#include "debug.h"
#include "server.h"
#include "writer.h"
#include "translate.h"
#include "printerset.h"

const int BUFSIZE = 256;

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
    log_perr(fderr, "socket", "Failed to create socket endpoint.");
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
    log_perr(fderr, "bind", "Failed to bind socket to address. " 
	                "Non-root users should set it >1023.");
	if(log_debug()) {
		log_date();
		printf("Socket bound to address.\n");
	}

    // Make the parent socket passive - accept incoming connections.
    // Also limit the number of connections to BACKLOG
    fderr = listen(*parentfd, BACKLOG);
    log_perr(fderr, "listen", "Failed to make socket passive.");
	if(log_info()) {
		log_date();
		if(log_debug()) {
			printf("Socket made passive. ");
		}
		printf("Now we listen...\n");
	}
    
    return 0;
}

int copris_read(int *parentfd, char *destination, int daemon, int trfile, int printerset, int limitnum) {
	int fderr;             // Error code of a socket operation
	int childfd;           // Child socket, which processes one client at a time
	int bytenum   = 0;     // Received/sent message (byte) size
	int discarded = 0;     // Discarded number of bytes, if limit is set
	struct sockaddr_in clientaddr;  // Client's address
	socklen_t clientlen;            // (Byte) size of client's address (sockaddr)
	char *hostaddrp;                // Host address string
	char host[NI_MAXHOST];          // Host info (IP, hostname). NI_MAXHOST is built in
	unsigned char buf[BUFSIZE + 1]; // Inbound message buffer
	unsigned char to_print[INSTRUC_LEN * BUFSIZE + 1]; // Final, converted stream
	char limit_message[] = "Send size limit exceeded, terminating connection.\n";

	// Get the struct size
	clientlen = sizeof(clientaddr);
	
	// Wait for a connection request and accept it
	childfd = accept(*parentfd, (struct sockaddr *)&clientaddr, &clientlen);
	log_perr(childfd, "accept", "Failed to accept the connection.");
	if(log_info()) {
		log_date();
		printf("Connection to socket accepted.\n");
	}
	
	// Prevent more than one connection if not a daemon
	if(!daemon) {
		fderr = close(*parentfd);
		log_perr(fderr, "close", "Failed to close the parent connection.");
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
		if(!destination[0])
			printf("; BOS\n");
	}

	// Empty out the inbound buffer
	memset(buf, '\0', BUFSIZE + 1);

	int z;
	int discard = 0;
	// Read the data sent by the client into the buffer
	while((fderr = read(childfd, buf, BUFSIZE)) > 0 && discard != 2) {
		bytenum += fderr; // Append read bytes to the total byte counter
		if(limitnum && bytenum > limitnum) {
			if(discard) {
				if(log_err())
					printf("Client exceeded send size limit (%d B/%d B), "
						   "terminating connection.\n", bytenum, limitnum);
				
				discarded = fderr;
				
				fderr = write(childfd, limit_message, strlen(limit_message));
				log_perr(fderr, "write", "Error sending termination text to socket.");
				
				break;
			} else {
				buf[limitnum] = '\0';
				discarded = bytenum - limitnum;
				discard = 2;
				
				fderr = write(childfd, limit_message, strlen(limit_message));
				log_perr(fderr, "write", "Error sending termination text to socket.");
			}
		}
		// Only for prset/trfile magic
		for(z = 0; z <= BUFSIZE; z++) {
			to_print[z] = buf[z];
		}
		to_print[z] = '\0';
		
		if(printerset) {
			copris_printerset(buf, fderr, to_print, printerset);
			if(trfile) {
				copris_translate(to_print, fderr, to_print);
			}
		} else if(trfile) {
			copris_translate(buf, fderr, to_print);
		}
		
		// Destination can be either stdout or a file
		if(!destination[0]) {
			printf("%s", to_print);              // Print received text to stdout
		} else {
			copris_write(destination, to_print); // Write to the output file/printer
		}
		
		memset(buf, '\0', BUFSIZE + 1); // Clear the buffer for next read.
	}
	log_perr(fderr, "read", "Error reading from socket.");

	free(input);
	free(replacement);
	
	// Close the current connection 
	fderr = close(childfd);
	log_perr(fderr, "close", "Failed to close the child connection.");
	
	if(log_err() && !destination[0])
		printf("; EOS\n");
	
	if(log_info())
		log_date();
	
	if(log_err()) {
		printf("End of stream, received %d B in %d chunk(s)", 
			   bytenum, (bytenum && bytenum < BUFSIZE) ? 1 : bytenum / BUFSIZE);
		
		if(discarded) {
			printf(", %d B %s.\n", discarded, (discard == 2) ? "cut off" : "discarded");
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
