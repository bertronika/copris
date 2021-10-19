/*
 * translate.c
 * Text and markdown translation/conversion for printers
 * 
 * Copyright (C) 2020-2021 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ini.h>    /* inih library   */
#include <uthash.h> /* uthash library */

#include "Copris.h"
#include "debug.h"
#include "translate.h"
#include "printerset.h"
#include "utf8.h"

int copris_loadtrfile(char *filename, struct Trfile **trfile) {
	FILE *file;
	int parse_error = 0;
	int definition_count;

	file = fopen(filename, "r");
	if(file == NULL)
		return raise_errno_perror(errno, "fopen", "Error opening translation file.");

	if(log_debug()) {
		log_date();
		printf("Parsing '%s'.\n", filename);
	}

	// `Your hash must be declared as a NULL-initialized pointer to your structure.'
	*trfile = NULL;

	parse_error = ini_parse_file(file, handler, trfile);

	// Negative return number - can be either:
	// -1  Error opening file - we've already handled this
	// -2  Memory allocation error - only if inih was built with INI_USE_STACK
	if(parse_error < 0) {
		fprintf(stderr, "inih: ini_malloc: Memory allocation error.\n");
		return 2;
	}

	// Positive return number - error on returned line number
	if(parse_error) {
		fprintf(stderr, "Error parsing translation file '%s', (first) fault on "
		                "line %d.\n", filename, parse_error);
		return 1;
	}

	if(log_info()) {
		definition_count = HASH_COUNT(*trfile);
		log_date();
		printf("Loaded translation file %s with %d definitions.\n", filename, definition_count);
	}

	// Close the file
	if(raise_perror(fclose(file), "close",
	                "Failed to close the translation file after reading."))
		return 1;

	return 0;
}

// [section]
// name = value
// key  = item
// `Handler should return nonzero on success, zero on error.'
int handler(void *user, const char *section, const char *name,
                   const char *value) {
	char *parserr;   // String to integer conversion error
	long temp_long;  // A temporary long integer
	(void)section;

	// Maximum name length can be 1 Unicode character
	if(utf8_count_codepoints(name, 2) > 1) {
		fprintf(stderr, "'%s': more than one character.\n", name);
		return 0;
	}

	// Convert value to a temporary long and check for correctness
	errno = 0;
	temp_long = strtol(value, &parserr, 0);

	if(raise_errno_perror(errno, "strtoul", "Error parsing number."))
		return 0;

	if(*parserr) {
		fprintf(stderr, "'%s': unrecognised character(s).\n", parserr);
		return 0;
	}

	// Read value must fit in an unsigned char
	if(temp_long < 0 || temp_long > 255) {
		fprintf(stderr, "'%s': value out of bounds.\n", value);
		return 0;
	}

	// Everything looks fine, insert into hash table.
	// The unique parameter for the table is the name string.
	struct Trfile **file = (struct Trfile**)user;
	struct Trfile *s;

	// Check for a duplicate key
	HASH_FIND_STR(*file, name, s);
	if(s == NULL) {
		// Key doesn't exist, add it
		s = malloc(sizeof(struct Trfile));
		strcpy(s->in, name);
		HASH_ADD_STR(*file, in, s);
	} else {
		if(log_error()) {
			log_date();
			printf("Definition for '%s' appears more than once in translation file, "
			       "overwriting old value.\n", name);
		}
	}

	// Item, however, can be assigned multiple times. Only the last
	// definition will take effect!
	s->out = (unsigned char)temp_long;

	if(log_debug()) {
		log_date();
		printf("%1s = %-4s | %-3ld (%zu)\n", name, value, temp_long, strlen(name));
	}

	return 1;
}

void copris_unload_trfile(struct Trfile **trfile) {
	struct Trfile *definition;
	struct Trfile *tmp;

	HASH_ITER(hh, *trfile, definition, tmp) {
		HASH_DEL(*trfile, definition);  // Delete definition
		free(definition);               // Free it
	}
}

