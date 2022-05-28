#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <stdbool.h> /* bool */

#define COPRIS_PARSE_FAILURE   0
#define COPRIS_PARSE_SUCCESS   1
#define COPRIS_PARSE_DUPLICATE 2

bool load_translation_file(char *filename, struct Trfile **trfile);
void unload_translation_file(struct Trfile **trfile);

char *copris_translate(char *source, int source_len, struct Trfile **trfile);
// void copris_printerset(unsigned char *source, int source_len, unsigned char *ret, int set);

#endif /* TRANSLATE_H */
