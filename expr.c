#include "clox.h"

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
        
        default:
            return NULL;
        break;
    }
    
    return ret;
}