
# Unit tests for COPRIS
# Possible targets:
#   - run-cmocka-standalone   run cmocka unit tests
#   - run-cmocka-prove        run cmocka unit tests through Perl's prove
#   - clean                   remove compiled object and binary files
#   - help                    show this text

# List of sources that have unit tests (sorted by approximate complexity)
TESTED_SOURCES = parse_value.c utf8.c read_stdin.c read_socket.c

# List of mocked functions for unit tests
MOCKS = fgets isatty accept close getnameinfo inet_ntoa read write

# Tests build configuration
DEFINES = -DDEBUG -DUNIT_TESTS -DBUFSIZE=10 -DMAX_INIFILE_ELEMENT_LENGTH=10
CFLAGS  = -Wall -Wextra -Wstrict-prototypes -Wshadow -Wundef -pedantic \
          -Og -g3 -ggdb -gdwarf \
          $(DEFINES)
MOCKFLAGS := $(shell pkg-config --cflags --libs cmocka) \
             $(foreach MOCK,$(MOCKS),-Wl,--wrap=$(MOCK))

TESTED_OBJECTS := $(TESTED_SOURCES:%.c=src_%.o)
TEST_BINARIES  := $(TESTED_SOURCES:%.c=cmocka-%)

.PHONY: all run_cmocka clean help
# .SECONDARY: $(TESTED_OBJECTS)
all: run-cmocka-standalone

run-cmocka-standalone: $(TEST_BINARIES)
	for test in $^; do ./$$test; done

run-cmocka-prove: $(TEST_BINARIES)
	CMOCKA_MESSAGE_OUTPUT=tap prove -f $(addprefix ./,$^)

# Test binaries
cmocka-%: cmocka-%.c src_%.o wrappers.o
	$(CC) $(CFLAGS) $(MOCKFLAGS) $^ -o $@

# Additional prerequisites
cmocka-read_socket: src_utf8.o

# Source objects to be tested
src_%.o: ../src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Mocked function wrappers
wrappers.o: wrappers.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TEST_BINARIES) wrappers.o src_utf8.o

help:
	head -n 14 $(firstword $(MAKEFILE_LIST))
	# Compiler flags for tests: $(CFLAGS)
