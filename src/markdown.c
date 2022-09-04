/*
 * A naïve Markdown parser
 *
 * Why so? Because it only recognises some simple text attributes and doesn't alter
 * any other text layout whatsoever. Attributes include:
 *  - emphasis (bold and italic)
 *  - 4 levels of headings (denoted with pound signs, not underlined)
 *  - blockquotes
 *  - inline code and code blocks
 *
 * Everything else in the text is left untouched -- white space, lists, rules, line
 * breaks, paragraphs. There's no HTML or other markup/layout engine at the end of
 * the printer pipeline that passes through COPRIS (in its intended use), and plain
 * text with a fixed pitch can be often laid out nicely on its own. Moreover, COPRIS
 * doesn't show any text preview. It would be a waste of paper, ink and time to print
 * out one or more pages, and then figure out it has been restructured against our
 * likings. The previous parser in COPRIS, 'cmark', was following that philosophy --
 * it would semantically parse Markdown elements, and leave the layout to another
 * program. Nothing wrong with that, of course, but it was too much of a hassle to
 * work around. In essence, a proper parser was too powerful.
 *
 * Some inspiration for the syntax is taken from the CommonMark specification, but
 * nothing more. No compliance is guaranteed. You've been warned :)
 *
 * Copyright (C) 2022 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <uthash.h>   /* uthash library - hash table       */
#include <utstring.h> /* uthash library - dynamic strings  */

#include "Copris.h"
#include "config.h"
#include "debug.h"
#include "markdown.h"

typedef enum attribute {
	NONE        = 0,
	BOLD        = 1 << 0,
	ITALIC      = 1 << 1,
	HEADING     = 1 << 2,
	BLOCKQUOTE  = 1 << 3,
	INLINE_CODE = 1 << 4,
	CODE_BLOCK  = 1 << 5,
	RULE        = 1 << 6
} attribute_t;

#define INSERT_TEXT(string)  \
        utstring_bincpy(converted_text, string, (sizeof string) - 1)

#define INSERT_CODE(string)  \
        insert_code_helper(string, prset, converted_text)

static void insert_code_helper(const char *, struct Inifile **, UT_string *);

/*
 * Asterisks present quite a lot of ambiguity. That's the reason the parser deals so
 * much with them. They can represent:
 *  - start and end of bold, italic or bold and italic text
 *  - a list element
 *  - a horizontal rule
 *
 * The latter two aren't touched by this function, but shall not be parsed as bold/italic.
 */
