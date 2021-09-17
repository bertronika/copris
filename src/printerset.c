/*
 * printerset.c
 * Printer feature sets
 * 
 * Copyright (C) 2020-2021 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
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
