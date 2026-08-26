#include "expr.h"
#include "token.h"

void pretty_print(CGS_Writer *sink, Expr *expr)
{
    if (expr->kind != EXPR_literal)
        cgs_append(sink, "(");

    switch (expr->kind)
    {
    case EXPR_unary:
        cgs_appendf(sink, "%? ", expr->unary.op->lexeme);
        pretty_print(sink, expr->unary.expr);
        break;
    case EXPR_binary:
        cgs_appendf(sink, "%? ", expr->binary.op->lexeme);
        pretty_print(sink, expr->binary.left);
        cgs_putc(sink, ' ');
        pretty_print(sink, expr->binary.right);
        break;
    case EXPR_literal:
        cgs_appendf(sink, "%?", expr->literal);
        break;
    }

    if (expr->kind != EXPR_literal)
        cgs_append(sink, ")");
}

void rpn_print(CGS_Writer *sink, Expr *expr)
{
    switch (expr->kind)
    {
    case EXPR_unary:
        rpn_print(sink, expr->unary.expr);
        cgs_appendf(sink, " %?", expr->unary.op->lexeme);
        break;
    case EXPR_binary:
        rpn_print(sink, expr->binary.left);
        cgs_append(sink, " ");
        rpn_print(sink, expr->binary.right);
        cgs_appendf(sink, " %?", expr->binary.op->lexeme);
        break;
    case EXPR_literal:
        cgs_appendf(sink, "%?", expr->literal);
        break;

    default:;
    }
}
