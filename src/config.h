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

/*
 * Default predefined values for printer feature set commands.
 *
 * They should be defined in the same format as the .ini files.
 * NOTE: if set improperly, an errpr will be thrown at run time, but only
 *       with '-r/--printer' or '--dump-commands' specified in the command line.
 */
#define P_LIST_ITEM "0x3D 0x3E"

#endif /* CONFIG_H */
