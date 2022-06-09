#ifndef PRINTERSET_H
#define PRINTERSET_H

// `Handler should return nonzero on success, zero on error.'
#define COPRIS_PARSE_FAILURE   0
#define COPRIS_PARSE_SUCCESS   1
#define COPRIS_PARSE_DUPLICATE 2

#define INSERT_TEXT(string) \
        utstring_bincpy(text, string, (sizeof string) - 1)

/*
 * Load printer set file `filename' into a hash table, passed by `prset'.
 * Return 0 on success.
 */
int load_printer_set_file(char *filename, struct Inifile **prset);

/*
 * Unload printer set hash table, passed by `prset'.
 */
void unload_printer_set_file(struct Inifile **prset);

/*
 * Take input text `copris_text' and convert the Markdown specifiers to printer's
 * escape codes, passed by `prset' hash table. Put converted text into `copris_text',
 * overwriting previous content.
 */
void convert_markdown(UT_string *copris_text, struct Inifile **prset);

#endif /* PRINTERSET_H */
