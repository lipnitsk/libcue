%{
/*
 * Copyright (c) 2004, 2005, 2006, 2007, Svend Sorensen
 * Copyright (c) 2009, 2010 Jochen Keil
 * For license terms, see the file COPYING in this distribution.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cd.h"
#include "cue_parser_state.h"
#include "cue_parser.h"

/* Store a token value in the per-parse string buffer, truncating it if needed. */
static char *set_string(struct cue_parser_state *st, const char *s, size_t len)
{
	if (len > sizeof(st->string) - 1)
		len = sizeof(st->string) - 1;
	memcpy(st->string, s, len);
	st->string[len] = '\0';
	return st->string;
}

/*
 * Convert a run of decimal digits.  Every number in a cue sheet must fit
 * an int, so anything larger is returned as -1, which the parser rejects.
 * (Saturating at LONG_MAX does not work where long is as wide as int:
 * LONG_MAX == INT_MAX there, and the value would be taken as valid.)
 */
static long parse_number(const char *s)
{
	long v = 0;

	for (; *s >= '0' && *s <= '9'; s++) {
		int d = *s - '0';
		if (v > (INT_MAX - d) / 10)
			return -1;
		v = v * 10 + d;
	}
	return v;
}
%}

ws		[ \t\r]
nonws		[^ \t\r\n]
/* what may follow a known REM keyword */
remsep		[ \t\r\"\']

%option reentrant bison-bridge
%option prefix="cue_yy"
%option extra-type="struct cue_parser_state *"
%option yylineno
%option noyywrap
%option noinput
%option nounput
%option caseless
%option never-interactive
/* every state must handle every input; flex refuses to build otherwise */
%option nodefault

%s NAME
%s TEXT
%x REM
%x REMSKIP
%x RPG
%x SKIP

%%


 /*
  * A quoted value ends at the last matching quote on its line, so quotes
  * inside it are kept (TITLE "Op. 66 "Fantaisie-Impromptu"").  It never
  * extends past the end of the line.
  */
\'[^\n]*\'	|
\"[^\n]*\"	{
		yylval->sval = set_string(yyextra, yytext + 1, yyleng - 2);
		BEGIN(INITIAL);
		return STRING;
		}

<NAME>{nonws}+	{
		yylval->sval = set_string(yyextra, yytext, yyleng);
		BEGIN(INITIAL);
		return STRING;
		}

 /*
  * Unquoted text value: the rest of the line, without trailing blanks.
  * An opening quote whose closing quote is missing is dropped.
  */
<TEXT>{nonws}([^\n]*{nonws})?	{
		if ((yytext[0] == '"' || yytext[0] == '\'')
		    && NULL == memchr(yytext + 1, yytext[0], yyleng - 1))
			yylval->sval = set_string(yyextra, yytext + 1, yyleng - 1);
		else
			yylval->sval = set_string(yyextra, yytext, yyleng);
		BEGIN(INITIAL);
		return STRING;
		}

CATALOG		{ BEGIN(NAME); return CATALOG; }
CDTEXTFILE	{ BEGIN(NAME); return CDTEXTFILE; }

FILE		{ BEGIN(NAME); return FFILE; }
BINARY		{ return BINARY; }
MOTOROLA	{ return MOTOROLA; }
AIFF		{ return AIFF; }
WAVE		{ return WAVE; }
MP3		{ return MP3; }
FLAC		{ return FLAC; }

TRACK		{ return TRACK; }
AUDIO		{ yylval->ival = MODE_AUDIO; return AUDIO; }
MODE1\/2048	{ yylval->ival = MODE_MODE1; return MODE1_2048; }
MODE1\/2352	{ yylval->ival = MODE_MODE1_RAW; return MODE1_2352; }
MODE2\/2336	{ yylval->ival = MODE_MODE2; return MODE2_2336; }
MODE2\/2048	{ yylval->ival = MODE_MODE2_FORM1; return MODE2_2048; }
MODE2\/2342	{ yylval->ival = MODE_MODE2_FORM2; return MODE2_2342; }
MODE2\/2332	{ yylval->ival = MODE_MODE2_FORM_MIX; return MODE2_2332; }
MODE2\/2352	{ yylval->ival = MODE_MODE2_RAW; return MODE2_2352; }

FLAGS		{ return FLAGS; }
PRE		{ yylval->ival = FLAG_PRE_EMPHASIS; return PRE; }
DCP		{ yylval->ival = FLAG_COPY_PERMITTED; return DCP; }
4CH		{ yylval->ival = FLAG_FOUR_CHANNEL; return FOUR_CH; }
SCMS		{ yylval->ival = FLAG_SCMS; return SCMS; }

PREGAP		{ return PREGAP; }
INDEX		{ return INDEX; }
POSTGAP		{ return POSTGAP; }

