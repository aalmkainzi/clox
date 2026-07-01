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

Expr *new_literal(Token *tok)
{
    Expr *ret = malloc(sizeof(Expr));
    ret->kind = EXPR_literal;
    ret->literal.tok = tok;
    
    switch(tok->kind)
    {
        case  TK_nil  :
            ret->literal.kind = LIT_nil;
        break;
        case  TK_str  :
            ret->literal.kind = LIT_str;
        break;
        case  TK_num  :
        case  TK_float :
            ret->literal.kind = LIT_num;
        break;
        case  TK_true :
        case  TK_false:
            ret->literal.kind = LIT_bool;
        break;
        
        default:
            free(ret);
            return NULL;
        break;
    }
    
    return ret;
}

void free_expr(Expr *expr)
{
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