/*
 * cdtext_get(): the old name of cue_cdtext_get(), still exported so that
 * binaries built against earlier versions keep working.  Code compiled
 * against the current libcue.h calls cue_cdtext_get() directly.
 *
 * It lives in its own object file so that, in the static library, it is
 * only pulled in by objects that were compiled against an old libcue.h,
 * and cannot clash with libcdio's cdtext_get() otherwise (#8).
 *
 * In the shared library it carries a symbol version (see libcue.map), so it
 * cannot capture libcdio's versioned cdtext_get references at run time.
 *
 * For license terms, see the file COPYING in this distribution.
 */

#define LIBCUE_NO_CDTEXT_GET
#include "libcue.h"

CUE_EXPORT const char *cdtext_get(enum Pti pti, const Cdtext *cdtext);

const char *cdtext_get(enum Pti pti, const Cdtext *cdtext)
{
	return cue_cdtext_get(pti, cdtext);
}
