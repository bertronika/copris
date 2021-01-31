/*
 * Compile-time options for COPRIS
 */

// Inbound message buffer size
//   Defines, how big each received chunk will be, either from
//   a socket or from stdin.
#define BUFSIZE 128

// Maximum filename length
//   For arguments, inputted via the command line.
#define FNAME_LEN 36
