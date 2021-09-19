#ifndef TRANSLATE_H
#define TRANSLATE_H

int copris_loadtrfile(char *filename);
void copris_translate(unsigned char *source, int source_len, unsigned char *ret);
void copris_printerset(unsigned char *source, int source_len, unsigned char *ret, int set);
int escinsert(unsigned char *ret, int r, char *printerset);
int power10(int exp);

#endif /* TRANSLATE_H */
