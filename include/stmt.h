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

c_declare_vec(StmtList, struct Stmt);

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
        StmtIf if_stmt;
        StmtWhile while_stmt;
    } data;
} Stmt;

typedef struct Ctx Ctx;
void execute_stmts(Ctx *ctx);

void exit_scope(Ctx *ctx);
void enter_scope(Ctx *ctx);

void free_stmt(Stmt *stmt);
void free_stmts(StmtList *stmts);

#define i_declared
#define T StmtList, Stmt
#include "stc/vec.h"

#endif
