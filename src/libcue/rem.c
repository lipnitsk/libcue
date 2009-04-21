/*
 * Copyright (c) 2009, Jochen Keil
 * For license terms, see the file COPYING in this distribution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rem.h"

struct Rem {
	unsigned int cmt;
	char *value;
};

Rem*
rem_new(	void)
{
	Rem* new_rem = NULL;

	Rem rem[] = {
		{REM_DATE,			NULL},
		{REM_REPLAYGAIN_ALBUM_GAIN,	NULL},
		{REM_REPLAYGAIN_ALBUM_PEAK,	NULL},
		{REM_REPLAYGAIN_TRACK_GAIN,	NULL},
		{REM_REPLAYGAIN_TRACK_PEAK,	NULL},
		{REM_END,			NULL}
	};

	/* sizeof(rem) = number of elements in rem[] * sizeof(Rem) */
	new_rem = (Rem*)calloc(sizeof(rem) / sizeof(Rem), sizeof(Rem));
	if (new_rem == NULL)
		fprintf(stderr, "rem_new(): problem allocating memory\n");
	else
		memcpy(new_rem, rem, sizeof(rem));

	return new_rem;
}

void
rem_free(	Rem* rem)
{
	if (rem == NULL)
		return;

	Rem* ptr = rem;

	do
	{
		free(ptr->value);
	}
	while ((++ptr)->cmt != REM_END);

	free(rem);
}

int
rem_is_emtpy(	Rem* rem)
{
	if (rem == NULL)
		return 1;

	do
	{
		if (rem->value != NULL)
			return 0;
	} while ((++rem)->cmt != REM_END);

	return 1;
}

void
rem_set(	unsigned int cmt,
		char* value,
		Rem* rem)
{
	if (rem == NULL || value == NULL)
		return;

	do
	{
		if (rem->cmt == cmt)
		{
			free(rem->value);
			rem->value = strdup(value);
			return;
		}
	} while ((++rem)->cmt != REM_END);
}

char*
rem_get(	unsigned int cmt,
		Rem* rem)
{
	if (rem == NULL)
		return NULL;

	do
	{
		if (rem->cmt == cmt)
		{
			if (rem->value != NULL)
				return rem->value;
			else
				return NULL;
		}
	} while ((++rem)->cmt != REM_END);

	return NULL;
}

void
rem_dump(	Rem* rem)
{
	if (rem == NULL)
		return;

	do
	{
		fprintf(stdout, "REM %u: %s\n", rem->cmt, rem->value);
	} while ((++rem)->cmt != REM_END);
}
