/*
 * Load printer feature file 'filename' into an internal hash table, passed on by 'prset'.
 * Return 0 on success or negative on failure, together with an error message.
 */
int load_printer_set_file(const char *filename, struct Inifile **prset);

/*
 * Print out all known printer commands in an INI-style format to stdout.
 * Return 0 on success or negative on failure, together with an error message.
 */
int dump_printer_set_commands(struct Inifile **prset);

/*
 * Unload internal printer feature hash table, passed on by 'prset'.
 */
void unload_printer_set_file(struct Inifile **prset);

/*
 * List of possible internal states that trigger session commands (see function below),
 */
typedef enum session {
	SESSION_PRINT,    /* A chunk of text is about to get printed  */
	SESSION_STARTUP,  /* COPRIS is starting up                    */
	SESSION_SHUTDOWN  /* COPRIS is shutting down                  */
} session_t;

/*
 * Prepend and append to 'copris_text' any session commands, passed on from 'prset'.
 * Session commands are read from printer feature file(s) and used for repetitive actions.
 * They are executed on various 'state's, contained in the above 'session_t' enumerated list.
 *
 * Return number of characters, appended to 'copris_text' (0 if none were added),
 * or negative on failure.
 */
int apply_session_commands(UT_string *copris_text, struct Inifile **prset, session_t state);
