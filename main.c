#include "clox.h"
#include "expr.h"
#include "stmt.h"
#include "token.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int run_file(const char *file_name, bool verbose);
static int repl_loop(bool verbose);

int main(int argc, char **argv)
{
    if (argc > 3)
    {
        fprintf(stderr, "usage: clox [script] [-v]");
        exit(EXIT_FAILURE);
    }

    bool verbose          = false;
    const char *file_name = NULL;
    for (int i = 1; i < argc; i++)
    {
        if (cgs_equal(argv[i], "-v"))
        {
            verbose = true;
        }
        else
        {
            file_name = argv[i];
        }
    }

    if (verbose)
        cgs_printfln("verbose = %?", verbose);

    if (file_name)
    {
        return run_file(file_name, verbose);
    }
    else
    {
        return repl_loop(verbose);
    }
}

static int run(CGS_StrView file_name, CGS_DStr *line, bool verbose);

static int run_file(const char *file_name, bool verbose)
{
    FILE *f            = fopen(file_name, "r");
    CGS_DStr file_data = cgs_dstr_init();

    CGS_Error err = cgs_append_fread_until(&file_data, f, EOF);
    if (err.ec != CGS_OK)
    {
        cgs_fprintf(stderr, "could not read file %?: %?: %?\n", file_name, err, strerror(errno));
    }

    int return_err = run(cgs_strv(file_name), &file_data, verbose);
    cgs_dstr_deinit(&file_data);
    return return_err;
}

static bool tokens_empty(Ctx *ctx)
{
    return TokensChain_back(&ctx->toks)->size <= 1;
}

static void print_tokens(Ctx *ctx)
{
    for (c_each(it, TokenList, ctx->toks.last->value))
    {
        cgs_printf("(%?, %?), ", tok2str[it.ref->kind], it.ref->lexeme);
    }
    cgs_putc(stdout, '\n');
}

static int run_repl_line(Ctx *ctx)
{
    ctx->error = false;

    tokenize(ctx);

    if (ctx->error || tokens_empty(ctx))
        return 0;

    if (ctx->verbose)
        print_tokens(ctx);

    parse(ctx);

    if (ctx->verbose)
        cgs_printfln("nb statements: %?", ctx->stmts.size);

    execute_stmts(ctx);

    for (c_each(it, StmtList, ctx->stmts))
    {
        free_stmt(it.ref);
    }
    StmtList_clear(&ctx->stmts);
    // StmtList_drop(&ctx->scope->stmts);

    ctx->lineno += 1;
    if (ctx->error)
        return 1;
    else
        return 0;
}

#define T Lines, CGS_DStr
#include "stc/vec.h"

static int repl_loop(bool verbose)
{
    Lines lines = Lines_init();

    Ctx *ctx = &(Ctx) {
        .lineno    = 1,
        .file_name = cgs_strv("<repl>"),
        .repl      = true,
        .verbose   = verbose,
        .scope     = calloc(1, sizeof(Scope)),
    };

    init_scope(ctx->scope);

    while (!feof(stdin))
    {
        cgs_printf("> ");
        CGS_DStr *line = Lines_push(&lines, cgs_dstr_init());
        CGS_Error err  = cgs_append_fread_until(line, stdin, '\n');
        if (err.ec == CGS_OK)
        {
            line->len -= 1;
            cgs_append(line, "        ");
            ctx->src = cgs_strv(line);
            run_repl_line(ctx);
        }
    }

    for (c_each(line_it, Lines, lines))
    {
        CGS_DStr *line = line_it.ref;
        cgs_dstr_deinit(line);
    }

    StmtList_drop(&ctx->stmts);
    TokensChain_drop(&ctx->toks);
    Lines_drop(&lines);
    free_scope(ctx->scope);

    return 0;
}

static int run(CGS_StrView file_name, CGS_DStr *src, bool verbose)
{
    // appending empty spaces in cases we try to match a long keyword
    cgs_append(src, "        ");

    Ctx *ctx = &(Ctx) {
        .lineno    = 1,
        .file_name = file_name,
        .src       = cgs_strv(src),
        .repl      = false,
        .verbose   = verbose,
        .scope     = calloc(1, sizeof(Scope)),
    };

    init_scope(ctx->scope);

    tokenize(ctx);
    if (ctx->error || tokens_empty(ctx))
        goto done;
    if (ctx->verbose)
    {
        print_tokens(ctx);
    }
    parse(ctx);
    execute_stmts(ctx);

done:
    for (c_each(it, StmtList, ctx->stmts))
    {
        free_stmt(it.ref);
    }
    StmtList_drop(&ctx->stmts);

    TokensChain_drop(&ctx->toks);
    free_scope(ctx->scope);

    if (ctx->error)
        return EXIT_FAILURE;
    else
        return 0;
}
