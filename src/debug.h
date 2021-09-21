#ifndef DEBUG_H
#define DEBUG_H

int log_perr(int return_value, char *function_name, char *message);
int log_errno_perror(int received_errno, char *function_name, char *message);

int  log_err();
int  log_info();
int  log_debug();
void log_date();

extern int verbosity;

#endif /* DEBUG_H */
