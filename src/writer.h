#ifndef WRITER_H
#define WRITER_H

#include <stdbool.h> /* bool */
#include <stddef.h> /* size_t */

bool copris_write_file(const char *dest, const char *text, size_t text_length);

#endif /* WRITER_H */
