# Makefile
# Part of COPRIS - a converting printer server

CC = gcc
CFLAGS = -Wall -Wextra
ODIR = obj
DEPS = debug.h server.h writer.h translate.h printerset.h copris.h
OBJ = debug.o server.o writer.o translate.o printerset.o copris.o

.PHONY: all debug clean

all: copris

# You normally don't want to have a debug recipe like this,
# because you could mix debug and non-debug object files. This
# situation is all right though, as only debugger symbols are
# added and files are not modified.
debug: CFLAGS += -g
debug: copris

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

copris: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o
	rm -f copris
