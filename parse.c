#include "clox.h"
#include "expr.h"
#include "token.h"

Expr *expression(Ctx *ctx);
Expr *or_expr(Ctx *ctx);
Expr *and_expr(Ctx *ctx);
Expr *equality(Ctx *ctx);
Expr *comparison(Ctx *ctx);
Expr *additive(Ctx *ctx);
Expr *multitive(Ctx *ctx);
Expr *unary(Ctx *ctx);
Expr *primary(Ctx *ctx);

Expr *parse(Ctx *ctx)
{
    ctx->cur = ctx->toks.data;

    Expr *ret = or_expr(ctx);
    return ret;
}

Expr *or_expr(Ctx *ctx)
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

Expr *and_expr(Ctx *ctx)
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

Expr *expression(Ctx *ctx)
{
    return equality(ctx);
}

Expr *equality(Ctx *ctx)
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

Expr *comparison(Ctx *ctx)
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

Expr *additive(Ctx *ctx)
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

Expr *multitive(Ctx *ctx)
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

Expr *unary(Ctx *ctx)
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

Expr *primary(Ctx *ctx)
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
