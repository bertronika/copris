/*
 * Compile-time options for COPRIS
 *
 * Values, specified here, are used by default, apart from unit tests.
 */

// Inbound text buffer size
#ifndef BUFSIZE
#   define BUFSIZE 128
#endif

// Maximum lengths of name and value strings in .ini files
#ifndef MAX_INIFILE_ELEMENT_LENGTH
#   define MAX_INIFILE_ELEMENT_LENGTH 64
#endif

