#ifndef WRITER_H
#define WRITER_H

/*
 * Write text from `copris_text' to output file `dest'.
 * Return 0 on success.
 */
int copris_write_file(const char *dest, UT_string *copris_text);

#endif /* WRITER_H */
