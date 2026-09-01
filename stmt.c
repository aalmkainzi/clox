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
                {
                    break;
                }
                cgs_printfln("%?", val);
                free_value(val);
                break;
            case STMT_var:
                CGS_StrView name = stmt.var.name->lexeme;
                Value value = eval(ctx, stmt.var.rhs);
                value.var = true;
                VarMap_result res = VarMap_insert(vars, name, value);
                if (!res.inserted)
                {
                    free_value(res.ref->second);
                    res.ref->second = value;
                }
                break;
            case STMT_expr:
                eval(ctx, stmt.expr);
                break;
        }
    }
}
