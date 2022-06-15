/*
 * Compile-time options for COPRIS
 */

#ifndef CONFIG_H
#define CONFIG_H

// Inbound text buffer size
#ifndef BUFSIZE
#   define BUFSIZE 16
#endif

// Maximum lengths of name and value strings in .ini files
#define MAX_INIFILE_ELEMENT_LENGTH 32

// Default predefined values for printer feature set commands
#define P_LIST_ITEM '+'

#endif /* CONFIG_H */
