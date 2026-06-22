#include "clox.h"

void error_at(CGS_StrView file_name, int lineno, int offset, CGS_StrView src, CGS_StrView message)
{
    CGS_StrViewArray lines = cgs_split(src, "\n");
    
    CGS_StrView line = lines.strs[lineno];
    
    cgs_fprintf(stderr, "%?:%?:%?: error: %?\n", file_name, lineno, offset, message);
    cgs_fprintf(stderr, "%? | %?\n", lineno, line);
    cgs_fprintf(stderr, "%?\n", cgs_alignfmt("^", CGS_ALIGN_RIGHT, 1));
}
