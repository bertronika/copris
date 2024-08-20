*Encoding* and *printer feature files* are expected to be in the INI file format:

```ini
# Comment
key1 = whitespace separated values
key2 = whitespace separated values ; inline comment
...
```

Left-hand keys should be written out directly as UTF-8 characters or strings, right-hand values are expected to be in either decimal, octal (prefixed by a `0`) or hexadecimal base (prefixed with `0x`). Don't prepend zeroes to decimal numbers for alignment, as they'll be interpreted as octal numbers. Instead, use any amount of spaces (or tabulators).

One line can contain exactly *one* definition/command.

In case you need a `key` to resolve to a blank `value`, set `@` as the value (e.g. if you want to remove certain characters from the text, or omit rendering some Markdown attributes).

Here are some examples:

```ini
Å = 0xC5                ; for an encoding file
F_BOLD_OFF = 0x1B 0x45  ; for a printer feature file
F_H3_ON = @             ; ditto
```
