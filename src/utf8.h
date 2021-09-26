#ifndef UTF8_H
#define UTF8_H

typedef uint32_t u8char_t;

#define UTF8_IS_CONTINUATION(c) ((c & 0xc0) == 0x80)

size_t utf8_count_codepoints(const char *s, size_t n);

#endif /* UTF8_H */
