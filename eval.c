#include "arena.h"
#include "clox.h"
#include "error.h"
#include "expr.h"
#include "stmt.h"
#include "token.h"
#include "var.h"
#include <float.h>
#include <malloc.h>
#include <math.h>
#include <stdlib.h>

#define eval_or_throw(val_ptr, expr) \
    do                               \
    {                                \
        Value _v = eval(ctx, expr);  \
        if (_v.error)                \
            return _v;               \
        *val_ptr = _v;               \
    } while (0)

static Value eval_unary(Ctx *ctx, Expr *expr);
static Value eval_binary(Ctx *ctx, Expr *expr);
static Value eval_call(Ctx *ctx, Expr *expr);

static bool expect_number(Ctx *ctx, Value *obj);
static bool expect_string(Ctx *ctx, Value *obj);
static bool expect_function(Ctx *ctx, Value *obj);

static Value copy_value(Ctx *ctx, Value from);
static Value error_value();

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
            return copy_value(ctx, expr->value);

        case EXPR_var:
        {
            const VarMap_value *var_entry = find_var(ctx->scope, expr->ident.lexeme);
            if (var_entry == NULL)
            {
                error_at_tok(ctx, expr_tok(expr), "undefined variable '%?'", expr->ident.lexeme);
                return error_value();
            }
            else if (var_entry->second.type.kind == T_none)
            {
                error_at_tok(ctx, expr_tok(expr), "variable not initialized");
                return error_value();
            }
            Value var_val = copy_value(ctx, var_entry->second);
            var_val.tok   = expr->ident;
            assert(!var_val.var);
            return var_val;
        }

        case EXPR_call:
            return eval_call(ctx, expr);
        default:
            unreachable();
    }
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

static Value eval_call(Ctx *ctx, Expr *expr)
{
    Value func_val;
    eval_or_throw(&func_val, expr->call.left);

    if (!expect_function(ctx, &func_val))
        return error_value();

    if (func_val.type.function.n_args != expr->call.args.size)
    {
        error_at(
            ctx,
            expr_view(expr),
            "expected %? arguments, got %?",
            func_val.type.function.n_args,
            expr->call.args.size
        );
        return error_value();
    }

    if (func_val.function.native)
    {
        return func_val.function.native_call(ctx, &expr->call.args);
    }
    else
    {
        enter_scope(ctx);

        ArenaCheckpoint checkpoint = arena_checkpoint(ctx->eval_arena);

        StrViewList_iter name_it = StrViewList_begin(&func_val.function.args);
        for (c_each(arg_it, ExprList, expr->call.args))
        {
            Value arg_val;
            eval_or_throw(&arg_val, *arg_it.ref);
            arg_val = var_value(arg_val);

            // pushing the arg value mapped to the arg name to the scope
            VarMap_put(&ctx->scope->vars, *name_it.ref, arg_val);
            StrViewList_next(&name_it);
        }

        // free all the intermediate values evaluated when going through each arg
        arena_rollback(ctx->eval_arena, checkpoint);

        ExecuteStmtResult res = execute_stmt_list(ctx, func_val.function.body);

        Value return_value = {.type.kind = T_nil};

        // if the function returns, move that return value into heap,
        // exit_scope to rollback arena, then put it back in the current scope's arena
        if (res.exit_kind == EXIT_RETURN)
        {
            res.value = var_value(res.value);
            exit_scope(ctx);
            return_value = copy_value(ctx, res.value);
            free_var_value(res.value);
        }
        else
        {
            exit_scope(ctx);
        }

        return return_value;
    }
}

