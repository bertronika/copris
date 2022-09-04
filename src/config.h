/*
 * Compile-time options for COPRIS
 *
 * Values, specified here, are used by default, apart from unit tests.
 */

#ifndef CONFIG_H
#define CONFIG_H

// Inbound text buffer size
#ifndef BUFSIZE
#   define BUFSIZE 128
#endif

// Maximum lengths of name and value strings in .ini files and
// predefined commands below
#ifndef MAX_INIFILE_ELEMENT_LENGTH
#   define MAX_INIFILE_ELEMENT_LENGTH 48
#endif

#endif /* CONFIG_H */
