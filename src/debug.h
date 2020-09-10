/* Header for the debug.c debugging and logging interfaces */

void log_perr(int errnum, char *perr, char *mesg);
int  log_err();
int  log_info();
int  log_debug();
void log_date();
void copris_help();
void copris_version();

extern int verbosity;
