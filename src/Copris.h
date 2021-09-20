#ifndef COPRIS_H
#define COPRIS_H

#ifndef FNAME_LEN
#	ifndef NAME_MAX
#		define FNAME_LEN 64
#	else
#		define FNAME_LEN NAME_MAX
#	endif
#endif

/*
 * Round backlog to a power of 2.
 * https://stackoverflow.com/a/5111841
 */
#define BACKLOG 2

#define HAS_DESTINATION 0x01
#define HAS_TRFILE      0x02
#define HAS_PRSET       0x04

// struct attrib {
// 	int exists;
// 	char *text;
// };

struct Attribs {
	int portno;        /* Listening port of this server                  */
	int prset;         /* Parsed printer set index number                */
	int daemon;        /* Run continuously                               */
	int limitnum;      /* Limit received number of bytes                 */
	int limit_cutoff;  /* Cut text off at limit instead of discarding it */

	int copris_flags;  /* Flags regarding user-specified arguments:      */
	char *prsetname;   /* Printer feature set name                       */
	char *trfile;      /* Translation file location                      */
	char *destination; /* Output file (printer)                          */
// 	int verbosity;
};

#endif /* COPRIS_H */
