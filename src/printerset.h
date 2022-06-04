#ifndef PRINTERSET_H
#define PRINTERSET_H

// `Handler should return nonzero on success, zero on error.'
#define COPRIS_PARSE_FAILURE   0
#define COPRIS_PARSE_SUCCESS   1
#define COPRIS_PARSE_DUPLICATE 2

/*
 * Load printer set file `filename' into a hash table, passed by `prset'.
 * Return 0 on success.
 */
int load_printer_set_file(char *filename, struct Inifile **prset);

/*
 * Unload printer set hash table, passed by `prset'.
 */
void unload_printer_set_file(struct Inifile **prset);

#endif /* PRINTERSET_H */
