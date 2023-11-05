# Get latest version tag if in a tagged git repository, else from a local file
VERSION = $(shell git describe --dirty 2>/dev/null || cat VERSION)

# Default, overridable build flags (REL = release, DBG = debug)
USERFLAGS ?=
CFLAGS    ?= -Wall -Wextra -Wstrict-prototypes -Wshadow -Wundef -pedantic $(USERFLAGS)
RELFLAGS  ?= -O2 -g -DNDEBUG
DBGFLAGS  ?= -Og -g3 -ggdb -gdwarf -DDEBUG
LDFLAGS   ?=

# -Wconversion

# Dynamic libraries to be linked
LIBRARIES = inih

# Object files
OBJECTS = src/feature.o       \
          src/filters.o       \
          src/main-helpers.o  \
          src/markdown.o      \
          src/parse_value.o   \
          src/socket_io.o     \
          src/stream_io.o     \
          src/recode.o        \
          src/user_command.o  \
          src/utf8.o          \
          src/writer.o        \
          src/main.o          \

# Additional compiler and linker library flags
CFLAGS  += $(shell pkg-config --cflags $(LIBRARIES)) -DVERSION=\"$(VERSION)\"
LDFLAGS += $(shell pkg-config --libs $(LIBRARIES))
