/*
 * Load printer set file 'filename' into a hash table, passed by 'prset'.
 * Return 0 on success.
 */
int load_printer_set_file(const char *filename, struct Inifile **prset);

/*
 * Print out all printer commands, recognised by COPRIS, in an INI-style format.
 */
int dump_printer_set_commands(struct Inifile **prset);

/*
 * Unload printer set hash table, passed on by 'prset'.
 */
void unload_printer_set_file(struct Inifile **prset);

/*
 * Prepend and append any session commands to 'copris_text', present in 'prset'.
 */
void apply_session_commands(UT_string *copris_text, struct Inifile **prset);
