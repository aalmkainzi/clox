#ifndef STMT_H
#define STMT_H

#include "expr.h"
#include "token.h"

typedef enum StmtKind
{
    STMT_expr,
    STMT_print,
    STMT_var,
} StmtKind;

typedef struct
{
    Token *name;
    Expr *rhs;
} StmtVar;

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
    };
} Stmt;

typedef struct Ctx Ctx;
void execute_stmts(Ctx *ctx);

void free_stmt(Stmt *stmt);

#define i_no_clone
#define i_keydrop free_stmt
#define T StmtList, Stmt
#include "stc/vec.h"

#endif
