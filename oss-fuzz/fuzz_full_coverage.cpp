#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "libcue.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size);

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION

/*
** Read files named on the command-line and invoke the fuzzer harness for
** each one.
*/
int main (int argc, char **argv)
{
  FILE *in;
  int i;
  int nErr = 0;
  uint8_t *zBuf = 0;
  size_t sz;

  for (i = 1; i < argc; i++)
  {
    const char *zFilename = argv[i];
    in = fopen (zFilename, "rb");
    if (in == 0)
    {
      fprintf (stderr, "cannot open \"%s\"\n", zFilename);
      nErr++;
      continue;
    }
    fseek (in, 0, SEEK_END);
    sz = ftell (in);
    rewind (in);
    zBuf = (uint8_t*)realloc (zBuf, sz);
    if (zBuf == 0)
    {
      fprintf(stderr, "cannot malloc() for %d bytes\n", (int)sz);
      exit(1);
    }
    if (fread (zBuf, sz, 1, in) != 1)
    {
      fprintf (stderr, "cannot read %d bytes from \"%s\"\n",
                       (int)sz, zFilename);
      nErr++;
    }
    else
    {
      printf ("%s... ", zFilename);
      fflush (stdout);
      (void)LLVMFuzzerTestOneInput (zBuf, sz);
      printf ("ok\n");
    }
    fclose (in);
  }
  free (zBuf);
  return nErr;
}

#endif

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
    char* str = (char*)malloc (size + 1);

    if (!str)
        return -1;

    memcpy (str, data, size);
    str[size] = '\0';

    Cd *cd = cue_parse_string (str);
    if (cd == NULL) {
        free (str);
        return -1;
    }

    /* Cover CD functions */
    cd_get_mode (cd);
    cd_get_cdtextfile (cd);
    const int ntracks = cd_get_ntrack (cd);

    /* Cover CDTEXT functions */
    Cdtext *cdtext = cd_get_cdtext (cd);
    for (int pti = -1; pti <= PTI_END; pti++)
      cdtext_get ((enum Pti)pti, cdtext);

    /* Cover REM functions */
    Rem *rem = cd_get_rem (cd);
    if (rem != NULL)
      for (unsigned int i = -1; i <= REM_END; i++)
        rem_get (i, rem);

    /* Cover Track functions */
    for (int i = -1; i <= ntracks; i++)
    {
      Track *track = cd_get_track (cd, i);
      if (track == NULL)
        continue;
      track_get_filename (track);
      track_get_start (track);
      track_get_length (track);
      track_get_mode (track);
      track_get_sub_mode (track);
      track_is_set_flag (track, (enum TrackFlag)0);
      track_get_zero_pre (track);
      track_get_zero_post (track);
      track_get_isrc (track);
      track_get_index (track, 0);
      track_get_index (track, -1);
      track_get_cdtext (track);
    }

    cd_delete (cd);
    
    free (str);
    return 0;
}