// Translate input string. Input may contain multibyte characters, translated output
// string won't contain them - every translated character is exactly one byte long.
// If there's a multibyte character in the input that doesn't have a definition,
// it'll get fully copied without any modifications.
char *copris_translate(char *source, int source_len, struct Trfile **trfile) {
	// A duplicate copy of input string
	errno = 0;
	char *tr_source = strdup(source);
	if(tr_source == NULL) {
		raise_errno_perror(errno, "strdup", "Memory allocation error");
		exit(EXIT_FAILURE);
	}

	// Space for a (multibyte) character to be matched with definitions
	char inspected_char[UTF8_MAX_LENGTH + 1];

	// Output string with all the translation
	errno = 0;
	char *tr_string = malloc(source_len + 1);
	if(tr_string == NULL) {
		raise_errno_perror(errno, "strdup", "Memory allocation error");
		exit(EXIT_FAILURE);
	}
	tr_string[0] = '\0';

	struct Trfile *s;

	int src_i  = 0; // Source string iterator
	int insp_i = 0; // Inspected char iterator
	int tr_i   = 0; // Translated string iterator

	while(src_i < source_len) {
		inspected_char[insp_i] = tr_source[src_i];

		// Check for a multibyte character
		if(insp_i < UTF8_MAX_LENGTH && UTF8_IS_MULTIBYTE(inspected_char[insp_i])) {
			// Multibyte char found, load its next byte
			insp_i++;
			src_i++;
			continue;
		} else {
			// Either:
			// - end of multibyte character
			// - character in question was only single-byte
			inspected_char[insp_i + 1] = '\0';
		}

		// Match the loaded char with a definition, if it exists
		HASH_FIND_STR(*trfile, inspected_char, s);
		if(s != NULL) {
			// Definition found
			tr_string[tr_i] = s->out;
			tr_i++;
		} else {
			// Definition not found
			if(insp_i > 0) {
				// Copy multibyte character
				strcat(tr_string, inspected_char);

				tr_i += insp_i + 1;
				//tr_i += strlen(inspected_char);
			} else {
				// Copy single character
				tr_string[tr_i++] = tr_source[src_i];
			}
		}
		insp_i = 0;
		src_i++;
	}

	tr_string[tr_i] = '\0';

	free(tr_source);

	return tr_string;
}

static unsigned char *input;       // Chars that should be picked out
static unsigned char *replacement; // Chars that should be put in instead

void copris_translate(unsigned char *source, int source_len, unsigned char *ret) {
	int i; // Source array iterator
	int j; // Return array iterator
	int k; // Input/replacement array iterator
	
	for(i = 0, j = 0; source[i] != '\0'; i++, j++) { // Loop through source text
// 		ret[j] = 0;
		for(k = 0; input[k] != '\0'; k++) {          // Loop through input chars
			// Source matches input, start character exchange
			if(source[i] == input[k] && source[i] != ' ') {
				if(replacement[k] != ' ') {
					ret[j] = replacement[k];
				} else {
					// Get rid of the extra leading spaces in replacement
					j--;
				}
				
				// Get rid of the extra trailing spaces in input
				if(input[k + 1] == ' ') {
					ret[++j] = replacement[++k];
				}
				
				break;
			
			// No match, return original char.
			// If out of ASCII range, return '!'.
			} else if(source[i] != ret[j]) {
				ret[j] = (source[i] <= 127) ? source[i] : '!';
			}
		}
		if(log_debug() && i < source_len - 1)
			printf("%3d  %x -> %c\n", i, source[i], (j > -1) ? ret[j] : ' ');
		
	}
	ret[j] = '\0'; // This odd zero looks somewhat important, I presume...
}

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

