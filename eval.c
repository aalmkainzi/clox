#include "arena.h"
#include "clox.h"
#include "error.h"
#include "expr.h"
#include "token.h"
#include "var.h"
#include <float.h>
#include <malloc.h>
#include <math.h>
#include <stdlib.h>

#define COMBINE_(a, b) a##b
#define COMBINE(a, b) COMBINE_(a, b)

#define NOPT(a, ...) COMBINE(NOPT_IMPL_, __VA_OPT__(0))(a)

#define NOPT_IMPL_0(...)
#define NOPT_IMPL_(...) __VA_ARGS__

// clang-format off
#define eval_or_throw(ctx, expr)    \
    ({                              \
        Value _v = eval(ctx, expr); \
        if (_v.error)               \
            return _v;              \
        _v;                         \
    })
// clang-format on

static Value eval_unary(Ctx *ctx, Expr *expr);
static Value eval_binary(Ctx *ctx, Expr *expr);

static bool expect_number(Ctx *ctx, Value *obj);

static Value copy_value(Value from);
static Value error_value();

static CGS_Allocator *eval_arena_allocator = arena_allocator(0); //(CGS_Allocator*)&(ArenaAllocator){};

static VarMap_entry *find_var(Scope *scope, CGS_StrView name);

Value eval(Ctx *ctx, Expr *expr)
{
    switch (expr->kind)
    {
        case EXPR_unary:
            return eval_unary(ctx, expr);

        case EXPR_binary:
            return eval_binary(ctx, expr);

        case EXPR_value:
            return copy_value(expr->value);

        case EXPR_var:
        {
            const VarMap_value *var_entry = find_var(ctx->scope, expr->ident.lexeme);
            if (var_entry == NULL)
            {
                error_at_tok(ctx, expr_tok(expr), "undefined variable '%?'", expr->ident.lexeme);
                return error_value();
            }
            else if (var_entry->second.type == T_none)
            {
                error_at_tok(ctx, expr_tok(expr), "variable not initialized");
                return error_value();
            }
            Value var_val = copy_value(var_entry->second);
            var_val.tok   = expr->ident;
            assert(!var_val.var);
            return var_val;
        }
    }
    unreachable();
}

static VarMap_entry *find_var(Scope *scope, CGS_StrView name)
{
    VarMap *vars    = &scope->vars;
    VarMap_value *v = VarMap_get_mut(vars, name);
    if (v)
        return v;

    if (scope->up)
        return find_var(scope->up, name);

    return NULL;
}

static Value eval_unary(Ctx *ctx, Expr *expr)
{
    Value obj = eval_or_throw(ctx, expr->unary.expr);

    switch (expr->unary.op.kind)
    {
        case TK_minus:
        {
            if (!expect_number(ctx, &obj))
            {
                return error_value();
            }

            Value result = copy_value(obj);
            result.number *= -1;

            return result;
        }
        break;
        case TK_not:
        {
            Value tmp = obj;
            tmp.type  = T_bool; // to avoid deep copy in copy_value if original type is T_str

            Value result   = copy_value(tmp);
            result.boolean = !is_truthy(obj);

            return result;
        }
        break;

        default:
            unreachable();
    }
}

static Value eval_binary_logical(Ctx *ctx, Expr *expr)
{
    Value left = eval_or_throw(ctx, expr->binary.left);

    Value result;

    switch (expr->binary.op.kind)
    {
        case TK_or:
        {
            bool left_truthy = is_truthy(left);
            if (left_truthy)
            {
                result = copy_value(left);
            }
            else
            {
                Value right = eval_or_throw(ctx, expr->binary.right);

                result            = copy_value(right);
                result.tok.lexeme = range_view(left.tok.lexeme, right.tok.lexeme);
            }
        }
        break;

        case TK_and:
        {
            bool left_truthy = is_truthy(left);
            if (!left_truthy)
            {
                result = copy_value(left);
            }
            else
            {
                Value right = eval_or_throw(ctx, expr->binary.right);

                result            = copy_value(right);
                result.tok.lexeme = range_view(left.tok.lexeme, right.tok.lexeme);
            }
        }
        break;

        default:
            unreachable();
            break;
    }

    return result;
}

