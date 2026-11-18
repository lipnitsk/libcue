%{
/*
 * Copyright (c) 2004, 2005, 2006, 2007, Svend Sorensen
 * Copyright (c) 2009, 2010 Jochen Keil
 * For license terms, see the file COPYING in this distribution.
 */

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cd.h"
#include "cue_time.h"
#include "cue_parser_state.h"

/* keep the generated symbols out of the global yy* namespace */
#define yyparse cue_yyparse
#define yylex cue_yylex

static void yyerror(void *scanner, struct cue_parser_state *st, const char *s);
static long msf_to_frame(long m, long s, long f);
static void copy_filename(char *dst, const char *src);
static void use_new_file_for_track(struct cue_parser_state *st);
%}

%define api.pure
%parse-param {void *scanner}
%parse-param {struct cue_parser_state *st}
%lex-param {void *scanner}

%start cuefile

%union {
	long ival;
	char *sval;
}

%{
int cue_yylex(YYSTYPE *yylval_param, void *yyscanner);
%}

%token <ival> NUMBER
%token <sval> STRING

/* global (header) */
%token CATALOG
%token CDTEXTFILE

%token FFILE
%token BINARY
%token MOTOROLA
%token AIFF
%token WAVE
%token MP3
%token FLAC

/* track */
%token TRACK

%token <ival> AUDIO
%token <ival> MODE1_2048
%token <ival> MODE1_2352
%token <ival> MODE2_2336
%token <ival> MODE2_2048
%token <ival> MODE2_2342
%token <ival> MODE2_2332
%token <ival> MODE2_2352

/* ISRC is with CD_TEXT */
%token TRACK_ISRC

%token FLAGS
%token <ival> PRE
%token <ival> DCP
%token <ival> FOUR_CH
%token <ival> SCMS

%token PREGAP
%token INDEX
%token POSTGAP

/* CD-TEXT */
%token <ival> TITLE
%token <ival> PERFORMER
%token <ival> SONGWRITER
%token <ival> COMPOSER
%token <ival> ARRANGER
%token <ival> MESSAGE
%token <ival> DISC_ID
%token <ival> GENRE
%token <ival> TOC_INFO1
%token <ival> TOC_INFO2
%token <ival> UPC_EAN
%token <ival> ISRC
%token <ival> SIZE_INFO

%type <ival> track_mode
%type <ival> track_flag
%type <ival> time
%type <ival> cdtext_item

/* REM */
%type <ival> rem_item
%token <ival> DATE
%token <ival> XXX_GENRE /* parsed in REM but stored in CD-TEXT */
%token <ival> XXX_DISCID
%token <ival> XXX_COMPOSER
%token <ival> REPLAYGAIN_ALBUM_GAIN
%token <ival> REPLAYGAIN_ALBUM_PEAK
%token <ival> REPLAYGAIN_TRACK_GAIN
%token <ival> REPLAYGAIN_TRACK_PEAK
%token <ival> COMMENT
%token <ival> DISCNUMBER
%token <ival> TOTALDISCS
%%

cuefile
	: new_cd global_statements track_list {
		/* FILE inside the last track, never followed by an INDEX */
		if (st->file_in_track && NULL != st->new_filename
		    && st->track != st->overflow)
			use_new_file_for_track(st);
	}
	;

new_cd
	: /* empty */ {
		st->cd = cd_init();
		if (NULL == st->cd)
			YYABORT;
		st->cdtext = cd_get_cdtext(st->cd);
		st->rem = cd_get_rem(st->cd);
	}
	;

global_statements
	: /* empty */
	| global_statements global_statement
	;

global_statement
	: CATALOG STRING '\n' { cd_set_catalog(st->cd, $2); }
	| CDTEXTFILE STRING '\n' { cd_set_cdtextfile(st->cd, $2); }
	| cdtext
	| rem
	| track_data
	| '\n'		/* empty statement, e.g. a line of unknown characters */
	| error '\n'
	;

