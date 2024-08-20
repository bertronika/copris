Before making one, check if COPRIS already includes one for your locale (`encodings/` directory in the source tree).

The format of an encoding file is as follows. Start the line by entering a your desired character, either via the keyboard or copied from somewhere else. Follow by an equals sign. Then, enter the numerical value of the same character your printer will understand.

Here's an example excerpt of the YUSCII encoding:

```ini
# Left-hand letters are readable to us, however, their UTF-8 codes
# are unknown to the printer. Therefore, COPRIS replaces them with
# the right-hand values, here represented in hexadecimal.
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

Have you transcribed a national or a well known encoding COPRIS doesn't yet include? Contributions are always welcome!

You may find out that your printer supports a code page, appropriate for your locale, but it isn't selected by default when you turn it on. Read on, the chapter about printer feature files explains session commands which can be used for setting up the printer.
