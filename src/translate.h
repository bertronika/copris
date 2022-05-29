#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <stdbool.h>  /* bool      */
#include <utstring.h> /* UT_string */

// `Handler should return nonzero on success, zero on error.'
#define COPRIS_PARSE_FAILURE   0
#define COPRIS_PARSE_SUCCESS   1
#define COPRIS_PARSE_DUPLICATE 2

bool load_translation_file(char *filename, struct Inifile **trfile);
void unload_translation_file(struct Inifile **trfile);

void translate_text(UT_string *copris_text, struct Inifile **trfile);

#endif /* TRANSLATE_H */
