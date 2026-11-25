#ifndef STMT_H
#define STMT_H

#include "expr.h"
#include "token.h"

typedef struct Stmt Stmt;

#define T StmtList, Stmt
#define i_header
#include "grow_list.h"

#define T StrViewList, CGS_StrView
#define i_header
#include "grow_list.h"

typedef enum StmtKind : unsigned char
{
    STMT_none = 0,
    STMT_expr,
    STMT_print,
    STMT_var,
    STMT_scope,
    STMT_if,
    STMT_while,
    STMT_break,
    STMT_return,
    STMT_fun,
} StmtKind;

typedef struct StmtVar
{
    Token *name;
    Expr *rhs;
} StmtVar;

typedef struct StmtIf
{
    Expr *cond;
    Stmt *if_branch;
    Stmt *else_branch;
} StmtIf;

typedef struct StmtWhile
{
    Expr *cond;
    Stmt *body;
} StmtWhile;

typedef struct StmtFunction
{
    Token *name;
    StrViewList args;
    StmtList body;
} StmtFunction;

struct Stmt
{
    Token *begin;

    StmtKind kind;
    union
    {
        Expr *expr;
        Expr *print_expr;
        StmtVar var;
        StmtFunction function;
        StmtList scope_stmts;
        StmtIf if_stmt;
        StmtWhile while_stmt;
        Expr *return_expr;
    } data;
};

typedef struct ExecuteStmtResult
{
    enum : unsigned char
    {
        EXIT_FAIL,
        EXIT_NORMAL,
        EXIT_BREAK,
        EXIT_RETURN
    } exit_kind;

    Value value; // for EXIT_RETURN
} ExecuteStmtResult;

typedef struct Ctx Ctx;

void execute_stmts(Ctx *ctx);
ExecuteStmtResult execute_stmt_list(Ctx *ctx, const StmtList *list);

void exit_scope(Ctx *ctx);
void enter_scope(Ctx *ctx);

#endif
