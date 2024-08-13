m4_changequote(`[[[', `]]]')m4_dnl
# COPRIS - a converting printer server

m4_include(common-description.md.m4)m4_dnl

# How do I make an encoding file?

Firstly, check if COPRIS already includes one for your locale (`encodings/` directory).

If not, you can search the Internet for a code page table/listing with characters and their numberical values, which you can then manually convert to an appropriate format.

As a fallback, to at least figure out the character codes of your character set, run the `printchar.sh` test script, included with COPRIS, and pipe its output into the printer. It'll output printable characters, along with their decimal and hexadecimal values.

The format of an encoding file is simple. Start the line by entering a human-readable character, either via your keyboard or copied from somewhere else. Follow by an equals sign (optionally surrounded by spaces). Then, enter the numerical value of that character which your printer will understand.

Here's an example excerpt of the YUSCII encoding:

```ini
# Left-hand letters are readable to us, however, their UTF-8 codes
# are unknown to the printer.  Therefore, COPRIS replaces them with
# the right-hand hexadecimal codes.
Č = 0x5E  ; replacing ^
Ž = 0x40  ; replacing @
Š = 0x7B  ; replacing [
```

You might have noticed that YUSCII overrides some ASCII characters. Luckily, printer character sets often consist of additional characters (Greek letters, box drawing and maths symbols etc.), which can be also specified in an encoding file to imitate the lost ones:

```ini
^ = 0x27  ; replacement is an acute accent
@ = 0xE8  ; replacement is the Greek letter Phi
...
```

Have you made an encoding file COPRIS doesn't yet include? Contributions are always welcome!

You may find out that your printer supports a code page, appropriate for your locale, but it isn't selected by default when you turn it on. Read on, the chapter about printer feature files isn't only about formatting text, it also explains the custom commands that are used for setting up the printer.


# How do I make a printer feature file?

As with encoding files, COPRIS might have the right one in the `feature-files/` directory. If the file name seems appropriate, check the first few lines of the file to see if your printer is supported.

Else, consult the printer's manual. There should be a section on escape codes. Generate a sample printer feature file (`copris --dump-commands > my-printer.ini`, see [copris(1) man page](man/copris.1.txt) for details). Uncomment the appropriate commands and append values from the manual, in the same way as with encoding files.

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


## Variables and session commands

You can use existing command names as variables, as long as you define the command *before* using it as a variable. Furthermore, you may define your own custom variables and use them in existing commands. For COPRIS to recognise them, they must be prefixed with `C_`! Variables may be interweaved with commands.

```ini
# lx300.ini - continued
C_UNDERLINE_ON  = 0x1B 0x2D 0x31
C_UNDERLINE_OFF = 0x1B 0x2D 0x30
F_H1_ON  = C_UNDERLINE_ON F_ITALIC_ON
F_H1_OFF = F_ITALIC_OFF C_UNDERLINE_OFF
```

COPRIS also provides **session commands**: two command pairs for sending repetitive settings to the printer. They may be used to set the code page, text margins, line spacing, font face, character density, initialise/reset the printer and so on:

- `S_AT_STARTUP` and `S_AT_SHUTDOWN` - sent to the printer once after COPRIS starts and once
  before it exits
- `S_BEFORE_TEXT` and `S_AFTER_TEXT` - sent to the printer each time text is received, in
  order `S_BEFORE_TEXT` - *received text* - `S_AFTER_TEXT`


## Parsing variables, numerical values and comments in input text

Any custom variable, specified in a printer feature file, can be invoked from within the input text. Specify the `-c` argument when running COPRIS and begin your text with one of the following commands: `$ENABLE_COMMANDS`, `$ENABLE_CMD` or `$CMD`. You may then call variables in the text file by omitting their `C_` prefix and prepending a dollar sign symbol to them. I.e., if your variable is `C_SERIF`, `$SERIF` is used in text to invoke it.

Furthermore, apart from already-defined variables, numerical values can be included in text. They must be prefixed with the same symbol as custom variables and then specified in decimal, octal or hexadecimal notation, as they would be in a printer feature file.

Lastly, comments can be passed in text. They consist of the same prefix character as custom variables, followed by a number sign and the text that needs to be commented out. **Be aware** that whitespace characters aren't permitted in a comment. This means that, for example in a series of custom variables or numerical values, each can be commented out separately, without impacting the surrounding ones. For commenting out multiple words, you must find some other character, such as underscore or a non-breaking space.

Here's an example of all three of the beforementioned commands. Note the use of non-breaking spaces in the comment.

```
$ENABLE_COMMANDS
$# Reduce line spacing
$ESC $0x33 $25
```


# How does COPRIS handle the output serial/parallel/USB/etc. connection?

It doesn't. It simply outputs the converted text to a specified file (or standard output if no file is specified), and lets the operating system's kernel do the rest.


## What can be specified as the output?

The last command line argument, output destination, can either be a character device (e.g. `/dev/usb/lp0`) or a normal text file. COPRIS simply appends any received text to it. If nothing is specified, data will be echoed to the terminal with corresponding `Begin-` and `End-Stream-Transcript` markers (`;BST` and `;EST`).


# Usage and examples

m4_include(common-usage.md.m4)m4_dnl

If you need to debug COPRIS or are curious about its internal status, use the `-v/--verbose` parameter up to two times.

For a summary of all command line arguments, invoke COPRIS with `-h/--help`. For a listing of program version, author and build-time options, invoke with `-V/--version`.

Note that you can only use `-l/--limit` and `--cutoff-limit` when running as a network server (on a port, specified with `-p/--port`). If you want to limit incoming text from a local source (by omitting the port argument and reading from standard input), use some other tool or manually amend your data.

COPRIS will show informative status messages and notes, if it assumes it is not invoked properly. You may use the `-q/--quiet` parameter to silence them, which will only leave you with possible warnings and fatal errors, routed to standard error. Note that if an output file is specified, no status messages are sent to it. If a non-fatal error occurs in quiet mode, COPRIS will disable the offending broken feature, notify you about it and continue execution.


# Building and installation

COPRIS requires, apart from a standard C library, three additional packages:

- pkg-config or pkgconf for the compilation process
- uthash ([Repology][1], [upstream][2])
- inih ([Repology][3], [upstream][4])

[1]: https://repology.org/project/uthash/versions
[2]: https://github.com/troydhanson/uthash
[3]: https://repology.org/project/inih/versions
[4]: https://github.com/benhoyt/inih

I've tried to pick common libraries, present in many Linux distributions and BSD's, meaning they should be easily installable with your package manager. Note that `inih` requires, unlike the header-only `uthash`, both development headers and the dynamic libraries themselves (e.g. `libinih-devel` and `libinih0`, names on your end may differ).

Build COPRIS using the included `Makefile` (you'll need GNU Make, if you're on a BSD). The procedure is as follows:

```
make
sudo make install [DESTDIR=...] [PREFIX=...]
```

`DESTDIR` and `PREFIX` are optional. I'd recommend using a symlink farm manager, such as GNU Stow, installing COPRIS into its own directory (e.g. `make PREFIX=/usr/local/stow/copris`) and then symlinking it into place (e.g. running `sudo stow copris` in `/usr/local/stow`).

By default a non-stripped release binary is built, which includes some debugging symbols for crash diagnosis. Many more targets are present for debugging and development purposes, run `make help` to review them.

