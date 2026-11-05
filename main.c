#include "clox.h"
#include "error.h"
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

static void clox_init();
static void clox_deinit();

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (argc > 4)
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
        else if (cgs_equal(argv[i], "-t") || cgs_equal(argv[i], "-test"))
        {
            opts.test = true;
        }
        else
        {
            if (argv[i][0] == '-')
            {
                error("unsupported flag '%?'", argv[i]);
                return EXIT_FAILURE;
            }
            if (opts.file_name)
            {
                error("only one file");
                return EXIT_FAILURE;
            }
            opts.file_name = argv[i];
        }
    }

    if (opts.verbose)
        cgs_printfln("verbose = %?", opts.verbose);

    int ret;
    if (opts.file_name)
    {
        ret = run_file(opts);
    }
    else
    {
        ret = repl_loop(opts);
    }
    clox_deinit();
    return ret;
}

static int run(Opts opts, CGS_DStr *line);

static int run_file(Opts opts)
{
    FILE *f = fopen(opts.file_name, "r");

    if (f == NULL)
    {
        error("could not open file '%?': %?", opts.file_name, strerror(errno));
        return EXIT_FAILURE;
    }
    CGS_DStr file_data = cgs_dstr_init();

    CGS_Error err = cgs_append_fread_until(&file_data, f, EOF);
    if (err.ec != CGS_OK)
    {
        error("could not read file %?: %?: %?\n", opts.file_name, err, strerror(errno));
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

static void run_repl_line(Ctx *ctx)
{
    ctx->error = false;

    tokenize(ctx);

    if (ctx->error || tokens_empty(ctx))
        return;

    if (ctx->verbose)
        print_tokens(ctx);

    ctx->stmts = stmt_list(&ctx->parser_arena);
    parse(ctx);

    if (ctx->verbose)
        cgs_printfln("nb statements: %?", ctx->stmts.count);

    if (!ctx->error)
        execute_stmts(ctx);
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
        .linemap   = {.cap = 1, .count = 1, .offsets = (int[1]) {0}}
    };

    arena_init(&ctx->parser_arena);

    enter_scope(ctx); // global scope

    while (!feof(stdin))
    {
        cgs_printf("> ");

        CGS_DStr *line = Lines_push(&lines, cgs_dstr_init());
        CGS_Error err  = cgs_append_fread_until(line, stdin, '\n');
        if (err.ec == CGS_OK)
        {
            line->len -= 1;
            ctx->src = cgs_strv(line);
            run_repl_line(ctx);
        }
    }

    exit_scope(ctx); // global scope

    for (c_each(line_it, Lines, lines))
    {
        CGS_DStr *line = line_it.ref;
        cgs_dstr_deinit(line);
    }

    TokensChain_drop(&ctx->toks);
    Lines_drop(&lines);

    arena_free(&ctx->parser_arena);

    return EXIT_SUCCESS;
}

static int run(Opts opts, CGS_DStr *src)
{
    Ctx *ctx = &(Ctx) {
        .file_name   = cgs_strv(opts.file_name),
        .src         = cgs_strv(src),
        .repl        = false,
        .verbose     = opts.verbose,
        .test        = opts.test,
        .passed_test = true,
    };

    ctx->stmts = stmt_list(&ctx->parser_arena),

    linemap_fill(&ctx->linemap, ctx->src);

    if (ctx->test)
        parse_tests(ctx);

    tokenize(ctx);

    if (ctx->error || tokens_empty(ctx))
        goto done;
    if (ctx->verbose)
        print_tokens(ctx);

    parse(ctx);

    if (!ctx->error)
    {
        enter_scope(ctx);

        execute_stmts(ctx);

        exit_scope(ctx);
    }

done:

    TokensChain_drop(&ctx->toks);
    linemap_free(&ctx->linemap);
    arena_free(&ctx->parser_arena);

    if (ctx->test)
    {
        if (ctx->test_data.verified_outputs != ctx->test_data.expected_outputs.size)
        {
            for (unsigned int i = ctx->test_data.verified_outputs; i < ctx->test_data.expected_outputs.size; i++)
            {
                error("expected output '%?' did not occur", ctx->test_data.expected_outputs.data[i]);
            }
            ctx->passed_test = false;
        }

        for (c_each(it, StrViews, ctx->test_data.expected_outputs))
        {
            free((void *)it.ref->chars);
        }

        StrViews_drop(&ctx->test_data.expected_outputs);
        ExpectedErrors_drop(&ctx->test_data.expected_errors);
        return ctx->passed_test ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (ctx->error)
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}

static void clox_init()
{
    // if a specific system needs an init, add its decl here, and call it
    void init_error_system();

    init_error_system();
}

static void clox_deinit()
{
}
