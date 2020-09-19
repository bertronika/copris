/*
 * Printer definitions for COPRIS
 */
#include "printerset.h"

/*
 * Instructions in printerset[]:
 * 0  newline + reset
 * 1  bell
 * 2  h1
 * 3  h2
 * 4  h3
 * 5  bold on
 * 6  bold off
 * 7  ital on
 * 8  ital off
 */

char prsets[][PRSET_LEN + 1] = {
	"epson",
	"dummy",
	{ '\0' }
};

char printerset[2][9][6] = {{
	"\x1b""!0\n", "\x07", 
	"\x1b""!168", "\x1b!""40", "\x1b""!32",
	"\x1b""E",    "\x1b""F",   "\x1b""4", "\x1b""5"
},{
	"A", "B", 
	"C", "D", "E",
	"F",  "G", "H", "I"
}};
