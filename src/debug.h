#ifndef DEBUG_H
#define DEBUG_H

#define MAX_FILENAME_LENGTH 17

/*
 * The debugging interface consists of preprocessor macros, divided into logging and
 * message printing categories.
 */

/*
 * COPRIS uses 4 verbosity levels to determine the amount of diagnostic text, written to
 * standard output and standard error. Level is determined via user-provided arguments
 * in main.c:parse_arguments() and passed to the global variable 'verbosity':
 *
 *  VERBOSITY  LEVEL          ARGUMENT  QUERY WITH
 *  0          silent/fatal   -q        if (!verbosity)
 *             messages only
 *  1          error          (none)    if (LOG_ERROR)
 *  2          info           -v        if (LOG_INFO)
 *  3          debug          -vv       if (LOG_DEBUG)
 */

extern int verbosity;

#define LOG_ERROR (verbosity > 0)
#define LOG_INFO  (verbosity > 1)
#define LOG_DEBUG (verbosity > 2)

/*
 * Following macros are used for printing text to the terminal. Note that their output may
 * change according to the build type (debug/release).
 *
 * PRINT_LOCATION(output);
 *           If in a debug build, prints file name and line number of the macro invocation,
 *           without a newline at the end. 'output' may be either 'stdout' or 'stderr'. Prints
 *           nothing if in a release build.
 *           Example:
 *               PRINT_LOCATION(stdout);
 *               -> src/main.c:397:
 *
 * PRINT_MSG(...);
 *           Invokes PRINT_LOCATION(stdout), prints the specified string to stdout. Takes
 *           printf-like variadic arguments.
 *           Example:
 *               PRINT_MSG("Verbosity level set to %d.", verbosity);
 *               -> src/main.c:336: Verbosity level set to 2.
 *
 * PRINT_ERROR_MSG(...);
 *           Invokes PRINT_LOCATION(stderr), prints the specified string to stderr. Takes
 *           printf-like variadic arguments.
 *           Example:
 *               PRINT_ERROR_MSG("Option '-%c' not recognised.", optopt);
 *               -> src/main.c:268: Option '-b' not recognised.
 *
 * PRINT_NOTE(str);
 *           Invokes PRINT_LOCATION(stdout) and prints 'str' with a newline to stdout if
 *           verbosity set to INFO or higher. Else, prints 'Note: ' followed by 'str' and
 *           a newline.
 *           Example:
 *               PRINT_NOTE("Limit number not used while reading from stdin.");
 *               -> src/main.c:344: Limit number not used while reading from stdin.
 *                  or
 *               -> Note: Limit number not used while reading from stdin.
 *
 * PRINT_SYSTEM_ERROR(name, msg);
 *           Passes 'msg' to PRINT_ERROR_MSG(), and 'name' to perror().
 *           Example:
 *               PRINT_SYSTEM_ERROR("fopen", "Error opening translation file.");
 *               -> src/translate.c: 33: Error opening translation file.
 *               -> fopen: No such file or directory
 */

#ifdef DEBUG
#   define PRINT_LOCATION(output)        \
           fprintf(output, "%*s:%3d: ", MAX_FILENAME_LENGTH, __FILE__, __LINE__)
#else
#   define PRINT_LOCATION(output)        \
           ((void)0)
#endif

#define _PRINT_MSG(output, ...)          \
    fprintf(output, __VA_ARGS__);        \
    fputs("\n", output)

#define PRINT_MSG(...)                   \
    do {                                 \
        PRINT_LOCATION(stdout);          \
        _PRINT_MSG(stdout, __VA_ARGS__); \
    } while (0)

// The two fputs() calls print terminal escape sequences
// for bold and normal text.
#define PRINT_ERROR_MSG(...)             \
    do {                                 \
        PRINT_LOCATION(stderr);          \
        fputs("\x1B[1m", stderr);        \
        _PRINT_MSG(stderr, __VA_ARGS__); \
        fputs("\x1B[0m", stderr);        \
    } while (0)

#define PRINT_NOTE(str)                  \
    do {                                 \
        if (LOG_INFO)                    \
            PRINT_LOCATION(stdout);      \
        else                             \
            fputs("Note: ", stdout);     \
                                         \
        puts(str);                       \
    } while (0)

// The (void)errno statement is added as a check to ensure
// errno.h is included in the file.
#define PRINT_SYSTEM_ERROR(name, msg)    \
    do {                                 \
        (void)errno; /* is missing */    \
        PRINT_ERROR_MSG(msg);            \
        perror(name);                    \
    } while (0)


#define CHECK_MALLOC(var)                \
    if (var == NULL) {                   \
        PRINT_ERROR_MSG("Memory allocation error."); \
        exit(-1);                        \
    }

#endif /* DEBUG_H */
