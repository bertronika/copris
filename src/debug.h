#ifndef DEBUG_H
#define DEBUG_H

int raise_perror(int return_value, char *function_name, char *message);
int raise_errno_perror(int received_errno, char *function_name, char *message);

/*
 * Verbosity level       Argument
 * 0  silent/fatal only  -q
 * 1  error              (default)
 * 2  info               -v
 * 3  debug              -vv
 */

int log_error();
int log_info();
int log_debug();

void log_date();

extern int verbosity;

#endif /* DEBUG_H */
