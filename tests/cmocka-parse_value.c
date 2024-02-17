#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <utstring.h>

#include "../src/parse_value.h"

int verbosity = 0;

#define CALL_PARSE(value) \
        parse_number_string(value, parsed_value, (sizeof parsed_value) - 1)

// Check if a valid string value is parsed correctly
static void parse_value_correct(void **state)
{
	(void)state;
	char parsed_value[MAX_INIFILE_ELEMENT_LENGTH];

	// Check if numbers in various bases get properly parsed
	int count = CALL_PARSE("0102 101 0x72 0x74");
	assert_int_equal(count, 4);
	assert_string_equal(parsed_value, "Bert");

	// Check if a NUL value gets properly parsed
	count = CALL_PARSE("0x01 0x00 0x01");
	assert_int_equal(count, 3);

	char raw_values[] = { 0x01, 0x00, 0x01 };
	assert_memory_equal(parsed_value, raw_values, count);
}

// Check if invalid string values throw errors
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

int main(int argc, char **argv)
{
	// Number of (arbitrary) arguments sets the verbosity level
	verbosity = argc - 1;
	(void)argv;

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(parse_value_correct),
		cmocka_unit_test(parse_value_erroneous),
	};

	return cmocka_run_group_tests_name("parse_value.c", tests, NULL, NULL);
}
