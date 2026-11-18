/*
 * Regression tests for parser/scanner bugs.
 * For license terms, see the file COPYING in this distribution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "libcue.h"
#include "minunit.h"

int tests_run;

#define MSF_TO_F(m,s,f) ((f) + ((m)*60 + (s))*CUE_FPS)
#define HDR "FILE \"a.wav\" WAVE\nTRACK 01 AUDIO\n"

static int streq(const char *a, const char *b)
{
	return a != NULL && b != NULL && strcmp(a, b) == 0;
}

/* <RPG> rule wrote one byte past the end of the string buffer */
static char *test_replaygain_overflow(void)
{
	size_t n = 3000;
	char *cue = malloc(n + 100);
	strcpy(cue, "REM REPLAYGAIN_ALBUM_GAIN ");
	memset(cue + strlen(cue), 'A', n);
	strcpy(cue + 26 + n, "\n" HDR "INDEX 01 00:00:00\n");

	Cd *cd = cue_parse_string(cue);
	free(cue);
	mu_assert("long replaygain: parse failed", cd != NULL);
	const char *v = rem_get(REM_REPLAYGAIN_ALBUM_GAIN, cd_get_rem(cd));
	mu_assert("long replaygain: value missing", v != NULL);
	mu_assert("long replaygain: not truncated", strlen(v) < n);
	cd_delete(cd);
	return NULL;
}

/* scanner start condition and EOF flag leaked from one parse into the next */
static char *test_no_state_between_parses(void)
{
	static const char *bad[] = {
		HDR "INDEX 01 00:00:00\nREM",
		HDR "INDEX 01 00:00:00\nTITLE",
		HDR "INDEX 01 00:00:00\nREM REPLAYGAIN_TRACK_GAIN",
		"TITLE \"x\n",
		"",
	};
	unsigned i;

	for (i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
		cd_delete(cue_parse_string(bad[i]));
		Cd *cd = cue_parse_string(HDR "INDEX 01 00:00:00");
		mu_assert("state leak: parse failed", cd != NULL);
		mu_assert("state leak: FILE lost",
			  streq(track_get_filename(cd_get_track(cd, 1)), "a.wav"));
		cd_delete(cd);
	}
	return NULL;
}

/* REM keywords were matched anywhere in a REM line */
static char *test_rem_keyword_must_be_first_word(void)
{
	Cd *cd = cue_parse_string(
		"REM DATE 1999\n"
		"REM ORIGINALDATE 1970\n"
		"REM DISCID 12345678\n"
		"REM MUSICBRAINZ_DISCID mbid\n"
		"REM COMMENTARY nope\n"
		"REM some text that mentions a genre here\n"
		"REM COMMENT \"real comment\"\n"
		HDR "INDEX 01 00:00:00\n");
	mu_assert("rem: parse failed", cd != NULL);
	mu_assert("rem: DATE clobbered", streq(rem_get(REM_DATE, cd_get_rem(cd)), "1999"));
	mu_assert("rem: DISCID clobbered", streq(cdtext_get(PTI_DISC_ID, cd_get_cdtext(cd)), "12345678"));
	mu_assert("rem: bogus GENRE", cdtext_get(PTI_GENRE, cd_get_cdtext(cd)) == NULL);
	mu_assert("rem: COMMENT", streq(rem_get(REM_COMMENT, cd_get_rem(cd)), "real comment"));
	cd_delete(cd);
	return NULL;
}

/* a keyword without a value swallowed the first word of the next line */
static char *test_keyword_without_value(void)
{
	Cd *cd = cue_parse_string("TITLE\n" HDR "INDEX 01 00:00:00\n");
	mu_assert("no value: parse failed", cd != NULL);
	mu_assert("no value: FILE lost",
		  streq(track_get_filename(cd_get_track(cd, 1)), "a.wav"));
	cd_delete(cd);

	cd = cue_parse_string("REM REPLAYGAIN_ALBUM_GAIN\nPERFORMER \"x\"\n" HDR "INDEX 01 00:00:00\n");
	mu_assert("rpg no value: parse failed", cd != NULL);
	mu_assert("rpg no value: next line eaten",
		  streq(cdtext_get(PTI_PERFORMER, cd_get_cdtext(cd)), "x"));
	mu_assert("rpg no value: bogus value",
		  rem_get(REM_REPLAYGAIN_ALBUM_GAIN, cd_get_rem(cd)) == NULL);
	cd_delete(cd);
	return NULL;
}

