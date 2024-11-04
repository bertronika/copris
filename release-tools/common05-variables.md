If you've defined a printer feature file, and it includes custom variables (prefixed with `C_`), you may use them directly in input text.

To allow for that, begin your text with a line, containing `COPRIS ENABLE-VARIABLES`. This is called the *modeline* and tells COPRIS to process variables in input text.

You may then call variables in the text file by prepending a dollar sign to them. I.e., if your variable is `C_SERIF`, `$C_SERIF` should be used in text. If you want to call multiple, write them out space-separated. Note that anything between the dollar sign and the new line is interpreted as a variable (or a series of them).

Furthermore, apart from already-defined variables, numerical values can be included in text. They must be written out in the same way as variables - prefixed with a dollar sign, and, if specifying a multitude of them, separated by white spaces. They must be in either decimal, octal or hexadecimal notation, as they would be in a printer feature file.

Lastly, comments can be passed in text. They again consist of a dollar sign, which is then followed by a number sign and your comment message.

Here are examples of all three notations:

```
COPRIS ENABLE-VARIABLES    (must be included at the top)
$# This reduces line spacing:
$C_ESC 0x33 25             (feature file has a C_ESC command defined)
```


## Mixing variables and text

Be wary of mixing variables and text in a sentence. Variables will only get detected properly if they're at the end of the sentence, not followed by any punctuation.

Meaning:

```
Word count is $C_WC        (works normally)
The sum is $C_WC words.    (fails - expects a variable named '$C_WC words.')
```

For reliable interpretation of variables, dedicate a whole line just for them, and make use of Markdown for mixing text and commands.

Lastly, if your input text contains a variable-symbol-prefixed word and you don't want COPRIS to interpret it, escape it by prefixing it with another dollar symbol.


## The modeline

It is expected in the first line of received text. Its format is:

```
COPRIS <required 1st option> [ optional 2nd option ]
```

Previous documentation already mentioned its use for for enabling variables. It serves one other purpose, disabling parsing Markdown in text.

Here are both modeline options and their short forms:

- `ENABLE-VARIABLES`; `ENABLE-VARS`
- `DISABLE-MARKDOWN`; `DISABLE-MD`

Letters are case-insensitive and the order of options isn't important. Thus, following lines are the same:

- `COPRIS ENABLE-VARIABLES DISABLE-MARKDOWN`
- `copris disable-md enable-vars`
