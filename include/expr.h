#ifndef EXPR_H
#define EXPR_H

#include "arena.h"
#include "cgs/cgs.h"
#include "token.h"

#define T StrViewList, CGS_StrView
#define i_header
#include "grow_list.h"

typedef enum ExprKind : unsigned char
{
    EXPR_unary,
    EXPR_binary,
    EXPR_value,
    EXPR_var,
    EXPR_call
} ExprKind;

typedef enum LiteralKind : unsigned char
{
    LIT_nil = 1,
    LIT_bool,
    LIT_num,
    LIT_str,
} LiteralKind;

typedef struct Literal
{
    LiteralKind kind;
    Token *tok;
} Literal;

typedef enum TypeKind : unsigned char
{
    T_none = 0,
    T_nil,
    T_bool,
    T_num,
    T_str,
    T_function,
} TypeKind;

typedef struct Type
{
    TypeKind kind;

    union
    {
        struct
        {
            unsigned int n_args;
        } function;
    };
} Type;

struct Ctx;
struct ExprList;
c_declare_vec(StrViews, CGS_StrView);

typedef struct Function
{
    CGS_StrView name;
    StrViewList args;

    bool native;
    union
    {
        const struct StmtList *body;
        struct Value (*native_call)(struct Ctx *ctx, struct ExprList *args);
    };
} Function;

typedef struct Value
{
    Type type;
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
        Function function;
    };
} Value;

#define T ExprList, struct Expr *
#define i_header
#include "grow_list.h"

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

        struct
        {
            Token lparen;
            Token rparen;
            struct Expr *left;
            ExprList args;
        } call;
    };
} Expr;

void pretty_print(CGS_Writer *sink, Expr *expr);
void rpn_print(CGS_Writer *sink, Expr *expr);

struct Ctx;

Expr *new_binary_expr(struct Ctx *ctx, Expr *left, Expr *right, Token op);
Expr *new_unary_expr(struct Ctx *ctx, Expr *left, Token op);
Expr *new_call_expr(struct Ctx *ctx, Expr *left, ExprList args, Token lparen, Token rparen);
Expr *new_paren_expr(struct Ctx *ctx, Expr *inside);
Expr *new_literal_expr(struct Ctx *ctx, Token tok);
Expr *new_var_expr(struct Ctx *ctx, Token tok);
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
