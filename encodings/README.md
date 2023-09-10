# Character encoding files

This directory contains encoding files for COPRIS. You are welcome to contribute any
missing ones!


Files are divided into two categories:

## Code pages (DOS/IBM)

File prefix: `cp`.

These can be generated with the included `generate-encoding` script. They're generated
using the *iconv(1)* tool. Find the proper encoding name with `iconv --list` and use
that name as an argument to the mentioned script.


## National encodings

File prefix: `enc-`.

These have to be sourced and assembled manually. I'm not aware of any tool that can
generate them automatically. Copy `template-postfix.txt` to a new file and prepend
appropriate files.
