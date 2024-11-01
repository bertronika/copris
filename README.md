# COPRIS - a converting printer server

COPRIS is a printer server and text conversion program that bridges the gap between modern,
UTF-8-encoded, Markdown-formatted text and (older) dot-matrix and thermal printers.

When provided with the appropriate configuration files and options, COPRIS reads plain text from
a local source or the network (with a built-in TCP socket server), applies optional processing
methods to it and sends it to the destination device (or file).

Two main text processing methods stand out in COPRIS; they are:


## Character recoding via encoding files

Older printers usually don't support multi-byte UTF-8-encoded text. Rather, they use code pages
and customised character sets which may vary between different manufacturers. COPRIS can load a
character *encoding file* that is used to recode received text. By writing or selecting a premade
one for your printer and locale, you allow COPRIS to recode (replace) received characters with
appropriate locale-specific ones, understood by your printer.

This is particulary useful in cases where printer distributors swapped lesser-used ASCII characters
(such as `~ @ {`) with those from the national alphabet, so they could be typed out directly
from the non-localised keyboard. By finding similar-looking characters in printer's character
set, you may, crudely, regain those lost characters.

However, *encoding files* aren't tied only to locale conversions, you may use them to remap any
character. Examples include typographic quotation marks, em dashes, copyright symbol and others,
commonly found in character sets of various printers.


## Markdown formatting via printer feature files

Many printers support commands (escape codes) for formatting printed text (e.g. Epson has its
ESC/P control language). COPRIS can read a *printer feature file* that specifies such commands,
invoked by following Markdown attributes:

- bold and italic *emphasis* via asterisks (`*`, `**`, `***`) or underscores (`_`, `__`, `___`)
- up to 4 levels of *headings* via number signs (`#`, `##`, `###`, `####`)
- *blockquotes* via greater-than signs and a space (`> `)
- *inline code* via single backticks (`` ` ``)
- *code blocks* via triple backticks (`` ``` ``) -- or four spaces, but those are disabled
by default
- *links* via angle brackets (`< >`)

Any command can be bound to mentioned attributes. If desired, bold emphasis can trigger
double-width text, headings may change the font face or size and links can be made
underlined. There are no limits, as long as your *printer feature file* contains commands that
are understood by the printer (or any other destination device).

You might have noticed that only a subset of the Markdown specification is supported. COPRIS
intentionally doesn't alter text layout (white space, paragraphs etc.), since there's no
markup/layout engine. You may freely format your input plain-text files to your liking,
and COPRIS will preserve the spacing and newlines (just be warned about four leading spaces
triggering a code block).


# File format

*Encoding* and *printer feature files* are expected to be in the INI file format:

```ini
# Comment
key1 = whitespace separated values
key2 = whitespace separated values ; inline comment
...
```

Left-hand keys should be written out directly as UTF-8 characters or strings, right-hand values
are expected to be in either decimal, octal (prefixed by a `0`) or hexadecimal base (prefixed
with `0x`). Don't prepend zeroes to decimal numbers for alignment, as they'll be interpreted
as octal numbers. Instead, use any amount of spaces (or tabulators).

One line can contain exactly *one* definition/command.

In case you need a `key` to resolve to a blank `value`, set `@` as the value (e.g. if you want
to remove certain characters from the text, or omit rendering some Markdown attributes).

Here are some examples:

```ini
Å = 0xC5                ; for an encoding file
F_BOLD_OFF = 0x1B 0x45  ; for a printer feature file
F_H3_ON = @             ; ditto
```


# Writing an encoding file

Before making one, check if COPRIS already includes one for your locale (`encodings` directory
in the source tree).

The format of an encoding file is as follows. Start the line by entering your desired character,
either via the keyboard or copied from somewhere else. Follow by an equals sign. Then, enter
the numerical value of the same character your printer will understand.

Here's an example excerpt of the YUSCII encoding:

```ini
# Left-hand letters are readable to us, however, their UTF-8 codes
# are unknown to the printer. Therefore, COPRIS replaces them with
# the right-hand values, here represented in hexadecimal.
Č = 0x5E  ; replacing ^
Ž = 0x40  ; replacing @
Š = 0x7B  ; replacing [
```

You might have noticed that YUSCII overrides some ASCII characters. Luckily, printer character
sets often consist of additional characters (Greek letters, box drawing and maths symbols etc.),
which can be also specified in an encoding file to imitate the lost ones:

