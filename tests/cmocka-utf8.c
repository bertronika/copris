#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <utstring.h> /* uthash library - dynamic strings */

#include "tests_cmocka.h"
#include "../src/config.h"
#include "../src/Copris.h"

#include "../src/parse_value.h"
#include "../src/read_stdin.h"
#include "../src/read_socket.h"
#include "../src/utf8.h"

int verbosity = 0;

// =========================== parse_value.c
#define CALL_PARSE(value) \
        parse_value_to_binary(value, parsed_value, (sizeof parsed_value) - 1)

static void parse_value_correct(void **state)
{
	(void)state;
	char parsed_value[MAX_INIFILE_ELEMENT_LENGTH];

	int count = CALL_PARSE("0102 101 0x72 0x74");
	assert_int_equal(count, 4);
	assert_string_equal(parsed_value, "Bert");
}

static void parse_value_erroneous(void **state)
{
	(void)state;
	char parsed_value[MAX_INIFILE_ELEMENT_LENGTH];

	int error = CALL_PARSE("0102 10P1"); // Erroneous 'P'
	assert_int_equal(error, -1);

	error = CALL_PARSE("0x70 486");      // '486' out of range
	assert_int_equal(error, -1);

	error = CALL_PARSE("0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74");
	assert_int_equal(error, -1);         // Too many numbers to parse
}


// =========================== read_stdin.c, read_socket.c

// Setup and teardown functions are called before and after all the grouped tests
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

static void expected_stats(size_t sizeof_bytes, int chunks)
{
	if (verbosity)
		printf("Expected %zu byte(s) in %d chunk(s) from stdin\n", sizeof_bytes - 1, chunks);
}

// Read no text from stdin
static void stdin_read_no_text(void **state)
{
	UT_string *copris_text = *state;

	will_return(__wrap_fgets, NULL); /* Signal an EOF */

	int no_text_read = copris_handle_stdin(copris_text);
	expected_stats(1, 0);

	assert_true(no_text_read);
	assert_string_equal(utstring_body(copris_text), "");
}

// Read two chunks of ASCII text
static void read_two_chunks(void **state)
{
	UT_string *copris_text = *state;
	const char input_1[] = "aaaBBBccc";
	const char input_2[] = "DDD";
	const char result[]  = "aaaBBBcccDDD";

	TEST_HANDLE_SOCKET(input_1, input_2, result);
	TEST_HANDLE_STDIN(input_1, input_2, result);
}

// Read a string with a 2-byte multibyte character split between two reads
static void read_multibyte_char2(void **state)
{
	UT_string *copris_text = *state;
	const char input_1[] = {'a', 'a', 'a', 'B', 'B', 'B', 'c' ,'c', '\xC4', '\0'};
	const char input_2[] = {'\x8D', '\0'};
	const char result[]  = "aaaBBBccč";

	TEST_HANDLE_SOCKET(input_1, input_2, result);
	TEST_HANDLE_STDIN(input_1, input_2, result);
}

// Read a string with a 3-byte multibyte character split between two reads, variant 1
static void read_multibyte_char3a(void **state)
{
	UT_string *copris_text = *state;
	const char input_1[] = {'a', 'a', 'a', 'B', 'B', 'B', 'c' ,'c', '\xE2', '\0'};
	const char input_2[] = {'\x82', '\xAC', '\0'};
	const char result[]  = "aaaBBBcc€";

	TEST_HANDLE_SOCKET(input_1, input_2, result);
	TEST_HANDLE_STDIN(input_1, input_2, result);
}

// Read a string with a 3-byte multibyte character split between two reads, variant 2
static void read_multibyte_char3b(void **state)
{
	UT_string *copris_text = *state;
	const char input_1[] = {'a', 'a', 'a', 'B', 'B', 'B', 'c', '\xE2', '\x82', '\0'};
	const char input_2[] = {'\xAC', '\0'};
	const char result[]  = "aaaBBBc€";

	TEST_HANDLE_SOCKET(input_1, input_2, result);
	TEST_HANDLE_STDIN(input_1, input_2, result);
}

