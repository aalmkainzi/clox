#include "stmt.h"
#include "clox.h"
#include "expr.h"
#include "var.h"

void free_scope(Scope *scope)
{
    for (c_each(scit, StmtList, scope->stmts))
    {
        free_stmt(scit.ref);
    }
    StmtList_drop(&scope->stmts);

    for (c_each(it, VarMap, scope->vars))
    {
        free_var_value(&it.ref->second);
    }
    VarMap_drop(&scope->vars);
}

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
            break;
        case STMT_scope:
            free_scope(stmt->scope);
            break;
    }
}

static void execute_stmts_in_scope(Ctx *ctx, Scope *scope)
{
    VarMap *vars = &scope->vars;

    for (c_each(it, StmtList, scope->stmts))
    {
        if (ctx->error)
            return;

        Stmt stmt = *it.ref;
        switch (stmt.kind)
        {
            case STMT_print:
                Value val = eval(ctx, stmt.print_expr);
                if (val.type == T_none)
                {
                    break;
                }
                cgs_printfln("%?", val);
                free_value(val);
                break;
            case STMT_var:
                CGS_StrView name = stmt.var.name->lexeme;
                Value value;
                if (stmt.var.rhs)
                {
                    value = copy_value(eval(ctx, stmt.var.rhs));
                }
                else
                {
                    value = (Value) {.type = T_nil};
                }
                value.var = true;

                VarMap_result res = VarMap_insert(vars, name, value);
                if (!res.inserted)
                {
                    free_var_value(&res.ref->second);
                    res.ref->second = value;
                }
                break;
            case STMT_expr:
                Value v = eval(ctx, stmt.expr);
                free_value(v);
                break;
            case STMT_scope:
                execute_stmts_in_scope(ctx, stmt.scope);
                break;
        }
    }
}

void execute_stmts(Ctx *ctx)
{
    execute_stmts_in_scope(ctx, ctx->scope);
}
