#ifndef STMT_H
#define STMT_H

#include "expr.h"
#include "token.h"

typedef enum StmtKind
{
    STMT_expr,
    STMT_print,
} StmtKind;

typedef struct Stmt
{
    StmtKind kind;
    Token *begin;
    Token *end;

    union
    {
        Expr *print_expr;
        Expr *expr;
    };
} Stmt;

#define T StmtList, Stmt
#include "stc/vec.h"

void free_stmt(Stmt stmt);
void free_stmtlist(StmtList *list);

#endif
