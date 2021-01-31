# Makefile
# Part of COPRIS - a converting printer server

CC = gcc
CFLAGS = -Wall -Wextra
ODIR = obj
DEPS = debug.h server.h writer.h translate.h printerset.h copris.h config.h
OBJ = debug.o server.o writer.o translate.o printerset.o copris.o
BIN = copris

.PHONY: all debug clean

all: debug

# You normally don't want to have a debug recipe like this,
# because you could mix debug and non-debug object files. This
# situation is all right though, as only debugger symbols are
# added and files are not modified.
debug: HASH = $(shell git rev-parse --short HEAD)
debug: BRANCH = $(shell git rev-parse --abbrev-ref HEAD)
debug: CFLAGS += -g -DREL=\"$(HASH)-$(BRANCH)\"
debug: copris

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJ)
	rm -f $(BIN)
