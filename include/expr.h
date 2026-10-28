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
    struct Scope *scope;

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

Expr *new_binary(Expr *left, Expr *right, Token op);
Expr *new_unary(Expr *left, Token op);
Expr *new_grouping(Expr *inside);
Expr *new_literal(Token tok);
Expr *new_var(Token tok);
Token expr_tok(Expr *expr);
CGS_StrView expr_view(Expr *expr);
// void free_value(Value value);
// void free_var_value(Value *val);

struct Ctx;

Value eval(struct Ctx *ctx, Expr *expr);
bool is_truthy(Value obj);

Value copy_value(Value from);

CGS_Error stringify_value(CGS_Writer *writer, Value val, CGS_StrView fmt_arg);
CGS_Error stringify_literal(CGS_Writer *writer, Literal lit, CGS_StrView fmt_arg);

// begin a region that should be freed together (e.g. scope)
Arena *begin_arena();
void end_arena(Arena *arena);

// ignores the current stack of arenas, uses the passed one until unset
void set_current_arena(Arena *arena);
void unset_current_arena();

#define ADD_TOSTR (Value, stringify_value)
#include "cgs/cgs.h"
#define ADD_TOSTR (Literal, stringify_literal)
#include "cgs/cgs.h"

#endif
