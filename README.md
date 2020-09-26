# COPRIS - a converting printer server

COPRIS is a multifunction printer server, made primarily for sending raw text
to a dot-matrix printer over a network, while performing optional text
conversion.

COPRIS differs from CUPS. Not just by being smaller with less features, it has
a different objective. Sure, CUPS is also a server, but it doesn't provide
neither character conversion nor it has some bonus features.

## Why convert characters?
In the late 1980's and 90's, when dot-matrix printers were getting affordable
and common in offices and homes, they usually included only a handful of
character sets (English, French, German, Italian, Spanish and such).
Languages, whose alphabets required additional letters, had to be incorporated
in some way. The solution was to sacrifice some of the keys of the American keyboard.
Printers were internally modified, so they'd print characters such as `~ @ [` as
locale-specific letters. Users could then print localised text without having special
keyboards or character sets.

COPRIS can optionally convert localised characters by using user specified
*Translation files*, selected via `-t`/`--trfile`.

Example: When a Slovenian translation file is set, word `čas` (time) gets converted
to `~as`, sent to printer, and gets printed out as the proper word `čas` again.

However, this means we could never print the actual tilde - it would always get
printed as `č`. Luckily, printer's charset consists of additional characters (Greek
letters, box drawing and math symbols etc.), which can also be put in a translation
file to imitate the ones we've lost.

## Translation file format
Translation files are made of newline (`\n`) separated definitions. Each
definition consists of a character that should be detected and a decimal number
of a character that should be sent to printer instead. Values are separated by
a single tab (`\t`).

Trfile example:
```
č       126
Ž       064
ž       096
```

Characters on the left are the ones COPRIS will take out of source text. Characters,
assembled from the decimal codes on the right will be sent to the printer instead.
To determine the codes of replacement characters, have your printer print out its
character set. To ease the process, a test file `printchar.sh` is included, which
prints characters and their corresponding decimal codes side by side.

A Slovenian (Yugoslavian) translation file for Epson printers, distributed by
Repro Ljubljana, is already included. Letters with carons should convert normally,
lost characters are replaced with my own selections.

## Markdown conversion
COPRIS understands Markdown and can (also optionally) print formatted text. This can
be achieved by selecting a printer set with `-r`/`--printer` according to your
printer's make/model. A list of available printer sets is displayed when using the
`-V`/`--version` argument.

## Usage
COPRIS requires at least the port parameter to be specified (`-p` or `--port`).
Superuser privileges are not required, as long as the specified port is
greater than 1023. For a summary of all options, run `-h` or `--help`.

If no errors are printed out, COPRIS should be listening on the specified
port. For a more verbose output, use the (`-v` or `--verbose`) parameter up
to two times.

The physical printer should be specified as the last argument. It may also
be a normal text file, COPRIS will append data to it. If no file is specified,
data will be output to the terminal with corresponding beginning and end of
stream markers (;BOS and ;EOS).

The server will notify you when an incoming connection is accepted and
closed, except if you use the `-q`/`--quiet` parameter. Note that if an output file
is specified, no debugging text is sent to it.

To connect to the server, use `netcat`, `telnet`, or a similar program on the client
side.

## Building
Requirements are `gcc` and `make`. No external dependencies are needed, the only
non-standard C library is `GNU getopt` for parsing program arguments.

Build COPRIS simply by running `make` within the `src/` directory. Additional
debugging symbols can be compiled by running `make debug`. Cleanup is done
with `make clean`. A single executable `copris` will be made in the same
directory.
