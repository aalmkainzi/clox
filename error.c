#include "error.h"
#include "clox.h"
#include "linemap.h"
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

_Thread_local char error_message_buf[1024]  = {};
_Thread_local char passed_message_buf[1024] = {};

static const char *error_color_reset = "";
static const char *error_red_begin   = "";
static const char *error_green_begin = "";
static const char *error_cyan_begin  = "";

void error_()
{
    cgs_fprintfln(stderr, "%?error:%? %?", error_red_begin, error_color_reset, error_message_buf);
}

void passed_()
{
    cgs_fprintfln(stderr, "%?passed%?: %?", error_green_begin, error_color_reset, passed_message_buf);
}

void error_at_(Ctx *ctx, CGS_StrView error_loc)
{
    if (error_loc.len == 0)
        error_loc.len = 1;

    ctx->error = true;

    int lineno = linemap_get_lineno(&ctx->linemap, error_loc.chars - ctx->src.chars);

    if (ctx->test)
    {
        auto expected_entry = ExpectedErrors_get(
            &ctx->test_data.expected_errors, linemap_get_lineno(&ctx->linemap, error_loc.chars - ctx->src.chars)
        );

        if (expected_entry && StrViews_contains(&expected_entry->second, cgs_strv(error_message_buf)))
        {
            passed("error '%?' has occured", error_message_buf);
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

    error_();

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

void error_at_tok_(Ctx *ctx, Token tok)
{
    if (tok.kind == TK_eof)
    {
        TokenList *toks     = TokensChain_back_mut(&ctx->toks);
        Token *end          = TokenList_back_mut(toks);
        Token *prev_tok     = end - 1;
        const char *eof_loc = prev_tok->lexeme.chars + prev_tok->lexeme.len;
        error_at_(ctx, (CGS_StrView) {.chars = eof_loc, .len = 1});
    }
    else
    {
        error_at_(ctx, tok.lexeme);
    }
}

void error_at_tok_range_(Ctx *ctx, Token begin_tok, Token end_tok)
{
    CGS_StrView range_view;
    range_view.chars = begin_tok.lexeme.chars;
    range_view.len   = (end_tok.lexeme.chars - begin_tok.lexeme.chars) + end_tok.lexeme.len;
    error_at_(ctx, range_view);
}

#define non_newline_whitespace " \f\r\t\v"

int parse_tests(Ctx *ctx)
{
    ctx->test_data.expected_errors  = ExpectedErrors_init();
    ctx->test_data.expected_outputs = StrViews_init();

    CGS_StrView walk                          = ctx->src;
    CGS_Result(CGS_StrView) first_line_result = cgs_next_tok(&walk, '\n');
    CGS_Result(int) found_expected_output;

    // first line may be: // expected-output {{"a", "b", "c"}}
    if (first_line_result.err.ec == CGS_OK &&
        (found_expected_output =
             cgs_starts_with_tokens(first_line_result.val, non_newline_whitespace, "//", "expected-output"))
                .err.ec == CGS_OK)
    {
        CGS_StrView after_directive = cgs_strv(first_line_result.val, found_expected_output.val);

        after_directive = cgs_skip_any(after_directive, non_newline_whitespace);
        if (!cgs_starts_with(after_directive, "{{"))
        {
            error_at(ctx, after_directive, "expected {{ ...strs }}");
            return EXIT_FAILURE;
        }

        CGS_StrView literal_list = cgs_strv(after_directive, 2); // message}}

        long long after_end_brackets = -1;
        for (long long i = literal_list.len - 1; i >= 0; i--)
        {
            switch (literal_list.chars[i])
            {
                case ' ':
                case '\f':
                case '\r':
                case '\t':
                case '\v':
                    continue;
            }
            if (i == 0 || literal_list.chars[i] != '}' || literal_list.chars[i - 1] != '}')
            {
                error_at(ctx, literal_list, "expected {{ ...strs }}");
                return EXIT_FAILURE;
            }
            after_end_brackets = i + 1;
            break;
        }

        if (after_end_brackets == -1)
        {
            error_at(ctx, literal_list, "expected {{ ...strs }}");
            return EXIT_FAILURE;
        }

        literal_list = cgs_strv(literal_list, 0, after_end_brackets - 2);

        CGS_StrView list_walk = literal_list;
        list_walk             = cgs_skip_any(list_walk, non_newline_whitespace);

        bool first = true;
        while (list_walk.len)
        {
            list_walk = cgs_skip_any(list_walk, non_newline_whitespace);
            if (first)
            {
                first = false;
            }
            else
            {
                if (list_walk.chars[0] != ',')
                {
                    error_at(ctx, list_walk, "expected ',' but got %?", list_walk.chars[0]);
                    return EXIT_FAILURE;
                }
                list_walk.chars += 1;
                list_walk.len -= 1;

                list_walk = cgs_skip_any(list_walk, non_newline_whitespace);
            }

            bool err      = false;
            Token str_tok = scan_str(ctx, list_walk.chars, &err);
            if (err || (str_tok.kind != TK_str))
            {
                error_at(ctx, list_walk, "expected string literal");
                return EXIT_FAILURE;
            }

            ptrdiff_t after_tok = (str_tok.lexeme.chars + str_tok.lexeme.len) - list_walk.chars;

            if (after_tok > list_walk.len)
            {
                cgs_dstr_deinit(&str_tok.data.string);
                error_at(ctx, list_walk, "unterminated string literal");
                return EXIT_FAILURE;
            }

            list_walk.chars += after_tok;
            list_walk.len -= after_tok;

            StrViews_push(&ctx->test_data.expected_outputs, cgs_strv(str_tok.data.string));
            list_walk = cgs_skip_any(list_walk, non_newline_whitespace);
        }
    }
    else
    {
        walk = ctx->src;
    }

    CGS_Result(int) found;
    while ((found = cgs_find(walk, "//")).err.ec == CGS_OK)
    {
        CGS_StrView comment           = cgs_strv(walk, found.val);
        CGS_Result(int) newline_found = cgs_find(comment, '\n');

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

            comment_walk = cgs_skip_any(comment_walk, non_newline_whitespace);
            if (!cgs_starts_with(comment_walk, "{{"))
            {
                error_at(ctx, comment_walk, "expected {{message}}");
                return EXIT_FAILURE;
            }

            CGS_Result(int) found_message_end = cgs_find(comment_walk, "}}");
            if (found_message_end.err.ec != CGS_OK)
            {
                error_at(ctx, comment_walk, "expected {{message}}, but got %?", comment_walk);
                return EXIT_FAILURE;
            }

            CGS_StrView msg = cgs_strv(comment_walk, 2, found_message_end.val);

            int lineno = linemap_get_lineno(&ctx->linemap, msg.chars - ctx->src.chars);

            auto entry = ExpectedErrors_get_mut(&ctx->test_data.expected_errors, lineno);
            if (entry)
            {
                StrViews_push(&entry->second, msg);
            }
            else
            {
                ExpectedErrors_result res =
                    ExpectedErrors_insert(&ctx->test_data.expected_errors, lineno, StrViews_init());
                StrViews_push(&res.ref->second, msg);
            }

            comment_walk = cgs_strv(comment_walk, found_message_end.val + 2);
        }
        walk = cgs_strv(walk, (comment.chars - walk.chars) + comment_end_pos);
    }
    return 0;
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
