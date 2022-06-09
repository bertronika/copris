static const char *printer_commands[] = {
	/* Non-printable commands */
	"C_RESET",
	"C_BELL",

	/* Commands that have no predefined values */
	"C_BOLD_ON",
	"C_BOLD_OFF",
	"C_ITALIC_ON",
	"C_ITALIC_OFF",
	"C_H1_ON",
	"C_H1_OFF",
	"C_H2_ON",
	"C_H2_OFF",
	"C_H3_ON",
	"C_H3_OFF",
	"C_H4_ON",
	"C_H4_OFF",
	"C_H5_ON",
	"C_H5_OFF",
	"C_H6_ON",
	"C_H6_OFF",
	"C_DOUBLE_WIDTH",
	"C_UNDERLINE_ON",
	"C_UNDERLINE_OFF",
	"C_CODE_ON",
	"C_CODE_OFF",
	"C_CODE_BLOCK_ON",
	"C_CODE_BLOCK_OFF",
	"C_LINK_ON",
	"C_LINK_OFF",

	/* Commands that have predefined values if the printer set file doesn't define them */
	"C_LIST_ITEM", /*  '-'  */
	NULL
};