TITLE		{ BEGIN(TEXT); yylval->ival = PTI_TITLE;  return TITLE; }
PERFORMER	{ BEGIN(TEXT); yylval->ival = PTI_PERFORMER;  return PERFORMER; }
SONGWRITER	{ BEGIN(TEXT); yylval->ival = PTI_SONGWRITER;  return SONGWRITER; }
COMPOSER	{ BEGIN(TEXT); yylval->ival = PTI_COMPOSER;  return COMPOSER; }
ARRANGER	{ BEGIN(TEXT); yylval->ival = PTI_ARRANGER;  return ARRANGER; }
MESSAGE		{ BEGIN(TEXT); yylval->ival = PTI_MESSAGE;  return MESSAGE; }
DISC_ID		{ BEGIN(TEXT); yylval->ival = PTI_DISC_ID;  return DISC_ID; }
GENRE		{ BEGIN(TEXT); yylval->ival = PTI_GENRE;  return GENRE; }
TOC_INFO1	{ BEGIN(TEXT); yylval->ival = PTI_TOC_INFO1;  return TOC_INFO1; }
TOC_INFO2	{ BEGIN(TEXT); yylval->ival = PTI_TOC_INFO2;  return TOC_INFO2; }
UPC_EAN		{ BEGIN(TEXT); yylval->ival = PTI_UPC_ISRC;  return UPC_EAN; }
ISRC/{ws}+\"	{ BEGIN(TEXT); yylval->ival = PTI_UPC_ISRC;  return ISRC; }
SIZE_INFO	{ BEGIN(TEXT); yylval->ival = PTI_SIZE_INFO;  return SIZE_INFO; }

ISRC		{ BEGIN(NAME); return TRACK_ISRC; }

REM		{ BEGIN(REM); /* exclusive rules for special exceptions */ }

 /*
  * A REM keyword is only recognized as the first word after REM.  Anything
  * else is an unknown (or free-form) comment and the whole line is skipped,
  * so that e.g. "REM ORIGINALDATE", "REM MUSICBRAINZ_DISCID" or a comment
  * that merely mentions a genre do not clobber the real fields.
  */
<REM>DATE/{remsep}			{ BEGIN(TEXT); yylval->ival = REM_DATE; return DATE; }
<REM>GENRE/{remsep}			{ BEGIN(TEXT); yylval->ival = PTI_GENRE; return XXX_GENRE; }
<REM>DISCID/{remsep}			{ BEGIN(TEXT); yylval->ival = PTI_DISC_ID; return XXX_DISCID; }
<REM>COMPOSER/{remsep}			{ BEGIN(TEXT); yylval->ival = PTI_COMPOSER; return XXX_COMPOSER; }
<REM>REPLAYGAIN_ALBUM_GAIN/{remsep} 	{ BEGIN(RPG); yylval->ival = REM_REPLAYGAIN_ALBUM_GAIN;
							return REPLAYGAIN_ALBUM_GAIN; }
<REM>REPLAYGAIN_ALBUM_PEAK/{remsep}	{ BEGIN(RPG); yylval->ival = REM_REPLAYGAIN_ALBUM_PEAK;
							return REPLAYGAIN_ALBUM_PEAK; }
<REM>REPLAYGAIN_TRACK_GAIN/{remsep}	{ BEGIN(RPG); yylval->ival = REM_REPLAYGAIN_TRACK_GAIN;
							return REPLAYGAIN_TRACK_GAIN; }
<REM>REPLAYGAIN_TRACK_PEAK/{remsep}	{ BEGIN(RPG); yylval->ival = REM_REPLAYGAIN_TRACK_PEAK;
							return REPLAYGAIN_TRACK_PEAK; }
<REM>COMMENT/{remsep}			{ BEGIN(TEXT); yylval->ival = REM_COMMENT; return COMMENT; }
<REM>DISCNUMBER/{remsep}		{ BEGIN(TEXT); yylval->ival = REM_DISCNUMBER; return DISCNUMBER; }
<REM>TOTALDISCS/{remsep}		{ BEGIN(TEXT); yylval->ival = REM_TOTALDISCS; return TOTALDISCS; }

<REM>{ws}+		{ /* skip whitespace before the keyword */ }
<REM>[^ \t\r\n\"\']+	|
<REM>.			{ BEGIN(REMSKIP); /* unknown REM keyword */ }
<REM>\n			{ BEGIN(INITIAL); }

<REMSKIP>[^\n]+		{ /* ignore the rest of an unknown REM line */ }
<REMSKIP>\n		{ BEGIN(INITIAL); }

<RPG>{nonws}+	{
		yylval->sval = set_string(yyextra, yytext, yyleng);
		BEGIN(SKIP);
		return STRING;
		}
<RPG>{ws}+	{ }
<RPG>\n		{ BEGIN(INITIAL); return '\n'; /* REPLAYGAIN_* without a value */ }

<SKIP>[^\n]*\n	{ BEGIN(INITIAL); return '\n'; }
<SKIP>[^\n]+	{ /* trailing text on the last line, no newline before EOF */ }

{ws}+		{ /* ignore whitespace */ }

[[:digit:]]+	{ yylval->ival = parse_number(yytext); return NUMBER; }
:		{ return yytext[0]; }

^{ws}*\n	{ BEGIN(INITIAL); /* blank line */ }
\n		{ BEGIN(INITIAL); return '\n'; }
.		{ fprintf(stderr, "bad character '%c'\n", yytext[0]); }

 /* make sure the last statement is terminated even without a final newline */
<<EOF>>		{
		if (!yyextra->eof_newline_sent) {
			yyextra->eof_newline_sent = 1;
			return '\n';
		}
		yyterminate();
		}

%%
