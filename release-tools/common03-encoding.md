Firstly, check if COPRIS already includes one for your locale (`encodings/` directory).

If not, you can search the Internet for a code page table/listing with characters and their numberical values, which you can then manually convert to an appropriate format.

As a fallback, to at least figure out the character codes of your character set, run the `printchar.sh` test script, included with COPRIS, and pipe its output into the printer. It'll output printable characters, along with their decimal and hexadecimal values.

The format of an encoding file is simple. Start the line by entering a human-readable character, either via your keyboard or copied from somewhere else. Follow by an equals sign (optionally surrounded by spaces). Then, enter the numerical value of that character which your printer will understand.

Here's an example excerpt of the YUSCII encoding:

```ini
# Left-hand letters are readable to us, however, their UTF-8 codes
# are unknown to the printer.  Therefore, COPRIS replaces them with
# the right-hand hexadecimal codes.
Č = 0x5E  ; replacing ^
Ž = 0x40  ; replacing @
Š = 0x7B  ; replacing [
```

You might have noticed that YUSCII overrides some ASCII characters. Luckily, printer character sets often consist of additional characters (Greek letters, box drawing and maths symbols etc.), which can be also specified in an encoding file to imitate the lost ones:

```ini
^ = 0x27  ; replacement is an acute accent
@ = 0xE8  ; replacement is the Greek letter Phi
...
```

Have you made an encoding file COPRIS doesn't yet include? Contributions are always welcome!

You may find out that your printer supports a code page, appropriate for your locale, but it isn't selected by default when you turn it on. Read on, the chapter about printer feature files isn't only about formatting text, it also explains the custom commands that are used for setting up the printer.
