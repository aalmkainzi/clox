#ifndef EXPR_H
#define EXPR_H

#include "arena.h"
#include "cgs/cgs.h"
#include "token.h"

typedef enum ExprKind : unsigned char
{
    EXPR_unary,
    EXPR_binary,
    EXPR_value,
    EXPR_var
} ExprKind;

typedef enum LiteralKind : unsigned char
{
    LIT_bool,
    LIT_num,
    LIT_str,
    LIT_nil,
} LiteralKind;

typedef struct Literal
{
    LiteralKind kind;
    Token *tok;
} Literal;

typedef enum TypeKind : unsigned char
{
    T_none = 0,
    T_bool,
    T_num,
    T_str,
    T_nil,
} TypeKind;

typedef struct Value
{
    TypeKind type;
    Token tok;
    bool var;
    bool error;

    // only not null in case of `.var = true`, and it represents the scope in which the var is declared
    // struct Scope *scope;

    union
    {
        CGS_DStr string;
        double number;
        bool boolean;
    };
} Value;

typedef struct Expr
{
    ExprKind kind;

    union
    {
        struct
        {
            Token op;
            struct Expr *expr;
        } unary;

        struct
        {
            Token op;
            struct Expr *left;
            struct Expr *right;
        } binary;

        Value value;

        Token ident;
    };
} Expr;

void pretty_print(CGS_Writer *sink, Expr *expr);
void rpn_print(CGS_Writer *sink, Expr *expr);

struct Ctx;

Expr *new_binary(struct Ctx *ctx, Expr *left, Expr *right, Token op);
Expr *new_unary(struct Ctx *ctx, Expr *left, Token op);
Expr *new_grouping(struct Ctx *ctx, Expr *inside);
Expr *new_literal(struct Ctx *ctx, Token tok);
Expr *new_var(struct Ctx *ctx, Token tok);
Token expr_tok(Expr *expr);
CGS_StrView expr_view(Expr *expr);

Value eval(struct Ctx *ctx, Expr *expr);
bool is_truthy(Value obj);

void free_var_value(Value v);

// the returned Value's lifetime doesn't end with current scope, but an explicit free_var_value call
Value var_value(Value val);

#define scoped_eval_(id, name, expr)                                                                              \
    for (bool c_JOIN(b1_, id) = true, c_JOIN(b2_, id) = true; c_JOIN(b1_, id) && c_JOIN(b2_, id);)                \
        for (ArenaCheckpoint c_JOIN(eval_lifetime_cp_, id) = arena_checkpoint(&ctx->eval_arena); c_JOIN(b2_, id); \
             arena_rollback(&ctx->eval_arena, c_JOIN(eval_lifetime_cp_, id)), c_JOIN(b2_, id) = false)            \
            for (name = eval(ctx, expr); c_JOIN(b1_, id); c_JOIN(b1_, id) = false)

// checkpoint is created before the next statement, and restore after it
#define scoped_eval(name, expr) scoped_eval_(__COUNTER__, name, expr)

CGS_Error stringify_value(CGS_Writer *writer, Value val, CGS_StrView fmt_arg);
CGS_Error stringify_literal(CGS_Writer *writer, Literal lit, CGS_StrView fmt_arg);

#define ADD_TOSTR (Value, stringify_value)
#include "cgs/cgs.h"
#define ADD_TOSTR (Literal, stringify_literal)
#include "cgs/cgs.h"

#endif
