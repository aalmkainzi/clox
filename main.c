#include "clox.h"
#include "expr.h"
#include "token.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define local_ref(x) \
// &(typeof((void)0,x)[]){x}

static int run_file(const char *file_name);
static int run_prompt();

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        fprintf(stderr, "usage: clox [script]");
        exit(EXIT_FAILURE);
    }
    else if (argc == 2)
    {
        return run_file(argv[1]);
    }
    else // just `clox`, run REPL
    {
        return run_prompt();
    }
}

static int run(CGS_StrView file_name, CGS_DStr line);

static int run_file(const char *file_name)
{
    FILE *f            = fopen(file_name, "r");
    CGS_DStr file_data = cgs_dstr_init();

    errno         = 0;
    CGS_Error err = cgs_fread_until(&file_data, f, EOF);
    if (err.ec != CGS_OK)
    {
        cgs_fprintf(stderr, "could not read file %?: %?: %?\n", file_name, err, strerror(errno));
    }

    int return_err = run(cgs_strv(file_name), file_data);
    cgs_dstr_deinit(&file_data);
    return return_err;
}

static int run_prompt()
{
    CGS_DStr line = cgs_dstr_init();

    while (!feof(stdin))
    {
        cgs_printf("> ");
        CGS_Error err = cgs_fread_until(&line, stdin, '\n');
        if (err.ec == CGS_OK)
        {
            line.len -= 1; // no need for newline
            run(cgs_strv("<repl>"), line);
        }
    }
    cgs_dstr_deinit(&line);
    return 0;
}

static int run(CGS_StrView file_name, CGS_DStr src)
{
    if (src.len == 0)
        return 0;
    // appending empty spaces in cases we try to match a long keyword
    cgs_append(&src, "        ");

    Ctx *ctx = &(Ctx) {
        .lineno    = 1,
        .file_name = file_name,
        .src       = cgs_strv(src),
    };

    tokenize(ctx);

    Expr *expr = parse(ctx);

    if (!ctx->error)
    {
        rpn_print(cgs_writer_ptr(stdout), expr);
        puts("");
        pretty_print(cgs_writer_ptr(stdout), expr);
        puts("");
        Literal result = eval(ctx, expr);
        cgs_printfln("%?", result);
        free_literal(result);
    }

    free_expr(expr);
    free_tokens(&ctx->toks);

    if (ctx->error)
        return 1;
    else
        return 0;
}
