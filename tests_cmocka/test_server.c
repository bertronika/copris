#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

// #include "../src/server.h"

static void integers(void **state) {
	assert_int_equal(15, 15);
}

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(integers)

	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
