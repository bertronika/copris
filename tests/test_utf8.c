#include "cmocka-boilerplate.h"
#include "../src/utf8.h"
#include <cmocka.h>

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

// Check if remaining bytes of a multibyte character are counted properly
static void utf8_test_needed_bytes(void **state)
{
	(void)state;
	const char example2[] = {'\xC4', '\0'}; // beginning of č
	const char example3[] = {'\xE2', '\0'}; // beginning of €
	const char example4[] = {'\xF0', '\0'}; // beginning of 🄌

	size_t result2 = utf8_calculate_needed_bytes(example2, (sizeof example2) - 1);
	assert_int_equal(result2, 1);

	size_t result3 = utf8_calculate_needed_bytes(example3, (sizeof example3) - 1);
	assert_int_equal(result3, 2);

	size_t result4 = utf8_calculate_needed_bytes(example4, (sizeof example4) - 1);
	assert_int_equal(result4, 3);
}

int main(int argc, char **argv)
{
	// Number of (arbitrary) arguments sets the verbosity level
	verbosity = argc - 1;
	(void)argv;

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(utf8_test_multibyte_string_length),
		cmocka_unit_test(utf8_test_codepoint_length),
		cmocka_unit_test(utf8_test_needed_bytes),
	};

	puts("Test utf8.c");
	return cmocka_run_group_tests(tests, NULL, NULL);
}
