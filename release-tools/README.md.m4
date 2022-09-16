m4_changequote(`[[[', `]]]')m4_dnl
m4_ifdef([[[GEN_MANPAGE]]], , [[[m4_dnl -- print if GEN_MANPAGE not defined
# COPRIS - a converting printer server

]]])m4_dnl endif GEN_MANPAGE
COPRIS is a printer server and text conversion program, intended to receive UTF-8 encoded text locally or over a network, process it and send it to a dot-matrix printer. In its core, COPRIS reads text from standard input or a built-in TCP socket server (similar to `telnet(1)` or `nc(1)`), and optionally applies two processing methods to it:


## Character translation

COPRIS can load a *translation file* that defines user-specified characters, not understood by printers - usually multi-byte (UTF-8) ones. They are swapped with appropriate (or at least similar) locale-specific replacements, defined in the same file.


## Printer feature sets

While COPRIS processes only plain text, some additional markup can be applied to it via *printer feature set files*. These may specify printer's escape codes for attributes, such as bold, italic, double-width text, they may change font faces, sizes, spacing and so on. COPRIS searches for a subset of Markdown attributes, present in input text, and replaces them with printer's escape codes, much the same as with translation files.


## File format

*Translation* and *printer feature set files* are expected to be in the INI file format:

```ini
# Comment
[section]
key = whitespace separated values ; inline comment
```

