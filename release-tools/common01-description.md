COPRIS is a printer server and text conversion program that bridges the gap between modern, UTF-8-encoded, Markdown-formatted text and (older) dot-matrix and thermal printers.

When provided with the appropriate configuration files and options, COPRIS reads plain text from a local source or the network (with a built-in TCP socket server), applies optional processing methods to it and sends it to the destination device (or file).

Two main text processing methods stand out in COPRIS; they are:


## Character recoding via encoding files

Older printers usually don't support multi-byte UTF-8-encoded text. Rather, they use code pages and customised character sets which may vary between different manufacturers. COPRIS can load a character *encoding file* that is used to recode received text. By writing or selecting a premade one for your printer and locale, you allow COPRIS to recode (replace) received characters with appropriate locale-specific ones, understood by your printer.

This is particulary useful in cases where printer distributors swapped lesser-used ASCII characters (such as `~ @ {`) with those from the national alphabet, so they could be typed out directly from the non-localised keyboard. By finding similar-looking characters in printer's character set, you may, crudely, regain those lost characters.

However, *encoding files* aren't tied only to locale conversions, you may use them to remap any character. Examples include typographic quotation marks, em dashes, copyright symbol and others, commonly found in character sets of various printers.


## Markdown formatting via printer feature files

Many printers support commands (escape codes) for formatting printed text (e.g. Epson has its ESC/P control language). COPRIS can read a *printer feature file* that specifies such commands, invoked by following Markdown attributes:

- bold and italic *emphasis* via asterisks (`*`, `**`, `***`) or underscores (`_`, `__`, `___`)
- up to 4 levels of *headings* via number signs (`#`, `##`, `###`, `####`)
- *blockquotes* via greater-than signs and a space (`> `)
- *inline code* via single backticks (`` ` ``)
- *code blocks* via triple backticks (`` ``` ``) -- or four spaces, but those are disabled by default
- *links* via angle brackets (`< >`)

Any command can be bound to mentioned attributes. If desired, bold emphasis can trigger double-width text, headings may change the font face or size and links can be made underlined. There are no limits, as long as your *printer feature file* contains commands that are understood by the printer (or any other destination device).

You might have noticed that only a subset of the Markdown specification is supported. COPRIS intentionally doesn't alter text layout (white space, paragraphs etc.), since there's no markup/layout engine. You may freely format your input plain-text files to your liking, and COPRIS will preserve the spacing and newlines (just be warned about four leading spaces triggering a code block).
