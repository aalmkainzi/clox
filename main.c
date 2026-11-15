#include "arena.h"
#include "clox.h"
#include "error.h"
#include "expr.h"
#include "stmt.h"
#include "test.h"
#include "token.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int run_file(Opts opts);
static int repl_loop(Opts opts);

static void clox_init(Ctx *ctx);
static void clox_deinit();

int main(int argc, char **argv)
{
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
        else if (cgs_equal(argv[i], "-ext"))
        {
            opts.ext = true;
        }
        else
        {
            if (argv[i][0] == '-')
            {
                error("unsupported flag '%?'", argv[i]);
                return EXIT_FAILURE;
            }
            if (opts.file_name.chars)
            {
                error("only one file");
                return EXIT_FAILURE;
            }
            opts.file_name = cgs_zstrv(argv[i]);
        }
    }

    if (opts.verbose)
        cgs_printfln("verbose = %?", opts.verbose);

    int ret;
    if (opts.file_name.chars)
    {
        ret = run_file(opts);
    }
    else
    {
        opts.repl      = true;
        opts.file_name = cgs_zstrv("<repl>");
        ret            = repl_loop(opts);
    }
    clox_deinit();
    return ret;
}

// Ctx holds pointers to these, they need to be stored somewhere. This struct represents their storage
typedef struct CtxState
{
    Arena parse_arena;
    Arena eval_arena;
    ArenaAllocator parse_arena_allocator;
    ArenaAllocator eval_arena_allocator;
} CtxState;

static void fill_ctx(Ctx *ctx, Opts opts, CGS_StrView src, CtxState *state)
{
    state->parse_arena_allocator = arena_allocator(&state->parse_arena);
    state->eval_arena_allocator  = arena_allocator(&state->eval_arena);

    ctx->opts            = opts;
    ctx->parse_arena     = &state->parse_arena;
    ctx->eval_arena      = &state->eval_arena;
    ctx->parse_allocator = (CGS_Allocator *)&state->parse_arena_allocator;
    ctx->eval_allocator  = (CGS_Allocator *)&state->eval_arena_allocator;
    ctx->toks.aux        = ctx->parse_arena;
    ctx->src             = src;
    ctx->linemap.arena   = ctx->parse_arena;

    if (opts.repl)
    {
        ctx->linemap.count      = 1;
        ctx->linemap.offsets    = arena_alloc(ctx->parse_arena, sizeof(int));
        ctx->linemap.offsets[0] = 0;
    }
    else
    {
        linemap_fill(&ctx->linemap, ctx->src);
    }
}

static int run(Opts opts, CGS_DStr *line);

static int run_file(Opts opts)
{
    FILE *f = fopen(opts.file_name.chars, "r");

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

    if (ctx->opts.verbose)
        print_tokens(ctx);

    ctx->stmts = StmtList_init(ctx->parse_arena);
    parse(ctx);

    if (ctx->opts.verbose)
        cgs_printfln("nb statements: %?", ctx->stmts.size);

    if (!ctx->error)
        execute_stmts(ctx);
}

#define T Lines, CGS_DStr
#define i_aux Arena *
#define i_malloc(n) arena_alloc(self->aux, n)
#define i_free(p, n) ((void)0)
#define i_realloc(p, old_n, new_n) NULL
#include "stc/list.h"

static int repl_loop(Opts opts)
{
    Ctx *ctx = &(Ctx) {};
    fill_ctx(ctx, opts, (CGS_StrView) {}, &(CtxState) {});

    Lines lines = {.aux = ctx->parse_arena};

    clox_init(ctx);

    enter_scope(ctx); // global scope

    while (!feof(stdin))
    {
        cgs_printf("> ");

        CGS_DStr *line = Lines_push(&lines, cgs_dstr_init(0, ctx->parse_allocator));
        CGS_Error err  = cgs_append_fread_until(line, stdin, '\n');
        if (err.ec == CGS_OK)
        {
            line->len -= 1;
            ctx->src = cgs_strv(line);
            run_repl_line(ctx);
        }
    }

    exit_scope(ctx); // global scope

    TokensChain_drop(&ctx->toks);
    Lines_drop(&lines);

    arena_free(ctx->eval_arena);
    arena_free(ctx->parse_arena);

    return EXIT_SUCCESS;
}

static int run(Opts opts, CGS_DStr *src)
{
    Ctx *ctx = &(Ctx) {};
    fill_ctx(ctx, opts, cgs_strv(*src), &(CtxState) {});

    clox_init(ctx);

    ctx->stmts = StmtList_init(ctx->parse_arena);

    if (ctx->opts.test)
        parse_tests(ctx);

    tokenize(ctx);

    if (ctx->error || tokens_empty(ctx))
        goto done;
    if (ctx->opts.verbose)
        print_tokens(ctx);

    parse(ctx);

    if (ctx->error)
        goto done;

    enter_scope(ctx);

    execute_stmts(ctx);

    exit_scope(ctx);

done:

    TokensChain_drop(&ctx->toks);
    arena_free(ctx->parse_arena);
    arena_free(ctx->eval_arena);

    if (ctx->opts.test)
    {
        check_remaining_expected_output(ctx);

        free_tests(ctx);
        return ctx->test_data.failed_test ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    return ctx->error ? EXIT_FAILURE : EXIT_SUCCESS;
}

static void clox_init(Ctx *ctx)
{
    // if a specific system needs an init, add its decl here, and call it
    void init_error_system(Ctx * ctx);

    init_error_system(ctx);
}

static void clox_deinit()
{
}
