As with encoding files, COPRIS' source tree might have the right one in the `feature-files/` directory. If the file name seems appropriate, check the first few lines of the file to see if your printer/use case is supported.

Else, consult the printer's manual. There should be a section on escape codes. Generate a sample printer feature file (`copris --dump-commands > my-printer.ini`). Uncomment the appropriate commands and append values from the manual, in the same way as with encoding files.

Example from Epson's LX-300 manual, page A-14 (91):

```
ASCII   Dec.   Hex.   Description
----------------------------------------
ESC 4   52     34     Select Italic Mode
ESC 5   53     35     Cancel Italic Mode
```

The corresponding printer feature file lines are:

```ini
# lx300.ini
F_ITALIC_ON  = 0x1B 0x34  ; hexadecimal notation, 0x1B = ESC
F_ITALIC_OFF = 27 53      ; decimal notation, 27 = ESC
```


## Variables in printer feature files

You can use existing command names as variables, as long as you define the command *before* using it as a variable. Furthermore, you may define your own custom variables and use them in existing commands. For COPRIS to recognise them, they must be prefixed with `C_`! Variables may be interweaved with commands.

Examples:

```ini
# lx300.ini - continued
C_UNDERLINE_ON  = 0x1B 0x2D 0x31
C_UNDERLINE_OFF = 0x1B 0x2D 0x30
F_H1_ON  = C_UNDERLINE_ON F_ITALIC_ON
F_H1_OFF = F_ITALIC_OFF C_UNDERLINE_OFF
```


## Session commands

COPRIS provides two command pairs for sending repetitive settings to the printer. They may be used to set the code page, text margins, line spacing, font face, character density, initialise/reset the printer and so on:

- `S_AT_STARTUP` and `S_AT_SHUTDOWN` - sent to the printer once after COPRIS starts and once
  before it exits
- `S_BEFORE_TEXT` and `S_AFTER_TEXT` - sent to the printer each time text is received, in
  order `S_BEFORE_TEXT` - *received text* - `S_AFTER_TEXT`

Examples:

```ini
# lx300.ini - continued
S_AT_STARTUP = 0x07  ; sound the bell (beeper)
S_AFTER_TEXT = 0x0C  ; trigger form feed after printing
```
