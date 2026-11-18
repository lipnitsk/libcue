/*
 * Private state shared by the scanner and the parser.
 *
 * Everything that used to live in file-scope (static or global) variables
 * of cue_scanner.l and cue_parser.y is kept here instead, one instance per
 * call to cue_parse_file() / cue_parse_string().  This makes the parser
 * reentrant and thread-safe, and guarantees that nothing left over from a
 * previous (possibly malformed) input can affect the next parse.
 *
 * For license terms, see the file COPYING in this distribution.
 */

#ifndef CUE_PARSER_STATE_H
#define CUE_PARSER_STATE_H

#include "cd.h"

struct cue_parser_state {
	/* scanner */
	char string[PARSER_BUFFER];	/* value of the last STRING token */
	int eof_newline_sent;		/* synthetic '\n' already returned at EOF */

	/* parser */
	Cd *cd;
	Track *track;			/* track being parsed */
	Track *prev_track;		/* previous track, to set its length */
	Track *overflow;		/* scratch track for tracks past MAXTRACK */
	Cdtext *cdtext;			/* CD-TEXT block statements apply to */
	Rem *rem;			/* REM block statements apply to */

	char new_fname[PARSER_BUFFER];	/* storage for new_filename */
	char cur_fname[PARSER_BUFFER];	/* storage for cur/prev_filename */
	char *prev_filename;		/* last file in or before last track */
	char *cur_filename;		/* file switched to in this track, if any */
	char *new_filename;		/* FILE seen for the upcoming track */
	int file_in_track;		/* new_filename appeared inside a track
					 * before its INDEX 01 */
};

#endif
