/*
 * Compile-time options for COPRIS
 */

#ifndef CONFIG_H
#define CONFIG_H

// Inbound message buffer size
//   Defines how big each received chunk will be, either from
//   a socket or from stdin. Number includes the NUL terminator.
#ifndef BUFSIZE
#	define BUFSIZE 16
#endif

// Maximum lengths of name and value strings in .ini files
#define MAX_INIFILE_ELEMENT_LENGTH 32

#endif /* CONFIG_H */
