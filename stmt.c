#include "stmt.h"
#include "clox.h"

void free_stmt(Stmt stmt)
{
    switch (stmt.kind)
    {
    case STMT_print:
        free_expr(stmt.print_expr);
        break;
    case STMT_expr:
        free_expr(stmt.expr);
        break;
    }
}

void free_stmtlist(StmtList *list)
{
    for (c_each(it, StmtList, *list))
    {
        free_stmt(*it.ref);
    }
    StmtList_drop(list);
}
