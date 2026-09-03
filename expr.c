#include "expr.h"
#include "clox.h"

Expr *new_binary(Expr *left, Expr *right, Token *op)
{
    Expr *ret         = calloc(1, sizeof(Expr));
    ret->kind         = EXPR_binary;
    ret->binary.left  = left;
    ret->binary.right = right;
    ret->binary.op    = op;

    return ret;
}

Expr *new_unary(Expr *left, Token *op)
{
    Expr *ret       = calloc(1, sizeof(Expr));
    ret->kind       = EXPR_unary;
    ret->unary.expr = left;
    ret->unary.op   = op;

    return ret;
}

Expr *alloc_literal_expr(LiteralKind litKind, Token *tok)
{
    static const TypeKind map[] = {
        [LIT_bool] = T_bool,
        [LIT_num]  = T_num,
        [LIT_str]  = T_str,
        [LIT_nil]  = T_nil,
    };

    TypeKind type = map[litKind];

    Expr *ret       = calloc(1, sizeof(Expr));
    ret->kind       = EXPR_value;
    ret->value.tok  = tok;
    ret->value.type = type;

    switch (type)
    {
        case T_bool:
            ret->value.boolean = tok->data.boolean;
            break;
        case T_num:
            ret->value.number = tok->data.dbl;
            break;
        case T_str:
            ret->value.string = cgs_dup(tok->data.string);
            break;
        case T_nil:
            break;
        case T_none:
            unreachable();
    }

    return ret;
}

Expr *new_literal(Token *tok)
{
    switch (tok->kind)
    {
        case TK_nil:
            return alloc_literal_expr(LIT_nil, tok);
        case TK_str:
            return alloc_literal_expr(LIT_str, tok);
        case TK_num:
            return alloc_literal_expr(LIT_num, tok);
        case TK_float:
            return alloc_literal_expr(LIT_num, tok);
        case TK_true:
            return alloc_literal_expr(LIT_bool, tok);
        case TK_false:
            return alloc_literal_expr(LIT_bool, tok);
        default:
            return NULL;
    }
}

Expr *new_var(Token *tok)
{
    Expr *ret  = calloc(1, sizeof(Expr));
    ret->kind  = EXPR_var;
    ret->ident = tok;
    return ret;
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
            free_value(expr->value);
            free(expr);
            break;
        case EXPR_var:
            free(expr);
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
            return expr->value.tok;
        case EXPR_var:
            return expr->ident;
    }
    unreachable();
}

CGS_StrView expr_view(Expr *expr)
{
    CGS_StrView view;
    switch (expr->kind)
    {
        case EXPR_unary:
            view = range_view(expr->unary.op->lexeme, expr_view(expr->unary.expr));
            break;
        case EXPR_binary:
            view = range_view(expr_view(expr->binary.left), expr_view(expr->binary.right));
            break;
        case EXPR_value:
            view = expr->value.tok->lexeme;
            break;
        case EXPR_var:
            view = expr->ident->lexeme;
            break;
    }
    
    return view;
}