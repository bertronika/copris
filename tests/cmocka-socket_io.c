#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <utstring.h>

#include "../src/Copris.h"
#include "../src/socket_io.h"

int verbosity = 0;

int parentfd = 0;
int childfd  = 0;
struct Attribs attrib;

static void expected_stats(size_t sizeof_bytes, int chunks)
{
	if (verbosity)
		printf("Expected %zu byte(s) in %d chunk(s) from stdin\n", sizeof_bytes - 1, chunks);
}

#define INPUT(str)                            \
        will_return(__wrap_read, str);        \
        will_return(__wrap_read, (sizeof str) - 1)
#define RESULT(str)                           \
        will_return_maybe(__wrap_read, NULL); \
        const char result[] = str
#define VERIFY                                \
        int error = copris_handle_socket(copris_text, &parentfd, &childfd, &attrib); \
        expected_stats(sizeof result, 2);     \
                                              \
        assert_false(error);                  \
        assert_string_equal(utstring_body(copris_text), result)

// Read two chunks of ASCII text.
// This function is written without any macros, ones below use them.
static void read_two_chunks(void **state)
{
	UT_string *copris_text = *state;
	INPUT("aaaBBBccc");
	INPUT("DDD");
	RESULT("aaaBBBcccDDD");

	VERIFY;
}

// Read a string with a 2-byte multibyte character split between two reads
static void read_2byte_char(void **state)
{
	UT_string *copris_text = *state;

	INPUT("aaaBBBcc\xC4");
	INPUT("\x8D");
	RESULT("aaaBBBccč");

	VERIFY;
}

// Read a string with a 3-byte multibyte character split between two reads, variant 1
static void read_3byte_char1(void **state)
{
	UT_string *copris_text = *state;

	INPUT("aaaBBBcc\xE2");
	INPUT("\x82\xAC");
	RESULT("aaaBBBcc€");

	VERIFY;
}

// Read a string with a 3-byte multibyte character split between two reads, variant 2
static void read_3byte_char2(void **state)
{
	UT_string *copris_text = *state;

	INPUT("aaaBBBc\xE2\x82");
	INPUT("\xAC");
	RESULT("aaaBBBc€");

	VERIFY;
}

// Read a string with a 4-byte multibyte character split between two reads
static void read_4byte_char(void **state)
{
	UT_string *copris_text = *state;

	INPUT("aaaBBBcc\xF0");
	INPUT("\x9F\x84\x8C");
	RESULT("aaaBBBcc🄌");

	VERIFY;
}

// Read a string with a null value
static void read_with_null_value(void **state)
{
	UT_string *copris_text = *state;

	INPUT ("aaa\0bbb");
	RESULT("aaa\0bbb");

	VERIFY;
}

#define MUST_DISCARD 0x00

// Check if text gets discarded properly
static void byte_limit_discard(void **state)
{
	UT_string *copris_text = *state;
	INPUT ("aaaBBBccc");
	RESULT("");

	attrib.copris_flags = MUST_DISCARD;
	attrib.limitnum     = 4;

	VERIFY;
}

static void byte_limit_discard_not(void **state)
{
	UT_string *copris_text = *state;
	INPUT ("aaaBBBccc");
	RESULT("aaaBBBccc");

	attrib.copris_flags = MUST_DISCARD;
	attrib.limitnum     = 9;

	VERIFY;
}

// Check if text gets cut off properly
static void byte_limit_cutoff(void **state)
{
	UT_string *copris_text = *state;
	INPUT ("aaaBBBccc");
	RESULT("aaaBB");

	attrib.copris_flags = MUST_CUTOFF;
	attrib.limitnum     = 5;

	VERIFY;
}

static void byte_limit_cutoff_not(void **state)
{
	UT_string *copris_text = *state;
	INPUT ("aaaBBBccc");
	RESULT("aaaBBBccc");

	attrib.copris_flags = MUST_CUTOFF;
	attrib.limitnum     = 9;

	VERIFY;
}

// Check if multibyte characters are stripped instead of being partially send through
static void byte_limit_cutoff_multibyte1(void **state)
{
	UT_string *copris_text = *state;
	INPUT ("abcč"); // strlen("abcč") == 5
	RESULT("abc");

	attrib.copris_flags = MUST_CUTOFF;
	attrib.limitnum     = 4;

	VERIFY;
}

static void byte_limit_cutoff_multibyte2(void **state)
{
	UT_string *copris_text = *state;
	INPUT ("aaBBcc€"); // strlen("aaBBcc€") == 9
	RESULT("aaBBcc");

	attrib.copris_flags = MUST_CUTOFF;
	attrib.limitnum     = 8;

	VERIFY;
}


static int setup_utstring(void **state)
{
	UT_string *copris_text;
	utstring_new(copris_text);

	*state = copris_text;
	return 0;
}

static int teardown_utstring(void **state)
{
	UT_string *copris_text = *state;
	utstring_free(copris_text);

	return 0;
}

static int clear_utstring(void **state)
{
	UT_string *copris_text = *state;
	utstring_clear(copris_text);

	return 0;
}

int main(int argc, char **argv)
{
	// Number of (arbitrary) arguments sets the verbosity level
	verbosity = argc - 1;
	(void)argv;

	attrib.daemon       = false;
	attrib.limitnum     = 0;
	attrib.copris_flags = 0x00;

	const struct CMUnitTest tests[] = {
		cmocka_unit_test_teardown(read_two_chunks,              clear_utstring),
		cmocka_unit_test_teardown(read_2byte_char,              clear_utstring),
		cmocka_unit_test_teardown(read_3byte_char1,             clear_utstring),
		cmocka_unit_test_teardown(read_3byte_char2,             clear_utstring),
		cmocka_unit_test_teardown(read_4byte_char,              clear_utstring),
		cmocka_unit_test_teardown(read_with_null_value,         clear_utstring),
		cmocka_unit_test_teardown(byte_limit_discard,           clear_utstring),
		cmocka_unit_test_teardown(byte_limit_discard_not,       clear_utstring),
		cmocka_unit_test_teardown(byte_limit_cutoff,            clear_utstring),
		cmocka_unit_test_teardown(byte_limit_cutoff_not,        clear_utstring),
		cmocka_unit_test_teardown(byte_limit_cutoff_multibyte1, clear_utstring),
		cmocka_unit_test(         byte_limit_cutoff_multibyte2)
	};

	return cmocka_run_group_tests_name((__FILE__) + 7, tests, setup_utstring, teardown_utstring);
}
