#ifndef PRINTERSET_H
#define PRINTERSET_H

#define STATUS_ERROR   true
#define STATUS_SUCCESS false

// `Handler should return nonzero on success, zero on error.'
#define COPRIS_PARSE_FAILURE   0
#define COPRIS_PARSE_SUCCESS   1
#define COPRIS_PARSE_DUPLICATE 2

bool load_printer_set_file(char *filename, struct Inifile **prset);
void unload_printer_set_file(struct Inifile **prset);

#endif /* PRINTERSET_H */
