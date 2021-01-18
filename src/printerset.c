/*
 * printerset.c
 * Printer feature sets
 * 
 * Part of COPRIS - a converting printer server
 * (c) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 * 
 * - https://epson.ru/upload/iblock/057/esc-p.pdf
 */

#include "printerset.h"

/*
 * Instructions in printerset[] (printerset.h):
 * C_RESET  reset
 * C_BELL   bell
 * C_DBLW   double-width
 * C_ULON   underline on
 * C_ULOFF  underline off
 * C_BON    bold on
 * C_BOFF   bold off
 * C_ION    italic on
 * C_IOFF   italic off
 */

// Preset definitions with the name as the first string
char printerset[][10][INSTRUC_LEN + 1] = {
{
	"epson",
	"\x1b\x40",     "\x07",
	"\x0e",
	"\x1b\x2d\x31", "\x1b\x2d\x30",
	"\x1b\x45",     "\x1b\x46",
	"\x1b\x34",     "\x1b\x35"
},
{
	"dummy",
	"+RES\n", "+BEL\n",
	"+DW",
	"+UL1", "+UL0",
	"+B1",  "+B0",
	"+I1",  "+I0"
},
{{'\0'}} // Nothing below this line
};
