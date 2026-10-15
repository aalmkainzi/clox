#include "clox.h"
#include "expr.h"
#include "stmt.h"
#include "token.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    bool verbose;
    bool test;
    const char *file_name;
} Opts;

static int run_file(Opts opts);
static int repl_loop(Opts opts);

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (argc > 3)
    {
        cgs_fwriteln(stderr, "usage: clox [script] [options]");
        exit(EXIT_FAILURE);
    }

    clox_init();

    Opts opts = {};
    for (int i = 1; i < argc; i++)
    {
        if (cgs_equal(argv[i], "-v"))
        {
            opts.verbose = true;
        }
        else if (cgs_equal(argv[i], "-t"))
        {
            opts.test = true;
        }
        else
        {
            if (argv[i][0] == '-')
            {
                error_message("unsupported flag '%?'", argv[i]);
                return EXIT_FAILURE;
            }
            if (opts.file_name)
            {
                error_message("only one file");
                return EXIT_FAILURE;
            }
            opts.file_name = argv[i];
        }
    }

    if (opts.verbose)
        cgs_printfln("verbose = %?", opts.verbose);

    if (opts.file_name)
    {
        return run_file(opts);
    }
    else
    {
        return repl_loop(opts);
    }
}

static int run(Opts opts, CGS_DStr *line);

static int run_file(Opts opts)
{
    FILE *f = fopen(opts.file_name, "r");
    if (f == NULL)
    {
        error_message("could not open file '%?': %?", opts.file_name, strerror(errno));
        return EXIT_FAILURE;
    }
    CGS_DStr file_data = cgs_dstr_init();

    CGS_Error err = cgs_append_fread_until(&file_data, f, EOF);
    if (err.ec != CGS_OK)
    {
        error_message("could not read file %?: %?: %?\n", opts.file_name, err, strerror(errno));
        return EXIT_FAILURE;
    }

    int return_err = run(opts, &file_data);
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

    if (!ctx->error)
        execute_stmts(ctx);

    for (c_each(it, StmtList, ctx->stmts))
    {
        free_stmt(it.ref);
    }
    StmtList_clear(&ctx->stmts);
    // StmtList_drop(&ctx->scope->stmts);

    if (ctx->error)
        return 1;
    else
        return 0;
}

#define T Lines, CGS_DStr
#include "stc/vec.h"

static int repl_loop(Opts opts)
{
    Lines lines = Lines_init();

    Ctx *ctx = &(Ctx) {
        .file_name = cgs_strv("<repl>"),
        .repl      = true,
        .verbose   = opts.verbose,
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
            cgs_append(line, "        "); // HACK delete this
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

static int run(Opts opts, CGS_DStr *src)
{
    // appending empty spaces in cases we try to match a long keyword
    cgs_append(src, "        "); // HACK delete this

    Ctx *ctx = &(Ctx) {
        .file_name = cgs_strv(opts.file_name),
        .src       = cgs_strv(src),
        .repl      = false,
        .verbose   = opts.verbose,
        .test      = opts.test,
        .scope     = calloc(1, sizeof(Scope)),
    };

    linemap_fill(&ctx->linemap, ctx->src);

    if (ctx->test)
    {
        parse_tests(ctx);
    }

    init_scope(ctx->scope);

    tokenize(ctx);
    if (ctx->error || tokens_empty(ctx))
        goto done;
    if (ctx->verbose)
    {
        print_tokens(ctx);
    }
    parse(ctx);

    if (!ctx->error)
        execute_stmts(ctx);

done:
    for (c_each(it, StmtList, ctx->stmts))
    {
        free_stmt(it.ref);
    }
    StmtList_drop(&ctx->stmts);

    TokensChain_drop(&ctx->toks);
    free_scope(ctx->scope);
    linemap_free(&ctx->linemap);
    if (ctx->test)
    {
        ExpectedErrors_drop(&ctx->expected_errors);
    }

    if (ctx->error)
        return EXIT_FAILURE;
    else
        return 0;
}
