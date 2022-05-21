#ifndef WRITER_H
#define WRITER_H

#include <stdbool.h>  /* bool      */
#include <stddef.h>   /* size_t    */
#include <utstring.h> /* UT_string */

bool copris_write_file(const char *dest, UT_string *copris_text);

#endif /* WRITER_H */
