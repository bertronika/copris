#ifndef PRINTERSET_H
#define PRINTERSET_H

/*
 * Load printer set file `filename' into a hash table, passed by `prset'.
 * Return 0 on success.
 */
int load_printer_set_file(const char *filename, struct Inifile **prset);

/*
 * Print out all printer commands, recognised by COPRIS, in an INI-style format.
 */
int dump_printer_set_commands(struct Inifile **prset);

/*
 * Unload printer set hash table, loaded from `filename', passed by `prset'.
 */
void unload_printer_set_file(const char *filename, struct Inifile **prset);

#endif /* PRINTERSET_H */
