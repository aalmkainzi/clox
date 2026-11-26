#ifndef EXPR_H
#define EXPR_H

#include "arena.h"
#include "cgs/cgs.h"
#include "token.h"

#define T StrViewList, CGS_StrView
#define i_header
#include "grow_list.h"

#define T ExprList, struct Expr *
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

typedef struct Ctx Ctx;

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
    CGS_StrView view;

    bool var;
    bool error;

    union
    {
        CGS_DStr string;
        double number;
        bool boolean;
        Function function;
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

        struct
        {
            Value v;
            Token tok;
        } value;

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

Expr *new_binary_expr(Ctx *ctx, Expr *left, Expr *right, Token op);
Expr *new_unary_expr(Ctx *ctx, Expr *left, Token op);
Expr *new_call_expr(Ctx *ctx, Expr *left, ExprList args, Token lparen, Token rparen);
Expr *new_paren_expr(Ctx *ctx, Expr *inside);
Expr *new_literal_expr(Ctx *ctx, Token tok);
Expr *new_var_expr(Ctx *ctx, Token tok);
Token expr_tok(Expr *expr);
CGS_StrView expr_view(Expr *expr);

Value eval(Ctx *ctx, Expr *expr);
bool is_truthy(Value obj);

// the returned Value's lifetime doesn't end with current scope, but an explicit free_var_value call
Value var_value(Value val);
void free_var_value(Value v);

bool expect_number(Ctx *ctx, Value *obj);
bool expect_string(Ctx *ctx, Value *obj);
bool expect_function(Ctx *ctx, Value *obj);

Value copy_value(Ctx *ctx, Value from);
Value error_value();

CGS_Error stringify_value(CGS_Writer *writer, Value val, CGS_StrView fmt_arg);
CGS_Error stringify_literal(CGS_Writer *writer, Literal lit, CGS_StrView fmt_arg);

#define ADD_TOSTR (Value, stringify_value)
#include "cgs/cgs.h"
#define ADD_TOSTR (Literal, stringify_literal)
#include "cgs/cgs.h"

#endif
