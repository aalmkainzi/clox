#ifndef STMT_H
#define STMT_H

#include "expr.h"
#include "token.h"

typedef enum StmtKind
{
    STMT_none = 0,
    STMT_expr,
    STMT_print,
    STMT_var,
    STMT_scope,
} StmtKind;

typedef struct
{
    Token *name;
    Expr *rhs;
} StmtVar;

declare_vec(StmtList, struct Stmt);

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
        StmtList scope_stmts;
    } data;
} Stmt;

typedef struct Ctx Ctx;
void execute_stmts(Ctx *ctx);

void free_stmt(Stmt *stmt);

#define i_declared
#define T StmtList, Stmt
#include "stc/vec.h"

#endif
