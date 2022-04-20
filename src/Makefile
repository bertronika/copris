# Build system for COPRIS
# Possible targets:
#	- all [no args]  synonym for `release'
#	- release        build the release build
#	- debug          build the debugging build with extra symbols
#	- unit-tests     build and run unit tests
#	- clean          clean object, dependency, binary and test files
#	- help           print this text

# The build system utilises automatic dependency tracking. If a header file
# is changed, only the files #include-ing it will be recompiled.

# *** Do not forget to recompile the whole project if switching to/from debug target.
#     You could mix debug and non-debug object files and get unexpected results in
#     program behaviour.

# https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html
# https://www.gnu.org/software/make/manual/html_node/Implicit-Variables.html

# Binary name
BIN = copris

# Last git commit hash
HASH := $(shell git rev-parse --short HEAD)

# Current git branch name
BRANCH := $(shell git rev-parse --abbrev-ref HEAD)

# Input source files, objects, dependencies
SRC  = debug.c server.c writer.c translate.c printerset.c utf8.c main.c
OBJ := $(SRC:.c=.o)
DEP := $(SRC:.c=.d)

# Unit test files
TESTS  = test_server
SRC_T := $(addsuffix .c, $(TESTS))

# Dynamic library linking
LIBS := $(shell pkg-config --cflags --libs inih libcmark)

# C compiler flags for release/debug builds and tests
COMMON  := -Wall -Wextra -pedantic -DREL="$(HASH)-$(BRANCH)"
CFLAGS  := $(LIBS) $(COMMON) -MMD -MP -O2 -s -DNDEBUG
TCFLAGS := $(shell pkg-config --cflags --libs cmocka) $(LIBS) $(COMMON) -Og -DDEBUG
# -Wconversion

.PHONY: all release debug unit-tests clean help

all: release
# all: debug

release: $(BIN)

debug: CFLAGS = $(LIBS) $(COMMON) -MMD -MP -Og -g3 -ggdb -gdwarf -DDEBUG
debug: $(BIN)

# Automatic variables of GNU Make:
# $@  The file name of the target of the rule.
# $<  The name of the first prerequisite.
# $^  The names of all the prerequisites, with spaces between them.

# Compile each object file, specified in $(OBJ)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile the final binary
$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile tests
$(TESTS): $(SRC_T)
	$(CC) $(TCFLAGS) -o $@ $^

# Run tests
unit-tests: $(TESTS)
	for utest in $(TESTS); do ./$$utest; done

# Automatic dependency tracking
-include $(DEP)

clean:
	$(RM) $(OBJ) $(DEP) $(BIN) $(TESTS)

help:
	head -n 16 $(firstword $(MAKEFILE_LIST))