void copris_printerset(unsigned char *source, int source_len, unsigned char *ret, int set) {
	int r = 0;
	
	// Text attributes, preserved over multiple printerset function calls
	static int bold_on   = 0;
	static int ital_on   = 0;
	static int ital_real = 0;
	static int bold_real = 0;
	static int reset_esc = 0; /* 1 for reset sequence, 2 for char. escape */
	static int heading_level = 0;
	static int heading_on    = 0;
	static char lastchar  = '\n'; // If last char is not newline, do not make a heading
	static char lastchar2 = '\n';
	
	for(int s = 0; s < source_len; s++) {
		/* Reset sequences */
		if(source[s] == '?' && lastchar == '\n' && reset_esc != 2) {
			reset_esc = 1;
			continue;
		}
		
		if(reset_esc == 1) {
			reset_esc = 0;
			if(source[s] == 'r') {
				r = escinsert(ret, r, printerset[set][C_RESET]);
			} else if(source[s] == 'b') {
				r = escinsert(ret, r, printerset[set][C_BELL]);
			} else {
				r = escinsert(ret, r, "?");
				goto copy_char;
			}
			s++; // Remove the newline
			continue;
		}
		
		if(source[s] == '\\' && reset_esc != 2) {
			reset_esc = 2;
			continue;
		}
		
		if(reset_esc == 2) {
			reset_esc = 0;
			if(source[s] != '\\') {
				goto copy_char; /* Oh no, goto */
			}
		}
		
		/* Italic and bold handling */
		if(source[s] == '*' && lastchar == '*' && lastchar2 == '*') {
			bold_on = bold_real ? -1 : 1;
			ital_on = ital_real ? -1 : 1;
			
			lastchar2 = lastchar;
			lastchar  = source[s];
			continue;
		}
		
		if(source[s] == '*' && lastchar == '*') {
			bold_on = bold_on ? -1 : 1;
			ital_on = 0; // Turn off italic if bold detected
			
			lastchar2 = lastchar;
			lastchar  = source[s];
			continue;
		}
		
		if(source[s] == '*') {
			ital_on = ital_real ? -1 : 1;
			
			lastchar2 = lastchar;
			lastchar  = source[s];
			continue;
		}
		
		/* Heading handling */
		if(source[s] == '#' && lastchar == '\n') {
			heading_level++;
			heading_on = 1;
			
			lastchar2 = lastchar;
			lastchar  = source[s];
			continue;
		}
		
		if(source[s] == '#' && heading_level > 0 && heading_level < 3) {
			heading_level++;
			
			lastchar2 = lastchar;
			lastchar  = source[s];
			continue;
		}
		
		if(heading_level) {
			// If there is no space after #, it is not a heading.
			// And if it's not, add it back.
			if(source[s] != ' ') {
				if(lastchar == '#') {
					for(; heading_level > 0; heading_level--) {
						r = escinsert(ret, r, "#");
					}
				}
				heading_on = 0;
				
			// If there is a space, skip putting it into output.
			} else if(lastchar == '#') {
				s++;
			}
		}
		
		/* Text output to returned array */
		if(source[s] == '\n') {
			if(heading_level == 1) {
				r = escinsert(ret, r, printerset[set][C_BOFF]);
				r = escinsert(ret, r, printerset[set][C_ULOFF]);
			} else if(heading_level == 2) {
				r = escinsert(ret, r, printerset[set][C_IOFF]);
				r = escinsert(ret, r, printerset[set][C_ULOFF]);
			}
			heading_level = 0;
		}
		
		if(bold_on == 1) {
			r = escinsert(ret, r, printerset[set][C_BON]);
			bold_on   = 2;
			bold_real = 2;
		} else if(bold_on == -1) {
			r = escinsert(ret, r, printerset[set][C_BOFF]);
			bold_on   = 0;
			bold_real = 0;
		}
		
		if(ital_on == 1) {
			r = escinsert(ret, r, printerset[set][C_ION]);
			ital_on   = 2;
			ital_real = 2;
		} else if(ital_on == -1) {
			r = escinsert(ret, r, printerset[set][C_IOFF]);
			ital_on   = 0;
			ital_real = 0;
		}
		
		if(heading_on) {
			r = escinsert(ret, r, printerset[set][C_DBLW]);
			
			if(heading_level == 1) {
				r = escinsert(ret, r, printerset[set][C_ULON]);
				r = escinsert(ret, r, printerset[set][C_BON]);
			} else if(heading_level == 2) {
				r = escinsert(ret, r, printerset[set][C_ULON]);
				r = escinsert(ret, r, printerset[set][C_ION]);
			}
			
			heading_on = 0;
		}
		
		copy_char:
		ret[r++]  = source[s];
		lastchar2 = lastchar;
		lastchar  = source[s];
	}

	ret[r] = '\0';
}

int escinsert(unsigned char *ret, int r, char *printerset) {
	for(int b = 0; printerset[b]; b++)
		ret[r++] = printerset[b];

	return r;
}
