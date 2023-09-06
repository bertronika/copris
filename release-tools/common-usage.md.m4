m4_changequote(`[[[', `]]]')m4_dnl
**Notice:** COPRIS is in active development. Some features are still missing, others have not been thoroughly tested yet. Command line option arguments may change in future. Version 1.0 will be tagged when the feature set will be deemed complete.

Run as a simple server on port 8080, perform no text recoding, output received data to the serial port and exit after one connection. Note that superuser privileges are required if the specified port is smaller than 1024.

```
copris -p 8080 /dev/ttyS0
```

Serve on port 8080 as a daemon (do not exit after first connection), recode text using the `slovene.ini` encoding file, limit any incoming text to a maximum of 100 characters and print received data to the terminal. Note that text limit works only when running as a server.

```
copris -p 8080 -d -e slovene.ini -l 100
```

Read local file `Manual.md` using the printer feature file `epson.ini`. Remove any character that isn't present in the ASCII character set. Output formatted text to a USB interface on the local computer:

```
copris -f epson.ini --remove-non-ascii /dev/ttyUSB0 < Manual.md
```
