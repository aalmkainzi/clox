#include "clox.h"
#include "linemap.h"
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

_Thread_local char error_message_buf[1024] = {};

static const char *error_color_reset = "";
static const char *error_red_begin   = "";
static const char *error_green_begin = "";
static const char *error_cyan_begin  = "";

void error_message_()
{
    cgs_fprintfln(stderr, "%?error:%? %?", error_red_begin, error_color_reset, error_message_buf);
}

void report_error_(Ctx *ctx, const CGS_StrView error_loc)
{
    ctx->error = true;

    int lineno = linemap_get_lineno(&ctx->linemap, error_loc.chars - ctx->src.chars);

    if (ctx->test)
    {
        auto expected_entry = ExpectedErrors_get(
            &ctx->expected_errors, linemap_get_lineno(&ctx->linemap, error_loc.chars - ctx->src.chars)
        );

        if (expected_entry && StrViews_contains(&expected_entry->second, cgs_strv(error_message_buf)))
        {
            cgs_fprintfln(
                stderr,
                "%?passed%?: error '%?' has occured",
                error_green_begin,
                error_color_reset,
                error_message_buf
            );
            return;
        }

        ctx->passed_test = false;
    }

    unsigned int line_begin = ctx->linemap.offsets[lineno - 1];
    unsigned int line_end;
    if (ctx->linemap.count == lineno)
    {
        for (line_end = line_begin; line_end < ctx->src.len; line_end++)
            ;
    }
    else
    {
        line_end = ctx->linemap.offsets[lineno] - 1;
    }

    CGS_StrView line    = cgs_strv(ctx->src, line_begin, line_end);
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

int parse_tests(Ctx *ctx)
{
    ctx->expected_errors = ExpectedErrors_init();

    CGS_StrView walk = ctx->src;
    CGS_Result(int) found;

    while ((found = cgs_find(walk, "//")).err.ec == CGS_OK)
    {
        CGS_StrView comment           = cgs_strv(walk, found.val);
        CGS_Result(int) newline_found = cgs_find(comment, "\n");

        unsigned int comment_end_pos;
        if (newline_found.err.ec == CGS_OK)
        {
            comment_end_pos = newline_found.val;
        }
        else
        {
            comment_end_pos = comment.len;
        }

        comment = cgs_strv(comment, 0, comment_end_pos);

        CGS_StrView comment_walk = comment;
        CGS_Result(int) expected_found;
        while ((expected_found = cgs_find(comment_walk, "expected-error")).err.ec == CGS_OK)
        {
            comment_walk = cgs_strv(comment_walk, expected_found.val);
            comment_walk.chars += strlen("expected-error");
            comment_walk.len -= strlen("expected-error");

            comment_walk = cgs_skip_any(comment_walk, " \t");
            if (!cgs_starts_with(comment_walk, "{{"))
            {
                report_error(ctx, comment_walk, "expected {{message}}, but got %?", comment_walk.chars[0]);
                return EXIT_FAILURE;
            }

            CGS_Result(int) found_message_end = cgs_find(comment_walk, "}}");
            if (found_message_end.err.ec != CGS_OK)
            {
                report_error(ctx, comment_walk, "expected {{message}}, but got %?", comment_walk);
                return EXIT_FAILURE;
            }

            CGS_StrView msg = cgs_strv(comment_walk, 2, found_message_end.val);

            int lineno = linemap_get_lineno(&ctx->linemap, msg.chars - ctx->src.chars);

            auto entry = ExpectedErrors_get_mut(&ctx->expected_errors, lineno);
            if (entry)
            {
                StrViews_push(&entry->second, msg);
            }
            else
            {
                ExpectedErrors_result res = ExpectedErrors_insert(&ctx->expected_errors, lineno, StrViews_init());
                StrViews_push(&res.ref->second, msg);
            }

            comment_walk = cgs_strv(comment_walk, found_message_end.val + 2);
        }
        walk = cgs_strv(walk, (comment.chars - walk.chars) + comment_end_pos);
    }
    return 0;
}

void init_error_c()
{
    if (isatty(stderr_fileno))
    {
        error_color_reset = "\x1b[0m";
        error_red_begin   = "\x1b[31m";
        error_green_begin = "\x1b[32m";
        error_cyan_begin  = "\x1b[36m";
    }
}
