# Build system for COPRIS
# Possible targets:
#	- all/no arg  build the default target (`copris')
#	- release     build the release build
#	- debug       build the debugging build with extra symbols
#	- clean       clean object, dependency and binary files

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

# Dynamic library linking
LIBS := $(shell pkg-config --cflags --libs inih libcmark)

# C compiler flags
# CFLAGS := -Wall -Wextra -Wconversion -pedantic -MMD -MP -DREL=\"$(HASH)-$(BRANCH)\"
CFLAGS := $(LIBS) -Wall -Wextra -pedantic -MMD -MP -DREL=\"$(HASH)-$(BRANCH)\"

.PHONY: all release debug clean

all: debug
# all: release

release: CFLAGS += -O2 -s -DNDEBUG
release: $(BIN)

# debug: CFLAGS += -g -DREL=\"$(HASH)-$(BRANCH)\"
# *** Do not forget to recompile the whole project if switching to/from debug ***
debug: CFLAGS += -g3 -ggdb -gdwarf -Og -DDEBUG
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

# Automatic dependency tracking
-include $(DEP)

clean:
	$(RM) $(OBJ) $(DEP) $(BIN)
