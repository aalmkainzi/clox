#include "expr.h"
#include "token.h"

static void pretty_print_(CGS_Writer *sink, Expr *expr)
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
            cgs_appendf(sink, "%?", expr->value.v);
            break;
        case EXPR_var:
            cgs_appendf(sink, "%?", expr->ident);
            break;
        case EXPR_call:
            pretty_print_(sink, expr->call.left);
            cgs_append(sink, "(");
            for(c_each(it, ExprList, expr->call.args))
            {
                pretty_print_(sink, *it.ref);
                cgs_append(sink, ", ");
            }
            cgs_append(sink, ")");
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

static void rpn_print_(CGS_Writer *sink, Expr *expr)
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
            cgs_appendf(sink, "%?", expr->value.v);
            break;

        default:;
    }
}

void rpn_print(CGS_Writer *sink, Expr *expr)
{
    rpn_print_(sink, expr);
    cgs_putc(sink, '\n');
}