#include "clox.h"
#include "expr.h"
#include "stmt.h"
#include "token.h"

static Expr *expression(Ctx *ctx);
static Expr *or_expr(Ctx *ctx);
static Expr *and_expr(Ctx *ctx);
static Expr *equality(Ctx *ctx);
static Expr *comparison(Ctx *ctx);
static Expr *additive(Ctx *ctx);
static Expr *multitive(Ctx *ctx);
static Expr *unary(Ctx *ctx);
static Expr *primary(Ctx *ctx);
static Stmt print_stmt(Ctx *ctx);
static Stmt expr_stmt(Ctx *ctx);

StmtList parse(Ctx *ctx)
{
    StmtList stmts = StmtList_init();
    ctx->cur       = ctx->toks.data;

    while (ctx->cur && ctx->cur->kind != TK_eof)
    {
        switch (ctx->cur->kind)
        {
        case TK_print:
            StmtList_push(&stmts, print_stmt(ctx));
            break;
        default:
            StmtList_push(&stmts, expr_stmt(ctx));
            break;
        }
    }

    return stmts;
}

static Stmt expr_stmt(Ctx *ctx)
{
    Stmt stmt = {.kind = STMT_expr, .begin = ctx->cur};

    stmt.expr = or_expr(ctx);
    stmt.end  = ctx->cur;

    skip_tok(ctx, TK_semicolon);

    return stmt;
}

static Stmt print_stmt(Ctx *ctx)
{
    Stmt stmt = {.kind = STMT_print, .begin = ctx->cur};
    skip_tok(ctx, TK_print);

    stmt.print_expr = or_expr(ctx);
    stmt.end        = ctx->cur;

    skip_tok(ctx, TK_semicolon);
    return stmt;
}

static Expr *or_expr(Ctx *ctx)
{
    Expr *expr = and_expr(ctx);

    while (check_any(ctx, TK_or))
    {
        Token *op   = advance(ctx);
        Expr *right = and_expr(ctx);

        expr = new_binary(expr, right, op);
    }

    return expr;
}

static Expr *and_expr(Ctx *ctx)
{
    Expr *expr = equality(ctx);

    while (check_any(ctx, TK_and))
    {
        Token *op   = advance(ctx);
        Expr *right = equality(ctx);

        expr = new_binary(expr, right, op);
    }

    return expr;
}

static Expr *expression(Ctx *ctx)
{
    return equality(ctx);
}

static Expr *equality(Ctx *ctx)
{
    Expr *expr = comparison(ctx);

    while (check_any(ctx, TK_equal_equal, TK_not_equal))
    {
        Token *op   = advance(ctx);
        Expr *right = comparison(ctx);

        expr = new_binary(expr, right, op);
    }

    return expr;
}

static Expr *comparison(Ctx *ctx)
{
    Expr *expr = additive(ctx);

    while (check_any(ctx, TK_lt, TK_lte, TK_gt, TK_gte))
    {
        Token *op   = advance(ctx);
        Expr *right = additive(ctx);

        expr = new_binary(expr, right, op);
    }

    return expr;
}

static Expr *additive(Ctx *ctx)
{
    Expr *expr = multitive(ctx);

    while (check_any(ctx, TK_plus, TK_minus))
    {
        Token *op   = advance(ctx);
        Expr *right = multitive(ctx);

        expr = new_binary(expr, right, op);
    }

    return expr;
}

static Expr *multitive(Ctx *ctx)
{
    Expr *expr = unary(ctx);

    while (check_any(ctx, TK_star, TK_slash))
    {
        Token *op   = advance(ctx);
        Expr *right = unary(ctx);

        expr = new_binary(expr, right, op);
    }

    return expr;
}

static Expr *unary(Ctx *ctx)
{
    if (check_any(ctx, TK_not, TK_minus))
    {
        Token *op = advance(ctx);
        Expr *exp = unary(ctx);
        Expr *new = new_unary(exp, op);
        return new;
    }
    else
    {
        return primary(ctx);
    }
}

static Expr *primary(Ctx *ctx)
{
    if (ctx->cur->kind == TK_lparen)
    {
        advance(ctx);
        Expr *inside = expression(ctx);
        skip_tok(ctx, TK_rparen);
        return inside;
    }

    Expr *expr = NULL;

    if (ctx->cur->kind == TK_ident)
    {
        report_tok_error(ctx, ctx->cur, "identifiers not supported in this context", tok2str[ctx->cur->kind]);
    }
    else
    {
        expr = new_literal(ctx->cur);

        if (expr == NULL)
            report_tok_error(ctx, ctx->cur, "unexpected %?", tok2str[ctx->cur->kind]);
    }

    advance(ctx);
    return expr;
}