/* the scanner's default rule echoed unmatched input to stdout */
static char *test_nothing_written_to_stdout(void)
{
#ifndef _WIN32
	FILE *tmp = tmpfile();
	int saved;
	long size;

	fflush(stdout);
	saved = dup(fileno(stdout));
	dup2(fileno(tmp), fileno(stdout));

	cd_delete(cue_parse_string(HDR "INDEX 01 00:00:00\nREM REPLAYGAIN_TRACK_GAIN -1.00 dB"));
	cd_delete(cue_parse_string("REM REPLAYGAIN_ALBUM_GAIN\n" HDR "INDEX 01 00:00:00\n"));

	fflush(stdout);
	dup2(saved, fileno(stdout));
	close(saved);
	fseek(tmp, 0, SEEK_END);
	size = ftell(tmp);
	fclose(tmp);
	mu_assert("library wrote to stdout", size == 0);
#endif
	return NULL;
}

/* numbers overflowed / were truncated; invalid times were used as -1 */
static char *test_numbers(void)
{
	Cd *cd = cue_parse_string(HDR
		"INDEX 4294967297 00:00:33\n"	/* used to become INDEX 01 */
		"INDEX 01 4294967296:00:00\n"	/* used to become 00:00:00 */
		"INDEX 02 99999999999999999999\n"
		"INDEX 03 00:02:00\n");
	mu_assert("numbers: parse failed", cd != NULL);
	Track *t = cd_get_track(cd, 1);
	mu_assert("numbers: bogus INDEX 01", track_get_index(t, 1) == -1);
	mu_assert("numbers: bogus start", track_get_start(t) == -1);
	mu_assert("numbers: bogus INDEX 02", track_get_index(t, 2) == -1);
	mu_assert("numbers: INDEX 03", track_get_index(t, 3) == MSF_TO_F(0, 2, 0));
	cd_delete(cd);

	cd = cue_parse_string(
		"FILE \"a.wav\" WAVE\n"
		"TRACK 01 AUDIO\nINDEX 01 00:00:00\n"
		"TRACK 02 AUDIO\nINDEX 01 00:61:00\n"	/* invalid seconds */
		"TRACK 03 AUDIO\nINDEX 01 01:00:00\n");
	mu_assert("invalid time: parse failed", cd != NULL);
	mu_assert("invalid time: track 1 length", track_get_length(cd_get_track(cd, 1)) == -1);
	mu_assert("invalid time: track 2 start", track_get_start(cd_get_track(cd, 2)) == -1);
	mu_assert("invalid time: track 2 length", track_get_length(cd_get_track(cd, 2)) == -1);
	mu_assert("invalid time: track 3 start", track_get_start(cd_get_track(cd, 3)) == MSF_TO_F(1, 0, 0));
	cd_delete(cd);
	return NULL;
}

/* tracks past 99 overwrote track 99 */
static char *test_too_many_tracks(void)
{
	char *cue = malloc(101 * 64 + 64), *p = cue;
	int i;

	p += sprintf(p, "FILE \"a.wav\" WAVE\n");
	for (i = 1; i <= 101; i++)
		p += sprintf(p, "TRACK %02d AUDIO\nTITLE \"t%d\"\nINDEX 01 %02d:00:00\n", i, i, i - 1);

	Cd *cd = cue_parse_string(cue);
	free(cue);
	mu_assert("101 tracks: parse failed", cd != NULL);
	mu_assert("101 tracks: ntrack", cd_get_ntrack(cd) == 99);
	Track *t = cd_get_track(cd, 99);
	mu_assert("101 tracks: track 99 title",
		  streq(cdtext_get(PTI_TITLE, track_get_cdtext(t)), "t99"));
	mu_assert("101 tracks: track 99 start", track_get_start(t) == MSF_TO_F(98, 0, 0));
	cd_delete(cd);
	return NULL;
}