In case you need a `key` to resolve to a blank value, set `@` as the value (e.g. if you want to remove instead of replace certain characters in the text, or omit rendering some Markdown attributes).
m4_ifdef([[[GEN_MANPAGE]]], , [[[m4_dnl -- print if GEN_MANPAGE not defined


# Why translate characters?

In the late 1980's and 90's, when small dot-matrix printers were getting common in offices and homes, they usually included only a handful of character sets (English, French, German, Italian, Spanish and such). Languages, whose alphabets required additional letters, had to be incorporated in some way. The solution was to sacrifice some of the ASCII characters. Printers were internally modified - some symbols, such as `~ @ {`, actually printed locale-specific characters. Users could then print localised text without having to send special commands to their printers.

COPRIS allows you to avoid manually swapping characters with the use of *translation files*, consisting of source-destination `key = raw values` definitions. Input text is scanned for occurrences of left-hand *keys*, which are replaced with appropriate right-hand *values*, specified in decimal, hexadecimal or octal bases.

To use a translation file, specify the `-t/--translate filename.ini` argument in the command line.


## How do I make a translation file?

To determine the codes of replacement characters, have your printer print out its character set. You can often trigger an automatic printout by holding certain buttons while powering it on. Consult the printer's manual for accurate information. Then, count the offset values of wanted characters, helping yourself with the ASCII code table (e.g. the `ascii(7)` man page). Alternatively, run the `printchar.sh` test file, included with COPRIS, and pipe it into the printer. It'll output printable characters, along with their decimal and hexadecimal codes.

Example excerpt for Slovene characters for Epson LX-300 (distributed by Repro Ljubljana):

```ini
# slovene.ini
Č = 94   ; replacing ^
Ž = 64   ; replacing @
Š = 123  ; replacing {
```

You might have noticed that some commonly used characters are now unavailable. Luckily, printer character sets often consist of additional characters (Greek letters, box drawing and maths symbols etc.), which can be also specified in a translation file to imitate the lost ones.


# How can I apply formatting to text?

Pass Markdown-formatted text to COPRIS. Before doing that, specify an appropriate printer feature set file with the `-r/--printer filename.ini` argument. Note that COPRIS will *not* modify the layout of the text, but only apply formatting attributes from commands, present in the feature set file.


## How do I make a printer feature set file?

Consult the printer's manual. There should be a section on escape codes. Then, generate a sample printer feature set file (`--dump-commands`, see man page) and append codes to the appropriate commands in either decimal, hexadecimal or octal format, with each one separated by at least one space.

Example from Epson's LX-300 manual, page A-14 (91):

```
ASCII   Dec.   Hex.   Description
----------------------------------------
ESC 4   52     34     Select Italic Mode
ESC 5   53     35     Cancel Italic Mode
```

Corresponding printer set file:

```ini
# lx300.ini
F_ITALIC_ON  = 0x1B 0x34  ; hexadecimal notation, 0x1B = ESC
F_ITALIC_OFF = 27 53      ; decimal notation, 27 = ESC
```

The `ascii(7)` man page might come in handy for determining values of various control codes.

You can use existing commands as variables, as long as you define the command *before* using it as a variable. Furthermore, you may define your own variables and use them in existing commands. For COPRIS to recognise them, they must be prefixed with `C_`. Variables may be interweaved with commands.

```ini
# lx300.ini - continued
C_UNDERLINE_ON  = 0x1B 0x2D 0x31
C_UNDERLINE_OFF = 0x1B 0x2D 0x30
F_H1_ON  = C_UNDERLINE_ON F_ITALIC_ON
F_H1_OFF = F_ITALIC_OFF C_UNDERLINE_OFF
```

# How does COPRIS handle the output serial/parallel/USB/etc. connection?

It doesn't. It simply outputs the converted text to a specified file (or standard output if no file is specified), and lets the operating system's kernel do the rest.


## What can be specified as the output?

The last argument, specified to COPRIS, can either be a character device (e.g. `/dev/ttyUSB0`) or a normal text file. COPRIS simply appends any received text to it. If nothing is specified, data will be echoed to the terminal with corresponding `Begin-` and `End-Stream-Transcript` markers (`;BST` and `;EST`).


# Usage and examples

m4_include(usage_examples.md.m4)m4_dnl

If you need to debug COPRIS or are curious about its internal status, use the `-v/--verbose` parameter up to two times.

For a summary of all command line arguments, invoke COPRIS with `-h/--help`. For a listing of program version, author and build-time options, invoke with `-V/--version`.

Note that you can only use `-l/--limit` and `--cutoff-limit` when running as a network server (on a port, specified with `-p/--port`). If you want to limit incoming text from a local source (by omitting the port argument and reading from standard input), use some other tool or manually amend your data.

COPRIS will show informative status messages and notes, if it assumes it is not invoked properly. You may use the `-q/--quiet` parameter to silence them, which will only leave you with possible warnings and fatal errors, routed to standard error. Note that if an output file is specified, no status messages are sent to it. If a non-fatal error occurs in quiet mode, COPRIS will disable the offending broken feature, notify you about it and continue execution.


# Installation

COPRIS requires, apart from a standard C library, two additional libraries:

- uthash ([Repology][1], [upstream][2])
- inih ([Repology][3], [upstream][4])

[1]: https://repology.org/project/uthash/versions
[2]: https://github.com/troydhanson/uthash
[3]: https://repology.org/project/inih/versions
[4]: https://github.com/benhoyt/inih

I've tried to pick common libraries, present in many Linux distributions and BSD's, meaning they should be easily installable with your package manager. Note that `inih` requires, unlike the header-only `uthash`, both development headers and the dynamically linked libraries themselves (e.g. `libinih-devel` and `libinih0`, names on your end may differ).

Build COPRIS using the included `Makefile` (you'll need GNU Make, if you're on a BSD). The procedure is as follows:

```
make
sudo make install [DESTDIR=...] [PREFIX=...]
```

`DESTDIR` and `PREFIX` are optional. I'd recommend using a symlink farm manager, such as GNU Stow, installing COPRIS into its own directory (e.g. `make PREFIX=/usr/local/stow/copris`) and then symlinking it into place (e.g. running `sudo stow copris` in `/usr/local/stow`).

By default a non-stripped release binary is built, which includes some debugging symbols for crash diagnosis. There are many more targets present for debugging and development purposes, run `make help` to review them.

]]])m4_dnl endif GEN_MANPAGE
