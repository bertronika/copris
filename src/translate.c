/*
 * translate.c
 * Text and markdown translation/conversion for printers
 * 
 * Part of COPRIS - a converting printer server
 * (c) 2020-21 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 * 
 * Strings in C are a bit challenging... Multibyte characters are a PITA
 */

#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "translate.h"
#include "printerset.h"

// These two variables are used by both trfile and translate functions
static unsigned char *input;       // Chars that should be picked out
static unsigned char *replacement; // Chars that should be put in instead

int copris_trfile(char *filename) {
	FILE *dat;        // Translation file
	int c;            // Character, read from *dat
	int lines    = 0; // Number of lines in *dat
	int newline  = 1; // Is last char \n?
	int i        = 0; // Input array iterator
	int j        = 0; // Replacement array iterator
	int i_cont   = 0; // Data still flowing into input array?
	int r_cont   = 0; // Data still flowing into replacement array?
	int is_input = 1; // Is char the next input or replacement?
	int digit    = 2; // Digit when concatenating decimal numbers
	int ferr;         // Error code of a file operation

	dat = fopen(filename, "r");
	if(!dat) {
		log_perr(-1, "fopen", "Failed to open translation file for reading.");
	} else {
		log_debug("Opened translation file %s\n", dat);
	}
	
	// Determine the number of lines/definitions
	while((c = fgetc(dat)) != EOF) {
		if(c == '\n' && newline) {
			lines++;
			newline = 0;
		} else {
			newline = 1;
		}
	}
	
// 	if(c == '\n')
// 		lines--;
	
	if(log_info()) {
		log_date();
		printf("Read %d translation definition(s).\n", lines);
	}
	
	// Put the file's stream pointer to the beginning
	rewind(dat);
	
	// Allocate as much space as there are characters in definitions
	input       = malloc(2 * lines + 1); // * 2 for UTF-8 wide chars
	replacement = malloc(3 * lines + 1); // * 3 for 3 ASCII decimal digits
	
	// Fill input and replacement arrays
	while((c = fgetc(dat)) != EOF) {
		if(c == '\t') {         // Switch into replacement mode
			is_input       = 0;
			replacement[j] = 0; // Zero out replacement field
			digit          = 2; // Start with hundreds
		} else if(c == '\n') {  // Switch into input mode
			is_input = 1;
		} else if(is_input) {   /* Input mode */
			r_cont     = 0;
			input[i++] = c;

			/* 
			 * If there's still data to be put into input array (eg. wide chars), 
			 * fill the same indexes in replacement array with spaces. That way 
			 * the array lengths will stay the same after processing.
			 */
			if(i_cont) {
				/* 
				 * Input not yet satisfied, put space into current replacement
				 * index to have the same array length in the end.
				 */
				replacement[j++] = ' ';
				i_cont = 0;
			} else {
				/* 
				 * Begin input continuation. If there are stil chars
				 * to be put into input array, enable i_cont that puts
				 * spaces into replacement array.
				 */
				i_cont = 1;
			}
			
		} else if(!is_input) {   /* Replacement mode */
			// Exit if first character bigger than ASCII 2 (representing hundreds)
			if(c > 50 && digit == 2) {
				fprintf(stderr, "One of integers in translation file is either too big "
				                "or lacking digits. ");
				lines = -1;
				break;
			}
			
			// Exit if character not in ASCII range [0-9]
			if(c < 48 || c > 57) {
				fprintf(stderr, "One of integers in translation file is errorneous. ");
				lines = -1;
				break;
			}
			
			i_cont = 0;
			
			// Assemble the integer
			replacement[j] += ((c - 48) * power10(digit));
			
			// Hundreds to tens to ones
			digit--;

			if(digit == -1) {
				j++;
				
				/* 
				 * Vice versa input mode. If there are still chars to be put into
				 * replacement array, put spaces into input array.
				 */
				if(r_cont) {
					/* 
					 * Replacement not yet satisfied, put space into current input
					 * index to have the same array length in the end.
					 */
					input[i++] = ' ';
					r_cont = 0;
				} else {
					/* 
					 * Begin replacement continuation. If there are stil chars
					 * to be put into replacement array, enable r_cont that puts
					 * spaces into input array.
					 */
					r_cont = 1;
				}
				
			} else if(digit < -1) {
				fprintf(stderr, "Found too many characters in translation file. ");
				lines = -1;
				break;
			}
		}
	}
	
	if(digit != -1 && lines > 0) {
		fprintf(stderr, "One of integers in translation file is lacking digits " 
						"(leading zero?). ");
		lines = -1;
	}
	
	ferr = fclose(dat);
	log_perr(ferr, "close", "Failed to close the translation file after reading.");
	
	input[lines < 0 ? 0 : i]       = '\0';
	replacement[lines < 0 ? 0 : j] = '\0';
	
	if(log_info() && lines > 0) {
		log_date();
		printf("%d translation definition(s) successfully loaded.\n", lines);
	}
	
	if(log_debug()) {
		printf("input      : ");
		for(int p = 0; input[p] != 0; p++) {
			printf("%3x ", input[p]);
		}
		
		printf("\nreplacement: ");
		for(int d = 0; replacement[d] != 0; d++) {
			printf("%3x ", replacement[d]);
		}
		
		printf("\nreplacement: ");
		for(int d = 0; replacement[d] != 0; d++) {
			printf("%3d ", replacement[d]);
		}
		
		printf("\n");
	}
	
	if(lines < 0) {
		free(input);
		free(replacement);
		return 1;
	} else
		return 0;
}

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
	set--;
	
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

// Yes, this function can overflow. Not with hardcoded maximum exponent though.
int power10(int exp) {
	int result = 1;
	
	for(; exp > 0; exp--) {
		result *= 10;
	}
	
	return result;
}
