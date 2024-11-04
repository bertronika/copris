#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <uthash.h>
#include <utstring.h>

#include "../src/Copris.h"
#include "../src/parse_vars.h"

int verbosity = 0;

#define MODELINE_TEST(str, ml)   \
  utstring_printf(text, str);    \
  ml_ret = parse_modeline(text); \
  assert_int_equal(ml_ret, ml);  \
  utstring_clear(text)

static void check_parse_modeline(void **state)
{
	(void)state;
	UT_string *text;
	utstring_new(text);

	modeline_t ml_ret;

	MODELINE_TEST("no modeline", NO_MODELINE);
	MODELINE_TEST("copri", NO_MODELINE);
	MODELINE_TEST("copris", ML_EMPTY);
	MODELINE_TEST("copris\n", ML_EMPTY);
	MODELINE_TEST("copris unknown", ML_UNKNOWN);
	MODELINE_TEST("copris enable-vars", ML_UNKNOWN | ML_ENABLE_VAR);
	MODELINE_TEST("copris disable-md", ML_UNKNOWN | ML_DISABLE_MD);
	MODELINE_TEST("COPRIS disable-md ENABLE-VARIABLES", ML_UNKNOWN | ML_DISABLE_MD | ML_ENABLE_VAR);
	MODELINE_TEST("Copris Enable-Vars Disable-Markdown", ML_UNKNOWN | ML_ENABLE_VAR | ML_DISABLE_MD);

	utstring_free(text);
}

#define APPLY_TEST(in, ml, out)                  \
  utstring_printf(text, in);                     \
  apply_modeline(text, ml);                      \
  assert_string_equal(utstring_body(text), out); \
  utstring_clear(text)

static void check_apply_modeline(void **state)
{
	(void)state;
	UT_string *text;
	utstring_new(text);

	APPLY_TEST("copris disable-md without newline", ML_DISABLE_MD, "");
	APPLY_TEST("copris enable-vars\nUntouched text.\n", ML_ENABLE_VAR, "Untouched text.\n");
	APPLY_TEST("copris unknown\nStill omitted.", ML_UNKNOWN, "Still omitted.");

	utstring_free(text);
}

int main(int argc, char **argv)
{
	// Number of (arbitrary) arguments sets the verbosity level
	verbosity = argc - 1;
	(void)argv;

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(check_parse_modeline),
		cmocka_unit_test(check_apply_modeline)
	};

	return cmocka_run_group_tests_name((__FILE__) + 7, tests, NULL, NULL);
}
