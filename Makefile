# Build system for COPRIS
# Possible targets (you don't need to specify one for a regular release build):
#	- all          synonym for `release'
#	- release      build the release build (executable name `copris')
#	- debug        build the debugging build (executable name `copris_dbg')
#	- unit-tests   build and run unit tests
#	- clean        clean object, dependency, binary and test files
#	- help         print this text

# Run `make' with `WITHOUT_CMARK=1' to omit Markdown support.
# GNU Make docs: https://www.gnu.org/software/make/manual/html_node/index.html

# Release and debug binary names
BIN_REL = copris
BIN_DBG = copris_dbg

# Source and tests directories
SRCDIR  = src
TESTDIR = tests_cmocka

# Last git commit hash and current branch name
HASH   := $(shell git rev-parse --short HEAD)
BRANCH := $(shell git rev-parse --abbrev-ref HEAD)

# Source, object and dependency files for release and debug builds
SRC = debug.c server.c writer.c translate.c printerset.c utf8.c main.c
OBJ_REL := $(SRC:%.c=$(SRCDIR)/%_rel.o)
DEP_REL := $(SRC:%.c=$(SRCDIR)/%_rel.d)
OBJ_DBG := $(SRC:%.c=$(SRCDIR)/%_dbg.o)
DEP_DBG := $(SRC:%.c=$(SRCDIR)/%_dbg.d)

# Unit test files
TESTS = test_server.c
SRC_TEST := $(TESTS:%=$(TESTDIR)/%)
BIN_TEST := $(TESTS:%.c=$(TESTDIR)/%)

# Dynamic libraries to be linked
LIBS = inih

# Check if Markdown support was explicitly disabled
WITHOUT_CMARK ?= 0
EXTRADEF =
ifeq ($(WITHOUT_CMARK), 0)
	LIBS += libcmark
else
	EXTRADEF = -DWITHOUT_CMARK
endif

# Common, release, debug and test build C compiler flags
CFLAGS    := $(shell pkg-config --cflags --libs $(LIBS)) -Wall -Wextra -pedantic $(EXTRADEF)
RELFLAGS  := $(CFLAGS) -MMD -MP -O2 -DNDEBUG -s
DBGFLAGS  := $(CFLAGS) -MMD -MP -Og -DDEBUG -g3 -ggdb -gdwarf -DREL=\"$(HASH)-$(BRANCH)\"
TESTFLAGS := $(shell pkg-config --cflags --libs cmocka) $(CFLAGS) -Og -DDEBUG
# -Wconversion

# Targets that do not produce an eponyomus file
.PHONY: all release debug unit-tests clean help

# all:     debug
all:     release
release: $(BIN_REL)
debug:   $(BIN_DBG)

# Automatic variables of GNU Make:
# $@  The file name of the target of the rule.
# $<  The name of the first prerequisite.
# $^  The names of all the prerequisites, with spaces between them.

# Compile the release binary (`copris')
$(BIN_REL): $(OBJ_REL)
	$(CC) $(RELFLAGS) -o $@ $^

%_rel.o: %.c
	$(CC) $(RELFLAGS) -c -o $@ $<

# Compile the debug binary (`copris_dbg')
$(BIN_DBG): $(OBJ_DBG)
	$(CC) $(DBGFLAGS) -o $@ $^

%_dbg.o: %.c
	$(CC) $(DBGFLAGS) -c -o $@ $<

# Compile and run tests
$(BIN_TEST): $(SRC_TEST)
	$(CC) $(TESTFLAGS) -o $@ $^

unit-tests: $(BIN_TEST)
	for utest in $(BIN_TEST); do ./$$utest; done

# Automatic dependency tracking (-MMD -MP). If a header file is
# changed, only the files including it will be recompiled.
-include $(DEP_REL) $(DEP_DBG)

clean:
	$(RM) $(OBJ_REL) $(DEP_REL) $(OBJ_DBG) $(DEP_DBG) $(BIN_REL) $(BIN_DBG) $(BIN_TEST)

help:
	head -n 10 $(firstword $(MAKEFILE_LIST))
