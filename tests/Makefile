
# Unit tests for COPRIS
# Possible targets:
#   - run-cmocka-standalone   run cmocka unit tests
#   - run-cmocka-prove        run cmocka unit tests through Perl's prove
#   - coverage                run cmocka unit tests and check sources coverage
#   - clean                   remove compiled object and binary files
#   - help                    show this text

# Run with `WITHOUT_CMARK=1' to skip checking `printerset.c'.

VERSION := $(shell git describe --tags --dirty 2>/dev/null || cat ../VERSION)
include ../Makefile.common
LIBRARIES += cmocka

# List of sources that have unit tests (sorted by approximate complexity)
TESTED_SOURCES = parse_value.c utf8.c read_stdin.c read_socket.c

# List of mocked functions for unit tests
MOCKS = fgets isatty accept close getnameinfo inet_ntoa read write

# Tests build configuration
DEFINES = -DUNIT_TESTS -DBUFSIZE=10 -DMAX_INIFILE_ELEMENT_LENGTH=10

CFLAGS    += $(DBGFLAGS) $(DEFINES)
MOCKFLAGS := $(foreach MOCK,$(MOCKS),-Wl,--wrap=$(MOCK))

TEST_BINARIES  := $(TESTED_SOURCES:%.c=cmocka-%)
SOURCE_OBJECTS := $(filter-out src_main.o,$(SOURCES:%.c=src_%.o))
TESTED_OBJECTS := $(TESTED_SOURCES:%.c=src_%.o)
TEST_GCDA      := $(TESTED_SOURCES:%.c=src_%.gcda)

.PHONY: all run-cmocka-standalone run-cmocka-prove coverage clean help
.SECONDARY: $(SOURCE_OBJECTS) cmocka-wrappers.o
all: run-cmocka-standalone

run-cmocka-standalone: $(TEST_BINARIES)
	for test in $^; do ./$$test; done

run-cmocka-prove: $(TEST_BINARIES)
	CMOCKA_MESSAGE_OUTPUT=tap prove -f $(addprefix ./,$^)

# Test binaries
cmocka-%: $(SOURCE_OBJECTS) cmocka-%.o cmocka-wrappers.o
	$(CC) $(CFLAGS) $(MOCKFLAGS) --coverage $^ $(LDFLAGS) -o $@

# Source objects. Compile with the coverage flag if they are to be tested
# (this prevents lcov/genhtml telling us untested files have 0 % coverage).
src_%.o: ../src/%.c
	if [ "$(filter $@, $(TESTED_OBJECTS))" = "$@" ]; then \
		$(CC) $(CFLAGS) --coverage -c $< -o $@; \
	else \
		$(CC) $(CFLAGS) -c $< -o $@; \
	fi

# Tests objects
cmocka-%.o: cmocka-%.c
	$(CC) $(CFLAGS) -c $< -o $@

# coverage: coverage.info
coverage: coverage_report/index.html

src_%.gcda: cmocka-%
	./$<

coverage.info: $(TEST_GCDA)
	lcov --capture --base-directory ../src --no-external --directory . --output-file $@

coverage_report/index.html: coverage.info
	mkdir -p $(dir $@)
	genhtml $< --legend --output-directory $(dir $@)

clean:
	rm -f $(TEST_BINARIES) $(SOURCE_OBJECTS) cmocka-wrappers.o
	rm -fr coverage_report/ coverage.info *.gcda *.gcno

help:
	head -n 11 $(firstword $(MAKEFILE_LIST))
	# Compiler flags for tests: $(CFLAGS)
