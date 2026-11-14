#include "linemap.h"
#include "arena.h"
#include <assert.h>

static void linemap_put(LineMap *lm, int line_begin_file_offset)
{
    lm->offsets[lm->count++] = line_begin_file_offset;
    assert(lm->count == 1 || (lm->offsets[lm->count - 1] > lm->offsets[lm->count - 2]));
}

int linemap_get_lineno(LineMap *lm, int file_offset)
{
    int left  = 0;
    int right = lm->count - 1;
    int mid;

    while (left <= right)
    {
        mid = left + (right - left) / 2;
        if (lm->offsets[mid] < file_offset)
            left = mid + 1;
        else if (lm->offsets[mid] > file_offset)
            right = mid - 1;
        else
            return mid + 1;
    }

    return right + 1;
}

void linemap_fill(LineMap *lm, CGS_StrView src)
{
    int n_lines = cgs_count(src, '\n') + 1;
    lm->offsets = arena_alloc(lm->arena, n_lines * sizeof(*lm->offsets));

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
