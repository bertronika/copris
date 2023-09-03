m4_changequote(`[[[', `]]]')m4_dnl
COPRIS is a printer server and text conversion program, intended to receive UTF-8 encoded text locally or over a network, process it and send it to a dot-matrix printer. In its core, COPRIS reads text from standard input or a built-in TCP socket server (similar to `telnet(1)` or `nc(1)`), and optionally applies two processing methods to it:


## Character translation

COPRIS can load a *translation file* that defines user-specified characters, not understood by printers - usually multi-byte (UTF-8) ones. They are swapped (reencoded) with appropriate, or at least similar locale-specific replacements, defined in the same file.


## Printer feature sets

While COPRIS processes only plain text, some additional markup can be applied to it via *printer feature set files*. These may specify printer's escape codes for attributes, such as bold, italic, double-width text, they may change font faces, sizes, spacing and so on. COPRIS searches for a subset of Markdown attributes, present in input text, and replaces or enhances them by adding appropriate escape codes to the text.


## File format

*Translation* and *printer feature set files* are expected to be in the INI file format:

```ini
# Comment
key1 = whitespace separated values
key2 = whitespace separated values ; inline comment
...
```

Left-hand keys should be written out as readable ASCII or UTF-8 characters or strings, right-hand values are expected to be in either decimal, octal (prefixed by a `0`) or hexadecimal base (prefixed with `0x`). Don't prepend zeroes to decimal numbers for alignment, as they'll be interpreted as octal numbers. Instead, use any amount of spaces (or tabulators).

In case you need a `key` to resolve to a blank `value`, set `@` as the value (e.g. if you want to remove certain characters in the text, or omit rendering some Markdown attributes).

