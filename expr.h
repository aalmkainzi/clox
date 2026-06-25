#ifndef EXPR_H
#define EXPR_H

typedef enum ExprKind
{
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_GROUPING,
    EXPR_LITERAL
} ExprKind;

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
            void *literal;
        };
    };
} Expr;

#endif