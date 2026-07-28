#include "clox.h"
#include <stdlib.h>

void report_error(CGS_StrView file_name, CGS_StrView src, CGS_StrView error_loc, CGS_StrView message)
{
    CGS_StrView src_up_to_error = cgs_strv(src, 0, error_loc.chars - src.chars);
    int lineno = cgs_count(src_up_to_error, "\n") + 1;
    
    int line_end = error_loc.chars - src.chars;
    for( ; line_end < src.len ; line_end++)
    {
        if(src.chars[line_end] == '\n')
        {
            break;
        }
    }
    
    int line_start = error_loc.chars - src.chars;
    for( ; line_start >= 0 ; line_start--)
    {
        if(src.chars[line_start] == '\n')
        {
            break;
        }
    }
    line_start += 1; // we dont want to start at the '\n' (or at -1)
    
    CGS_StrView line = cgs_strv(src, line_start, line_end);
    unsigned int offset = error_loc.chars - line.chars + 1;
    
    unsigned int lineno_vertical_bar_length = 0;
    CGS_ChainWriter writer = cgs_writer(stderr, &lineno_vertical_bar_length);
    
    const char *color_reset = "\x1b[0m";
    
    const char *red_begin = "\x1b[31m";
    const char *cyan_begin = "\x1b[36m";
    
    cgs_appendf(stderr, "%?:%?:%?: %?error:%? %?\n",
                file_name, lineno, offset, red_begin, color_reset, message);
    cgs_appendf(&writer, "%?| ",
                cgs_alignfmt(lineno, LEFT, 3));
    cgs_appendf(stderr, "%?\n", line);
    cgs_appendf(stderr, "%?%?%?%?\n" , cgs_alignfmt("| ", RIGHT, lineno_vertical_bar_length), cyan_begin, cgs_alignfmt("^", RIGHT, offset), color_reset);
    
    // exit(EXIT_FAILURE);
}

void report_tok_error(CGS_StrView file_name, CGS_StrView src, Token *tok, CGS_StrView message)
{
    if(tok->kind == TK_eof)
    {
        Token *prev_tok = tok - 1;
        char *eof_loc = prev_tok->lexeme.chars + prev_tok->lexeme.len;
        report_error(file_name, src, (CGS_StrView){.chars = eof_loc, .len = 1}, message);
    }
    else
    {
        report_error(file_name, src, tok->lexeme, message);
    }
}