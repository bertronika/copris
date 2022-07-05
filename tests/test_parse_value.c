#include "cmocka-boilerplate.h"
#include "../src/parse_value.h"
#include <cmocka.h>
#include <limits.h>

#define CALL_PARSE(value) \
        parse_value_to_binary(value, parsed_value, (sizeof parsed_value) - 1)

int verbosity = 0;

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

	int count = CALL_PARSE("0102 10P1");
	assert_int_equal(count, -1);

	count = CALL_PARSE("0x70 486");
	assert_int_equal(count, -1);

	count = CALL_PARSE("0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74 0x74");
	assert_int_equal(count, -1);
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

	puts("Test parse_value.c");
	return cmocka_run_group_tests(tests, NULL, NULL);
}
