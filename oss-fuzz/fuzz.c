#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "libcue.h"

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    if (Size == 0)
        return 0;

    char* str = malloc (Size);
    if (!str)
        return -1;

    memcpy(str, Data, Size);
    str[Size - 1] = '\0';

    Cd *cd = cue_parse_string (str);
    cd_delete (cd);
    
    free (str);
    return 0;
}