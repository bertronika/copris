/*
 * This is a list of every possible command that can be specified in a printer feature file.
 */

static const char *printer_commands[] = {
	// Formatting commands; both parts of a pair must be defined.
	// These map to Markdown attributes.
	"F_BOLD_ON",
	"F_BOLD_OFF",
	"F_ITALIC_ON",
	"F_ITALIC_OFF",
	"F_H1_ON",
	"F_H1_OFF",
	"F_H2_ON",
	"F_H2_OFF",
	"F_H3_ON",
	"F_H3_OFF",
	"F_H4_ON",
	"F_H4_OFF",
	"F_BLOCKQUOTE_ON",
	"F_BLOCKQUOTE_OFF",
	"F_INLINE_CODE_ON",
	"F_INLINE_CODE_OFF",
	"F_CODE_BLOCK_ON",
	"F_CODE_BLOCK_OFF",
	"F_ANGLE_BRACKET_ON",
	"F_ANGLE_BRACKET_OFF",

	// Session commands; used before and after printing received text,
	// or when COPRIS starts and before it exits.
	"S_BEFORE_TEXT",
	"S_AFTER_TEXT",
	"S_AT_STARTUP",
	"S_AT_SHUTDOWN",

	NULL
};
