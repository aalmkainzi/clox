#include "clox.h"
#include "expr.h"
#include "stmt.h"
#include "token.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int run_file(const char *file_name);
static int repl_loop();

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
        return repl_loop();
    }
}

static int run(CGS_StrView file_name, CGS_DStr *line);

static int run_file(const char *file_name)
{
    FILE *f            = fopen(file_name, "r");
    CGS_DStr file_data = cgs_dstr_init();

    CGS_Error err = cgs_fread_until(&file_data, f, EOF);
    if (err.ec != CGS_OK)
    {
        cgs_fprintf(stderr, "could not read file %?: %?: %?\n", file_name, err, strerror(errno));
    }

    int return_err = run(cgs_strv(file_name), &file_data);
    cgs_dstr_deinit(&file_data);
    return return_err;
}

static bool tokens_empty(Ctx *ctx)
{
    return TokensChain_back(&ctx->toks)->size <= 1;
}

static int run_repl_line(Ctx *ctx)
{
    ctx->error = false;

    tokenize(ctx);

    if (ctx->error || tokens_empty(ctx))
        return 0;

    parse(ctx);
    execute_stmts(ctx);
    StmtList_drop(&ctx->stmts);

    ctx->lineno += 1;
    if (ctx->error)
        return 1;
    else
        return 0;
}

#define T Lines, CGS_DStr
#include "stc/vec.h"

static int repl_loop()
{
    Lines lines = Lines_init();

    Ctx *ctx = &(Ctx) {
        .lineno    = 1,
        .file_name = cgs_strv("<repl>"),
        .repl      = true,
        .vars      = VarMap_init(),
    };

    while (!feof(stdin))
    {
        cgs_printf("> ");
        CGS_DStr *line = Lines_push(&lines, cgs_dstr_init());
        CGS_Error err  = cgs_fread_until(line, stdin, '\n');
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

    Lines_drop(&lines);
    TokensChain_drop(&ctx->toks);
    VarMap_drop(&ctx->vars);

    return 0;
}

static int run(CGS_StrView file_name, CGS_DStr *src)
{
    // appending empty spaces in cases we try to match a long keyword
    cgs_append(src, "        ");

    Ctx *ctx = &(Ctx) {.lineno = 1, .file_name = file_name, .src = cgs_strv(src), .repl = false, .vars = VarMap_init()};

    tokenize(ctx);
    if (ctx->error || tokens_empty(ctx))
        goto done;

    parse(ctx);
    execute_stmts(ctx);

done:
    StmtList_drop(&ctx->stmts);
    TokensChain_drop(&ctx->toks);
    VarMap_drop(&ctx->vars);

    if (ctx->error)
        return 1;
    else
        return 0;
}
