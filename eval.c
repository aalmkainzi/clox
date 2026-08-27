#include "clox.h"
#include "expr.h"
#include "token.h"
#include <stdlib.h>

Value eval_literal(Ctx *ctx, Expr *expr);
Value eval_unary(Ctx *ctx, Expr *expr);
Value eval_binary(Ctx *ctx, Expr *expr);

bool expect_number(Ctx *ctx, Value obj);
bool expect_string(Ctx *ctx, Value obj);
bool expect_boolean(Ctx *ctx, Value obj);
Value copy_value(Value from);

Value eval(Ctx *ctx, Expr *expr)
{
    switch (expr->kind)
    {
    case EXPR_unary:
        return eval_unary(ctx, expr);

    case EXPR_binary:
        return eval_binary(ctx, expr);

    case EXPR_value:
        return expr->value;

    default:
        __builtin_unreachable();
    }
}

Value eval_unary(Ctx *ctx, Expr *expr)
{
    Value obj = eval(ctx, expr->unary.expr);

    switch (expr->unary.op->kind)
    {
    case TK_minus:
    {
        if (obj.type != T_num)
        {
            report_tok_error(ctx, expr_tok(expr->unary.expr), "expected number");
        }
        else
        {
            Value result = copy_value(obj);
            result.number *= -1;

            free_value(obj);
            return result;
        }
    }
    break;
    case TK_not:
    {
        if (!(obj.type == T_bool || obj.type == T_nil))
        {
            report_tok_error(ctx, expr_tok(expr->unary.expr), "expected boolean");
        }
        else
        {
            Value result   = copy_value(obj);
            result.boolean = !result.boolean;

            free_value(obj);
            return result;
        }
    }
    break;

    default:
        __builtin_unreachable();
    }

    return obj;
}

Value eval_binary(Ctx *ctx, Expr *expr)
{
    Value left  = eval(ctx, expr->binary.left);
    Value right = eval(ctx, expr->binary.right);

    Value result       = copy_value(left);
    result.tok->lexeme = range_view(left.tok->lexeme, right.tok->lexeme);

    switch (expr->binary.op->kind)
    {
    case TK_minus:
    {
        if (!expect_number(ctx, left) || !expect_number(ctx, right))
            break;

        result.number -= right.number;
    }
    break;

    case TK_plus:
    {
        if (left.type == T_str)
        {
            switch (right.type)
            {
            case T_bool:
                cgs_append_tostr(&result.string, right.boolean);
                break;
            case T_num:
                cgs_append_tostr(&result.string, right.number);
                break;
            case T_str:
                cgs_append_tostr(&result.string, right.string);
                break;
            case T_nil:
                cgs_append_tostr(&result.string, "nil");
                break;
            }
        }
        else if (right.type == T_str)
        {
            result.type = T_str;
            switch (left.type)
            {
            case T_bool:
                result.string = cgs_asprintf("%?%?", left.boolean, right.string);
                break;
            case T_num:
                result.string = cgs_asprintf("%?%?", left.number, right.string);
                break;
            case T_str:
                // not actually possible, previous branch wouldve caught it
                result.string = cgs_asprintf("%?%?", left.string, right.string);
                break;
            case T_nil:
                result.string = cgs_asprintf("%?%?", "nil", right.string);
                break;
            }
        }
        else
        {
            if (!expect_number(ctx, right) || !expect_number(ctx, left))
                break;

            result.tok->lexeme = range_view(left.tok->lexeme, right.tok->lexeme);
            result.number += right.number;
        }
    }
    break;

    case TK_star:
    {
        if (!expect_number(ctx, left) || !expect_number(ctx, right))
            break;

        result.number *= right.number;
    }
    break;

    case TK_slash:
    {
        if (!expect_number(ctx, left) || !expect_number(ctx, right))
            break;

        result.number /= right.number;
    }
    break;

    case TK_or:
    {
        if (!expect_boolean(ctx, left) || !expect_boolean(ctx, right))
            break;

        result.boolean = left.boolean || right.boolean;
    }
    break;

    case TK_and:
    {
        if (!expect_boolean(ctx, left) || !expect_boolean(ctx, right))
            break;

        result.boolean = left.boolean && right.boolean;
    }
    break;

    case TK_gt:
    {
        if (!expect_number(ctx, left) || !expect_number(ctx, right))
            break;

        result.type    = T_bool;
        result.boolean = left.number > right.number;
    }
    break;

    case TK_lt:
    {
        if (!expect_number(ctx, left) || !expect_number(ctx, right))
            break;

        result.type    = T_bool;
        result.boolean = left.number < right.number;
    }
    break;

    case TK_gte:
    {
        if (!expect_number(ctx, left) || !expect_number(ctx, right))
            break;

        result.type    = T_bool;
        result.boolean = left.number >= right.number;
    }
    break;

    case TK_lte:
    {
        if (!expect_number(ctx, left) || !expect_number(ctx, right))
            break;

        result.type    = T_bool;
        result.boolean = left.number <= right.number;
    }
    break;

    case TK_equal_equal:
    {
        if (!expect_number(ctx, left) || !expect_number(ctx, right))
            break;

        result.type    = T_bool;
        result.boolean = left.number == right.number;
    }
    break;

    default:
        report_tok_error(ctx, expr->binary.op, "operator %? not yet implemented", tok2str[expr->binary.op->kind]);
    }

    free_value(left);
    free_value(right);
    return result;
}

bool expect_number(Ctx *ctx, Value obj)
{
    if (obj.type != T_num)
    {
        report_tok_error(ctx, obj.tok, "expected number");
        return false;
    }
    return true;
}

bool expect_string(Ctx *ctx, Value obj)
{
    if (obj.type != T_str)
    {
        report_tok_error(ctx, obj.tok, "expected string");
        return false;
    }
    return true;
}

bool expect_boolean(Ctx *ctx, Value obj)
{
    if (obj.type != T_bool)
    {
        report_tok_error(ctx, obj.tok, "expected boolean");
        return false;
    }
    return true;
}

CGS_Error stringify_value(CGS_Writer *writer, Value val)
{
    switch (val.type)
    {
    case T_str:
        return cgs_append(writer, val.string);
        break;
    case T_num:
        return cgs_append_tostr(writer, val.number);
        break;
    case T_bool:
        return cgs_append_tostr(writer, val.boolean);
        break;
    case T_nil:
        return cgs_append(writer, "nil");
        break;

    default:
        __builtin_unreachable();
    }
}

CGS_Error stringify_literal(CGS_Writer *writer, Literal lit)
{
    switch (lit.kind)
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

static Token *new_eval_token(Token *tok)
{
    Token *new_tok    = copy_token(tok);
    new_tok->seperate = true;
    return new_tok;
}

Value copy_value(Value from)
{
    Value ret = from;
    ret.tok   = new_eval_token(from.tok);
    return ret;
}

void free_value(Value val)
{
    if (val.tok->seperate)
        free_token(val.tok);
}
