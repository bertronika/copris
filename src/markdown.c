/*
 * A naïve Markdown parser
 *
 * Why so? Because it only recognises some simple text attributes and doesn't alter
 * any other text layout whatsoever. Attributes include:
 *  - bold and italic emphasis (up to three asterisks and underscores)
 *  - 4 levels of headings (pound signs)
 *  - blockquotes (greater-than signs)
 *  - inline code and code blocks (one or three backticks/four whitespaces [*])
 *  - links, enclosed only in angle brackets (and none other)
 *
 *  [*] Code blocks with four whitespaces are disabled by default, check 'config.h'.
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
 * GNU GPLv3 or later. See files 'main.c' and 'COPYING' for more details.
 *
 * TODO parse underlined headings
 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <uthash.h>   /* uthash library - hash table       */
#include <utstring.h> /* uthash library - dynamic strings  */

#include "Copris.h"
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
	RULE        = 1 << 6,
	LINK        = 1 << 7
} attribute_t;

#define INSERT_TEXT(string)  \
        utstring_bincpy(converted_text, string, (sizeof string) - 1)

#define INSERT_CODE(string)  \
        insert_code_helper(string, features, converted_text)

#define MARKUP_ALLOWED       \
        (!inline_code_on && !code_block_on && !code_block_open && !link_on)

static void insert_code_helper(const char *, struct Inifile **, UT_string *);

/*
 * Asterisks bring quite a lot of ambiguity. That's the reason the parser deals so
 * much with them. They can represent:
 *  - start and end of bold, italic or bold and italic text
 *  - a list element
 *  - a horizontal rule
 *
 * The latter two aren't touched by this function, but shall not be mistakenly
 * parsed as bold/italic.
 */
