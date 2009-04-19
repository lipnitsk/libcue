/*
 * Copyright (c) 2009, Jochen Keil
 * For license terms, see the file COPYING in this distribution.
 */

#ifndef REM_H
#define REM_H

#include <stdio.h>

enum Cmt {
	REM_DATE,	/* date of cd/track */
	REM_REPLAYGAIN_ALBUM_GAIN,
	REM_REPLAYGAIN_ALBUM_PEAK,
	REM_REPLAYGAIN_TRACK_GAIN,
	REM_REPLAYGAIN_TRACK_PEAK,
	REM_END		/* terminating REM (for stepping through REMs) */
};

typedef struct Rem Rem;

/**
 * return new allocated rem struct
 */
Rem*
rem_new(	void);

/**
 * free rem struct
 */
void
rem_free(	Rem*);

/**
 * return true if allocated rem struct is empty
 */
int
rem_is_emtpy(	Rem*);

/**
 * set value of rem comment
 * @param unsigned int: enum of rem comment
 */
void
rem_set(	unsigned int,
		char*,
		Rem*);

/**
 * return pointer to value for rem comment
 * @param unsigned int: enum of rem comment
 */
char*
rem_get(	unsigned int,
		Rem*);

/**
 * dump all rem comments in human readable form
 * @param bool: if track: true; else false
 */
void
rem_dump(	Rem*);

#endif
