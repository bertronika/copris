% INTERCOPRIS(1) | m4_esyscmd(`git describe --dirty 2>/dev/null || cat ../VERSION')m4_dnl
%
% m4_esyscmd(`date "+%Y-%m-%d"')m4_dnl
m4_changequote(`[[[', `]]]')m4_dnl

# NAME

**intercopris** - an interactive command interpreter


# SYNOPSIS

| **intercopris** \[*options*\] \[*printer device* or *output file*\]


# DESCRIPTION

Intercopris is a tool that provides a readline-based command line interpreter with the same parsing mechanism as in COPRIS' printer feature files and custom variables.

It is intended to be used for experimenting with printers by providing a straight-forward interface between human-readable numbers and raw data, understood by printers.

Intercopris gives you a command prompt, similar to that of a shell. It understands three different types of data:

- numerical values in hexadecimal (0x*NN*), decimal (*NN*) or octal (0*NN*) notation, separated with whitespace
- plain text, prefixed with '**t** '
- various built-in commands (e.g. `dump`, `hex`, `last` etc.), listed when starting the program

After completing a line and pressing return, Intercopris will parse the entered data. If it is a built-in command, it will toggle its internal state or echo out any possible result. If commands are entered, Copris will process them using the supplied printer feature file (which must be specified with `-f FILE`) and send them to the output.

Intercopris supports saving and reloading command history. To use it, you must create a file whose filename is told to you by the output of `intercopris -h`.


# OPTIONS

**-f** *FILE*
: Read commands from printer feature *FILE*.

**-h**
: Show a short option summary and expected location of the history file.

**-n**
: Do not use the history file.

Last argument should be the output character device or file path. If omitted, text will be echoed to the terminal.


# DEVELOPMENT

Intercopris is a part of COPRIS, a converting printer server.

Its development repository resides at <https://github.com/bertronika/copris>.


# SEE ALSO

**copris**(1), **readline**(3)
