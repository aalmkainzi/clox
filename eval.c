#include "arena.h"
#include "clox.h"
#include "expr.h"
#include "token.h"
#include "var.h"
#include <float.h>
#include <malloc.h>
#include <math.h>
#include <stdlib.h>

#define T Arenas, Arena
#include "stc/list.h"

#define COMBINE_(a, b) a##b
#define COMBINE(a, b) COMBINE_(a, b)

#define NOPT(a, ...) COMBINE(NOPT_IMPL_, __VA_OPT__(0))(a)

#define NOPT_IMPL_0(...)
#define NOPT_IMPL_(...) __VA_ARGS__

// clang-format off
#define eval_or_throw(ctx, expr, ...)                                   \
    ({                                                                  \
        Value _v = eval_depends_(                                       \
            ctx,                                                        \
            expr,                                                       \
            __VA_OPT__((Value[]) {__VA_ARGS__})                         \
            NOPT(NULL, __VA_ARGS__),                                    \
            __VA_OPT__(sizeof((Value[]) {__VA_ARGS__}) / sizeof(Value)) \
            NOPT(0, __VA_ARGS__)                                        \
        );                                                              \
        if (_v.error)                                                   \
            return _v;                                                  \
        _v;                                                             \
    })
// clang-format on

Value eval_depends_(Ctx *ctx, Expr *expr, Value deps[], int n_deps);

CGS_Allocation eval_arena_alloc(CGS_Allocator *a, size_t alignment, size_t n);
void eval_arena_dealloc(CGS_Allocator *a, void *p, size_t n);
CGS_Allocation eval_arena_realloc(CGS_Allocator *a, void *p, size_t alignment, size_t old_n, size_t new_n);

static Arenas active_eval_arenas;
static CGS_Allocator *eval_arena_allocator = &(CGS_Allocator) {
    .alloc   = eval_arena_alloc,
    .dealloc = eval_arena_dealloc,
    .realloc = eval_arena_realloc,
};

Value eval_unary(Ctx *ctx, Expr *expr);
Value eval_binary(Ctx *ctx, Expr *expr);

static bool expect_number(Ctx *ctx, Value *obj);

bool is_truthy(Value obj);

Value copy_value(Value from);
Value error_value(Value *free_this);

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
            const VarMap_value *var = find_var(ctx->scope, expr->ident.lexeme);
            if (var == NULL)
            {
                report_tok_error(ctx, expr_tok(expr), "undefined variable '%?'", expr->ident.lexeme);
                return error_value(NULL);
            }
            else if (var->second.type == T_none)
            {
                report_tok_error(ctx, expr_tok(expr), "variable not initialized");
                return error_value(NULL);
            }
            Value var_val = copy_value(var->second);
            // free(var_val.tok);
            var_val.tok = expr->ident;
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

Value eval_unary(Ctx *ctx, Expr *expr)
{
    Value obj = eval_or_throw(ctx, expr->unary.expr);

    switch (expr->unary.op.kind)
    {
        case TK_minus:
        {
            if (!expect_number(ctx, &obj))
            {
                return error_value(&obj);
            }

            Value result = copy_value(obj);
            result.number *= -1;

            free_value(obj);
            return result;
        }
        break;
        case TK_not:
        {
            Value tmp = obj;
            tmp.type  = T_bool; // to avoid deep copy in copy_value if original type is T_str

            Value result   = copy_value(tmp);
            result.boolean = !is_truthy(obj);

            free_value(obj);
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
                Value right = eval_or_throw(ctx, expr->binary.right, left);

                result            = copy_value(right);
                result.tok.lexeme = range_view(left.tok.lexeme, right.tok.lexeme);
                free_value(right);
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
                Value right = eval_or_throw(ctx, expr->binary.right, left);

                result            = copy_value(right);
                result.tok.lexeme = range_view(left.tok.lexeme, right.tok.lexeme);
                free_value(right);
            }
        }
        break;

        default:
            unreachable();
            break;
    }

    free_value(left);
    return result;
}

