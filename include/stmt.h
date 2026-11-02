#ifndef STMT_H
#define STMT_H

#include "expr.h"
#include "grow_list.h"
#include "token.h"

typedef enum StmtKind : unsigned char
{
    STMT_none = 0,
    STMT_expr,
    STMT_print,
    STMT_var,
    STMT_scope,
    STMT_if,
    STMT_while,
} StmtKind;

typedef struct StmtVar
{
    Token *name;
    Expr *rhs;
} StmtVar;

typedef struct StmtIf
{
    Expr *cond;
    struct Stmt *if_branch;
    struct Stmt *else_branch;
} StmtIf;

typedef struct StmtWhile
{
    Expr *cond;
    struct Stmt *body;
} StmtWhile;

typedef struct Stmt
{
    StmtKind kind;
    Token *begin;
    Token *end;

    union
    {
        Expr *expr;
        Expr *print_expr;
        StmtVar var;
        GrowList scope_stmts;
        StmtIf if_stmt;
        StmtWhile while_stmt;
    } data;
} Stmt;

typedef struct Ctx Ctx;
void execute_stmts(Ctx *ctx);

void exit_scope(Ctx *ctx);
void enter_scope(Ctx *ctx);

#endif
