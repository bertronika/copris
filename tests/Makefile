
# Unit tests for COPRIS
# Possible targets:
#   - run-cmocka-standalone   run cmocka unit tests
#   - run-cmocka-prove        run cmocka unit tests through Perl's prove
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
SOURCE_OBJECTS := $(filter-out src_main.o,$(OBJECTS:src/%.o=src_%.o))
TESTED_OBJECTS := $(TESTED_SOURCES:%.c=src_%.o)

.PHONY: all run-cmocka-standalone run-cmocka-prove clean help
.SECONDARY: $(SOURCE_OBJECTS) cmocka-wrappers.o
all: run-cmocka-standalone

run-cmocka-standalone: $(TEST_BINARIES)
	for test in $^; do ./$$test -v -v -v || exit 1; done

run-cmocka-prove: $(TEST_BINARIES)
	CMOCKA_MESSAGE_OUTPUT=tap prove -f $(addprefix ./,$^)

# Test binaries
cmocka-%: $(SOURCE_OBJECTS) cmocka-%.o cmocka-wrappers.o
	$(CC) $(MOCKFLAGS) $^ $(LDFLAGS) -o $@

# Source objects
src_%.o: ../src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Tests objects
cmocka-%.o: cmocka-%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TEST_BINARIES) $(SOURCE_OBJECTS) cmocka-wrappers.o

help:
	head -n 8 $(firstword $(MAKEFILE_LIST))
	# Compiler flags for tests: $(CFLAGS)
