/*
 * time.c -- time functions
 *
 * Copyright (C) 2004, 2005, 2006, 2007 Svend Sorensen
 * For license terms, see the file COPYING in this distribution.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cue_time.h"
#include "libcue.h"

/* largest minute value whose frame count still fits in an int */
#define MAX_MINUTES	((INT_MAX / CUE_FPS - 59) / 60)

long time_msf_to_frame(int m, int s, int f)
{
	/*
	 * Minutes are not limited to 99: single-file images of audiobooks,
	 * DJ mixes and the like routinely use INDEX times past 99:59:74.
	 */
	if (m < 0 || m > MAX_MINUTES || s < 0 || s >= 60 || f < 0 || f >= CUE_FPS) {
		return -1;
	}
	return ((long)m * 60 + s) * CUE_FPS + f;
}

void time_frame_to_msf(long frame, int *m, int *s, int *f)
{
	*f = frame % CUE_FPS;           /* 0 <= frames <= 74 */
	frame /= CUE_FPS;
	*s = frame % 60;          /* 0 <= seconds <= 59 */
	frame /= 60;
	*m = frame;               /* 0 <= minutes */
}

/* print frame in mm:ss:ff format */
char *time_frame_to_mmssff(long f)
{
	static char msf[9];
	int minutes, seconds, frames;

	if (f < 0 || f >= CUE_FPS * 60 * 100) {
		strcpy(msf, "00:00:00");
		return msf;
	}

	time_frame_to_msf(f, &minutes, &seconds, &frames);
	snprintf(msf, sizeof(msf), "%02d:%02d:%02d", minutes, seconds, frames);

	return msf;
}
