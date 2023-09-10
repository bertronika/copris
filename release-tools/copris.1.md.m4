% COPRIS(1) | m4_esyscmd(`git describe --tags --dirty 2>/dev/null || cat ../VERSION')m4_dnl
%
% m4_esyscmd(`date --iso-8601')m4_dnl
m4_changequote(`[[[', `]]]')m4_dnl

# NAME

**copris** - a converting printer server


# SYNOPSIS

| **copris** \[*options*\] \[*printer device* or *output file*\]
| **copris** **\--dump-commands** > new_feature_file.ini


# DESCRIPTION

m4_include(common-description.md.m4)m4_dnl


# OPTIONS

**-p**, **\--port** *NUMBER*
: Run COPRIS as a network server on port *NUMBER*. Superuser
  privileges are required if *NUMBER* is less than 1024.

**-e**, **\--encoding** *FILE*
: Recode characters in received text according to definitions from encoding
  *FILE*. This option can be specified multiple times.

**-f**, **\--feature** *FILE*
: Process Markdown formatting in received text and apply session commands
  according to commands from printer feature *FILE*. This option can be
  specified multiple times.

**\--dump-commands**
: Show all possible printer feature commands in INI file format
  (e.g. to be piped into a new printer feature file you are making).

**-d**, **\--daemon**
: If running as a network server, do not exit after the first connection.

**-l**, **\--limit** *NUMBER*
: If running as a network server, limit number of received bytes to *NUMBER*.

**\--cutoff-limit**
: If limit is active, cut text on *NUMBER* count instead of
  discarding the whole chunk.

**\-R**, **\--remove-non-ascii**
: Remove all characters (or bytes) that aren't part of the ASCII character set.
  As (old) dot-matrix printers generally don't support Unicode-encoded
  characters, this will both prevent them being wrongly printed, but at the same
  time possibly corrupt the meaning of the text.

**-v**, **\--verbose**
: Show informative status messages. If specified twice, show even more messages.

**-q**, **\--quiet**
: Do not show any unneccessary messages, except warnings and fatal errors, routed
  to *stderr*. This also omits *notes*, shown if COPRIS assumes it is not
  invoked properly.

**-h**, **\--help**
: Show a short option summary.

**-V**, **\--version**
: Show program version, author and build-time options.

Do not specify a port number if you want to read from standard input. Likewise,
omit the output file to have text echoed out to standard output (or piped
elsewhere).


# EXAMPLES

m4_include(common-usage.md.m4)m4_dnl


# DEVELOPMENT

COPRIS' development repository resides at <https://github.com/bertronika/copris>.