track_data
	: FFILE STRING file_format '\n' {
		if (NULL != st->new_filename) {
			yyerror(scanner, st, "too many files specified");
		}
		copy_filename(st->new_fname, $2);
		st->new_filename = st->new_fname;
		/*
		 * A FILE inside a track that has no INDEX 01 yet is ambiguous:
		 * if an INDEX follows, the track continues in the new file
		 * (e.g. INDEX 00 in the old file, INDEX 01 in the new one);
		 * if TRACK follows, the file is for the next track.
		 */
		st->file_in_track = (NULL != st->track
				     && track_get_index(st->track, 1) == -1);
	}
	;

track_list
	: track
	| track_list track
	;

track
	: new_track track_def track_statements
	;

file_format
	: BINARY
	| MOTOROLA
	| AIFF
	| WAVE
	| MP3
	| FLAC
	;

new_track
	: /*empty */ {
		/* save previous track, to later set length */
		st->prev_track = st->track;

		if (cd_get_ntrack(st->cd) >= MAXTRACK) {
			/*
			 * Parse surplus tracks into a scratch track that is
			 * thrown away, so they cannot clobber the last real one.
			 */
			yyerror(scanner, st, "too many tracks");
			if (NULL != st->overflow) {
				st->prev_track = NULL;
				track_delete(st->overflow);
			}
			st->track = st->overflow = track_init();
		} else {
			st->track = cd_add_track(st->cd);
		}
		if (NULL == st->track)
			YYABORT;

		st->cdtext = track_get_cdtext(st->track);
		st->rem = track_get_rem(st->track);

		st->cur_filename = NULL;
		st->file_in_track = 0;
		if (NULL != st->new_filename) {
			copy_filename(st->cur_fname, st->new_filename);
			st->cur_filename = st->prev_filename = st->cur_fname;
		}

		if (NULL == st->prev_filename)
			yyerror(scanner, st, "no file specified for track");
		else
			track_set_filename(st->track, st->prev_filename);

		st->new_filename = NULL;
	}
	;

track_def
	: TRACK NUMBER track_mode '\n' {
		track_set_mode(st->track, $3);
	}
	;

track_mode
	: AUDIO
	| MODE1_2048
	| MODE1_2352
	| MODE2_2336
	| MODE2_2048
	| MODE2_2342
	| MODE2_2332
	| MODE2_2352
	;

track_statements
	: track_statement
	| track_statements track_statement
	;

track_statement
	: cdtext
	| rem
	| FLAGS track_flags '\n'
	| TRACK_ISRC STRING '\n' { track_set_isrc(st->track, $2); }
	| PREGAP time '\n' { track_set_zero_pre(st->track, $2); }
	| INDEX NUMBER time '\n' {
		if ($2 < 0 || $2 > MAXINDEX) {
			yyerror(scanner, st, "invalid index number");
		} else if ($3 < 0) {
			yyerror(scanner, st, "invalid index time");
		} else {
			if (st->file_in_track && NULL != st->new_filename)
				use_new_file_for_track(st);

			/* Set previous track length if it has not been set */
			if (NULL != st->prev_track && NULL == st->cur_filename
			    && track_get_length(st->prev_track) == -1
			    && track_get_start(st->prev_track) != -1) {
				/* track shares file with previous track */
				track_set_length(st->prev_track,
					$3 - track_get_start(st->prev_track));
			}

			if (1 == $2) {
				/* INDEX 01 */
				track_set_start(st->track, $3);

				long idx00 = track_get_index(st->track, 0);

				if (idx00 != -1 && $3 != 0)
					track_set_zero_pre(st->track, $3 - idx00);
			}

			track_set_index(st->track, (int)$2, $3);
		}
	}
	| POSTGAP time '\n' { track_set_zero_post(st->track, $2); }
	| track_data
	| '\n'		/* empty statement, also absorbs the newline added at EOF */
	| error '\n'
	;

track_flags
	: /* empty */
	| track_flags track_flag { track_set_flag(st->track, $2); }
	;

track_flag
	: PRE
	| DCP
	| FOUR_CH
	| SCMS
	;

cdtext
	: cdtext_item STRING '\n' { cdtext_set($1, $2, st->cdtext); }
	;