// Read a string with a 4-byte multibyte character split between two reads
static void read_multibyte_char4(void **state)
{
	UT_string *copris_text = *state;
	const char input_1[] = {'a', 'a', 'a', 'B', 'B', 'B', 'c' ,'c', '\xF0', '\0'};
	const char input_2[] = {'\x9F', '\x84', '\x8C', '\0'};
	const char result[]  = "aaaBBBcc🄌";

	TEST_HANDLE_SOCKET(input_1, input_2, result);
	TEST_HANDLE_STDIN(input_1, input_2, result);
}

#define MUST_DISCARD 0x00

// Check if text limit gets applied properly
static void byte_limit_discard_and_cutoff(void **state)
{
	UT_string *copris_text = *state;
	const char input[]  = "aaaBBBccc";
	const char result[] = "aaaBB";

	TEST_HANDLE_SOCKET_LIMITED(input, "", 1, MUST_DISCARD);
	TEST_HANDLE_SOCKET_LIMITED(input, result, 5, MUST_CUTOFF);
}

// Check if multibyte characters are stripped instead of being partially send through
static void byte_limit_discard_and_cutoff2(void **state)
{
	UT_string *copris_text = *state;
	const char input1[] = "abcč"; // strlen("abcč") == 5
	const char result1[] = "abc";

	const char input2[] = "aaBBcc€"; // strlen("aaBBcc€") == 9
	const char result2[] = "aaBBcc";

	TEST_HANDLE_SOCKET_LIMITED(input1, "", 1, MUST_DISCARD);
	TEST_HANDLE_SOCKET_LIMITED(input1, result1, 4, MUST_CUTOFF);
	TEST_HANDLE_SOCKET_LIMITED(input2, result2, 8, MUST_CUTOFF);
}


// =========================== utf8.c

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

	utf8_terminate_incomplete_buffer(example2, (sizeof example2) - 1);
	assert_string_equal(example2, "");

	utf8_terminate_incomplete_buffer(example3, (sizeof example3) - 1);
	assert_string_equal(example3, "€");

	utf8_terminate_incomplete_buffer(example4, (sizeof example4) - 1);
	assert_string_equal(example4, "");

	utf8_terminate_incomplete_buffer(example5, (sizeof example5) - 1);
	assert_string_equal(example5, "hroš");

	utf8_terminate_incomplete_buffer(example6, (sizeof example6) - 1);
	assert_string_equal(example6, "50");
}

// ===========================
#define EXIT_ON_ERROR \
	    if (error) { return error; }

int main(int argc, char **argv)
{
	// Number of (arbitrary) arguments sets the verbosity level
	verbosity = argc - 1;
	(void)argv;

	int error = 0;

	const struct CMUnitTest t_parse_value[] = {
		cmocka_unit_test(parse_value_correct),
		cmocka_unit_test(parse_value_erroneous),
	};

	const struct CMUnitTest t_socket_stdin[] = {
		cmocka_unit_test(stdin_read_no_text),
		cmocka_unit_test(read_two_chunks),
		cmocka_unit_test(read_multibyte_char2),
		cmocka_unit_test(read_multibyte_char3a),
		cmocka_unit_test(read_multibyte_char3b),
		cmocka_unit_test(read_multibyte_char4),
		cmocka_unit_test(byte_limit_discard_and_cutoff),
		cmocka_unit_test(byte_limit_discard_and_cutoff2),
	};

	const struct CMUnitTest t_utf8[] = {
		cmocka_unit_test(utf8_test_multibyte_string_length),
		cmocka_unit_test(utf8_test_codepoint_length),
		cmocka_unit_test(utf8_test_incomplete_buffer)
	};

	error = cmocka_run_group_tests_name("Test parse_value.c", t_parse_value, NULL, NULL);
	EXIT_ON_ERROR;

	error = cmocka_run_group_tests_name("Test utf8.c", t_utf8, NULL, NULL);
	EXIT_ON_ERROR;

	error = cmocka_run_group_tests_name("Test read_socket.c and read_stdin.c",
	                                    t_socket_stdin, setup_utstring, teardown_utstring);
	EXIT_ON_ERROR;

	return error;
}
