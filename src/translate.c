/*
 * translate.c
 * Translation file processing and translation for foreign printer code tables
 * 
 * Part of COPRIS - a converting printer server
 * (c) 2020 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 * 
 * Strings in C are a bit challenging... Multibyte characters are a PITA
 */

#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "translate.h"

// These two variables are used by both trfile and translate functions
unsigned char *input;       // Chars that should be picked out
unsigned char *replacement; // Chars that should be put in instead

void copris_trfile(char *filename) {
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
		log_debug("Loaded translation file %s\n", dat);
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
			 * the array lenghts will stay the same after processing.
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
				fprintf(stderr, "Integers in translation file either too big " 
				                "or lacking digits. Exiting...\n");
				exit(1);
			}
			
			// Exit if character not in ASCII range [0-9]
			if(c < 48 || c > 57) {
				fprintf(stderr, "Errorneous integers in translation file. Exiting...\n");
				exit(1);
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
				fprintf(stderr, "Found too many characters in translation file. " 
				                "Exiting...\n");
				exit(1);
			}
		}
	}
		if(digit != -1) {
			fprintf(stderr, "Integers in translation file lacking digits " 
			                "(leading 0?). Exiting...\n");
			exit(1);
		}
	
	input[i]       = '\0';
	replacement[j] = '\0';
	
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
	
// 	printf("\n");
	ferr = fclose(dat);
	log_perr(ferr, "close", "Failed to close the translation file after reading.");
}

unsigned char *copris_translate(unsigned char *source,       // Input string
								int            source_len,
								unsigned char *t_input,      // Chars to be picked out
								unsigned char *t_replacement // Chars to be put in
							   ) {
	unsigned char *ret = malloc(2 * source_len + 1); // Final translated array
	int i; // Source array iterator
	int j; // Return array iterator
	int k; // Input/replacement array iterator
	
	for(i = 0; source[i] != '\0'; i++) {      // Loop through source text
		for(k = 0; t_input[k] != '\0'; k++) { // Loop through input chars
			// Source matches input, start character exchange
			if(source[i] == t_input[k] && source[i] != ' ') {
				if(t_replacement[k] != ' ') {
					ret[j] = t_replacement[k];
				} else {
					// Get rid of the extra leading spaces in t_replacement
					j--;
				}
				
				// Get rid of the extra trailing spaces in t_input
				if(t_input[k + 1] == ' ') {
					ret[++j] = t_replacement[++k];
				}
				
				break;
			
			// No match, return original char.
			// If out of ASCII range, return '!'.
			} else if(source[i] != ret[j]) {
				ret[j] = (source[i] <= 127) ? source[i] : '!';
			}
		}
			
		if(log_debug())
			printf("   %x -> %c\n", source[i], ret[j]);
		
		j++;
	}
	ret[j] = '\0'; // This odd zero looks somewhat important, I presume...
	
	return ret;
}

// Yes, this function can overflow. Not with hardcoded maximum exponent though.
int power10(int exp)
{
	int result = 1;
	
	for(; exp > 0; exp--) {
		result *= 10;
	}
	
	return result;
}
