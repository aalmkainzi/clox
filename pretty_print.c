#include "expr.h"
#include "token.h"

void pretty_print_(CGS_Writer *sink, Expr *expr)
{
    if (expr->kind != EXPR_value)
        cgs_append(sink, "(");

    switch (expr->kind)
    {
        case EXPR_unary:
            cgs_appendf(sink, "%? ", expr->unary.op.lexeme);
            pretty_print_(sink, expr->unary.expr);
            break;
        case EXPR_binary:
            cgs_appendf(sink, "%? ", expr->binary.op.lexeme);
            pretty_print_(sink, expr->binary.left);
            cgs_putc(sink, ' ');
            pretty_print_(sink, expr->binary.right);
            break;
        case EXPR_value:
            cgs_appendf(sink, "%?", expr->value);
            break;
        case EXPR_var:
            cgs_appendf(sink, "%?", expr->ident);
            break;
    }

    if (expr->kind != EXPR_value)
        cgs_append(sink, ")");
}

void pretty_print(CGS_Writer *sink, Expr *expr)
{
    pretty_print_(sink, expr);
    cgs_putc(sink, '\n');
}

void rpn_print_(CGS_Writer *sink, Expr *expr)
{
    switch (expr->kind)
    {
        case EXPR_unary:
            rpn_print_(sink, expr->unary.expr);
            cgs_appendf(sink, " %?", expr->unary.op.lexeme);
            break;
        case EXPR_binary:
            rpn_print_(sink, expr->binary.left);
            cgs_append(sink, " ");
            rpn_print_(sink, expr->binary.right);
            cgs_appendf(sink, " %?", expr->binary.op.lexeme);
            break;
        case EXPR_value:
            cgs_appendf(sink, "%?", expr->value);
            break;

        default:;
    }
}

void rpn_print(CGS_Writer *sink, Expr *expr)
{
    rpn_print_(sink, expr);
    cgs_putc(sink, '\n');
}