#include "expr.h"
#include "clox.h"

Expr *new_binary(Expr *left, Expr *right, Token *op)
{
    Expr *ret         = malloc(sizeof(Expr));
    ret->kind         = EXPR_binary;
    ret->binary.left  = left;
    ret->binary.right = right;
    ret->binary.op    = op;

    return ret;
}

Expr *new_unary(Expr *left, Token *op)
{
    Expr *ret       = malloc(sizeof(Expr));
    ret->kind       = EXPR_unary;
    ret->unary.expr = left;
    ret->unary.op   = op;

    return ret;
}

Expr *alloc_literal_expr(LiteralKind litKind, Token *tok)
{
    TypeKind map[] = {
        [LIT_bool] = T_bool,
        [LIT_num]  = T_num,
        [LIT_str]  = T_str,
        [LIT_nil]  = T_nil,
    };

    TypeKind type = map[litKind];
    
    Expr *ret           = malloc(sizeof(Expr));
    ret->kind           = EXPR_value;
    ret->value.original = tok;
    ret->value.type     = type;
    
    switch(type)
    {
        case T_bool: ret->value.boolean = tok->data.boolean; break;
        case T_num : ret->value.number  = tok->data.dbl;     break;
        case T_str : ret->value.string  = tok->data.string;  break;
        case T_nil :                                         break;
    }

    return ret;
}

Expr *new_literal(Token *tok)
{
    LiteralKind map[] = {
        [TK_nil]   = LIT_nil,
        [TK_str]   = LIT_str,
        [TK_num]   = LIT_num,
        [TK_float] = LIT_num,
        [TK_true]  = LIT_bool,
        [TK_false] = LIT_bool,
    };

    return alloc_literal_expr(map[tok->kind], tok);
}

void free_expr(Expr *expr)
{
    if (expr == NULL)
        return;

    switch (expr->kind)
    {
    case EXPR_unary:
        free_expr(expr->unary.expr);
        free(expr);
        break;
    case EXPR_binary:
        free_expr(expr->binary.left);
        free_expr(expr->binary.right);
        free(expr);
        break;
    case EXPR_value:
        free(expr);
        break;
    }
}

Token *expr_tok(Expr *expr)
{
    switch (expr->kind)
    {
    case EXPR_unary:
        return expr->unary.op;
    case EXPR_binary:
        return expr->binary.op;
    case EXPR_value:
        return expr->value.original;
    default:
        __builtin_unreachable();
    }
}