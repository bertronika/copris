#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <utstring.h>

#include "../src/Copris.h"
#include "../src/utf8.h"

int verbosity = 0;

// Check if multibyte string's length is counted properly
static void utf8_test_multibyte_string_length(void **state)
{
	(void)state;
	const char string[] = "Račun znaša 9,49 €";

	size_t string_length = utf8_count_codepoints(string, (sizeof string) - 1);
	assert_int_equal(string_length, 18);
}

// Check if multibyte characters' number of bytes is counted properly
static void utf8_test_codepoint_length(void **state)
{
	(void)state;
	const char example2[] = "č";  // printf 'č' | wc -c: 2
	const char example3[] = "€";  // printf '€' | wc -c: 3
	const char example4[] = "🄌"; // printf '🄌' | wc -c: 4

	size_t result2 = utf8_codepoint_length(*example2);
	assert_int_equal(result2, 2);

	size_t result3 = utf8_codepoint_length(*example3);
	assert_int_equal(result3, 3);

	size_t result4 = utf8_codepoint_length(*example4);
	assert_int_equal(result4, 4);
}

static void utf8_test_incomplete_buffer(void **state)
{
	(void)state;
	char example2[] = {'\xC4', '\0'}; // first byte of č
	char example3[] = {'\xE2', '\x82', '\xAC', '\0'}; // complete €
	char example4[] = {'\xF0', '\x9f', '\0'}; // first and second bytes of 🄌

	// Missing last byte
	char example5[] = {'h', 'r', 'o', '\xC5', '\xA1', '\xC4', '\0'}; // "hrošč"
	char example6[] = {'5', '0', '\xE2', '\x82', '\0'}; // "50€"

	int was_terminated = utf8_terminate_incomplete_buffer(example2, (sizeof example2) - 1);
	assert_string_equal(example2, "");
	assert_true(was_terminated);

	was_terminated = utf8_terminate_incomplete_buffer(example3, (sizeof example3) - 1);
	assert_string_equal(example3, "€");
	assert_false(was_terminated);

	was_terminated = utf8_terminate_incomplete_buffer(example4, (sizeof example4) - 1);
	assert_string_equal(example4, "");
	assert_true(was_terminated);

	was_terminated = utf8_terminate_incomplete_buffer(example5, (sizeof example5) - 1);
	assert_string_equal(example5, "hroš");
	assert_true(was_terminated);

	was_terminated = utf8_terminate_incomplete_buffer(example6, (sizeof example6) - 1);
	assert_string_equal(example6, "50");
	assert_true(was_terminated);
}

int main(int argc, char **argv)
{
	// Number of (arbitrary) arguments sets the verbosity level
	verbosity = argc - 1;
	(void)argv;

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(utf8_test_multibyte_string_length),
		cmocka_unit_test(utf8_test_codepoint_length),
		cmocka_unit_test(utf8_test_incomplete_buffer)
	};

	return cmocka_run_group_tests_name("utf8.c", tests, NULL, NULL);
}
