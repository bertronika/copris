#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <utstring.h>

#include "../src/Copris.h"
#include "../src/stream_io.h"

int verbosity = 0;

static void expected_stats(size_t sizeof_bytes, int chunks)
{
	if (verbosity)
		printf("                        Expected %zu byte(s) in %d chunk(s) from stdin\n",
		       sizeof_bytes - 1, chunks);
}

// Read no text from stdin
static void stdin_read_no_text(void **state)
{
	UT_string *copris_text = *state;

	will_return(__wrap_fread, NULL); /* Signal an EOF */

	int no_text_read = copris_handle_stdin(copris_text);
	expected_stats(1, 0);

	assert_true(no_text_read);
	assert_string_equal(utstring_body(copris_text), "");
}

#define INPUT(str)                        \
        will_return(__wrap_fread, str);   \
        will_return(__wrap_fread, (sizeof str) - 1)
#define RESULT(str)                       \
        will_return(__wrap_fread, NULL);  \
        const char result[] = str
#define VERIFY                            \
        int error = copris_handle_stdin(copris_text); \
        expected_stats(sizeof result, 2); \
                                          \
        assert_false(error);              \
        assert_string_equal(utstring_body(copris_text), result)

// Read two chunks of ASCII text
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

	const struct CMUnitTest tests[] = {
		cmocka_unit_test_teardown(stdin_read_no_text, clear_utstring),
		cmocka_unit_test_teardown(read_two_chunks,  clear_utstring),
		cmocka_unit_test_teardown(read_2byte_char,  clear_utstring),
		cmocka_unit_test_teardown(read_3byte_char1, clear_utstring),
		cmocka_unit_test_teardown(read_3byte_char2, clear_utstring),
		cmocka_unit_test_teardown(read_4byte_char,  clear_utstring),
		cmocka_unit_test(read_with_null_value)
	};

	return cmocka_run_group_tests_name((__FILE__) + 7, tests, setup_utstring, teardown_utstring);
}
