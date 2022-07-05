% COPRIS(1) | m4_esyscmd(`git describe --tags --dirty 2>/dev/null || cat ../VERSION')m4_dnl
%
% m4_esyscmd(`date --iso-8601')m4_dnl
m4_changequote(`[[[', `]]]')m4_dnl

# NAME

**copris** - a converting printer server


# SYNOPSIS

| **copris** \[*options*\] \[*printer* or *output file*\]
| **copris** **\--dump-commands** > new_printer_set.ini


# DESCRIPTION

m4_define([[[GEN_MANPAGE]]])m4_dnl
m4_changequote`'m4_dnl
m4_include(README.md.m4)m4_dnl
m4_changequote(`[[[', `]]]')m4_dnl


# OPTIONS

**-p**, **\--port** *NUMBER*
: Run COPRIS as a network server on port *NUMBER*. Superuser
  privileges are required if *NUMBER* is less than 1024.

**-t**, **\--trfile** *FILE*
: Enable character translation with definitions from character translation
  *FILE*.

**-r**, **\--printer** *FILE*
: Enable Markdown parsing with definitions from printer feature set
  *FILE*. COPRIS must be compiled with Markdown support for this option to
  be available.

**\--dump-commands**
: Show all possible printer feature set commands in INI file format
  (e.g. to be piped into a new printer feature set file you are making).
  COPRIS must be compiled with Markdown support for this option to
  be available.

**-d**, **\--daemon**
: If running as a network server, do not exit after the first connection.

**-l**, **\--limit** *NUMBER*
: If running as a network server, limit number of received bytes to *NUMBER*.

**\--cutoff-limit**
: If limit is active, cut text on *NUMBER* count instead of
  discarding the whole chunk.

**-v**, **\--verbose**
: Show informative status messages. If specified twice, show even more messages.

**-q**, **\--quiet**
: Do not show any unneccessary messages, except warnings and fatal errors, routed
  to *stderr*. This also omits *notes*, shown if COPRIS assumes it is not
  invoked properly.

**-h**, **\--help**
: Show a short option summary.

**-V**, **\--version**
: Show program version, author and build-time options (e.g. if Markdown support
  is present).

Do not specify a port number if you want to read from standard input. Likewise,
omit the output file to have text echoed out to standard output (or piped
elsewhere).


# EXAMPLES

m4_changequote`'m4_dnl
m4_include(usage_examples.md.m4)m4_dnl
m4_changequote(`[[[', `]]]')m4_dnl


# FILES

Example translation and feature set files can be found in
`/usr/share/copris`.


# DEVELOPMENT

COPRIS' development repository resides at <https://github.com/bertronika/copris>.

