#include "clox.h"
#include "expr.h"
#include "token.h"

Expr *expression(Ctx *ctx);
Expr *comma(Ctx *ctx);
Expr *ternary(Ctx *ctx);
Expr *equality(Ctx *ctx);
Expr *comparison(Ctx *ctx);
Expr *additive(Ctx *ctx);
Expr *multitive(Ctx *ctx);
Expr *unary(Ctx *ctx);
Expr *primary(Ctx *ctx);

Expr *parse(Ctx *ctx)
{
    ctx->cur = ctx->toks.data;
    
    Expr *ret = expression(ctx);
    return ret;
}

Expr *expression(Ctx *ctx)
{
    return comma(ctx);
}

Expr *comma(Ctx *ctx)
{
    Expr *expr = ternary(ctx);
    
    while(check_any(TK_comma))
    {
        Token *op = advance();
        Expr *right = ternary(ctx);
        expr = new_binary(expr, right, op);
    }
    
    return expr;
}

Expr *ternary(Ctx *ctx)
{
    Expr *expr = equality(ctx);
    
    if(check_any(TK_qmark))
    {
        advance();
        Expr *true_expr = expression(ctx);
        skip(TK_colon);
        Expr *false_expr = ternary(ctx);
        expr = new_ternary(expr, true_expr, false_expr);
    }
    
    return expr;
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
    if(check_any(TK_plus, TK_star, TK_slash, TK_comma))
    {
        error_at_tok(ctx->cur, "missing left operand of %?\n", tok2str[ctx->cur->kind]);
        advance();
        return unary(ctx);
    }
    
    if(check_any(TK_not, TK_minus))
    {
        Token *op = advance();
        Expr *expr = unary(ctx);
        Expr *new = new_unary(expr, op);
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