static Value eval_unary(Ctx *ctx, Expr *expr)
{
    Value obj;
    eval_or_throw(&obj, expr->unary.expr);

    switch (expr->unary.op.kind)
    {
        case TK_minus:
        {
            if (!expect_number(ctx, &obj))
            {
                return error_value();
            }

            Value result = copy_value(ctx, obj);
            result.number *= -1;

            return result;
        }
        break;
        case TK_not:
        {
            Value tmp     = obj;
            tmp.type.kind = T_bool; // to avoid deep copy in copy_value if original type is T_str

            Value result   = copy_value(ctx, tmp);
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
    Value left;
    eval_or_throw(&left, expr->binary.left);

    Value result;

    switch (expr->binary.op.kind)
    {
        case TK_or:
        {
            bool left_truthy = is_truthy(left);
            if (left_truthy)
            {
                result = copy_value(ctx, left);
            }
            else
            {
                Value right;
                eval_or_throw(&right, expr->binary.right);

                result            = copy_value(ctx, right);
                result.tok.lexeme = range_view(left.tok.lexeme, right.tok.lexeme);
            }
        }
        break;

        case TK_and:
        {
            bool left_truthy = is_truthy(left);
            if (!left_truthy)
            {
                result = copy_value(ctx, left);
            }
            else
            {
                Value right;
                eval_or_throw(&right, expr->binary.right);

                result            = copy_value(ctx, right);
                result.tok.lexeme = range_view(left.tok.lexeme, right.tok.lexeme);
            }
        }
        break;

        default:
            unreachable();
    }

    return result;
}

static Value eval_binary(Ctx *ctx, Expr *expr)
{
    if (expr->binary.op.kind == TK_and || expr->binary.op.kind == TK_or)
    {
        return eval_binary_logical(ctx, expr);
    }

    Value left;
    Value right;
    eval_or_throw(&left, expr->binary.left);
    eval_or_throw(&right, expr->binary.right);

    Value result = copy_value(ctx, left);
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
            if (left.type.kind == T_str)
            {
                if (ctx->opts.ext)
                {
                    switch (right.type.kind)
                    {
                        case T_bool:
                            cgs_append_tostr(&result.string, right.boolean);
                            break;
                        case T_num:
                            cgs_append_tostr(&result.string, right.number);
                            break;
                        case T_str:
                            cgs_append(&result.string, right.string);
                            break;
                        case T_nil:
                            cgs_append_tostr(&result.string, "nil");
                            break;
                        default:
                            error_at_tok(ctx, left.tok, "type cannot be added to string");
                            return error_value();
                        case T_none:
                            unreachable();
                    }
                }
                else
                {
                    if (!expect_string(ctx, &right))
                        break;
                    cgs_append(&result.string, right.string);
                }
            }
            else if (ctx->opts.ext && right.type.kind == T_str)
            {
                result.type.kind = T_str;
                switch (left.type.kind)
                {
                    case T_bool:
                        result.string = cgs_asprintf(ctx->eval_allocator, "%?%?", left.boolean, right.string);
                        break;
                    case T_num:
                        result.string = cgs_asprintf(ctx->eval_allocator, "%?%?", left.number, right.string);
                        break;
                    case T_str:
                        // not actually possible, previous branch wouldve caught it
                        result.string = cgs_asprintf(ctx->eval_allocator, "%?%?", left.string, right.string);
                        break;
                    case T_nil:
                        result.string = cgs_asprintf(ctx->eval_allocator, "%?%?", "nil", right.string);
                        break;
                    default:
                        error_at_tok(ctx, left.tok, "type cannot be added to string");
                        return error_value();
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

            result.type.kind = T_bool;
            result.boolean   = left.number > right.number;
        }
        break;

        case TK_lt:
        {
            if (!expect_number(ctx, &left) || !expect_number(ctx, &right))
                break;

            result.type.kind = T_bool;
            result.boolean   = left.number < right.number;
        }
        break;

        case TK_gte:
        {
            if (!expect_number(ctx, &left) || !expect_number(ctx, &right))
                break;

            result.type.kind = T_bool;
            result.boolean   = left.number >= right.number;
        }
        break;

        case TK_lte:
        {
            if (!expect_number(ctx, &left) || !expect_number(ctx, &right))
                break;

            result.type.kind = T_bool;
            result.boolean   = left.number <= right.number;
        }
        break;

        case TK_equal_equal:
        {
            if (left.type.kind == T_str)
            {
                Value str_equality_result = {
                    .tok       = result.tok,
                    .type.kind = T_bool,
                    .boolean   = right.type.kind == T_str && cgs_equal(left.string, right.string)
                };
                result = str_equality_result;
            }
            else if (left.type.kind == T_num)
            {
                result.type.kind = T_bool;
                result.boolean   = right.type.kind == T_num && (left.number == right.number);
            }
            else if (left.type.kind == T_nil)
            {
                result.type.kind = T_bool;
                result.boolean   = right.type.kind == T_nil;
            }
            else // bool
            {
                result.type.kind = T_bool;
                result.boolean   = right.type.kind == T_bool && (left.boolean == right.boolean);
            }
        }
        break;

        case TK_not_equal:
        {
            if (left.type.kind == T_str)
            {
                Value str_equality_result = {
                    .tok       = result.tok,
                    .type.kind = T_bool,
                    .boolean   = right.type.kind != T_str || !cgs_equal(left.string, right.string)
                };
                result = str_equality_result;
            }
            else if (left.type.kind == T_num)
            {
                result.type.kind = T_bool;
                result.boolean   = right.type.kind != T_num || (left.number != right.number);
            }
            else if (left.type.kind == T_nil)
            {
                result.type.kind = T_bool;
                result.boolean   = right.type.kind != T_nil;
            }
            else // bool
            {
                result.type.kind = T_bool;
                result.boolean   = right.type.kind != T_bool || (left.boolean != right.boolean);
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
    if (obj->type.kind != T_num)
    {
        obj->error = true;
        error_at_tok(ctx, obj->tok, "expected a number");
        return false;
    }
    return true;
}

static bool expect_string(Ctx *ctx, Value *obj)
{
    if (obj->type.kind != T_str)
    {
        obj->error = true;
        error_at_tok(ctx, obj->tok, "expected a string");
        return false;
    }
    return true;
}

static bool expect_function(Ctx *ctx, Value *obj)
{
    if (obj->type.kind != T_function)
    {
        obj->error = true;
        error_at_tok(ctx, obj->tok, "expected a function");
        return false;
    }
    return true;
}

bool is_truthy(Value obj)
{
    if (obj.type.kind == T_bool)
    {
        return obj.boolean;
    }
    if (obj.type.kind == T_nil)
    {
        return false;
    }
    return true;
}

CGS_Error stringify_value(CGS_Writer *writer, Value val, CGS_StrView fmt_arg)
{
    (void)fmt_arg;
    switch (val.type.kind)
    {
        case T_str:
            return cgs_append(writer, val.string);
        case T_num:
            return cgs_append_tostr(writer, val.number);
        case T_bool:
            return cgs_append_tostr(writer, val.boolean);
        case T_function:
            return cgs_appendf(writer, "<fn %?>", val.function.name);
        case T_nil:
            return cgs_append(writer, "nil");
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
        case LIT_num:
            return cgs_append_tostr(writer, lit.tok->data.dbl);
        case LIT_bool:
            return cgs_append_tostr(writer, lit.tok->data.boolean);
        case LIT_nil:
            return cgs_append(writer, "nil");
        default:
            unreachable();
    }
}

void free_var_value(Value v)
{
    assert(v.var);
    if (v.type.kind == T_str)
    {
        cgs_dstr_deinit(&v.string);
    }
}

static Value copy_value(Ctx *ctx, Value from)
{
    Value ret = from;
    if (from.type.kind == T_str)
        ret.string = cgs_dup(ret.string, ctx->eval_allocator);
    ret.var = false;
    return ret;
}

Value var_value(Value val)
{
    val.var = true;
    if (val.type.kind == T_str)
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
