#include "expr.h"
#include "token.h"

void pretty_print(CGS_Writer *sink, Expr *expr)
{
    if(expr->kind != EXPR_LITERAL)
        cgs_append(sink, "(");
    
    switch(expr->kind)
    {
        case EXPR_UNARY:
            cgs_appendf(sink, "%? ", expr->unary.op->lexeme);
            pretty_print(sink, expr->unary.expr);
        break;
        case EXPR_BINARY:
            cgs_appendf(sink, "%? ", expr->binary.op->lexeme);
            pretty_print(sink, expr->binary.left);
            cgs_putc(sink, ' ');
            pretty_print(sink, expr->binary.right);
        break;
        case EXPR_TERNARY:
            cgs_append(sink, "?: ");
            pretty_print(sink, expr->ternary.cond);
            cgs_putc(sink, ' ');
            pretty_print(sink, expr->ternary.true_exp);
            cgs_putc(sink, ' ');
            pretty_print(sink, expr->ternary.false_exp);
        break;
        case EXPR_LITERAL:
            cgs_appendf(sink, "%?", expr->literal.tok);
        break;
        case EXPR_GROUPING:
            cgs_append(sink, "group ");
            pretty_print(sink, expr->grouping.expr);
        break;
    }
    
    if(expr->kind != EXPR_LITERAL)
        cgs_append(sink, ")");
}

void rpn_print(CGS_Writer *sink, Expr *expr)
{
    switch(expr->kind)
    {
        case EXPR_UNARY:
            rpn_print(sink, expr->unary.expr);
            cgs_appendf(sink, " %?", expr->unary.op->lexeme);
        break;
        case EXPR_BINARY:
            rpn_print(sink, expr->binary.left);
            cgs_append(sink, " ");
            rpn_print(sink, expr->binary.right);
            cgs_appendf(sink, " %?", expr->binary.op->lexeme);
        break;
        case EXPR_TERNARY:
            rpn_print(sink, expr->ternary.cond);
            cgs_putc(sink, ' ');
            rpn_print(sink, expr->ternary.true_exp);
            cgs_putc(sink, ' ');
            rpn_print(sink, expr->ternary.false_exp);
            cgs_append(sink, " ?:");
        break;
        case EXPR_LITERAL:
            cgs_appendf(sink, "%?", expr->literal.tok);
        break;
        case EXPR_GROUPING:
            rpn_print(sink, expr->grouping.expr);
        break;
    }
}
