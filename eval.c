#include "expr.h"
#include "clox.h"
#include <stdlib.h>

Literal eval_unary(Ctx *ctx, Expr *expr);
Literal eval_binary(Ctx *ctx, Expr *expr);

void expect_number(Ctx *ctx, Literal obj);
void expect_string(Ctx *ctx, Literal obj);
void expect_boolean(Ctx *ctx, Literal obj);

Literal eval(Ctx *ctx, Expr *expr)
{
    switch(expr->kind)
    {
        case EXPR_unary:
            return eval_unary(ctx, expr);
        break;
        
        case EXPR_binary:
            return eval_binary(ctx, expr);
        break;
        
        case EXPR_grouping:
            return eval(ctx, expr->grouping.expr);
        break;
        
        case EXPR_literal:
            return expr->literal;
        break;
        
        default:
            __builtin_unreachable();
    }
}

Literal eval_unary(Ctx *ctx, Expr *expr)
{
    Literal obj = eval(ctx, expr->unary.expr);
    
    switch(expr->unary.op->kind)
    {
        case TK_minus:
        {
            if(obj.kind != LIT_num)
            {
                error_at(expr_tok(expr)->lexeme, "expected number");
            }
            else
            {
                obj.tok = copy_token(ctx, obj.tok);
                obj.tok->data.dbl *= -1;
                return obj;
            }
        }
        break;
        case TK_not:
        {
            if(!(obj.kind == LIT_bool || obj.kind == LIT_nil))
            {
                error_at(expr_tok(expr)->lexeme, "expected boolean");
            }
            else
            {
                obj.tok = copy_token(ctx, obj.tok);
                obj.tok->data.boolean = !obj.tok->data.boolean;
                return obj;
            }
        }
        break;
        
        default:
            __builtin_unreachable();
    }
    
    return obj;
}

Literal eval_binary(Ctx *ctx, Expr *expr)
{
    Literal left = eval(ctx, expr->binary.left);
    Literal right = eval(ctx, expr->binary.right);
    
    switch(expr->binary.op->kind)
    {
        case TK_minus:
        {
            expect_number(ctx, left);
            expect_number(ctx, right);
            
            Literal res = left;
            res.tok = copy_token(ctx, res.tok);
            res.tok->data.dbl -= right.tok->data.dbl;
            
            return res;
        }
        break;
        
        case TK_plus:
        {
            if(left.kind == LIT_num)
            {
                expect_number(ctx, right);
                
                Literal res = left;
                res.tok = copy_token(ctx, res.tok);
                res.tok->data.dbl += right.tok->data.dbl;
                
                return res;
            }
            else if(left.kind == LIT_str)
            {
                expect_string(ctx, right);
                
                Literal res = left;
                res.tok = copy_token(ctx, res.tok);
                cgs_append(&res.tok->data.string, right.tok->data.string);
                
                return res;
            }
            else
            {
                error_at(left.tok->lexeme, "operands of '+' must be numbers or strings");
            }
        }
        break;
        
        case TK_star:
        {
            expect_number(ctx, left);
            expect_number(ctx, right);
            
            Literal res = left;
            res.tok = copy_token(ctx, res.tok);
            res.tok->data.dbl *= right.tok->data.dbl;
            
            return res;
        }
        break;
        
        case TK_slash:
        {
            expect_number(ctx, left);
            expect_number(ctx, right);
            
            Literal res = left;
            res.tok = copy_token(ctx, res.tok);
            res.tok->data.dbl /= right.tok->data.dbl;
            
            return res;
        }
        break;
        
        case TK_or:
        {
            expect_boolean(ctx, left);
            expect_boolean(ctx, right);
            
            Literal res = left;
            res.tok = copy_token(ctx, res.tok);
            res.tok->data.boolean = left.tok->data.boolean || right.tok->data.boolean;
            
            return res;
        }
        break;
        
        case TK_and:
        {
            expect_boolean(ctx, left);
            expect_boolean(ctx, right);
            
            Literal res = left;
            res.tok = copy_token(ctx, res.tok);
            res.tok->data.boolean = left.tok->data.boolean && right.tok->data.boolean;
            
            return res;
        }
        break;
        
        default:
            __builtin_unreachable();
    }
    
    return left;
}

void expect_number(Ctx *ctx, Literal obj)
{
    if(obj.kind != LIT_num)
    {
        error_at(obj.tok->lexeme, "expected number");
    }
}

void expect_string(Ctx *ctx, Literal obj)
{
    if(obj.kind != LIT_str)
    {
        error_at(obj.tok->lexeme, "expected string");
    }
}

void expect_boolean(Ctx *ctx, Literal obj)
{
    if(obj.kind != LIT_bool)
    {
        error_at(obj.tok->lexeme, "expected boolean");
    }
}

CGS_Error stringify_literal(CGS_Writer *writer, Literal lit)
{
    switch(lit.kind)
    {
        case LIT_str:
            return cgs_append(writer, lit.tok->data.string);
        break;
        case LIT_num:
            return cgs_append_tostr(writer, lit.tok->data.dbl);
        break;
        case LIT_bool:
            return cgs_append_tostr(writer, lit.tok->data.boolean);
        break;
        case LIT_nil:
            return cgs_append(writer, "nil");
        break;
        
        default:
            __builtin_unreachable();
    }
}