#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>
#include <stdbool.h>
#include <utstring.h>

#include "../src/config.h"
#include "../src/read_stdin.h"

int verbosity = 0;

// isatty;

// No text read from stdin
static void stdin_no_text(void **state)
{
	(void)state;

	UT_string *copris_text;
	utstring_new(copris_text);

	will_return(__wrap_fgets, NULL); /* Signal an EOF */

	bool no_text_read = copris_handle_stdin(copris_text);

	assert_true(no_text_read);
	assert_string_equal(utstring_body(copris_text), "");
}


// Read two chunks from stdin
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

	assert_false(no_text_read);
	assert_string_equal(utstring_body(copris_text), result);
}

static void stdin_multibyte_char(void **state)
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
		cmocka_unit_test(stdin_multibyte_char)
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
