#include "clox.h"
#include "linemap.h"
#include <stdlib.h>

#ifdef _WIN32
    #include <io.h>
    #define isatty _isatty
    #define stdout_fileno _fileno(stdout)
#else
    #include <unistd.h>
    #define stdout_fileno STDOUT_FILENO
#endif

_Thread_local char error_message_buf[1024] = {};

static const char *error_color_reset = "";
static const char *error_red_begin   = "";
static const char *error_cyan_begin  = "";

void error_message_()
{
    cgs_fprintfln(stderr, "%?error:%? %?", error_red_begin, error_color_reset, error_message_buf);
}

void report_error_(Ctx *ctx, const CGS_StrView error_loc)
{
    ctx->error = true;

    int lineno                  = linemap_get_lineno(&ctx->linemap, error_loc.chars - ctx->src.chars);

    if (ctx->test)
    {
        const CGS_StrView *expected = ExpectedErrors_at(&ctx->expected_errors, 0); // TODO not 0
        if (cgs_equal(*expected, error_message_buf))
        {
            return;
        }
    }
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

    if (!ctx->repl)
    {
        cgs_appendf(stderr, "%?:%?:%?: ", ctx->file_name, lineno, offset);
    }

    error_message_(); // prints "error: error_message_buf"

    cgs_appendf(&stderr_writer_and_len_counter, "%?| ", cgs_alignfmt(lineno, LEFT, 3));
    cgs_appendf(stderr, "%?\n", line);
    cgs_appendf(
        stderr,
        "%?%?%?%?%?\n",
        cgs_alignfmt("| ", RIGHT, lineno_vertical_bar_length),
        error_cyan_begin,
        cgs_alignfmt("^", RIGHT, offset),
        cgs_repeatfmt("~", error_loc.len - 1),
        error_color_reset
    );
}

void report_tok_error_(Ctx *ctx, Token *tok)
{
    if (tok->kind == TK_eof)
    {
        Token *prev_tok     = tok - 1;
        const char *eof_loc = prev_tok->lexeme.chars + prev_tok->lexeme.len;
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

void clox_init()
{
    if (isatty(stdout_fileno))
    {
        error_color_reset = "\x1b[0m";
        error_red_begin   = "\x1b[31m";
        error_cyan_begin  = "\x1b[36m";
    }
}

// SLOW for now
int parse_tests(Ctx *ctx)
{
    CGS_StrView found = {};
    CGS_StrView walk  = ctx->src;
    // TODO dont find {{, instead find "// expected-error" and skip non-line whitespace
    while ((found = cgs_find(walk, "// expected-error {{"), found.chars))
    {
        unsigned int offset = (found.chars + found.len) - ctx->src.chars;
        CGS_StrView rem     = cgs_strv(ctx->src, offset);
        CGS_StrView end     = cgs_find(rem, "}}");
        CGS_StrView msg     = cgs_strv(rem, 0, end.chars - rem.chars);
        if (cgs_find(msg, "\n").chars)
        {
            error_message("unterminated expected-error message");
            ExpectedErrors_drop(&ctx->expected_errors);
            return EXIT_FAILURE;
        }
        int lineno = linemap_get_lineno(&ctx->linemap, offset);
        ExpectedErrors_insert(&ctx->expected_errors, lineno, msg);

        walk = rem;
    }
    return 0;
}
