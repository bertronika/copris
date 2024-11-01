% COPRIS(1) | m4_esyscmd(`git describe --dirty 2>/dev/null || cat ../VERSION')m4_dnl
%
% m4_esyscmd(`date "+%Y-%m-%d"')m4_dnl
m4_changequote(`[[[', `]]]')m4_dnl

# NAME

**copris** - a converting printer server


# SYNOPSIS

| **copris** \[*options*\] \[*printer device* or *output file*\]
| **copris** **\--dump-commands** > new_feature_file.ini


# DESCRIPTION

m4_include(common01-description.md)m4_dnl


# FILE FORMAT

m4_include(common02-file_format.md)m4_dnl


# THE ENCODING FILE

m4_include(common03-encoding.md)m4_dnl


# THE PRINTER FEATURE FILE

m4_include(common04-feature.md)m4_dnl


# VARIABLES, NUMERICAL VALUES AND COMMENTS

m4_include(common05-variables.md)m4_dnl


# COMMAND LINE OPTIONS

**-p**, **\--port** *NUMBER*
: Run COPRIS as a network server on port *NUMBER*. Superuser
  privileges are required if *NUMBER* is less than 1024.

**-e**, **\--encoding** *FILE*
: Recode characters in received text according to definitions from encoding
  *FILE*. This option can be specified multiple times with different *FILE*s.

**\--ignore-missing**
: If recoding characters, do not terminate the program if received text
  contains any possibly unwanted multi-byte characters that were
  not handled by specified *FILE*s.

**-f**, **\--feature** *FILE*
: Process Markdown markup and variables in received text and apply session
  commands according to printer feature *FILE*. This option can be specified
  multiple times with different *FILE*s.

  To use commands from FILE as variables in received text, they must be
  prefixed with a predefined symbol, which is shown when invoking
  `copris --version`. This feature only works when the received text
  *starts with* `COPRIS ENABLE-VARIABLES`.

  To use commands from FILE but ignore parsing Markdown in received text,
  it should begin with `COPRIS DISABLE-MARKDOWN`.

  Read the *Modeline* chapter for more information.

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


# EXAMPLES OF INVOKING COPRIS

m4_include(common06-usage.md)m4_dnl


# DEVELOPMENT

COPRIS' development repository resides at <https://github.com/bertronika/copris>.


# SEE ALSO

**stty**(1), **intercopris**(1)
