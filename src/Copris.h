#ifndef COPRIS_H
#define COPRIS_H

#include <stddef.h> /* size_t */
#include <uthash.h> /* uthash library */

/*
 * Round backlog to a power of 2.
 * https://stackoverflow.com/a/5111841
 */
#define BACKLOG 2

#define HAS_DESTINATION (1 << 0)
#define HAS_TRFILE      (1 << 1)
#define HAS_PRSET       (1 << 2)
#define MUST_CUTOFF     (1 << 3)

struct Attribs {
	unsigned int portno; /* Listening port of this server                  */
	int prset;           /* Parsed printer set index number                */
	int daemon;          /* Run continuously                               */
	size_t limitnum;     /* Limit received number of bytes                 */

	int copris_flags;    /* Flags regarding user-specified arguments:      */
	char *prsetname;     /* Printer feature set name                       */
	char *trfile;        /* Translation file location                      */
	char *destination;   /* Output file (printer)                          */
};

struct Trfile {
	char in[5];        /* key (name)   */
	unsigned char out; /* item (value) */
	UT_hash_handle hh;

	// Input key holds at most one 4-byte Unicode
	// character and 1 terminating NUL byte.
};

struct Prset {
	char code[20];
	char command[10];
	UT_hash_handle hh;
};

#endif /* COPRIS_H */
