If you've defined a printer feature file, and it includes custom variables (prefixed with `C_`), you may use them directly in input text.

To allow for that, specify the `-c` argument when running COPRIS and begin your text with a line, containing `COPRIS ENABLE-COMMANDS`. This is called the *modeline* and tells COPRIS to process variables in input text.

You may then call variables in the text file by omitting their `C_` prefix and prepending a dollar sign to them. I.e., if your variable is `C_SERIF`, `$SERIF` should be used in text.

Furthermore, apart from already-defined variables, numerical values can be included in text. They must be also be prefixed with a dollar sign and then specified in decimal, octal or hexadecimal notation, as they would be in a printer feature file.

Lastly, comments can be passed in text. They again consist of a dollar sign, which is then followed by a number sign and the word that needs to be commented out. You cannot comment out multiple space-separated words, only single words. To circumvent that, separate them with some other character, such as an underscore or a non-breaking space.

Here are examples of all three notations:

```
COPRIS ENABLE-COMMANDS     (must be included at the top)
$# Reduce line spacing     (non-breaking spaces are used in this line)
$ESC $0x33 $25             (feature file has a C_ESC command defined)
```

## Separating variables from text

COPRIS leaves white space surrounding variables intact when extracting them from text. If you'd like to omit it, terminate consecutive variables with a semicolon and leave no white space after singular ones:

```
Characters $0x3B $0x3B terminate $0x2E  ->  Characters ; ; terminate .
Characters $0x3B;$0x3B terminate$0x2E   ->  Characters ;; terminate.
```

Note that punctuation can be left adjoined with the command. If a semicolon is wanted after it being used as a separator, specify it twice:

```
It sums up to 86 $PERCENT_SIGN.  ->  It sums up to 86 %.
val = $BIT_1;$BIT_0;;            ->  val = 10;
```

Lines with only one variable can be useful for sending configuration parameters to a printer. However, the new line they leave behind can be unwanted. Therefore, terminate such variables with the same symbol they begin with:

```
---
$SERIF   |
text...  | text...
---
$SERIF$  | text...
text...  |
---
```

You might want to have a variable-symbol-prefixed word in your text without COPRIS complaining it isn't defined. In that case, prefix such word with two dollar symbols.


## The modeline

It is expected in the first line of input text when COPRIS is started with `-c`. Its format is:

```
COPRIS <required 1st option> [ optional 2nd option ]
```

Previous documentation already mentioned its use for for enabling variables. It serves one other purpose, disabling parsing Markdown in text.

Here are both modeline options and their short forms:

- `ENABLE-COMMANDS`; `ENABLE-CMD`
- `DISABLE-MARKDOWN`; `DISABLE-MD`

Letters are case-insensitive and the order of options isn't important. Thus, following lines are the same:

- `COPRIS ENABLE-COMMANDS DISABLE-MARKDOWN`
- `copris disable-md enable-cmd`
