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
 * Instructions in printerset[]:
 * 0  reset
 * 1  bell
 * 2  double-width
 * 3  underline on
 * 4  underline off
 * 5  bold on
 * 6  bold off
 * 7  italic on
 * 8  italic off
 */

// Available presets to be listed when invoking the -v argument
char prsets[][PRSET_LEN + 1] = {
	"epson",
	"dummy",
	{ '\0' }
};

// Preset definitions, order corresponding to prsets[]
char printerset[][9][INSTRUC_LEN + 1] = {{
	"\x1b\x40",     "\x07",
	"\x0e",
	"\x1b\x2d\x31", "\x1b\x2d\x30",
	"\x1b\x45",     "\x1b\x46",
	"\x1b\x34",     "\x1b\x35"
},{
	"+RES", "+BEL",
	"+DW",
	"+UL1", "+UL0",
	"+B1",  "+B0",
	"+I1",  "+I0"
}};
