static const char *printer_commands[] = {
	/* Non-printable commands */
	"C_RESET",
	"C_BELL",

	/* Formatting commands; both parts of a pair must be defined */
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
	"F_LINK_ON",
	"F_LINK_OFF",

	NULL
};