/* FILE inside a track: wrong file for the following tracks, bogus lengths */
static char *test_file_inside_track(void)
{
	Cd *cd = cue_parse_string(
		"FILE \"a.wav\" WAVE\n"
		"TRACK 01 AUDIO\nINDEX 01 00:00:00\n"
		"TRACK 02 AUDIO\nFILE \"b.wav\" WAVE\nINDEX 01 00:00:00\n"
		"TRACK 03 AUDIO\nINDEX 01 03:00:00\n");
	mu_assert("file in track: parse failed", cd != NULL);
	mu_assert("file in track: track 1 file", streq(track_get_filename(cd_get_track(cd, 1)), "a.wav"));
	mu_assert("file in track: track 1 length", track_get_length(cd_get_track(cd, 1)) == -1);
	mu_assert("file in track: track 2 file", streq(track_get_filename(cd_get_track(cd, 2)), "b.wav"));
	mu_assert("file in track: track 2 length", track_get_length(cd_get_track(cd, 2)) == MSF_TO_F(3, 0, 0));
	mu_assert("file in track: track 3 file", streq(track_get_filename(cd_get_track(cd, 3)), "b.wav"));
	cd_delete(cd);

	/* issue 10 layout: tracks with only INDEX 00, FILE before each TRACK */
	cd = cue_parse_string(
		"FILE \"1.wav\" WAVE\nTRACK 01 AUDIO\nINDEX 00 00:00:00\nTITLE \"one\"\n"
		"FILE \"2.wav\" WAVE\nTRACK 02 AUDIO\nINDEX 00 00:00:00\nTITLE \"two\"\n"
		"FILE \"3.wav\" WAVE\nTRACK 03 AUDIO\nINDEX 00 00:00:00\nTITLE \"three\"\n");
	mu_assert("issue10: parse failed", cd != NULL);
	mu_assert("issue10: track 1 file", streq(track_get_filename(cd_get_track(cd, 1)), "1.wav"));
	mu_assert("issue10: track 2 file", streq(track_get_filename(cd_get_track(cd, 2)), "2.wav"));
	mu_assert("issue10: track 3 file", streq(track_get_filename(cd_get_track(cd, 3)), "3.wav"));
	cd_delete(cd);
	return NULL;
}

/* a line without any token made the whole parse fail */
static char *test_garbage_line(void)
{
	Cd *cd = cue_parse_string(HDR "INDEX 01 00:00:00\n???\nTRACK 02 AUDIO\nINDEX 01 01:00:00\n");
	mu_assert("garbage line: parse failed", cd != NULL);
	mu_assert("garbage line: ntrack", cd_get_ntrack(cd) == 2);
	cd_delete(cd);
	return NULL;
}

/* an unbalanced quote swallowed the following lines */
static char *test_quote_does_not_span_lines(void)
{
	Cd *cd = cue_parse_string(
		"FILE \"a.wav\" WAVE\n"
		"TRACK 01 AUDIO\nTITLE \"12\" Single\"\nINDEX 01 00:00:00\n"
		"TRACK 02 AUDIO\nTITLE \"Two\"\nINDEX 01 01:00:00\n");
	mu_assert("quote: parse failed", cd != NULL);
	mu_assert("quote: ntrack", cd_get_ntrack(cd) == 2);
	mu_assert("quote: track 1 start", track_get_start(cd_get_track(cd, 1)) == 0);
	mu_assert("quote: track 2 title",
		  streq(cdtext_get(PTI_TITLE, track_get_cdtext(cd_get_track(cd, 2))), "Two"));
	cd_delete(cd);
	return NULL;
}

/* NULL arguments crashed */
static char *test_null_arguments(void)
{
	mu_assert("cue_parse_string(NULL)", cue_parse_string(NULL) == NULL);
	mu_assert("cue_parse_file(NULL)", cue_parse_file(NULL) == NULL);
	cd_delete(NULL);
	mu_assert("cd_get_ntrack(NULL)", cd_get_ntrack(NULL) == 0);
	mu_assert("cd_get_catalog(NULL)", cd_get_catalog(NULL) == NULL);
	mu_assert("cdtext_get(NULL)", cdtext_get(PTI_TITLE, cd_get_cdtext(NULL)) == NULL);
	mu_assert("rem_get(NULL)", rem_get(REM_DATE, cd_get_rem(NULL)) == NULL);
	mu_assert("cd_get_track(NULL)", cd_get_track(NULL, 1) == NULL);
	mu_assert("track_get_filename(NULL)", track_get_filename(NULL) == NULL);
	mu_assert("track_get_start(NULL)", track_get_start(NULL) == -1);
	mu_assert("track_get_index(NULL)", track_get_index(NULL, 1) == -1);
	mu_assert("track_is_set_flag(NULL)", track_is_set_flag(NULL, FLAG_ANY) == 0);
	return NULL;
}

static char *run_tests(void)
{
	mu_run_test(test_replaygain_overflow);
	mu_run_test(test_no_state_between_parses);
	mu_run_test(test_rem_keyword_must_be_first_word);
	mu_run_test(test_keyword_without_value);
	mu_run_test(test_nothing_written_to_stdout);
	mu_run_test(test_numbers);
	mu_run_test(test_too_many_tracks);
	mu_run_test(test_file_inside_track);
	mu_run_test(test_garbage_line);
	mu_run_test(test_quote_does_not_span_lines);
	mu_run_test(test_null_arguments);
	return NULL;
}

int main(int argc, char **argv)
{
	char *result;

	(void)argc;
	(void)argv;
	result = run_tests();
	if (result != NULL)
		printf("%s\n", result);
	else
		printf("All tests passed!\n");

	printf("Tests run: %d\n", tests_run);

	return result != NULL;
}
