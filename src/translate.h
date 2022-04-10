#ifndef TRANSLATE_H
#define TRANSLATE_H

int copris_loadtrfile(char *filename, struct Trfile **trfile);
void copris_unload_trfile(struct Trfile **trfile);

char *copris_translate(char *source, int source_len, struct Trfile **trfile);
// void copris_printerset(unsigned char *source, int source_len, unsigned char *ret, int set);

#endif /* TRANSLATE_H */
