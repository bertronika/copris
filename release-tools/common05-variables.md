Any custom variable, specified in a printer feature file, can be invoked from within the input text. Specify the `-c` argument when running COPRIS and begin your text with `COPRIS ENABLE-COMMANDS` and a new line. You may then call variables in the text file by omitting their `C_` prefix, prepending them a special symbol, usually a dollar sign (this is configurable in `config.h`. I.e., if your variable is `C_SERIF`, `$SERIF` is used in text to invoke it.

Furthermore, apart from already-defined variables, numerical values can be included in text. They must be prefixed with the same symbol as custom variables and then specified in decimal, octal or hexadecimal notation, as they would be in a printer feature file.

Lastly, comments can be passed in text. They consist of the same prefix character as custom variables, followed by a number sign and the text that needs to be commented out. **Be aware** that whitespace characters aren't permitted in a comment. This means that, for example in a series of custom variables or numerical values, each can be commented out separately, without impacting the surrounding ones. For commenting out multiple words, you must find some other character, such as underscore or a non-breaking space.

Here's an example of all three of the beforementioned commands:

```
$COPRIS ENABLE-COMMANDS
$# Reduce line spacing    (non-breaking spaces are used in this line)
$ESC $0x33 $25            (feature file has a C_ESC command defined)
```
