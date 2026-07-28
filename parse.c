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
    
    while(check_any(TK_or))
    {
        Token *op = advance();
        Expr *right = and_expr(ctx);
        
        expr = new_binary(expr, right, op);
    }
    
    return expr;
}

Expr *and_expr(Ctx *ctx)
{
    Expr *expr = equality(ctx);
    
    while(check_any(TK_and))
    {
        Token *op = advance();
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
    
    while(check_any(TK_equal_equal, TK_not_equal))
    {
        Token *op = advance();
        Expr *right = comparison(ctx);
        
        expr = new_binary(expr, right, op);
    }
    
    return expr;
}

Expr *comparison(Ctx *ctx)
{
    Expr *expr = additive(ctx);
    
    while(check_any(TK_lt, TK_lte, TK_gt, TK_gte))
    {
        Token *op = advance();
        Expr *right = additive(ctx);
        
        expr = new_binary(expr, right, op);
    }
    
    return expr;
}

Expr *additive(Ctx *ctx)
{
    Expr *expr = multitive(ctx);
    
    while(check_any(TK_plus, TK_minus))
    {
        Token *op = advance();
        Expr *right = multitive(ctx);
        
        expr = new_binary(expr, right, op);
    }
    
    return expr;
}

Expr *multitive(Ctx *ctx)
{
    Expr *expr = unary(ctx);
    
    while(check_any(TK_star, TK_slash))
    {
        Token *op = advance();
        Expr *right = unary(ctx);
        
        expr = new_binary(expr, right, op);
    }
    
    return expr;
}

Expr *unary(Ctx *ctx)
{
    if(check_any(TK_not, TK_minus))
    {
        Token *op = advance();
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
    if(ctx->cur->kind == TK_open_paren)
    {
        advance();
        Expr *inside = expression(ctx);
        skip(TK_close_paren);
        return inside;
    }
    
    Expr *expr = new_literal(ctx->cur);
    
    if(expr == NULL)
    {
        char msg[64];
        cgs_fmt(msg, "unexpected %?", tok2str[ctx->cur->kind]);
        
        error_at_tok(ctx->cur, msg);
    }
    
    advance();
    
    return expr;
}
