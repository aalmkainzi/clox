#ifndef LINEMAP_H
#define LINEMAP_H

#include "cgs/cgs.h"
#include <stddef.h>

typedef struct LineMap
{
    struct Arena *arena;
    int *offsets;
    int count;
} LineMap;

void linemap_fill(LineMap *lm, CGS_StrView src);
int linemap_get_lineno(LineMap *lm, int file_offset);

#endif