cdtext_item
	: TITLE
	| PERFORMER
	| SONGWRITER
	| COMPOSER
	| ARRANGER
	| MESSAGE
	| DISC_ID
	| GENRE
	| TOC_INFO1
	| TOC_INFO2
	| UPC_EAN
	| ISRC
	| SIZE_INFO
	;

time
	: NUMBER { $$ = ($1 > INT_MAX) ? -1 : $1; }
	| NUMBER ':' NUMBER ':' NUMBER { $$ = msf_to_frame($1, $3, $5); }
	;

rem
	: rem_item STRING '\n' { rem_set($1, $2, st->rem); }
	| XXX_GENRE STRING '\n' { cdtext_set($1, $2, st->cdtext); }
	| XXX_DISCID STRING '\n' { cdtext_set($1, $2, st->cdtext); }
	| XXX_COMPOSER STRING '\n' { cdtext_set($1, $2, st->cdtext); }
	;

rem_item
	: DATE
	| REPLAYGAIN_ALBUM_GAIN
	| REPLAYGAIN_ALBUM_PEAK
	| REPLAYGAIN_TRACK_GAIN
	| REPLAYGAIN_TRACK_PEAK
	| COMMENT
	| DISCNUMBER
	| TOTALDISCS
	;
%%

/* lexer interface */
int cue_yylex_init_extra(struct cue_parser_state *extra, void **scanner);
int cue_yylex_destroy(void *scanner);
struct yy_buffer_state *cue_yy_create_buffer(FILE *file, int size, void *scanner);
void cue_yy_switch_to_buffer(struct yy_buffer_state *buffer, void *scanner);
void cue_yyset_lineno(int line, void *scanner);
int cue_yyget_lineno(void *scanner);
struct yy_buffer_state *cue_yy_scan_string(const char *str, void *scanner);

static void yyerror(void *scanner, struct cue_parser_state *st, const char *s)
{
	(void)st;
	fprintf(stderr, "%d: %s\n", cue_yyget_lineno(scanner), s);
}

/* MSF to frames; -1 for anything that is not a valid MSF time */
static long msf_to_frame(long m, long s, long f)
{
	if (m < 0 || m > INT_MAX || s < 0 || s > INT_MAX || f < 0 || f > INT_MAX)
		return -1;
	return time_msf_to_frame((int)m, (int)s, (int)f);
}

static void copy_filename(char *dst, const char *src)
{
	strncpy(dst, src, PARSER_BUFFER - 1);
	dst[PARSER_BUFFER - 1] = '\0';
}

/* the pending FILE belongs to the current track, and the tracks after it */
static void use_new_file_for_track(struct cue_parser_state *st)
{
	track_set_filename(st->track, st->new_filename);
	copy_filename(st->cur_fname, st->new_filename);
	st->cur_filename = st->prev_filename = st->cur_fname;
	st->new_filename = NULL;
	st->file_in_track = 0;
}

/* parse either fp or string, using fresh state for every call */
static Cd *cue_parse(FILE *fp, const char *string)
{
	struct cue_parser_state *st;
	void *scanner = NULL;
	Cd *ret_cd = NULL;

	st = calloc(1, sizeof(*st));
	if (NULL == st)
		return NULL;

	if (0 != cue_yylex_init_extra(st, &scanner)) {
		free(st);
		return NULL;
	}

	/* the buffer is owned and freed by the scanner (cue_yylex_destroy) */
	if (NULL != fp)
		cue_yy_switch_to_buffer(cue_yy_create_buffer(fp, 16384, scanner), scanner);
	else
		cue_yy_scan_string(string, scanner);

	/* not initialized by flex for string buffers */
	cue_yyset_lineno(1, scanner);

	if (0 == cue_yyparse(scanner, st))
		ret_cd = st->cd;
	else
		cd_delete(st->cd);

	cue_yylex_destroy(scanner);
	track_delete(st->overflow);
	free(st);

	return ret_cd;
}

Cd *cue_parse_file(FILE *fp)
{
	if (NULL == fp)
		return NULL;

	return cue_parse(fp, NULL);
}

Cd *cue_parse_string(const char* string)
{
	if (NULL == string)
		return NULL;

	return cue_parse(NULL, string);
}
