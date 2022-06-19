Run as a simple server on port 8080, perform no text conversion, output
received data to the serial port and exit after one connection. Note
that superuser privileges are required if the specified port is smaller
than 1024.

```
copris -p 8080 /dev/ttyS0
```

Serve on port 8080 as a daemon (do not exit after first connection),
translate characters using the `slovene.ini` translation file, limit
any incoming text to maximum 100 characters and print received data to
the terminal. Note that text limit works only when running as a server.

```
copris -p 8080 -d -t slovene.ini -l 100
```

Read local file `Manual.md` using the specified printer feature set
`epson.ini` and output formatted text to a USB interface on the local
computer:

```
copris -r epson.ini /dev/ttyUSB0 < Manual.md
```

If you need to debug COPRIS or are curious about its internal status,
use the `-v/--verbose` parameter up to two times.

For a summary of all command line arguments, invoke COPRIS with
`-h/--help`. For a listing of build-type options, version and author
information, invoke with `-V/--version`.

Note that you can only use `-l/--limit` and `--cutoff-limit` when reading
from the network. If you want to limit incoming text from a local source,
use some other tool or amend your data.

COPRIS will print informative status messages to the terminal, except
if you use the `-q/--quiet` parameter, which will hide all non-essential
messages, except fatal errors. Note that if an output file is specified,
no status messages are sent to it. If a non-fatal error occurs in quiet
mode, COPRIS will disable the offending broken feature, notify you about
it and continue execution.
