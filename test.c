#include "clox.h"
#include "error.h"

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

        CGS_StrView literal_list = cgs_strv(after_directive, 2); // ...}}

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
        // literal list is now just the comma seperated list

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

bool verify_expected_output(Ctx *ctx, CGS_DStr output)
{
    if (ctx->test_data.expected_outputs.size == ctx->test_data.verified_outputs)
    {
        error("unexpected output '%?'", output);
        ctx->failed_test = true;
    }
    else if (!cgs_equal(output, ctx->test_data.expected_outputs.data[ctx->test_data.verified_outputs]))
    {
        error(
            "expected output '%?', but got '%?'",
            ctx->test_data.expected_outputs.data[ctx->test_data.verified_outputs],
            output
        );

        ctx->failed_test = true;
    }
    else
    {
        passed("expected output '%?' has been printed", output);
        ctx->test_data.verified_outputs++;
        return true;
    }
    return false;
}

bool verify_expected_error(Ctx *ctx, CGS_StrView error_loc, CGS_StrView error_message)
{
    auto expected_entry = ExpectedErrors_get(
        &ctx->test_data.expected_errors, linemap_get_lineno(&ctx->linemap, error_loc.chars - ctx->src.chars)
    );

    if (expected_entry && StrViews_contains(&expected_entry->second, error_message))
    {
        passed("error '%?' has occured", error_message);
        return true;
    }

    ctx->failed_test = true;
    return false;
}

void check_remaining_expected_output(Ctx *ctx)
{
    if (ctx->test_data.verified_outputs != ctx->test_data.expected_outputs.size)
    {
        ctx->failed_test = true;
        for (unsigned int i = ctx->test_data.verified_outputs; i < ctx->test_data.expected_outputs.size; i++)
        {
            error("expected output '%?' did not occur", ctx->test_data.expected_outputs.data[i]);
        }
    }
}

void free_tests(Ctx *ctx)
{
    for (c_each(it, StrViews, ctx->test_data.expected_outputs))
    {
        free((void *)it.ref->chars);
    }

    StrViews_drop(&ctx->test_data.expected_outputs);
    ExpectedErrors_drop(&ctx->test_data.expected_errors);
}
