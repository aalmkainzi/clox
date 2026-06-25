#include "clox.h"
#include <stdlib.h>

void error_at(CGS_StrView file_name, CGS_StrView src, CGS_StrView error_loc, CGS_StrView message)
{
    CGS_StrView up_to_error_loc = cgs_strv(src, 0, error_loc.chars - src.chars);
    int lineno = cgs_count(up_to_error_loc, "\n");
    
    CGS_StrView after_error_loc = cgs_strv(src, error_loc.chars - src.chars);
    
    int line_end = 0;
    for( ; line_end < after_error_loc.len ; line_end++)
    {
        if(after_error_loc.chars[line_end] == '\n')
        {
            break;
        }
    }
    
    int line_start = up_to_error_loc.len - 1;
    for( ; line_start >= 0 ; line_start--)
    {
        if(up_to_error_loc.chars[line_start] == '\n')
        {
            break;
        }
    }
    line_start += 1;
    
    unsigned int line_begin_in_src = &up_to_error_loc.chars[line_start] - src.chars;
    unsigned int line_end_in_src   = &after_error_loc.chars[line_end]   - src.chars;
    CGS_StrView line = cgs_strv(src, line_begin_in_src, line_end_in_src);
    unsigned int offset = error_loc.chars - line.chars;
    
    unsigned int count = 0;
    CGS_ChainWriter writer = cgs_writer(stderr, &count);
    
    const char *color_reset = "\x1b[0m";
    
    const char *red_begin = "\x1b[31m";
    const char *cyan_begin = "\x1b[36m";
    
    cgs_appendf(stderr, "%?:%?:%?: %?error: %?%?\n", file_name, lineno, offset, red_begin, message, color_reset);
    cgs_appendf(&writer, "%? | ", lineno);
    cgs_appendf(stderr, "%?\n%?%?%?\n", line, cyan_begin, cgs_alignfmt("^", CGS_ALIGN_RIGHT, offset + count + 1), color_reset);
    
    exit(EXIT_FAILURE);
}
