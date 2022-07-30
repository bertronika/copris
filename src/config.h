/*
 * Compile-time options for COPRIS
 *
 * Values, specified here, are used by default, apart from unit tests.
 */

#ifndef CONFIG_H
#define CONFIG_H

// Inbound text buffer size
#ifndef BUFSIZE
#   define BUFSIZE 16
#endif

// Maximum lengths of name and value strings in .ini files and
// predefined commands below
#ifndef MAX_INIFILE_ELEMENT_LENGTH
#   define MAX_INIFILE_ELEMENT_LENGTH 48
#endif

/*
 * Default predefined values for printer feature set commands
 *
 * They should be defined in the same format as the .ini files.
 * NOTE: if set improperly, an error will be thrown at run time, but only
 *       with '-r/--printer' or '--dump-commands' specified in the command line.
 */
#define P_LIST_ITEM      "0x3D 0x3E"  /* => */
#define P_THEMATIC_BREAK "0x2D 0x3D 0x2D 0x3D 0x2D 0x3D 0x2D"  /* -=-=-=- */

#endif /* CONFIG_H */
