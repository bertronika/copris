#ifndef DEBUG_H
#define DEBUG_H

int  log_perr(int errnum, char *perr, char *mesg);
int  log_err();
int  log_info();
int  log_debug();
void log_date();

extern int verbosity;

#endif /* DEBUG_H */
