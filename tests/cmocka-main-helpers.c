#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <uthash.h>
#include <utstring.h>

#include "../src/Copris.h"
#include "../src/main-helpers.h"

int verbosity = 0;

static void test_append_file_name(void **state)
{
	(void)state;
	char *filenames[20];
	const char f0[] = "serial_hid_bu86x.c";
	const char f1[] = "serial_hid_ch9325.c";
	const char f2[] = "serial_hid_ch9325.c";
	int count = 0;

	append_file_name(f0, filenames, count++);
	append_file_name(f1, filenames, count++);
	append_file_name(f2, filenames, count++);

	assert_string_equal(filenames[0], f0);
	assert_string_equal(filenames[1], f1);
	assert_string_equal(filenames[2], f2);

	free_filenames(filenames, count);
}

int main(int argc, char **argv)
{
	// Number of (arbitrary) arguments sets the verbosity level
	verbosity = argc - 1;
	(void)argv;

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_append_file_name)
	};

	return cmocka_run_group_tests_name((__FILE__) + 7, tests, NULL, NULL);
}
