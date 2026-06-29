#ifndef EXPR_H
#define EXPR_H

#include "cgs/cgs.h"

typedef enum ExprKind
{
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_GROUPING,
    EXPR_LITERAL
} ExprKind;

typedef enum LiteralKind
{
    LIT_STR,
    LIT_NUM,
    LIT_BOOL,
    LIT_IDENT,
    LIT_NIL,
} LiteralKind;

typedef struct Expr
{
    ExprKind kind;
    
    union {
        struct
        {
            struct Token *op;
            struct Expr *expr;
        } unary;
        
        struct
        {
            struct Token *op;
            struct Expr *left;
            struct Expr *right;
        } binary;
        
        struct
        {
            struct Expr *expr;
        } grouping;
        
        struct
        {
            LiteralKind kind;
            struct Token *tok;
        } literal;
    };
} Expr;

typedef struct Token Token;

void pretty_print(CGS_Writer *sink, Expr *expr);
void rpn_print(CGS_Writer *sink, Expr *expr);

Expr *new_binary(Expr *left, Expr *right, Token *op);
Expr *new_unary(Expr *left, Token *op);
Expr *new_grouping(Expr *inside);
Expr *new_literal(Token *tok);

#endif