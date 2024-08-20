m4_changequote(`[[[', `]]]')m4_dnl
# COPRIS - a converting printer server

m4_include(common01-description.md)m4_dnl


# File format

m4_include(common02-file_format.md)m4_dnl


# Writing an encoding file

m4_include(common03-encoding.md)m4_dnl


# Writing a printer feature file

m4_include(common04-feature.md)m4_dnl


# Variables, numerical values and comments in input text

m4_include(common05-variables.md)m4_dnl


# How does COPRIS handle the output serial/parallel/USB/etc. connection?

It doesn't. It simply outputs the converted text to a specified file (or standard output if no file is specified), and lets the operating system's kernel do the rest.


## What can be specified as the output?

The last command line argument, output destination, can either be a character device (e.g. `/dev/usb/lp0`) or a normal text file. COPRIS by default writes to it every time it receives and processes data, overwriting any previous contents (this can be configured to append instead). If no destination is specified, data will be echoed to the terminal with corresponding `Begin-` and `End-Stream-Transcript` markers (`;BST` and `;EST`).


# Usage and examples

m4_include(common06-usage.md)m4_dnl

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

