#ifndef DEBUG_H
#define DEBUG_H

#define MAX_FILENAME_LENGTH 17

extern int verbosity;

// ***** Error reporting *****
/*
 * If `return_value' is less than 0, display specified `message', followed by a
 * perror(3) call with the specified `function_name'. Return value is 1 if an
 * error is printed, otherwise 0. Message is printed to stderr.
 */
int raise_perror(int return_value, char *function_name, char *message);

/*
 * If `received_errno' is different from 0, display specified `message', followed
 * by a perror(3) call with the specified `function_name'. Return value is 1 if an
 * error is printed, otherwise 0. Message is printed to stderr.
 */
int raise_errno_perror(int received_errno, char *function_name, char *message);

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
int log_error();
int log_info();
int log_debug();

#define LOG_ERROR (verbosity > 0)
#define LOG_INFO  (verbosity > 1)
#define LOG_DEBUG (verbosity > 2)


#ifdef DEBUG
#	define LOG_LOCATION()  (printf("%*s:%3d: ",     MAX_FILENAME_LENGTH, __FILE__, __LINE__     ))
#	define LOG_STRING(str) (printf("%*s:%3d: %s\n", MAX_FILENAME_LENGTH, __FILE__, __LINE__, str))
#else
#	define LOG_LOCATION()  ((void)0)
#	define LOG_STRING(str) (fputs(str, stdout))
#endif

/*
 * Print current date and time in a custom format, without a newline character at the end.
 * Format is subject to change, ideally using the system locale (TODO).
 *
 * This function works only when compiling the debug target (-DDEBUG).
 */
void log_date();

#endif /* DEBUG_H */
