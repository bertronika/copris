#ifndef UTF8_H
#define UTF8_H

/*
 * UTF-8 Encoding
 *
 * Binary    Hex          Comment
 * 0xxxxxxx  0x00..0x7F   Only byte of a 1-byte character encoding
 * 10xxxxxx  0x80..0xBF   Continuation byte: one of 1-3 bytes following the first
 * 110xxxxx  0xC0..0xDF   First byte of a 2-byte character encoding
 * 1110xxxx  0xE0..0xEF   First byte of a 3-byte character encoding
 * 11110xxx  0xF0..0xF7   First byte of a 4-byte character encoding
 *
 *  Range           Byte 1   Byte 2   Byte 3   Byte 4
 *   0000 -   007F  0xxxxxxx
 *   0080 -   07FF  110xxxxx 10xxxxxx
 *   0800 -   FFFF  1110xxxx 10xxxxxx 10xxxxxx
 *  10000 - 10FFFF  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 */

// Maximum length of a multibyte character
#define UTF8_MAX_LENGTH 4

#define UTF8_IS_CONTINUATION(c) ((c & 0xC0) == 0x80)
#define UTF8_IS_MULTIBYTE(c)    ((c & 0xC0) == 0xC0)

/*
 * Count the number of characters in string `s' of length `n', where any character
 * can be from 1 to 4 bytes long.
 * Return number of counted characters.
 */
size_t utf8_count_codepoints(const char *s, size_t n);

/*
 * Determine the byte length of a (multibyte) character `s' by analysing its first byte.
 * Return byte length of the character.
 */
size_t utf8_codepoint_length(const char s);

/*
 * Check for incomplete multibyte characters in input string `str' of length `len'. If
 * any found, terminate the string before letting them (and any following text) through.
 * Return -1 if buffer was prematurely terminated, 0 otherwise.
 */
int utf8_terminate_incomplete_buffer(char *str, size_t len);

#endif /* UTF8_H */
