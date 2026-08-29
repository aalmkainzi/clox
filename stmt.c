#include "stmt.h"
#include "clox.h"
#include "var.h"

void free_stmt(Stmt *stmt)
{
    switch (stmt->kind)
    {
    case STMT_print:
        free_expr(stmt->print_expr);
        break;
    case STMT_expr:
        free_expr(stmt->expr);
        break;
    case STMT_var:
        free_expr(stmt->var.rhs);
    }
}

void execute_stmts(Ctx *ctx)
{
    VarMap *vars = &ctx->vars;

    for (c_each(it, StmtList, ctx->stmts))
    {
        if (ctx->error)
            return;
        Stmt stmt = *it.ref;
        switch (stmt.kind)
        {
        case STMT_print:
            Value val = eval(ctx, stmt.expr);
            if (val.type == T_none)
                break;
            cgs_printfln("%?", val);
            free_value(val);
            break;
        case STMT_var:
            VarMap_result res = VarMap_insert(vars, stmt.var.name->lexeme, eval(ctx, stmt.var.rhs));
            if (!res.inserted)
            {
                report_tok_error(ctx, stmt.var.name, "var '%?' already declared", stmt.var.name->lexeme);
            }
            break;
        case STMT_expr:
            eval(ctx, stmt.expr);
            break;
        }
    }
}
