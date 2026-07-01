#include "expr.h"
#include "clox.h"
#include <stdlib.h>

double eval(Expr *expr)
{
    switch(expr->kind)
    {
        case EXPR_unary:
            switch(expr->unary.op->kind)
            {
                case TK_minus:
                    return -eval(expr->unary.expr);
                break;
                case TK_not:
                    return !eval(expr->unary.expr);
                break;
                
                default:
                    __builtin_unreachable();
            }
        break;
        
        case EXPR_binary:
            switch(expr->binary.op->kind)
            {
                case TK_minus:
                    return eval(expr->binary.left) - eval(expr->binary.right);
                case TK_plus:
                    return eval(expr->binary.left) + eval(expr->binary.right);
                case TK_star:
                    return eval(expr->binary.left) * eval(expr->binary.right);
                case TK_slash:
                    return eval(expr->binary.left) / eval(expr->binary.right);
                default:
                    __builtin_unreachable();
            }
        break;
        
        case EXPR_grouping:
            return eval(expr->grouping.expr);
        break;
        
        case EXPR_literal:
            assert(expr->literal.kind == LIT_num);
            if(expr->literal.tok->kind == TK_float)
            {
                return expr->literal.tok->data.dbl;
            }
            else if(expr->literal.tok->kind == TK_num)
            {
                return expr->literal.tok->data.integer;
            }
            else
            {
                __builtin_unreachable();
            }
        break;
        
        default: __builtin_unreachable();
    }
}