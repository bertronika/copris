# Get latest version tag if in a tagged git repository, else from a local file
VERSION = $(shell git describe --tags --dirty 2>/dev/null || cat VERSION)

# Default, overridable build flags (REL = release, DBG = debug)
CFLAGS   ?= -Wall -Wextra -Wstrict-prototypes -Wshadow -Wundef -pedantic
RELFLAGS ?= -O2 -g -DNDEBUG
DBGFLAGS ?= -Og -g3 -ggdb -gdwarf -DDEBUG
LDFLAGS  ?=

# -Wconversion

# Dynamic libraries to be linked
LIBRARIES = inih

SOURCES = utf8.c writer.c parse_value.c read_stdin.c translate.c read_socket.c \
          printerset.c markdown.c main.c

# Additional compiler and linker library flags
CFLAGS  += $(shell pkg-config --cflags $(LIBRARIES)) -DVERSION=\"$(VERSION)\"
LDFLAGS += $(shell pkg-config --libs $(LIBRARIES))
