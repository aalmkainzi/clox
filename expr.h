#ifndef EXPR_H
#define EXPR_H

#include "cgs/cgs.h"

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
        
        struct Token *ident;
    };
} Expr;



typedef struct EvalResult
{
    
} EvalResult;

typedef struct Token Token;

void pretty_print(CGS_Writer *sink, Expr *expr);
void rpn_print(CGS_Writer *sink, Expr *expr);

Expr *new_binary(Expr *left, Expr *right, Token *op);
Expr *new_unary(Expr *left, Token *op);
Expr *new_grouping(Expr *inside);
Expr *new_literal(Token *tok);

#endif