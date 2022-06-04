# Build system for COPRIS
# Possible targets (you don't need to specify one for a regular release build):
#	- all       synonym for `release'
#	- release   build the release build (executable name `copris')
#	- debug     build the debugging build (executable name `copris_dbg')
#	- clean     clean object, dependency, binary and test files
#	- help      print this text

# Run `make' with `WITHOUT_CMARK=1' to omit Markdown support.

# For code analysis purposes run:
#	- unit-tests              build and run unit tests
#	- analyse                 same as `debug', but compile with GCC's static analyser
#	- analyse-cppcheck        analyse codebase with Cppcheck, print results to stdout
#	- analyse-cppcheck-html   analyse codebase with Cppcheck, generate a HTML report

# GNU Make docs: https://www.gnu.org/software/make/manual/html_node/index.html

# Get latest version tag
VERSION := $(shell git describe --tags --dirty)

# Source, object and dependency files for release and debug builds
SOURCES = read_socket.c read_stdin.c writer.c translate.c printerset.c utf8.c \
          parse_value.c main.c
OBJS_REL := $(SOURCES:%.c=src/%_rel.o)
DEPS_REL := $(SOURCES:%.c=src/%_rel.d)
OBJS_DBG := $(SOURCES:%.c=src/%_dbg.o)
DEPS_DBG := $(SOURCES:%.c=src/%_dbg.d)

# Unit test files (executables will be prefixed with 'run_')
TESTS = test_read_socket_stdin.c test_utf8.c
TESTS_BINS := $(TESTS:%.c=tests/run_%)
TESTS_OBJS := $(filter-out src/main_dbg.o, $(OBJS_DBG))

# Dynamic libraries to be linked
LIBRARIES = inih

# Default common, release and build compiler flags
CFLAGS   ?= -Wall -Wextra -pedantic
RELFLAGS ?= -O2 -g -DNDEBUG
DBGFLAGS ?= -Og -g3 -ggdb -gdwarf -DDEBUG

# Determine if linking with libcmark is needed
ifndef WITHOUT_CMARK
	LIBRARIES += libcmark
	CFLAGS += -DW_CMARK
endif

CFLAGS  += $(shell pkg-config --cflags $(LIBRARIES)) -DVERSION=\"$(VERSION)\"
LDFLAGS += $(shell pkg-config --libs $(LIBRARIES))

# List of mocked functions for unit tests
MOCKS = fgets isatty accept close getnameinfo inet_ntoa read write

# Compiler flags for unit tests
TESTFLAGS := $(shell pkg-config --cflags --libs cmocka) $(DBGFLAGS) -DBUFSIZE=10 \
             $(foreach MOCK, $(MOCKS), -Wl,--wrap=$(MOCK))
# -Wconversion

# Cppcheck settings. Note that 'style' includes 'warning', 'performance' and 'portability'.
CPPCHECK_DIR   = cppcheck_report
CPPCHECK_XML   = $(CPPCHECK_DIR)/report.xml
CPPCHECK_FLAGS = --cppcheck-build-dir=$(CPPCHECK_DIR) --enable=style,information,missingInclude

# Targets that do not produce an eponymous file
.PHONY: all release debug analyse unit-tests analyse-cppcheck analyse-cppcheck-html clean help

all:     release
release: copris
debug:   copris_dbg
analyse: DBGFLAGS += -fanalyzer
analyse: debug

# Automatic variables of GNU Make:
# $@  The file name of the target of the rule.
# $<  The name of the first prerequisite.
# $^  The names of all the prerequisites, with spaces between them.

# Compile the release binary
copris: $(OBJS_REL)
	$(CC) $(LDFLAGS) $(CFLAGS) $(RELFLAGS) $^ -o $@

%_rel.o: %.c
	$(CC) $(CFLAGS) $(RELFLAGS) -MMD -MP -c $< -o $@

# Compile the debug binary
copris_dbg: $(OBJS_DBG)
	$(CC) $(LDFLAGS) $(CFLAGS) $(DBGFLAGS) $^ -o $@

%_dbg.o: %.c
	$(CC) $(CFLAGS) $(DBGFLAGS) -MMD -MP -c $< -o $@

# Compile and run tests
tests/run_%: tests/%.c tests/wrappers.c $(TESTS_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) $(TESTFLAGS) $^ -o $@

unit-tests: $(TESTS_BINS)
	for utest in $(TESTS_BINS); do ./$$utest; echo; done

# Run Cppcheck code analysis (first recipe prints to stdout, second generates a HTML report)
analyse-cppcheck:
	mkdir -p $(CPPCHECK_DIR)
	cppcheck $(CPPCHECK_FLAGS) src/

analyse-cppcheck-html: $(CPPCHECK_DIR)/index.html
$(CPPCHECK_DIR)/index.html: $(SOURCES)
	mkdir -p $(CPPCHECK_DIR)
	cppcheck $(CPPCHECK_FLAGS) --xml src/ 2>$(CPPCHECK_XML)
	cppcheck-htmlreport --file=$(CPPCHECK_XML) --report-dir=$(CPPCHECK_DIR)

# Automatic dependency tracking (-MMD -MP). If a header file is
# changed, only the files including it will be recompiled.
-include $(DEPS_REL) $(DEPS_DBG)

clean:
	rm -f $(OBJS_REL) $(DEPS_REL) copris
	rm -f $(OBJS_DBG) $(DEPS_DBG) copris_dbg
	rm -f $(TESTS_BINS)
	rm -fr $(CPPCHECK_DIR)

help:
	head -n 16 $(firstword $(MAKEFILE_LIST)); \
	grep -m 3 -C 1 -E '(CFLAGS|RELFLAGS|DBGFLAGS)' $(firstword $(MAKEFILE_LIST))
