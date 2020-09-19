/*
 * Printer definitions for COPRIS
 */

#define PRSET_LEN 12

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

static char prsets[][PRSET_LEN + 1] = { "epson", "oki", "star" , { '\0' } };

//char printerset[9][6] = { "E!0\n", "BEL", "E!168", "E!40", "E!32", "EE", "EF", "E4", "E5" };
