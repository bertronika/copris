# COPRIS development

This document describes various aspects of COPRIS development.


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


## Various utilities

- m4 macro processor (<https://www.gnu.org/software/m4/manual/html_node/index.html>)

  Used for generating text for `README.md` and the `copris.1` Man page.

- pandoc document converter (<https://pandoc.org/MANUAL.html#extension-pandoc_title_block>)

  Used when generating the Man page from the Markdown sources (with the
  `pandoc_title_block` extension).
