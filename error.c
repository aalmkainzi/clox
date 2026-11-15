#include "error.h"
#include "clox.h"
#include "linemap.h"
#include "test.h"
#include "token.h"
#include <stdlib.h>

#ifdef _WIN32
    #include <io.h>
    #define isatty _isatty
    #define stdout_fileno _fileno(stdout)
    #define stderr_fileno _fileno(stderr)
#else
    #include <unistd.h>
    #define stdout_fileno STDOUT_FILENO
    #define stderr_fileno STDERR_FILENO
#endif

static const char *error_color_reset = "";
static const char *error_red_begin   = "";
static const char *error_green_begin = "";
static const char *error_cyan_begin  = "";

void error_(CGS_DStr message)
{
    cgs_fprintfln(stderr, "%?error:%? %?", error_red_begin, error_color_reset, message);
    cgs_dstr_deinit(&message);
}

void passed_(CGS_DStr message)
{
    cgs_fprintfln(stderr, "%?passed%?: %?", error_green_begin, error_color_reset, message);
    cgs_dstr_deinit(&message);
}

void error_at_(Ctx *ctx, CGS_StrView error_loc, CGS_DStr message)
{
    if (error_loc.len == 0)
        error_loc.len = 1;

    ctx->error = true;

    int lineno = linemap_get_lineno(&ctx->linemap, error_loc.chars - ctx->src.chars);

    if (ctx->opts.test)
    {
        if (verify_expected_error(ctx, error_loc, cgs_strv(message)))
        {
            cgs_dstr_deinit(&message);
            return;
        }
    }

    unsigned int line_begin = ctx->linemap.offsets[lineno - 1];
    unsigned int line_end;
    if (ctx->linemap.count == lineno)
    {
        // for (line_end = line_begin; line_end < ctx->src.len; line_end++)
        //     ;
        line_end = ctx->src.len;
    }
    else
    {
        line_end = ctx->linemap.offsets[lineno] - 1;
    }

    CGS_StrView line    = cgs_strv(ctx->src, line_begin, line_end);
    unsigned int offset = error_loc.chars - line.chars + 1;

    unsigned int lineno_vertical_bar_length       = 0;
    CGS_ChainWriter stderr_writer_and_len_counter = cgs_writer(stderr, &lineno_vertical_bar_length);

    if (!ctx->opts.repl)
    {
        cgs_appendf(stderr, "%?:%?:%?: ", ctx->opts.file_name, lineno, offset);
    }

    error_(message);

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

void error_at_tok_(Ctx *ctx, Token tok, CGS_DStr message)
{
    if (tok.kind == TK_eof)
    {
        TokenList *toks = TokensChain_back_mut(&ctx->toks);
        Token *end      = TokenList_back(toks).ref;
        error_at_(ctx, (CGS_StrView) {.chars = end->lexeme.chars, .len = 1}, message);
    }
    else
    {
        error_at_(ctx, tok.lexeme, message);
    }
}

void error_at_tok_range_(Ctx *ctx, Token begin_tok, Token end_tok, CGS_DStr message)
{
    CGS_StrView range_view;
    range_view.chars = begin_tok.lexeme.chars;
    range_view.len   = (end_tok.lexeme.chars - begin_tok.lexeme.chars) + end_tok.lexeme.len;
    error_at_(ctx, range_view, message);
}

void init_error_system(Ctx *ctx)
{
    (void)ctx;
    if (isatty(stderr_fileno))
    {
        error_color_reset = "\x1b[0m";
        error_red_begin   = "\x1b[31m";
        error_green_begin = "\x1b[32m";
        error_cyan_begin  = "\x1b[36m";
    }
}
