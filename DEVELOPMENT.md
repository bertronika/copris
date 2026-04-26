# COPRIS development

This document describes various aspects of COPRIS development.


## Building COPRIS, its documentation and unit tests

Making release and debug builds is done via the *Makefile* in the root directory. Running `make help`
will show most of the available targets.

Build flags, linker settings and object files are set in *Makefile-common.mk* in the root directory.

*README.md* and Man pages are generated via files in the *release-tools* directory. Each chapter
gets its own Markdown file.

There are two categories of unit tests. Some are written in Bash and reside in the *tests-bash*
directory. Others, located in the *tests* directory are written in C and compiled via their own
*Makefile*. Running `make help` there will show the important targets.


## External libraries

- inih (<https://github.com/benhoyt/inih#readme>)

  Used for parsing encoding and printer feature files.
  
- uthash.h (<https://troydhanson.github.io/uthash/userguide.html>)

  Used for storing definitions, provided by encoding and printer feature files.
  
- utstring.h (<https://troydhanson.github.io/uthash/utstring.html>)

  Used all over COPRIS for storing and manipulating strings of text.


## Markdown references

- <https://commonmark.org/help/>
- <https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet>


## Unit testing

- cmocka framework (<https://api.cmocka.org/>)

Useful commands for testing single files:

```
cd tests
make cmocka-recode && ./$_
make cmocka-parse_value && ./$_ 2>/dev/null
make USERFLAGS=-Wno-unused-function cmocka-parse_vars && ./$_

```

## Various utilities

- m4 macro processor (<https://www.gnu.org/software/m4/manual/html_node/index.html>)

  Used for generating text for `README.md` and the `copris.1` Man page.

- pandoc document converter (<https://pandoc.org/MANUAL.html#extension-pandoc_title_block>)

  Used when generating the Man page from the Markdown sources (with the
  `pandoc_title_block` extension).
