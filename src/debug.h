#ifndef DEBUG_H
#define DEBUG_H

#define MAX_FILENAME_LENGTH 17

extern int verbosity;

// ***** Message logging *****
/*
 * Return `1' if current verbosity level, set in the global `verbosity' variable,
 * matches the function name, `0' otherwise.
 * Verbosity is set when parsing argument options according to the following table:
 *
 * VERBOSITY LEVEL       ARGUMENT
 * 0  silent/fatal only  -q
 * 1  error              (default)
 * 2  info               -v
 * 3  debug              -vv
 *
 * Fatal messages are handled with error reporting functions and don't have a logging
 * interface.
 */

#define LOG_ERROR (verbosity > 0)
#define LOG_INFO  (verbosity > 1)
#define LOG_DEBUG (verbosity > 2)

#define _PRINT_MSG(output, ...)              \
    fprintf(output, __VA_ARGS__);            \
    fputs("\n", output)

#ifdef DEBUG
#   define PRINT_LOCATION(output)            \
           fprintf(output, "%*s:%3d: ", MAX_FILENAME_LENGTH, __FILE__, __LINE__)
#   define PRINT_MSG(...)                    \
        do {                                 \
            PRINT_LOCATION(stdout);          \
            _PRINT_MSG(stdout, __VA_ARGS__); \
        } while (0)
#   define PRINT_ERROR_MSG(...)              \
        do {                                 \
            fputs("\x1B[1m", stderr);        \
            PRINT_LOCATION(stderr);          \
            _PRINT_MSG(stderr, __VA_ARGS__); \
            fputs("\x1B[0m", stderr);        \
        } while (0)
#else
#   define PRINT_LOCATION(output) ((void)0)
#   define PRINT_MSG(...)         do { _PRINT_MSG(stdout, __VA_ARGS__); } while (0)
#   define PRINT_ERROR_MSG(...)   do { _PRINT_MSG(stderr, __VA_ARGS__); } while (0)
#endif

#define PRINT_NOTE(str)              \
    do {                             \
        if (LOG_INFO)                \
            PRINT_LOCATION(stdout);  \
        else                         \
            fputs("Note: ", stdout); \
                                     \
        puts(str);                   \
    } while (0)

#define PRINT_SYSTEM_ERROR(name, msg) \
    do {                              \
        (void)errno; /* is missing */ \
        PRINT_ERROR_MSG(msg);         \
        perror(name);                 \
    } while (0);

#endif /* DEBUG_H */