static Value eval_binary(Ctx *ctx, Expr *expr)
{
    if (expr->binary.op.kind == TK_and || expr->binary.op.kind == TK_or)
    {
        return eval_binary_logical(ctx, expr);
    }

    Value left  = eval_or_throw(ctx, expr->binary.left);
    Value right = eval_or_throw(ctx, expr->binary.right);

    Value result = copy_value(left);
    if (left.tok.lexeme.chars && right.tok.lexeme.chars)
        result.tok.lexeme = range_view(left.tok.lexeme, right.tok.lexeme);

    switch (expr->binary.op.kind)
    {
        case TK_minus:
        {
            if (!expect_number(ctx, &left) || !expect_number(ctx, &right))
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
                    case T_none:
                        unreachable();
                }
            }
            else if (right.type == T_str)
            {
                result.type = T_str;
                switch (left.type)
                {
                    case T_bool:
                        result.string = cgs_asprintf(eval_arena_allocator, "%?%?", left.boolean, right.string);
                        break;
                    case T_num:
                        result.string = cgs_asprintf(eval_arena_allocator, "%?%?", left.number, right.string);
                        break;
                    case T_str:
                        // not actually possible, previous branch wouldve caught it
                        result.string = cgs_asprintf(eval_arena_allocator, "%?%?", left.string, right.string);
                        break;
                    case T_nil:
                        result.string = cgs_asprintf(eval_arena_allocator, "%?%?", "nil", right.string);
                        break;
                    case T_none:
                        unreachable();
                }
            }
            else
            {
                if (!expect_number(ctx, &right) || !expect_number(ctx, &left))
                    break;

                result.tok.lexeme = range_view(left.tok.lexeme, right.tok.lexeme);
                result.number += right.number;
            }
        }
        break;

        case TK_star:
        {
            if (!expect_number(ctx, &left) || !expect_number(ctx, &right))
                break;

            result.number *= right.number;
        }
        break;

        case TK_slash:
        {
            if (!expect_number(ctx, &left) || !expect_number(ctx, &right))
                break;

            if (right.number == 0)
            {
                error_at_tok(ctx, right.tok, "division by zero");
                result.number = INFINITY;
            }
            else
            {
                result.number /= right.number;
            }
        }
        break;

        case TK_gt:
        {
            if (!expect_number(ctx, &left) || !expect_number(ctx, &right))
                break;

            result.type    = T_bool;
            result.boolean = left.number > right.number;
        }
        break;

        case TK_lt:
        {
            if (!expect_number(ctx, &left) || !expect_number(ctx, &right))
                break;

            result.type    = T_bool;
            result.boolean = left.number < right.number;
        }
        break;

        case TK_gte:
        {
            if (!expect_number(ctx, &left) || !expect_number(ctx, &right))
                break;

            result.type    = T_bool;
            result.boolean = left.number >= right.number;
        }
        break;

        case TK_lte:
        {
            if (!expect_number(ctx, &left) || !expect_number(ctx, &right))
                break;

            result.type    = T_bool;
            result.boolean = left.number <= right.number;
        }
        break;

        case TK_equal_equal:
        {
            if (left.type == T_str)
            {
                Value str_equality_result = {
                    .tok     = result.tok,
                    .type    = T_bool,
                    .boolean = right.type == T_str && cgs_equal(left.string, right.string)
                };
                result = str_equality_result;
            }
            else if (left.type == T_num)
            {
                result.type    = T_bool;
                result.boolean = right.type == T_num && (left.number == right.number);
            }
            else if (left.type == T_nil)
            {
                result.type    = T_bool;
                result.boolean = right.type == T_nil;
            }
            else // bool
            {
                result.type    = T_bool;
                result.boolean = right.type == T_bool && (left.boolean == right.boolean);
            }
        }
        break;

        case TK_not_equal:
        {
            if (left.type == T_str)
            {
                Value str_equality_result = {
                    .tok     = result.tok,
                    .type    = T_bool,
                    .boolean = right.type != T_str || !cgs_equal(left.string, right.string)
                };
                result = str_equality_result;
            }
            else if (left.type == T_num)
            {
                result.type    = T_bool;
                result.boolean = right.type != T_num || (left.number != right.number);
            }
            else if (left.type == T_nil)
            {
                result.type    = T_bool;
                result.boolean = right.type != T_nil;
            }
            else // bool
            {
                result.type    = T_bool;
                result.boolean = right.type != T_bool || (left.boolean != right.boolean);
            }
        }
        break;

        case TK_equal:
        {
            Token var_name          = expr->binary.left->ident;
            VarMap_value *var_entry = find_var(ctx->scope, var_name.lexeme);
            if (var_entry && !right.error)
            {
                free_var_value(var_entry->second);
                Value var_val     = var_value(right);
                var_entry->second = var_val;
                result            = var_entry->second;
            }
            else
            {
                // no need to diagnose, `eval` did already
            }
        }
        break;

        default:
            result = error_value();
            error_at_tok(ctx, expr->binary.op, "operator %? not yet implemented", tok2str[expr->binary.op.kind]);
    }

    return result;
}

static bool expect_number(Ctx *ctx, Value *obj)
{
    if (obj->type != T_num)
    {
        obj->error = true;
        error_at_tok(ctx, obj->tok, "expected a number");
        return false;
    }
    return true;
}

bool is_truthy(Value obj)
{
    if (obj.type == T_bool)
    {
        return obj.boolean;
    }
    if (obj.type == T_nil)
    {
        return false;
    }
    return true;
}

CGS_Error stringify_value(CGS_Writer *writer, Value val, CGS_StrView fmt_arg)
{
    (void)fmt_arg;
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
            unreachable();
    }
}

CGS_Error stringify_literal(CGS_Writer *writer, Literal lit, CGS_StrView fmt_arg)
{
    (void)fmt_arg;
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
            unreachable();
    }
}

void free_var_value(Value v)
{
    assert(v.var);
    if (v.type == T_str)
    {
        cgs_dstr_deinit(&v.string);
    }
}

static Value copy_value(Value from)
{
    Value ret = from;
    if (from.type == T_str)
        ret.string = cgs_dup(ret.string, eval_arena_allocator);
    ret.var = false;
    return ret;
}

Value var_value(Value val)
{
    val.var = true;
    if (val.type == T_str)
    {
        Value v  = val;
        v.string = cgs_dstr_init_from(val.string);
        return v;
    }
    return val;
}

static Value error_value()
{
    return (Value) {.error = true};
}

// TODO maybe a better is if Ctx has the allocators, more powerful than holding the Arena (the Arena can be in the
// stack frame in main.c)
void init_eval_system(Ctx *ctx)
{
    ((ArenaAllocator *)eval_arena_allocator)->arena = &ctx->eval_arena;
}
