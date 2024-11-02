#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <utstring.h>

#include "../src/Copris.h"
#include "../src/parse_value.h"

int verbosity = 0;

#define CALL_PARSE(value) \
        parse_values(value, parsed_value, (sizeof parsed_value) - 1)

// Check if a valid string value is parsed correctly
static void parse_values_correct(void **state)
{
	(void)state;
	char parsed_value[MAX_INIFILE_ELEMENT_LENGTH];

	// Check if numbers in various bases get properly parsed
	int count = CALL_PARSE("0102 101 0x72 0x74");
	assert_int_equal(count, 4);
	assert_memory_equal(parsed_value, "Bert", count);

	// Check if a NUL value gets properly parsed
	count = CALL_PARSE("0x01 0x00 0x01");
	assert_int_equal(count, 3);

	char raw_values[] = { 0x01, 0x00, 0x01 };
	assert_memory_equal(parsed_value, raw_values, count);
}

// Check if invalid string values throw errors
static void parse_values_erroneous(void **state)
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

#define ADD_ELEMENT(name, value) { \
  s = malloc(sizeof *s);           \
  strcpy(s->in, name);             \
  HASH_ADD_STR(features, in, s);   \
  strcpy(s->out, value);           \
  s->out_len = sizeof value - 1;   \
}

static void parse_values_with_variables_correct(void **state)
{
	(void)state;

	struct Inifile *features = NULL;
	struct Inifile *s;

	ADD_ELEMENT("C_1", "1");
	ADD_ELEMENT("C_2", "2");
	ADD_ELEMENT("C_3", "3");

	UT_string *parsed_value;
	utstring_new(parsed_value);

	int count = parse_values_with_variables("C_1 C_2 C_3", 11, parsed_value, &features);
	assert_int_equal(count, 3);
	assert_string_equal(utstring_body(parsed_value), "123");

	utstring_clear(parsed_value);

	count = parse_values_with_variables("C_1 0x61 C_2 0x62 C_3 0x63", 26, parsed_value, &features);
	assert_int_equal(count, 6);
	assert_string_equal(utstring_body(parsed_value), "1a2b3c");

	// Clear hash table
	struct Inifile *command;
	struct Inifile *tmp;
	HASH_ITER(hh, features, command, tmp) {
		HASH_DEL(features, command);
		free(command);
	}
}

static void parse_values_with_variables_erroneous(void **state)
{
	(void)state;

	struct Inifile *features = NULL;

	int error = parse_values_with_variables("NO-PREFIX", 9, NULL, &features);
	assert_int_equal(error, -1);

	error = parse_values_with_variables("C_AAAAAAAAAAAAAAA", 17, NULL, &features);
	assert_int_equal(error, -1);

	error = parse_values_with_variables("C_NOT", 9, NULL, &features);
	assert_int_equal(error, -1);
}

int main(int argc, char **argv)
{
	// Number of (arbitrary) arguments sets the verbosity level
	verbosity = argc - 1;
	(void)argv;

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(parse_values_correct),
		cmocka_unit_test(parse_values_erroneous),
		cmocka_unit_test(parse_values_with_variables_correct),
		cmocka_unit_test(parse_values_with_variables_erroneous),
	};

	return cmocka_run_group_tests_name("parse_value.c", tests, NULL, NULL);
}
