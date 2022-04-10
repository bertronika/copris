/*
 * printerset.c
 * Printer feature sets
 * 
 * Copyright (C) 2020-2022 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 *
 * This file is part of COPRIS, a converting printer server, licensed under the
 * GNU GPLv3 or later. See files `main.c' and `COPYING' for more details.
 */

#include <stdio.h>
#include <errno.h>
#include <ini.h>    /* inih library   */
#include <uthash.h> /* uthash library */

#include "Copris.h"
#include "debug.h"
#include "printerset.h"
#include "utf8.h"

void copris_initprset(struct Prset **prset) {
	const char *all_codes[] = {
		"C_RESET",
		"C_BELL",
		"C_DOUBLE_WIDTH",
		"C_UNDERLINE_ON",
		"C_UNDERLINE_OFF",
		"C_BOLD_ON",
		"C_BOLD_OFF",
		"C_ITALIC_ON",
		"C_ITALIC_OFF",
		NULL
	};

	// `Your hash must be declared as a NULL-initialized pointer to your structure.'
	*prset = NULL;

	struct Prset *s;

	for (int i = 0; all_codes[i] != NULL; i++) {
		// Check for a duplicate key. Shouldn't happen with the above hardcoded table, but
		// better be safe. Better put this in a unit test (TODO).
		HASH_FIND_STR(*prset, all_codes[i], s);
		if(s != NULL)
			continue;

		// Insert the (unique) code
		s = malloc(sizeof(struct Prset));
		strcpy(s->code, all_codes[i]);
		HASH_ADD_STR(*prset, code, s);

		// Insert the blank-by-default command
		strcpy(s->command, "");
	}

	// Debug format: TODO
	if(log_debug()) {
		log_date();
		printf("Dump of loaded printer set definitions:\n");
		for (s = *prset; s != NULL; s = s->hh.next) {
			log_date();
			printf("%20s = 0x", s->code);
			for (int i = 0; s->command[i] != '\0'; i++) {
				printf("%X", s->command[i]);
			}
			printf("\n");
		}
	}
}