void parse_markdown(UT_string *copris_text, struct Inifile **prset)
{
	// Create a temporary string
	UT_string *converted_text;
	utstring_new(converted_text);

	attribute_t text_attribute = 0;
	bool bold_on = false;
	bool italic_on = false;
	bool inline_code_on = false;
	bool code_block_on = false;

	int heading_level = 0;
	bool code_block_open = false;
	bool blockquote_open = false;

	char *text      = utstring_body(copris_text);
	size_t text_len = utstring_len(copris_text);
	char last_char  = ' ';

	int current_line = 1;
	int error_in_line = 0;
// 	size_t line_length = 0;

// 	for (; text[line_length] != '\n'; line_length++);
// 	for (size_t i = 0; i < chars_until_nl; i++)

	for (size_t i = 0; text[i]; i++) {
		// Catch horizontal rules (`***') and copy them to output.
		if (i + 3 < text_len && text[i] == '*' && text[i + 1] == '*' &&
		    text[i + 2] == '*' && text[i + 3] == '\n') {
			text_attribute = RULE;
			i += 3;

		// Emphasis: inline `*'/`_' pairs for italic, `**`/`__' for bold,
		//           `***'/`___' for both.
		} else if ((text[i] == '*' || text[i] == '_') &&
		           (i + 1 < text_len && text[i + 1] != ' ')) {
			if (i + 1 < text_len && (text[i + 1] == '*' || text[i + 1] == '_')) {
				if (i + 2 < text_len && (text[i + 2] == '*' || text[i + 2] == '_')) {
					text_attribute |= ITALIC | BOLD;
					bold_on   = !bold_on;
					italic_on = !italic_on;
					i += 2;
				} else {
					text_attribute |= BOLD;
					bold_on = !bold_on;
					i += 1;
				}
			} else {
				text_attribute |= ITALIC;
				italic_on = !italic_on;
			}

		// Headings: `#' through `####' on a blank line. More than one space after the
		//           pound sign will be preserved (e.g. to center titles).
		} else if ((i == 0 || last_char == '\n') &&
		           (i + 1 < text_len && text[i] == '#')) {
			text_attribute |= HEADING;

			if (i + 2 < text_len && text[i + 1] == '#') {
				if (i + 3 < text_len && text[i + 2] == '#') {
					if (i + 4 < text_len && text[i + 3] == '#' && text[i + 4] == ' ') {
						heading_level = 4;
						i += 4;
					} else if (text[i + 3] == ' ') {
						heading_level = 3;
						i += 3;
					}
				} else if (text[i + 2] == ' ') {
					heading_level = 2;
					i += 2;
				}
			} else if (text[i + 1] == ' ') {
				heading_level = 1;
				i += 1;
			}

		// Blockquote: `> '
		} else if ((i == 0 || last_char == '\n') &&
		           (i + 1 < text_len && text[i] == '>' && text[i + 1] == ' ')) {
			text_attribute = BLOCKQUOTE;
			blockquote_open = true;
			i += 1;

		// Inline code: "`" and two block code syntaxes: "```" at beginning and end
		//              or "    " on each line.
		} else if (text[i] == '`' && !code_block_open) {
			if (i + 2 < text_len && text[i + 1] == '`' && text[i + 2] == '`') {
				text_attribute |= CODE_BLOCK;
				code_block_on = !code_block_on;
				i += 2;
			} else {
				text_attribute |= INLINE_CODE;
				inline_code_on = !inline_code_on;
			}
		} else if ((i == 0 || last_char == '\n') && !code_block_on &&
			       (i + 3 < text_len && text[i] == ' ' && text[i + 1] == ' ' &&
			        text[i + 2] == ' ' && text[i + 3] == ' ')) {
			text_attribute |= CODE_BLOCK;
			code_block_open = true;
			i += 3;
		}

		if (text_attribute == NONE) {
			// Reset open attributes on a new line
			if (text[i] == '\n') {
				if (heading_level) {
					char heading_code[9];
					snprintf(heading_code, 9, "F_H%d_OFF", heading_level);
					INSERT_CODE(heading_code);
					heading_level = 0;
				} else if (blockquote_open) {
					INSERT_CODE("F_BLOCKQUOTE_OFF");
					blockquote_open = false;
				} else if (code_block_open) {
					INSERT_CODE("F_CODE_BLOCK_OFF");
					code_block_open = false;
				}
			}

			utstring_bincpy(converted_text, &text[i], 1);

		} else if (text_attribute == (ITALIC | BOLD)) {
			if (bold_on)
				INSERT_CODE("F_BOLD_ON");

			INSERT_CODE((italic_on ? "F_ITALIC_ON" : "F_ITALIC_OFF"));

			if (!bold_on)
				INSERT_CODE("F_BOLD_OFF");

			text_attribute &= ~(ITALIC | BOLD);

		} else if (text_attribute == ITALIC) {
			INSERT_CODE((italic_on ? "F_ITALIC_ON" : "F_ITALIC_OFF"));
			text_attribute &= ~(ITALIC);
			if (italic_on)
				error_in_line = current_line;

		} else if (text_attribute == BOLD) {
			INSERT_CODE((bold_on ? "F_BOLD_ON" : "F_BOLD_OFF"));
			text_attribute &= ~(BOLD);
			if (bold_on)
				error_in_line = current_line;

		} else if (text_attribute == HEADING) {
			char heading_code[8];
			snprintf(heading_code, 8, "F_H%d_ON", heading_level);
			INSERT_CODE(heading_code);
			text_attribute &= ~(HEADING);

		} else if (text_attribute == BLOCKQUOTE) {
			INSERT_CODE("F_BLOCKQUOTE_ON");
			text_attribute &= ~(BLOCKQUOTE);

		} else if (text_attribute == INLINE_CODE) {
			INSERT_CODE(inline_code_on ? "F_INLINE_CODE_ON" : "F_INLINE_CODE_OFF");
			text_attribute &= ~(INLINE_CODE);
			if (inline_code_on)
				error_in_line = current_line;

		} else if (text_attribute == CODE_BLOCK) {
			INSERT_CODE((code_block_on || code_block_open) ?
			            "F_CODE_BLOCK_ON" : "F_CODE_BLOCK_OFF");
			text_attribute &= ~(CODE_BLOCK);
			if (code_block_on)
				error_in_line = current_line;

		} else if (text_attribute == RULE) {
			INSERT_TEXT("***\n");
			text_attribute &= ~(RULE);
		}

		last_char = text[i];
		if (last_char == '\n')
			current_line++;
	}

	// Close missing tags
	if (code_block_on)
		INSERT_CODE("F_CODE_BLOCK_OFF");

	if (inline_code_on)
		INSERT_CODE("F_INLINE_CODE_OFF");

	if (bold_on)
		INSERT_CODE("F_BOLD_OFF");

	if (italic_on)
		INSERT_CODE("F_ITALIC_OFF");

	// Notify about the first occurence. The order matters!
	if (LOG_ERROR) {
		if (code_block_on) {
			PRINT_MSG("Warning: code block still open on EOF, possibly in line %d.",
			          error_in_line);
			error_in_line = 0;

		} else if (error_in_line && inline_code_on) {
			PRINT_MSG("Warning: inline code still open on EOF, possibly in line %d.",
			          error_in_line);
			error_in_line = 0;

		} else if (error_in_line && bold_on) {
			PRINT_MSG("Warning: bold text still open on EOF, possibly in line %d.",
			          error_in_line);
			error_in_line = 0;

		} else if (error_in_line && italic_on) {
			PRINT_MSG("Warning: italic text still open on EOF, possibly in line %d.",
			          error_in_line);
			error_in_line = 0;
		}
	}

	// Overwrite input text
	utstring_clear(copris_text);
	utstring_bincpy(copris_text, utstring_body(converted_text), utstring_len(converted_text));
	utstring_free(converted_text);
}

static void insert_code_helper(const char *code, struct Inifile **prset, UT_string *text)
{
	struct Inifile *s;
	HASH_FIND_STR(*prset, code, s);

	assert(s != NULL);

	if (*s->out == '\0')
		return;

	size_t code_len = strlen(s->out);
	utstring_bincpy(text, s->out, code_len);
}
