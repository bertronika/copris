/* Header for the translate.c file processing and code table translations */

void copris_trfile(char *filename);
/* TODO: make this cleaner */
unsigned char *copris_translate(unsigned char *source, 
								int source_len, 
								unsigned char *input, 
								unsigned char *replacement);
int power10(int exp);

extern unsigned char *input;
extern unsigned char *replacement;
