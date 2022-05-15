#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>
#include <stdbool.h>
#include <utstring.h>

#include "../src/config.h"
#include "../src/read_stdin.h"

// Test expect the following buffer size:
#if BUFSIZE != 10
#	error "Buffer size set improperly for unit tests."
#endif

int verbosity = 0;

static void expected_stats(size_t sizeof_bytes, int chunks)
{
	if (verbosity)
		printf("Expected %zu byte(s) in %d chunk(s) from stdin\n", sizeof_bytes - 1, chunks);
}

// Read no text from stdin
static void stdin_no_text(void **state)
{
	(void)state;

	UT_string *copris_text;
	utstring_new(copris_text);

	will_return(__wrap_fgets, NULL); /* Signal an EOF */

	bool no_text_read = copris_handle_stdin(copris_text);
	expected_stats(1, 0);

	assert_true(no_text_read);
	assert_string_equal(utstring_body(copris_text), "");
}


// Read two chunks of ASCII text from stdin
static void stdin_two_chunks(void **state)
{
	(void)state;
	const char input_1[] = "aaaBBBccc";
	const char input_2[] = "DDD";
	const char result[]  = "aaaBBBcccDDD";

	UT_string *copris_text;
	utstring_new(copris_text);

	will_return(__wrap_fgets, input_1);
	will_return(__wrap_fgets, input_2);
	will_return(__wrap_fgets, NULL); /* Signal an EOF */

	bool no_text_read = copris_handle_stdin(copris_text);
	expected_stats(sizeof result, 2);

	assert_false(no_text_read);
	assert_string_equal(utstring_body(copris_text), result);
}

// Read a string with a 2-byte multibyte character split between two reads
static void stdin_multibyte_char2(void **state)
{
	(void)state;
	const char input_1[] = {'a', 'a', 'a', 'B', 'B', 'B', 'c' ,'c', '\xC4', '\0'};
	const char input_2[] = {'\x8D', '\0'};
	const char result[]  = "aaaBBBccč";

	UT_string *copris_text;
	utstring_new(copris_text);

	will_return(__wrap_fgets, input_1);
	will_return(__wrap_fgets, input_2);
	will_return(__wrap_fgets, NULL); /* Signal an EOF */

	bool no_text_read = copris_handle_stdin(copris_text);
	expected_stats(sizeof result, 2);

	assert_false(no_text_read);
	assert_string_equal(utstring_body(copris_text), result);
}

// Read a string with a 3-byte multibyte character split between two reads
static void stdin_multibyte_char3a(void **state)
{
	(void)state;
	const char input_1[] = {'a', 'a', 'a', 'B', 'B', 'B', 'c' ,'c', '\xE2', '\0'};
	const char input_2[] = {'\x82', '\xAC', '\0'};
	const char result[]  = "aaaBBBcc€";

	UT_string *copris_text;
	utstring_new(copris_text);

	will_return(__wrap_fgets, input_1);
	will_return(__wrap_fgets, input_2);
	will_return(__wrap_fgets, NULL); /* Signal an EOF */

	bool no_text_read = copris_handle_stdin(copris_text);
	expected_stats(sizeof result, 2);

	assert_false(no_text_read);
	assert_string_equal(utstring_body(copris_text), result);
}

// Read a string with a 3-byte multibyte character split between two reads
static void stdin_multibyte_char3b(void **state)
{
	(void)state;
	const char input_1[] = {'a', 'a', 'a', 'B', 'B', 'B', 'c', '\xE2', '\x82', '\0'};
	const char input_2[] = {'\xAC', '\0'};
	const char result[]  = "aaaBBBc€";

	UT_string *copris_text;
	utstring_new(copris_text);

	will_return(__wrap_fgets, input_1);
	will_return(__wrap_fgets, input_2);
	will_return(__wrap_fgets, NULL); /* Signal an EOF */

	bool no_text_read = copris_handle_stdin(copris_text);
	expected_stats(sizeof result, 2);

	assert_false(no_text_read);
	assert_string_equal(utstring_body(copris_text), result);
}

// Read a string with a 4-byte multibyte character split between two reads
static void stdin_multibyte_char4(void **state)
{
	(void)state;
	const char input_1[] = {'a', 'a', 'a', 'B', 'B', 'B', 'c' ,'c', '\xF0', '\0'};
	const char input_2[] = {'\x9F', '\x84', '\x8C', '\0'};
	const char result[]  = "aaaBBBcc🄌";

	UT_string *copris_text;
	utstring_new(copris_text);

	will_return(__wrap_fgets, input_1);
	will_return(__wrap_fgets, input_2);
	will_return(__wrap_fgets, NULL); /* Signal an EOF */

	bool no_text_read = copris_handle_stdin(copris_text);
	expected_stats(sizeof result, 2);

	assert_false(no_text_read);
	assert_string_equal(utstring_body(copris_text), result);
}

int main(int argc, char **argv)
{
	// Number of (arbitrary) arguments sets the verbosity level
	verbosity = argc - 1;
	(void)argv;

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(stdin_no_text),
		cmocka_unit_test(stdin_two_chunks),
		cmocka_unit_test(stdin_multibyte_char2),
		cmocka_unit_test(stdin_multibyte_char3a),
		cmocka_unit_test(stdin_multibyte_char3b),
		cmocka_unit_test(stdin_multibyte_char4)
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
