
# Unit tests for COPRIS
# Possible targets:
#   - run-cmocka-standalone   run cmocka unit tests
#   - run-cmocka-prove        run cmocka unit tests through Perl's prove
#   - coverage                run cmocka unit tests and check sources coverage
#   - clean                   remove compiled object and binary files
#   - help                    show this text

include ../Makefile-common.mk
LIBRARIES += cmocka

# List of sources that have unit tests (sorted by approximate complexity)
TESTED_SOURCES = parse_value.c utf8.c stream_io.c socket_io.c

# List of mocked functions for unit tests
MOCKS = fgets isatty accept close getnameinfo inet_ntoa read write \
        printf fprintf puts fputs

# Tests build configuration
DEFINES = -DUNIT_TESTS -DBUFSIZE=10 -DMAX_INIFILE_ELEMENT_LENGTH=10

CFLAGS    += $(DBGFLAGS) $(DEFINES) -fno-builtin-printf -fno-builtin-fprintf \
             -fno-builtin-puts -fno-builtin-fputs
MOCKFLAGS := $(foreach MOCK,$(MOCKS),-Wl,--wrap=$(MOCK))

TEST_BINARIES  := $(TESTED_SOURCES:%.c=cmocka-%)
SOURCE_OBJECTS := $(OBJECTS:src/%.o=src_%.o)
TESTED_OBJECTS := $(TESTED_SOURCES:%.c=src_%.o)
TEST_GCDA      := $(TESTED_SOURCES:%.c=src_%.gcda)

.PHONY: all run-cmocka-standalone run-cmocka-prove coverage clean help
.SECONDARY: $(SOURCE_OBJECTS) cmocka-wrappers.o
all: run-cmocka-standalone

run-cmocka-standalone: $(TEST_BINARIES)
	for test in $^; do ./$$test -v -v -v || exit 1; done

run-cmocka-prove: $(TEST_BINARIES)
	CMOCKA_MESSAGE_OUTPUT=tap prove -f $(addprefix ./,$^)

# Test binaries
cmocka-%: $(SOURCE_OBJECTS) cmocka-%.o cmocka-wrappers.o
	$(CC) $(MOCKFLAGS) --coverage $^ $(LDFLAGS) -o $@

# Source objects with coverage instrumentation
src_%.o: ../src/%.c
	$(CC) $(CFLAGS) --coverage -c $< -o $@

# Tests objects
cmocka-%.o: cmocka-%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Test coverage
coverage: coverage_report/src/index.html
	#
	# Report generated at file://$(realpath $<)
	#

# Generate a HTML coverage report
coverage_report/src/index.html: coverage.info
	mkdir -p coverage_report/
	genhtml $< --legend --output-directory coverage_report/

# Generate a text coverage report, including only tested files
coverage.info: $(TEST_GCDA)
	lcov --capture --directory . --base-directory ../src --no-external --output-file $@ \
	$(foreach SRC,$(TESTED_SOURCES),--include $(realpath ../src/$(SRC)))

# Run instrumented test binaries with maximum verbosity
src_%.gcda: cmocka-%
	./$< -v -v -v

clean:
	rm -f $(TEST_BINARIES) $(SOURCE_OBJECTS) cmocka-wrappers.o
	rm -fr coverage_report/ coverage.info *.gcda *.gcno

help:
	head -n 9 $(firstword $(MAKEFILE_LIST))
	# Compiler flags for tests: $(CFLAGS)
