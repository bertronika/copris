#ifndef TRANSLATE_H
#define TRANSLATE_H

int copris_loadtrfile(char *filename, struct Trfile **trfile);
int handler(void *user, const char *section, const char *name,
            const char *value);
void copris_unload_trfile(struct Trfile **trfile);

void copris_translate(unsigned char *source, int source_len, unsigned char *ret);
void copris_printerset(unsigned char *source, int source_len, unsigned char *ret, int set);
int escinsert(unsigned char *ret, int r, char *printerset);

#endif /* TRANSLATE_H */
