/*
 * Compile-time options for COPRIS
 *
 * Values, specified here, are used by default, apart from unit tests.
 */

// Code blocks denoted with four white spaces are disabled by default
// to avoid confusion with leading whitespace entering a code block.
// To enable this feature, comment-out or remove the following line:
#define DISABLE_WHITESPACE_CODE_BLOCK

// Inbound text buffer size
#ifndef BUFSIZE
#   define BUFSIZE 128
#endif

// Maximum lengths of name and value strings in .ini files
#ifndef MAX_INIFILE_ELEMENT_LENGTH
#   define MAX_INIFILE_ELEMENT_LENGTH 64
#endif

// Number of each encoding and printer feature files that
// can be loaded simultaneously
#ifndef NUM_OF_INPUT_FILES
#   define NUM_OF_INPUT_FILES 16
#endif

// Symbols for variable detection
#define VAR_SYMBOL     '$'
#define VAR_TERMINATOR ';'
#define VAR_COMMENT    '#'
#define VAR_SEPARATORS " \n\t!\"%&\'()*+,./:;<=>?@[\\]^`{|}~"
// removed: -_$#

// Uncomment the following line to overwrite the output
// file instead of appending to it:
// #define OVERWRITE_OUTPUT_FILE