```ini
^ = 0x27  ; replacement is an acute accent
@ = 0xE8  ; replacement is the Greek letter Phi
...
```

Have you transcribed a national or a well known encoding COPRIS doesn't yet include? Contributions
are always welcome!

You may find out that your printer supports a code page, appropriate for your locale, but it
isn't selected by default when you turn it on. Read on, the chapter about printer feature files
explains session commands which can be used for setting up the printer.


# Writing a printer feature file

As with encoding files, COPRIS' source tree might have the right one in the `feature-files/`
directory. If the file name seems appropriate, check the first few lines of the file to see if
your printer/use case is supported.

Else, consult the printer's manual. There should be a section on escape codes. Generate a sample
printer feature file (`copris --dump-commands > my-printer.ini`). Uncomment the appropriate
commands and append values from the manual, in the same way as with encoding files.

Example from Epson's LX-300 manual, page A-14 (91):

```
ASCII   Dec.   Hex.   Description
----------------------------------------
ESC 4   52     34     Select Italic Mode
ESC 5   53     35     Cancel Italic Mode
```

The corresponding printer feature file lines are:

```ini
# lx300.ini
F_ITALIC_ON  = 0x1B 0x34  ; hexadecimal notation, 0x1B = ESC
F_ITALIC_OFF = 27 53      ; decimal notation, 27 = ESC
```


## Variables in printer feature files

You can use existing command names as variables, as long as you define the command *before*
using it as a variable. Furthermore, you may define your own custom variables and use them in
existing commands. For COPRIS to recognise them, they must be prefixed with `C_`! Variables
may be interweaved with commands.

Examples:

```ini
# lx300.ini - continued
C_UNDERLINE_ON  = 0x1B 0x2D 0x31
C_UNDERLINE_OFF = 0x1B 0x2D 0x30
F_H1_ON  = C_UNDERLINE_ON F_ITALIC_ON
F_H1_OFF = F_ITALIC_OFF C_UNDERLINE_OFF
```


## Session commands

COPRIS provides two command pairs for sending repetitive settings to the printer. They may
be used to set the code page, text margins, line spacing, font face, character density,
initialise/reset the printer and so on:

- `S_AT_STARTUP` and `S_AT_SHUTDOWN` - sent to the printer once after COPRIS starts and once
  before it exits
- `S_BEFORE_TEXT` and `S_AFTER_TEXT` - sent to the printer each time text is received, in
  order `S_BEFORE_TEXT` - *received text* - `S_AFTER_TEXT`

Examples:

```ini
# lx300.ini - continued
S_AT_STARTUP = 0x07  ; sound the bell (beeper)
S_AFTER_TEXT = 0x0C  ; trigger form feed after printing
```


# Variables, numerical values and comments in input text

If you've defined a printer feature file, and it includes custom variables (prefixed with `C_`),
you may use them directly in input text.

To allow for that, specify the `-P/--parse-variables` argument when running COPRIS and begin
your text with a line, containing `COPRIS ENABLE-VARIABLES`. This is called the *modeline*
and tells COPRIS to process variables in input text.

You may then call variables in the text file by prepending a dollar sign to them. I.e., if
your variable is `C_SERIF`, `$C_SERIF` should be used in text. If you want to call multiple,
write them out space-separated. Note that anything between the dollar sign and the new line is
interpreted as a variable (or a series of them).

Furthermore, apart from already-defined variables, numerical values can be included in
text. They must be written out in the same way as variables - prefixed with a dollar sign, and,
if specifing a multitude of them, separated by white spaces. They must be in either decimal,
octal or hexadecimal notation, as they would be in a printer feature file.

Lastly, comments can be passed in text. They again consist of a dollar sign, which is then
followed by a number sign and your comment message.

Here are examples of all three notations:

```
COPRIS ENABLE-VARIABLES    (must be included at the top)
$# This reduces line spacing:
$C_ESC 0x33 25             (feature file has a C_ESC command defined)
```


## Mixing variables and text

Be wary of mixing variables and text in a sentence. Variables will only get detected properly
if they're at the end of the sentence, not followed by any punctuation.

Meaning:

```
Word count is $C_WC        (works normally)
The sum is $C_WC words.    (fails - expects a variable named '$WC words.')
```

For reliable interpretation of variables, dedicate a whole line just for them, and make use of
Markdown for mixing text and commands.

Lastly, if your input text contains a variable-symbol-prefixed word and you don't want COPRIS
to interpret it, escape it by prefixing it with another dollar symbols.


## The modeline

