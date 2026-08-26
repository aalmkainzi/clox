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
Value copy_value(Ctx *ctx, Value from);

Value eval(Ctx *ctx, Expr *expr)
{
    switch (expr->kind)
    {
    case EXPR_unary:
        return eval_unary(ctx, expr);

    case EXPR_binary:
        return eval_binary(ctx, expr);

    case EXPR_literal:
        return eval_literal(ctx, expr);

    default:
        __builtin_unreachable();
    }
}

Value eval_literal(Ctx *ctx, Expr *expr)
{
    TypeKind map[] = {
        [LIT_bool] = T_bool,
        [LIT_num]  = T_num,
        [LIT_str]  = T_str,
        [LIT_nil]  = T_nil,
    };

    return (Value) {.type = map[expr->literal.kind], .original = expr->literal.tok};
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
            Value result = copy_value(ctx, obj);
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
            Value result   = copy_value(ctx, obj);
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

    Value result            = copy_value(ctx, left);
    result.original->lexeme = range_view(left.original->lexeme, right.original->lexeme);

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
        if (left.type == T_num)
        {
            if (!expect_number(ctx, right))
                break;

            result.original->lexeme = range_view(left.original->lexeme, right.original->lexeme);
            result.number += right.number;
        }
        else if (left.type == T_str)
        {
            expect_string(ctx, right);

            cgs_append(&result.string, right.string);
        }
        else
        {
            report_tok_error(ctx, left.original, "operands of '+' must be numbers or strings");
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

        result.type              = T_bool;
        result.boolean = left.number >= right.number;
    }
    break;

    case TK_lte:
    {
        if (!expect_number(ctx, left) || !expect_number(ctx, right))
            break;

        result.type              = T_bool;
        result.boolean = left.number <= right.number;
    }
    break;

    case TK_equal_equal:
    {
        if (!expect_number(ctx, left) || !expect_number(ctx, right))
            break;

        result.type              = T_bool;
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
        report_tok_error(ctx, obj.original, "expected number");
        return false;
    }
    return true;
}

bool expect_string(Ctx *ctx, Value obj)
{
    if (obj.type != T_str)
    {
        report_tok_error(ctx, obj.original, "expected string");
        return false;
    }
    return true;
}

bool expect_boolean(Ctx *ctx, Value obj)
{
    if (obj.type != T_bool)
    {
        report_tok_error(ctx, obj.original, "expected boolean");
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

static Token *new_eval_token(Ctx *ctx, Token *tok)
{
    Token *new_tok = copy_token(ctx, tok);
    new_tok->eval  = true;
    return new_tok;
}

Value copy_value(Ctx *ctx, Value from)
{
    Value ret = from;
    ret.original = new_eval_token(ctx, from.original);
    return ret;
}

void free_value(Value val)
{
    if (val.original->eval)
        free_token(val.original);
}
