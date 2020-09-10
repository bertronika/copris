# Makefile
# Part of COPRIS - a converting printer server

CC = gcc
CFLAGS = -Wall -Wextra
ODIR = obj
DEPS = debug.h server.h writer.h translate.h
OBJ = debug.o server.o writer.o translate.o copris.o


%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

copris: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -f *.o
	rm -f copris