void parse_markdown(UT_string *copris_text, struct Inifile **features)
{
	// Create a temporary string
	UT_string *converted_text;
	utstring_new(converted_text);

	attribute_t text_attribute = 0;
	bool bold_on = false;
	bool italic_on = false;
	bool inline_code_on = false;
	bool code_block_on = false;
	bool link_on = false;

	int heading_level = 0;
	bool code_block_open = false;
	bool blockquote_open = false;

	const char *text = utstring_body(copris_text);
	size_t text_len = utstring_len(copris_text);
	char last_char = ' ';

	static char *heading_on[]  = {0, "F_H1_ON", "F_H2_ON", "F_H3_ON", "F_H4_ON"};
	static char *heading_off[] = {0, "F_H1_OFF", "F_H2_OFF", "F_H3_OFF", "F_H4_OFF"};

	int current_line = 1;
	struct Error_line {
		int bold;
		int italic;
		int inline_code;
		int code_block;
		int link;
	} error_line = {0, 0, 0, 0, 0};

	size_t line_char_i = 0;

	bool escaped_char = (text[0] == '\\'); // Detect early escape character

	for (size_t i = 0; text[i] != '\0'; i++) {
		// Catch horizontal rules ('***'/'---') and copy them to output.
		if (!escaped_char && (i + 4 < text_len && text[i + 4] == '\n' && text[i] == '\n') &&
		    ((text[i + 1] == '*' && text[i + 2] == '*' && text[i + 3] == '*') ||
		     (text[i + 1] == '-' && text[i + 2] == '-' && text[i + 3] == '-'))) {
			text_attribute = RULE;
			i += 4;

		// Emphasis: inline '*'/'_' pairs for italic, '**'/'__' for bold,
		//           '***'/'___' for both.
		} else if (MARKUP_ALLOWED && !escaped_char && (text[i] == '*' || text[i] == '_')) {
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

			} else if ((!italic_on && i + 1 < text_len && text[i + 1] != ' ') ||
			           (italic_on && line_char_i != 0)) {
				text_attribute |= ITALIC;
				italic_on = !italic_on;
			}

		// Headings: '#' through '####' on a blank line. More than one space after the
		//           pound sign will be preserved (e.g. to center titles).
		} else if (MARKUP_ALLOWED && !escaped_char && (i == 0 || last_char == '\n') &&
		           (i + 1 < text_len && text[i] == '#')) {
			if (i + 2 < text_len && text[i + 1] == '#') {
				if (i + 3 < text_len && text[i + 2] == '#') {
					if (i + 4 < text_len && text[i + 3] == '#' && text[i + 4] == ' ') {
						heading_level = 4;
						i += 4;
						text_attribute |= HEADING;
					} else if (text[i + 3] == ' ') {
						heading_level = 3;
						i += 3;
						text_attribute |= HEADING;
					}
				} else if (text[i + 2] == ' ') {
					heading_level = 2;
					i += 2;
					text_attribute |= HEADING;
				}
			} else if (text[i + 1] == ' ') {
				heading_level = 1;
				i += 1;
				text_attribute |= HEADING;
			}

		// Blockquote: '> ' or '>\n' on a new line.
		} else if (MARKUP_ALLOWED && !escaped_char && (i == 0 || last_char == '\n') && (
		           i + 1 < text_len && text[i] == '>' &&
		           (text[i + 1] == ' ' || text[i + 1] == '\n'))) {
			text_attribute = BLOCKQUOTE;
			blockquote_open = true;

			if (text[i + 1] != '\n') // Keep blockquotes without any text
				i += 1;

		// Inline code: "`", and two block code variants: "```" at beginning/end of block
		//              or "    " (four spaces) on each line.
		} else if (!escaped_char && text[i] == '`' && !code_block_open) {
			if (i + 2 < text_len && text[i + 1] == '`' && text[i + 2] == '`') {
				text_attribute |= CODE_BLOCK;
				code_block_on = !code_block_on;
				i += 2;

				// Skip the info string (usually for syntax highlighting)
				while (text[i] != '\n' && i + 1 < text_len)
					i++;

			} else {
				text_attribute |= INLINE_CODE;
				inline_code_on = !inline_code_on;
			}
#ifndef DISABLE_WHITESPACE_CODE_BLOCK
		} else if (!escaped_char && (i == 0 || last_char == '\n') && !code_block_on &&
			       (i + 3 < text_len && text[i] == ' ' && text[i + 1] == ' ' &&
			        text[i + 2] == ' ' && text[i + 3] == ' ')) {
			text_attribute |= CODE_BLOCK;
			code_block_open = true;
			i += 3;
#endif
		// Link in angle brackets: inline '<'/'>' pairs
		} else if (MARKUP_ALLOWED && !escaped_char && text[i] == '<' && !code_block_open) {
			text_attribute = LINK;
			link_on = true;
		} else if (!escaped_char && link_on && text[i] == '>' && !code_block_open) {
			text_attribute = LINK;
			link_on = false;
		}

		// Check for an escape character. If found, skip converting the following markup element.
		// Note that each character has to be escaped separately.
		escaped_char = (text[i] == '\\');

		if (text_attribute == NONE) {
			// Reset open attributes on a new line
			if (text[i] == '\n') {
				if (heading_level) {
					assert(heading_level <= 4);
					INSERT_CODE(heading_off[heading_level]);
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
				error_line.italic = current_line;

		} else if (text_attribute == BOLD) {
			INSERT_CODE((bold_on ? "F_BOLD_ON" : "F_BOLD_OFF"));
			text_attribute &= ~(BOLD);
			if (bold_on)
				error_line.bold = current_line;

		} else if (text_attribute == HEADING) {
			assert(heading_level > 0 && heading_level <= 4);
			INSERT_CODE(heading_on[heading_level]);
			text_attribute &= ~(HEADING);

		} else if (text_attribute == BLOCKQUOTE) {
			INSERT_CODE("F_BLOCKQUOTE_ON");
			char quote_text[] = "> ";
			INSERT_TEXT(quote_text);
			text_attribute &= ~(BLOCKQUOTE);

		} else if (text_attribute == INLINE_CODE) {
			INSERT_CODE(inline_code_on ? "F_INLINE_CODE_ON" : "F_INLINE_CODE_OFF");
			text_attribute &= ~(INLINE_CODE);
			if (inline_code_on)
				error_line.inline_code = current_line;

		} else if (text_attribute == CODE_BLOCK) {
			INSERT_CODE((code_block_on || code_block_open) ?
			            "F_CODE_BLOCK_ON" : "F_CODE_BLOCK_OFF");
			text_attribute &= ~(CODE_BLOCK);
			if (code_block_on)
				error_line.code_block = current_line;

		} else if (text_attribute == RULE) {
			char rule_text[6];
			rule_text[0] = text[i - 4]; // \n
			rule_text[1] = text[i - 3]; // -
			rule_text[2] = text[i - 2]; // -
			rule_text[3] = text[i - 1]; // -
			rule_text[4] = text[i];     // \n
			rule_text[5] = '\0';
			INSERT_TEXT(rule_text);
			text_attribute &= ~(RULE);
		} else if (text_attribute == LINK) {
			if (link_on) {
				INSERT_TEXT("<");
				INSERT_CODE("F_ANGLE_BRACKET_ON");
				error_line.link = current_line;
			} else {
				INSERT_CODE("F_ANGLE_BRACKET_OFF");
				INSERT_TEXT(">");
			}
			text_attribute &= ~(LINK);
		}

		last_char = text[i];
		if (last_char == '\n') {
			current_line++;
			line_char_i = 0;
		} else {
			line_char_i++;
		}
	}

	// Close missing tags, notify user
	if (link_on) {
		INSERT_CODE("F_ANGLE_BRACKET_OFF");
		if (LOG_ERROR)
			PRINT_MSG("Warning: angle brackets still open on EOF, possibly in line %d.",
					  error_line.link);
	}

	if (code_block_on) {
		INSERT_CODE("F_CODE_BLOCK_OFF");
		if (LOG_ERROR)
			PRINT_MSG("Warning: code block still open on EOF, possibly in line %d.",
			          error_line.code_block);
	}

	if (inline_code_on) {
		INSERT_CODE("F_INLINE_CODE_OFF");
		if (LOG_ERROR)
			PRINT_MSG("Warning: inline code still open on EOF, possibly in line %d.",
			          error_line.inline_code);
	}

	if (bold_on) {
		INSERT_CODE("F_BOLD_OFF");
		if (LOG_ERROR)
			PRINT_MSG("Warning: bold text still open on EOF, possibly in line %d.",
			          error_line.bold);
	}

	if (italic_on) {
		INSERT_CODE("F_ITALIC_OFF");
		if (LOG_ERROR)
			PRINT_MSG("Warning: italic text still open on EOF, possibly in line %d.",
			          error_line.italic);
	}

	// Overwrite input text
	utstring_clear(copris_text);
	utstring_concat(copris_text, converted_text);
	utstring_free(converted_text);
}

static void insert_code_helper(const char *code, struct Inifile **features, UT_string *text)
{
	struct Inifile *s;
	HASH_FIND_STR(*features, code, s);

	assert(s != NULL);

	if (s->out_len == 0)
		return;

	utstring_bincpy(text, s->out, s->out_len);
}
