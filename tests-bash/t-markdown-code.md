In case you need a `key` to resolve to a blank `value`, set ` @ ` as the value.

---

```c
# define SAMPLE
// Flush streams to file and close it
int tmperr = fclose(file_ptr);
if (tmperr != 0) {
	PRINT_SYSTEM_ERROR("fclose", "Failed to close the output file.");
	return -1;
}
```

---

    Å = 0xC5                ; for an encoding file
    F_BOLD_OFF = 0x1B 0x45  ; for a printer feature file
    F_H3_ON = @             ; ditto

---

Symbols in code blocks `should *not* # be __converted__`.

```
This **shouldn't work**.
## This shouldn't work.
> This shouldn't work.
<This shouldn't work>.

***

The horizontal rule should stay there.
```
