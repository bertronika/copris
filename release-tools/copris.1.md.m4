% COPRIS(1) | m4_esyscmd(`git describe --tags --dirty 2>/dev/null || cat ../VERSION')m4_dnl
%
% m4_esyscmd(`date --iso-8601')m4_dnl
m4_changequote(`[[[', `]]]')m4_dnl

# NAME

**copris** - a converting printer server


# SYNOPSIS

| **copris** \[*options*\] \[*output file*\]
| **copris** **\--dump-commands** > new_printer_set.ini


# DESCRIPTION

m4_define([[[GEN_MANPAGE]]])m4_dnl
m4_changequote`'m4_dnl
m4_include(README.md.m4)m4_dnl
m4_changequote(`[[[', `]]]')m4_dnl


# OPTIONS

**-p**, **\--port** *NUMBER*
: Numeric port number, if listening on a network.

**-t**, **\--trfile** *TRFILE*
: Path to the character translation file.

**-r**, **\--printer** *PRSET*
: Path to the printer feature set file.

**\--dump-commands**
: Print all possible printer feature set commands in an INI file format
  (e.g. to be piped into a new printer feature set file you are making)

**-d**, **\--daemon**
: Run as a daemon, if listening on a network.

**-l**, **\--limit** *NUMBER*
: Limit number of received bytes, if listening on a network.

**\--cutoff-limit**
: If **limit** is active, cut text right on the *NUMBER* count instead of
  omitting the whole chunk.

**-v**, **\--verbose**
: Be verbose. Specify twice to be even more verbose.

**-q**, **\--quiet**
: Be quiet, print only fatal errors.

**-h**, **\--help**
: Print a short option summary.

**-V**, **\--version**
: Print program version, author and build-time options.


# EXAMPLES

m4_changequote`'m4_dnl
m4_include(usage_examples.md.m4)m4_dnl
m4_changequote(`[[[', `]]]')m4_dnl


# FILES

Example translation and feature set files can be found in
`/usr/share/copris`.


# DEVELOPMENT

COPRIS is developed at <https://github.com/bertronika/copris>.

