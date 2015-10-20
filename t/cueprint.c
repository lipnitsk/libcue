/*
 * Copyright (c) 2009, Jochen Keil
 * For license terms, see the file COPYING in this distribution.
 */

#include <stdio.h>
#include <stdlib.h>		/* exit() */
#include <string.h>		/* strcmp() */
#include "cue.h"
#include "cd.h"

#include <metadata.h>

int main (int argc, char **argv)
{
	struct Cd* cd = NULL;
	const char* ptr = NULL;
	const char* fname = NULL;
	char* string = NULL;
	FILE* fp = NULL;
	int i = 0;

	getchar();
	while (i++ < 100000)
	{
		cd = cd_init();
		cd_delete(cd);
		cd = NULL;
	}
	i = 0;
	getchar();

	FLAC__StreamMetadata* vc = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
	
	if (argc == 2 && vc != 0)
	{
		fname = strrchr(argv[1], '.');

		if (strcmp(++fname, "flac") == 0)
		{
			FLAC__metadata_get_tags(argv[1], &vc);

			ptr = (const char*)vc->data.vorbis_comment.comments[0].entry;

			while (*(++ptr) != '='); ptr++;

			string = strdup(ptr);

			FLAC__metadata_object_delete(vc);

			getchar();

			while (true)
			{
				i++;

				cd = cue_parse_string(string);

				if (i == 1)
				{
					fprintf(stderr, "%i\n", i);
					if (cd != NULL)
						cd_dump(cd);
					getchar();
				}

				if (i%1000 == 0)
					fprintf(stderr, "%i\t", i);

				if (i%10000 == 0)
				{
					if (cd != NULL)
						cd_dump(cd);
					getchar();
				}

				if (cd != NULL)
				{
					cd_delete(cd);
					cd = NULL;
				}
			}

			free(string);
		}
		else
		{
			getchar();

			if ((fp = fopen(argv[1], "r")) == NULL)
				return 0;

			while (true)
			{
				i++;

				cd = cue_parse_file(fp);

				rewind(fp);

				if (i == 1)
				{
					fprintf(stderr, "%i\t", i);
					if (cd != NULL)
						cd_dump(cd);
					//fprintf(stderr, "%s", cdtext_get(PTI_GENRE, (cd_get_cdtext(cd))));
					//fprintf(stderr, "%s", cdtext_get(PTI_TITLE, track_get_cdtext(cd_get_track(cd, 2))));
					getchar();
				}

				if (i%1000 == 0)
					fprintf(stderr, "%i\t", i);

				if (i%10000 == 0)
				{
					if (cd != NULL)
						cd_dump(cd);
					getchar();
				}

				if (cd != NULL)
				{
					cd_delete(cd);
					cd = NULL;
				}
			}

			fclose(fp);
		}
	}

	getchar();

	return 0;
}
