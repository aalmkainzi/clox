#include "clox.h"

#ifdef _WIN32
    #include <io.h>
    #define isatty _isatty
    #define stdout_fileno _fileno(stdout)
#else
    #include <unistd.h>
    #define stdout_fileno STDOUT_FILENO
#endif

_Thread_local char error_message_buf[1024] = {};

void report_error_(Ctx *ctx, const CGS_StrView error_loc)
{
    ctx->error = true;

    CGS_StrView src_up_to_error = cgs_strv(ctx->src, 0, error_loc.chars - ctx->src.chars);
    int lineno                  = cgs_count(src_up_to_error, "\n") + 1;

    unsigned int line_end = error_loc.chars - ctx->src.chars;
    for (; line_end < ctx->src.len; line_end++)
    {
        if (ctx->src.chars[line_end] == '\n')
        {
            break;
        }
    }

    int line_start = error_loc.chars - ctx->src.chars;
    for (; line_start >= 0; line_start--)
    {
        if (ctx->src.chars[line_start] == '\n')
        {
            break;
        }
    }
    line_start += 1; // we dont want to start at the '\n' (or at -1)

    CGS_StrView line    = cgs_strv(ctx->src, line_start, line_end);
    unsigned int offset = error_loc.chars - line.chars + 1;

    unsigned int lineno_vertical_bar_length       = 0;
    CGS_ChainWriter stderr_writer_and_len_counter = cgs_writer(stderr, &lineno_vertical_bar_length);

    const char *color_reset = "";
    const char *red_begin   = "";
    const char *cyan_begin  = "";

    if (isatty(stdout_fileno))
    {
        color_reset = "\x1b[0m";
        red_begin   = "\x1b[31m";
        cyan_begin  = "\x1b[36m";
    }

    if (!ctx->repl)
    {
        cgs_appendf(stderr, "%?:%?:%?: ", ctx->file_name, lineno, offset);
    }
    cgs_appendf(stderr, "%?error:%? %?\n", red_begin, color_reset, error_message_buf);
    cgs_appendf(&stderr_writer_and_len_counter, "%?| ", cgs_alignfmt(lineno, LEFT, 3));
    cgs_appendf(stderr, "%?\n", line);
    cgs_appendf(
        stderr,
        "%?%?%?%?%?\n",
        cgs_alignfmt("| ", RIGHT, lineno_vertical_bar_length),
        cyan_begin,
        cgs_alignfmt("^", RIGHT, offset),
        cgs_repeatfmt("~", error_loc.len - 1),
        color_reset
    );
}

void report_tok_error_(Ctx *ctx, Token *tok)
{
    if (tok->kind == TK_eof)
    {
        Token *prev_tok = tok - 1;
        char *eof_loc   = prev_tok->lexeme.chars + prev_tok->lexeme.len;
        report_error_(ctx, (CGS_StrView) {.chars = eof_loc, .len = 1});
    }
    else
    {
        report_error_(ctx, tok->lexeme);
    }
}

void report_tok_range_error_(Ctx *ctx, Token *begin_tok, Token *end_tok)
{
    CGS_StrView range_view;
    range_view.chars = begin_tok->lexeme.chars;
    range_view.len   = (end_tok->lexeme.chars - begin_tok->lexeme.chars) + end_tok->lexeme.len;
    report_error_(ctx, range_view);
}
