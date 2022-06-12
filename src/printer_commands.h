static const char *printer_commands[] = {
	/* Non-printable commands */
	"C_RESET",
	"C_BELL",

	/* Formatting commands that have no predefined values */
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
	"F_H5_ON",
	"F_H5_OFF",
	"F_H6_ON",
	"F_H6_OFF",
	"F_DOUBLE_WIDTH",
	"F_UNDERLINE_ON",
	"F_UNDERLINE_OFF",
	"F_CODE_ON",
	"F_CODE_OFF",
	"F_CODE_BLOCK_ON",
	"F_CODE_BLOCK_OFF",
	"F_LINK_ON",
	"F_LINK_OFF",

	/* Commands with predefined values if the printer set file doesn't define them */
	"P_LIST_ITEM", /*  '-'  */
	NULL
};
