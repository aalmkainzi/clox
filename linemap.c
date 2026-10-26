#include "linemap.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void linemap_put(LineMap *lm, int line_begin_file_offset)
{
    if (lm->count >= lm->cap)
    {
        if (lm->cap == 0)
        {
            lm->cap     = 16;
            lm->offsets = malloc(lm->cap * sizeof(lm->offsets[0]));
        }
        else
        {
            lm->cap *= 2;
            lm->offsets = realloc(lm->offsets, lm->cap * sizeof(lm->offsets[0]));
        }
    }

    lm->offsets[lm->count++] = line_begin_file_offset;

    assert(lm->count == 1 || (lm->offsets[lm->count - 1] > lm->offsets[lm->count - 2]));
}

int linemap_get_lineno(LineMap *lm, int file_offset)
{
    for (int i = 0; i < lm->count - 1; i++)
    {
        if (file_offset >= lm->offsets[i])
        {
            if (file_offset < lm->offsets[i + 1])
            {
                return i + 1; // line numbers start from 1
            }
        }
    }

    return lm->count;
}

void linemap_fill(LineMap *lm, CGS_StrView src)
{
    CGS_StrView walk = src;
    linemap_put(lm, 0);

    CGS_Result(int) found_newline;
    while ((found_newline = cgs_find(walk, "\n")).err.ec == CGS_OK)
    {
        linemap_put(lm, found_newline.val + (walk.chars - src.chars) + 1);
        walk = cgs_strv(walk, found_newline.val + 1);
    }

    if (lm->count == 0)
    {
        linemap_put(lm, 0);
    }
}

void linemap_free(LineMap *lm)
{
    free(lm->offsets);
    memset(lm, 0, sizeof(LineMap));
}
