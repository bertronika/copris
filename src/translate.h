extern unsigned char *input;
extern unsigned char *replacement;

int copris_trfile(char *filename);
void copris_translate(unsigned char *source, int source_len, unsigned char *ret);
void copris_printerset(unsigned char *source, int source_len, unsigned char *ret, int set);
int escinsert(unsigned char *ret, int r, char *printerset);
int power10(int exp);
