#ifndef EXPR_H
#define EXPR_H

#include "cgs/cgs.h"
#include "token.h"

typedef enum ExprKind
{
    EXPR_unary,
    EXPR_binary,
    EXPR_grouping,
    EXPR_literal,
    EXPR_ident
} ExprKind;

typedef enum LiteralKind
{
    LIT_str,
    LIT_num,
    LIT_bool,
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
} TypeKind;

typedef struct Expr
{
    ExprKind kind;
    
    union {
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
        
        struct
        {
            struct Expr *expr;
        } grouping;
        
        Literal literal;
        
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
void free_literal(Literal lit);

#endif