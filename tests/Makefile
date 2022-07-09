
# Unit tests for COPRIS
# Possible targets:
#	- run_cmocka
#	- clean
#	- help

# List of sources that have unit tests
TESTED_SOURCES = parse_value.c read_stdin.c read_socket.c utf8.c

# List of mocked functions for unit tests
MOCKS = fgets isatty accept close getnameinfo inet_ntoa read write

# Tests build configuration
DEFINES    = -DDEBUG -DBUFSIZE=10 -DMAX_INIFILE_ELEMENT_LENGTH=10
CFLAGS     = -Wall -Wextra -Wstrict-prototypes -Wshadow -Wundef -pedantic \
             -Og -g3 -ggdb -gdwarf \
             $(DEFINES)
MOCKFLAGS := $(shell pkg-config --cflags --libs cmocka) \
             $(foreach MOCK,$(MOCKS),-Wl,--wrap=$(MOCK))

.PHONY: run_cmocka clean help
TESTED_OBJECTS := $(TESTED_SOURCES:%.c=test-%.o)

run_cmocka: tests_cmocka
	./$<

tests_cmocka: $(TESTED_OBJECTS) tests_cmocka.c wrappers.c
	$(CC) $(CFLAGS) $(MOCKFLAGS) $^ -o $@

test-%.o: ../src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f tests_cmocka $(TESTED_OBJECTS)

help:
	head -n 4 $(firstword $(MAKEFILE_LIST))
	# Compiler flags for tests: $(CFLAGS)

