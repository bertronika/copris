If you've defined a printer feature file, and it includes custom variables (prefixed with `C_`), you may use them directly in input text.

To allow for that, specify the `-c` argument when running COPRIS and begin your text with a line, containing `COPRIS ENABLE-COMMANDS`. This is called the *modeline* and tells COPRIS to process variables in input text.

You may then call variables in the text file by omitting their `C_` prefix and prepending a special symbol to them, usually a dollar sign (this is configurable in `config.h`. I.e., if your variable is `C_SERIF`, `$SERIF` is used in text to invoke it.

Furthermore, apart from already-defined variables, numerical values can be included in text. They must be prefixed with the same symbol as custom variables and then specified in decimal, octal or hexadecimal notation, as they would be in a printer feature file.

Lastly, comments can be passed in text. They consist of the same prefix character as custom variables, followed by a number sign and the word that needs to be commented out. 

You cannot comment out a whole line (or multiple space-separated words), only single words. To circumvent that, separate your words with some other character, such as an underscore or a non-breaking space.

For proper detection of all of the above commands, they must be prefixed and suffixed by (at least) one space.

Here are examples of all three of the beforementioned commands:

```
COPRIS ENABLE-COMMANDS     (must be included at the top)
$# Reduce line spacing     (non-breaking spaces are used in this line)
$ESC $0x33 $25             (feature file has a C_ESC command defined)
```


## The modeline

It is expected in the first line of input text in the following format:

```
COPRIS <required 1st option> [ optional 2nd option ]
```

Above text already mentioned its use for for enabling command detection. It serves one other purpose, disabling parsing Markdown in text.

Here are both modeline options and their short forms:

- `ENABLE-COMMANDS`; `ENABLE-CMD`
- `DISABLE-MARKDOWN`; `DISABLE-MD`

Letters are case-insensitive and the order of options isn't important. Thus, following lines are the same:

- `COPRIS ENABLE-COMMANDS DISABLE-MARKDOWN`
- `copris disable-md enable-cmd`
