#ifndef EXPR_H
#define EXPR_H

#include "cgs/cgs.h"
#include "token.h"

typedef enum ExprKind
{
    EXPR_unary,
    EXPR_binary,
    EXPR_value
} ExprKind;

typedef enum LiteralKind
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

typedef enum TypeKind
{
    T_bool,
    T_num,
    T_str,
    T_nil,
} TypeKind;

typedef struct Value
{
    TypeKind type;
    Token *tok;
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
            Token *op;
            struct Expr *expr;
        } unary;

        struct
        {
            Token *op;
            struct Expr *left;
            struct Expr *right;
        } binary;

        Value value;

        Token *ident;
    };
} Expr;

void pretty_print(CGS_Writer *sink, Expr *expr);
void rpn_print(CGS_Writer *sink, Expr *expr);

Expr *new_binary(Expr *left, Expr *right, Token *op);
Expr *new_unary(Expr *left, Token *op);
Expr *new_grouping(Expr *inside);
Expr *new_literal(Token *tok);
Token *expr_tok(Expr *expr);
void free_value(Value value);

struct Ctx;
Value eval(struct Ctx *ctx, Expr *expr);

CGS_Error stringify_value(CGS_Writer *writer, Value val);
CGS_Error stringify_literal(CGS_Writer *writer, Literal lit);

#define ADD_TOSTR (Value, stringify_value)
#include "cgs/cgs.h"
#define ADD_TOSTR (Literal, stringify_literal)
#include "cgs/cgs.h"

#endif