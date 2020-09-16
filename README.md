# COPRIS - a converting printer server

COPRIS is a multifunction printer server, made primarily for sending raw text
to a dot-matrix printer over a network, while performing optional text
conversion.

COPRIS differs from CUPS. Not just by being smaller with less features, it has
a different objective. Sure, CUPS is also a server, but it doesn't provide
neither character conversion nor it has some bonus features, described later.

## Why convert characters?
In the late 1980's and 90's, when dot-matrix printers were getting affordable
and common in offices and homes, no character set standards were defined for
them outside of the standard ASCII. The same was for earlier versions of DOS,
which included only a handful of code pages. Languages, which required 
additional letters to be written properly, had to be incorporated in some way.
The solution was to sacrifice some of the keys of the American keyboard.
Characters, such as `~ @ [`, would print out as `č Ž Š`, so the user
could still type them out without requiring a special keyboard.

COPRIS automatically converts such characters by using *Translation files*,
that specify which should be replaced before being sent to the printer.

Example: `čas` (time in Slovenian) gets converted to `~as`, sent to printer,
which prints out the proper word `čas` again.

This presents a problem. We can't use the tilde any more, because the printer
would always translate it to the letter `č`. Luckily, printer's charset
consists of additional characters (including some Greek letters), which can
imitate the ones we've lost in the process.

## Translation file format
Translation files are made of newline (\n) separated ``definitions''. Each
definition consists of a character that should be detected and a decimal number
of a character that should be sent to printer instead. Values are separated by
a single tab (\t).

Trfile example:
```
č       126
Ž       064
ž       096
```

Characters on the left should be identical to the ones COPRIS receives.
Decimal codes on the right, however, should conform to your printer's
character set. Each printer and each country have a different set. Check
your printer's manual or experiment on your own (a test file is included).

Also included is a Slovenian translation file (or Yugoslavian, as the keyboard
layout doesn't differ). Letters with carons should convert normally, lost
characters are replaced with my own selections.

## Usage
COPRIS requires at least the port parameter to be specified (`-p` or `--port`).
Superuser privileges are not required, as long as the specified port is
greater than 1023. For a summary of all options, run `-h` or `--help`.

If no errors are printed out, COPRIS should be listening on the specified
port. You may use the verbose parameter (`-v` or `--verbose`) up to three
times to debug. More parameters, more verbosity.

The physical printer should be specified as the last argument. It may also
be a normal text file, COPRIS will append data to it. If no file is specified,
data will be output to the terminal with corresponding beginning and end of
stream markers (;BOS and ;EOS).

The server will notify you when an incoming connection is accepted and
closed. If you want no status text to be present, just the received data,
use the `-q`/`--quiet` parameter. Note that if an output file is specified,
no information/debugging text is sent to it.

## Building
Apart from standard C libraries, COPRIS uses the `GNU getopt` library to parse
program arguments. GNU/Linux users should be fine here, BSD's and others may
have to obtain the library manually.

Build COPRIS simply by running `make` within the `src/` directory. Additional
debugging symbols can be compiled by running `make debug`. Cleanup is done
with `make clean`. A single executable `copris` will be made in the same
directory.
