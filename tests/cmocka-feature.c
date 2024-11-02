#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <uthash.h>
#include <utstring.h>

#include "../src/Copris.h"
#include "../src/feature.h"
#include "../src/printer_commands.h"

int verbosity = 0;

static void duplicate_printer_commands(void **state)
{
	(void)state;

	struct Inifile *features = NULL;
	struct Inifile *s;

	for (int i = 0; printer_commands[i] != NULL; i++) {
		HASH_FIND_STR(features, printer_commands[i], s);
		if (s != NULL) {
			puts(printer_commands[i]);
			assert_true(false);
		}

		s = malloc(sizeof *s);
		memccpy(s->in, printer_commands[i], '\0', MAX_INIFILE_ELEMENT_LENGTH);
		HASH_ADD_STR(features, in, s);
	}

	struct Inifile *command;
	HASH_ITER(hh, features, command, s) {
		HASH_DEL(features, command);
		free(command);
	}
}

int main(int argc, char **argv)
{
	// Number of (arbitrary) arguments sets the verbosity level
	verbosity = argc - 1;
	(void)argv;

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(duplicate_printer_commands),
	};

	return cmocka_run_group_tests_name((__FILE__) + 7, tests, NULL, NULL);
}