Value eval_binary(Ctx *ctx, Expr *expr)
{
    if (expr->binary.op.kind == TK_and || expr->binary.op.kind == TK_or)
    {
        return eval_binary_logical(ctx, expr);
    }

    Value left  = eval_or_throw(ctx, expr->binary.left);
    Value right = eval_or_throw(ctx, expr->binary.right, left);

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
                report_tok_error(ctx, right.tok, "division by zero");
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
                free_value(result);
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
                free_value(result);
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
            Token var_name    = expr->binary.left->ident;
            VarMap_value *val = find_var(ctx->scope, var_name.lexeme);
            if (val && !right.error)
            {
                free_var_value(&val->second);
                val->second     = copy_value(right);
                val->second.var = true;

                free_var_value(&result);
                free_var_value(&left);
                result = val->second;
            }
            else
            {
                // no need to diagnose, `eval` did already
            }
        }
        break;

        default:
            result = error_value(&result);
            report_tok_error(
                ctx, expr->binary.op, "operator %? not yet implemented", tok2str[expr->binary.op.kind]
            );
    }

    free_value(left);
    free_value(right);
    return result;
}

static bool expect_number(Ctx *ctx, Value *obj)
{
    if (obj->type != T_num)
    {
        obj->error = true;
        report_tok_error(ctx, obj->tok, "expected number");
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

Value copy_value(Value from)
{
    Value ret = from;
    if (from.type == T_str)
        ret.string = cgs_dup(ret.string, eval_arena_allocator);
    ret.var = false;
    return ret;
}

void free_value(Value val)
{
    if (val.var)
        return;
    free_var_value(&val);
}

void free_var_value(Value *val)
{
    if (val->tok.lexeme.chars == NULL)
        return;
    // if (val->tok->seperate)
    //     free(val->tok);
    if (val->type == T_str)
        cgs_dstr_deinit(&val->string);

    *val = (Value) {};
}

Value error_value(Value *free_this)
{
    if (free_this)
        free_value(*free_this);
    return (Value) {.error = true};
}

Value eval_depends_(Ctx *ctx, Expr *expr, Value deps[], int n_deps)
{
    Value ret = eval(ctx, expr);
    if (ret.error)
    {
        for (int i = 0; i < n_deps; i++)
        {
            free_value(deps[i]);
        }
    }
    return ret;
}

Arena *begin_eval_arena()
{
    return Arenas_push(&active_eval_arenas, (Arena) {});
}

static void end_eval_arena_node(Arenas_iter it)
{
    if (it.ref == NULL)
        return;

    Arenas_iter next = it;
    Arenas_next(&next);

    end_eval_arena_node(next);
    arena_free(it.ref);
}

void end_eval_arena(Arena *arena)
{
    Arenas_iter it = {.ref = arena, ._last = &active_eval_arenas.last};
    end_eval_arena_node(it);
}

CGS_Allocation eval_arena_alloc(CGS_Allocator *a, size_t alignment, size_t n)
{
    (void)a;
    (void)alignment;

    Arena *arena = Arenas_back_mut(&active_eval_arenas);
    void *p      = arena_alloc(arena, n);
    return (CGS_Allocation) {.ptr = p, .n = n};
}

void eval_arena_dealloc(CGS_Allocator *a, void *p, size_t n)
{
    (void)a;
    (void)p;
    (void)n;
}

CGS_Allocation eval_arena_realloc(CGS_Allocator *a, void *p, size_t alignment, size_t old_n, size_t new_n)
{
    (void)p;
    (void)old_n;

    Arena *arena = Arenas_back_mut(&active_eval_arenas);

    return arena_realloc_callback(arena_allocator(arena), p, alignment, old_n, new_n);
}

void init_eval_c()
{
    Arenas_push(&active_eval_arenas, (Arena) {});
}

void deinit_eval_c()
{
    Arena *front = Arenas_front_mut(&active_eval_arenas);
    arena_free(front);
    Arenas_drop(&active_eval_arenas);
}
