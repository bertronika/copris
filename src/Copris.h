#include <stddef.h>   /* size_t         */
#include <stdbool.h>  /* bool           */
#include <uthash.h>   /* UT_hash_handle */

#include "config.h"

#define HAS_DESTINATION  (1 << 0)
#define HAS_TRFILE       (1 << 1)
#define HAS_PRSET        (1 << 2)
#define MUST_CUTOFF      (1 << 3)
#define FILTER_NON_ASCII (1 << 4)

struct Attribs {
	unsigned int portno; /* Listening port of this server                  */
	bool daemon;         /* True if COPRIS runs continuously               */
	size_t limitnum;     /* Maximum allowed number of received bytes       */

	int copris_flags;    /* Flags regarding user-specified arguments       */
	char *prset;         /* Name of the printer feature file               */
	char *trfile;        /* Name of the encoding file                      */
	char *destination;   /* Output device or file                          */
};

struct Stats {
	int chunks;             // Number of read chunks
	size_t sum;             // Sum of all read (received) bytes
	bool size_limit_active;
	size_t discarded;       // Discarded number of bytes, if limit is set
};

static const struct Stats STATS_INIT = {
	0, 0, false, 0
};

struct Inifile {
	char in[MAX_INIFILE_ELEMENT_LENGTH];   /* key (name)   */
	char out[MAX_INIFILE_ELEMENT_LENGTH];  /* item (value) */
	UT_hash_handle hh;
};

