#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <uthash.h>
#include <utstring.h>

#include "../src/Copris.h"
#include "../src/recode.h"

int verbosity = 0;
struct Inifile *encoding = NULL;

static void load_definitions(void **state)
{
	(void)state;

	int error = load_encoding_file("cmocka-recode.ini", &encoding);
	assert_false(error);
}

static void unload_definitions(void **state)
{
	(void)state;

	// No tests for this one, only runtime check
	unload_encoding_definitions(&encoding);
}

static void recode(void **state)
{
	(void)state;
	UT_string *text;
	utstring_new(text);
	utstring_printf(text, "čAžBšC");

	int error = recode_text(text, &encoding);
	assert_false(error);

	assert_string_equal(utstring_body(text), "cAzBsC");

	utstring_free(text);
}

int main(int argc, char **argv)
{
	// Number of (arbitrary) arguments sets the verbosity level
	verbosity = argc - 1;
	(void)argv;

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(load_definitions),
		cmocka_unit_test(recode),
		cmocka_unit_test(unload_definitions),
	};

	return cmocka_run_group_tests_name((__FILE__) + 7, tests, NULL, NULL);
}
