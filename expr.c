#include "expr.h"
#include "clox.h"

Expr *new_ternary(Expr *cond, Expr *true_expr, Expr *false_expr)
{
    Expr *ret = malloc(sizeof(Expr));
    ret->kind = EXPR_TERNARY;
    ret->ternary.cond = cond;
    ret->ternary.true_exp = true_expr;
    ret->ternary.false_exp = false_expr;
    
    return ret;
}

Expr *new_binary(Expr *left, Expr *right, Token *op)
{
    Expr *ret = malloc(sizeof(Expr));
    ret->kind = EXPR_BINARY;
    ret->binary.left = left;
    ret->binary.right = right;
    ret->binary.op = op;
    
    return ret;
}

Expr *new_unary(Expr *left, Token *op)
{
    Expr *ret = malloc(sizeof(Expr));
    ret->kind = EXPR_UNARY;
    ret->unary.expr = left;
    ret->unary.op = op;
    
    return ret;
}

Expr *new_grouping(Expr *inside)
{
    Expr *ret = malloc(sizeof(Expr));
    ret->kind = EXPR_GROUPING;
    ret->grouping.expr = inside;
    
    return ret;
}

Expr *new_literal(Token *tok)
{
    Expr *ret = malloc(sizeof(Expr));
    ret->kind = EXPR_LITERAL;
    ret->literal.tok = tok;
    
    switch(tok->kind)
    {
        case  TK_nil  :
            ret->literal.kind = LIT_NIL;
        break;
        
        case  TK_str  :
            ret->literal.kind = LIT_STR;
        break;
        
        case  TK_num  :
        case  TK_float :
            ret->literal.kind = LIT_NUM;
        break;
        
        case  TK_true :
        case  TK_false:
            ret->literal.kind = LIT_BOOL;
        break;
        
        case TK_ident:
            ret->literal.kind = LIT_IDENT;
        break;
        
        default:
            return NULL;
        break;
    }
    
    return ret;
}

void free_expr(Expr *expr)
{
    switch(expr->kind)
    {
        case EXPR_UNARY:
            free_expr(expr->unary.expr);
            free(expr);
        break;
        case EXPR_BINARY:
            free_expr(expr->binary.left);
            free_expr(expr->binary.right);
            free(expr);
        break;
        case EXPR_TERNARY:
            free_expr(expr->ternary.cond);
            free_expr(expr->ternary.true_exp);
            free_expr(expr->ternary.false_exp);
            free(expr);
        break;
        case EXPR_GROUPING:
            free_expr(expr->grouping.expr);
        break;
        case EXPR_LITERAL:
            free(expr);
        break;
    }
}
