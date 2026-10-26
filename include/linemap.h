#ifndef LINEMAP_H
#define LINEMAP_H

#include "cgs/cgs.h"
#include <stddef.h>

typedef struct LineMap
{
    int *offsets;
    int count, cap;
} LineMap;

void linemap_put(LineMap *lm, int line_begin_file_offset);
int linemap_get_lineno(LineMap *lm, int file_offset);
void linemap_fill(LineMap *lm, CGS_StrView src);
void linemap_free(LineMap *lm);

#endif
