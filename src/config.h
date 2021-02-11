/*
 * Compile-time options for COPRIS
 */

// Inbound message buffer size
//   Defines how big each received chunk will be, either from
//   a socket or from stdin.
#define BUFSIZE 128

// Maximum filename length
//   Uncomment this line only if you want to manually set the limit, as
//   it is otherwise set automatically using your systems' default value.
//#define FNAME_LEN 36
