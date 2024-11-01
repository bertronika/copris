> Error messages received after running copris with -t brasil.ini argument:
>
> > Value '192' in '192' is out of bounds.
> > Value '193' in '193' is out of bounds.
> > Value '194' in '194' is out of bounds.
> > ...
>
> I tried with hexadecimal values too and received the same "out of bounds" massage.

It appears you've hit an unintended limitation. Currently, the parser only works with numbers up to 127 (the maximum of a signed char), which is obviously unwanted.

> I would appreciate to know what I am doing wrong and if there is any limitation using characters above 126.

I'll amend the problem, time permitting. I'll update this issue for you to test it.