It is expected in the first line of input text when COPRIS is started with
`-P/--parse-variables`. Its format is:

```
COPRIS <required 1st option> [ optional 2nd option ]
```

Previous documentation already mentioned its use for for enabling variables. It serves one
other purpose, disabling parsing Markdown in text.

Here are both modeline options and their short forms:

- `ENABLE-VARIABLES`; `ENABLE-VARS`
- `DISABLE-MARKDOWN`; `DISABLE-MD`

Letters are case-insensitive and the order of options isn't important. Thus, following lines
are the same:

- `COPRIS ENABLE-VARIABLES DISABLE-MARKDOWN`
- `copris disable-md enable-vars`


# How does COPRIS handle the output serial/parallel/USB/etc. connection?

It doesn't. It simply outputs the converted text to a specified file (or standard output if no
file is specified), and lets the operating system's kernel do the rest.


## What can be specified as the output?

The last command line argument, output destination, can either be a character device
(e.g. `/dev/usb/lp0`) or a normal text file. COPRIS by default writes to it every time it
receives and processes data, overwriting any previous contents (this can be configured to append
instead). If no destination is specified, data will be echoed to the terminal with corresponding
`Begin-` and `End-Stream-Transcript` markers (`;BST` and `;EST`).


# Usage and examples

**Notice:** COPRIS is in active development. Some features are still missing, others have not
been thoroughly tested yet. Command line option arguments may change in future. Version 1.0
will be tagged when the feature set will be deemed complete.

Run as a simple server on port 8080, perform no text recoding, output received data to the
serial port and exit after one connection. Note that superuser privileges are required if the
specified port is smaller than 1024.

```
copris -p 8080 /dev/ttyS0
```

Serve on port 8080 as a daemon (do not exit after first connection), recode text using the
`slovene.ini` encoding file, limit any incoming text to a maximum of 100 characters and print
received data to the terminal. Note that text limit works only when running as a server.

```
copris -p 8080 -d -e slovene.ini -l 100
```

Read local file `font-showcase.md` using the printer feature file `epson-escp.ini`. Interpret
any possible user commands, found in the local file. Output formatted text to an USB printer
interface on the local computer:

```
copris -f epson-escp.ini -c /dev/usb/lp0 < font-showcase.md
```

If you need to debug COPRIS or are curious about its internal status, use the `-v/--verbose`
parameter up to two times.

For a summary of all command line arguments, invoke COPRIS with `-h/--help`. For a listing of
program version, author and build-time options, invoke with `-V/--version`.

Note that you can only use `-l/--limit` and `--cutoff-limit` when running as a network server
(on a port, specified with `-p/--port`). If you want to limit incoming text from a local source
(by omitting the port argument and reading from standard input), use some other tool or manually
amend your data.

COPRIS will show informative status messages and notes, if it assumes it is not invoked
properly. You may use the `-q/--quiet` parameter to silence them, which will only leave you
with possible warnings and fatal errors, routed to standard error. Note that if an output file
is specified, no status messages are sent to it. If a non-fatal error occurs in quiet mode,
COPRIS will disable the offending broken feature, notify you about it and continue execution.


# Building and installation

COPRIS requires, apart from a standard C library, three additional packages:

- pkg-config or pkgconf for the compilation process
- uthash ([Repology][1], [upstream][2])
- inih ([Repology][3], [upstream][4])

[1]: https://repology.org/project/uthash/versions
[2]: https://github.com/troydhanson/uthash
[3]: https://repology.org/project/inih/versions
[4]: https://github.com/benhoyt/inih

I've tried to pick common libraries, present in many Linux distributions and BSD's, meaning
they should be easily installable with your package manager. Note that `inih` requires,
unlike the header-only `uthash`, both development headers and the dynamic libraries themselves
(e.g. `libinih-devel` and `libinih0`, names on your end may differ).

Build COPRIS using the included `Makefile` (you'll need GNU Make, if you're on a BSD). The
procedure is as follows:

```
make
sudo make install [DESTDIR=...] [PREFIX=...]
```

`DESTDIR` and `PREFIX` are optional. I'd recommend using a symlink farm manager, such as GNU
Stow, installing COPRIS into its own directory (e.g. `make PREFIX=/usr/local/stow/copris`)
and then symlinking it into place (e.g. running `sudo stow copris` in `/usr/local/stow`).

By default a non-stripped release binary is built, which includes some debugging symbols for
crash diagnosis. Many more targets are present for debugging and development purposes, run
`make help` to review them.

