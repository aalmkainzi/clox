#include "expr.h"
#include "clox.h"

Expr *new_binary(Expr *left, Expr *right, Token *op)
{
    Expr *ret = malloc(sizeof(Expr));
    ret->kind = EXPR_binary;
    ret->binary.left = left;
    ret->binary.right = right;
    ret->binary.op = op;
    
    return ret;
}

Expr *new_unary(Expr *left, Token *op)
{
    Expr *ret = malloc(sizeof(Expr));
    ret->kind = EXPR_unary;
    ret->unary.expr = left;
    ret->unary.op = op;
    
    return ret;
}

Expr *new_grouping(Expr *inside)
{
    Expr *ret = malloc(sizeof(Expr));
    ret->kind = EXPR_grouping;
    ret->grouping.expr = inside;
    
    return ret;
}

Expr *alloc_literal_expr(LiteralKind litKind, Token *tok)
{
    Expr *ret = malloc(sizeof(Expr));
    ret->kind = EXPR_literal;
    ret->literal.tok = tok;
    ret->literal.kind = litKind;
    return ret;
}

Expr *new_literal(Token *tok)
{
    switch(tok->kind)
    {
        case  TK_nil  :
            return alloc_literal_expr(LIT_nil, tok);
        case  TK_str  :
            return alloc_literal_expr(LIT_str, tok);
        case  TK_num  :
        case  TK_float:
            return alloc_literal_expr(LIT_num, tok);
        case  TK_true :
        case  TK_false:
            return alloc_literal_expr(LIT_bool, tok);
        default:
            return NULL;
    }
}

void free_expr(Expr *expr)
{
    if (expr == NULL)
        return;

    switch(expr->kind)
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
        case EXPR_grouping:
            free_expr(expr->grouping.expr);
        break;
        case EXPR_literal:
            free(expr);
        break;
        
        case EXPR_ident:
        break;
    }
}

Token *expr_tok(Expr *expr)
{
    switch(expr->kind)
    {
        case EXPR_unary:
            return expr->unary.op;
        case EXPR_binary:
            return expr->binary.op;
        case EXPR_grouping:
            return expr_tok(expr->grouping.expr);
        case EXPR_literal:
            return expr->literal.tok;
        case EXPR_ident:
            return expr->ident;
        default:
            __builtin_unreachable();
    }
}