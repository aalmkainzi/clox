#include "stmt.h"
#include "clox.h"
#include "expr.h"
#include "var.h"

static void enter_scope(Ctx *ctx);
static void exit_scope(Ctx *ctx);

void free_stmts(StmtList *stmts)
{
    for (c_each(scit, StmtList, *stmts))
    {
        free_stmt(scit.ref);
    }

    StmtList_drop(stmts);
}

void free_stmt(Stmt *stmt)
{
    switch (stmt->kind)
    {
        case STMT_print:
            free_expr(stmt->data.print_expr);
            break;
        case STMT_expr:
            free_expr(stmt->data.expr);
            break;
        case STMT_var:
            free_expr(stmt->data.var.rhs);
            break;
        case STMT_scope:
            free_stmts(&stmt->data.scope_stmts);
            break;

        case STMT_none:
            unreachable();
    }
}

static void execute_stmts_in_scope(Ctx *ctx, StmtList *stmts, Scope *scope)
{
    for (c_each(it, StmtList, *stmts))
    {
        if (ctx->error)
            return;

        Stmt stmt = *it.ref;
        switch (stmt.kind)
        {
            case STMT_print:
            {
                Value val = eval(ctx, stmt.data.print_expr);
                if (val.type == T_none)
                    break;
                cgs_writeln_tostr(val);
                free_value(val);
            }
            break;
            case STMT_var:
            {
                CGS_StrView name = stmt.data.var.name->lexeme;
                Value val = {};
                if (stmt.data.var.rhs)
                {
                    val = eval(ctx, stmt.data.var.rhs);
                    if (val.type == T_none)
                        break;
                }
                else
                {
                    val = (Value) {.type = T_none};
                }
                val.var = true;

                VarMap_result res = VarMap_insert(&ctx->scope->vars, name, val);
                if (!res.inserted)
                {
                    free_var_value(&res.ref->second);
                    res.ref->second = val;
                }
            }
            break;
            case STMT_expr:
            {
                Value val = eval(ctx, stmt.data.expr);
                if (val.type == T_none)
                    break;
                cgs_writeln_tostr(val);
                free_value(val);
            }
            break;
            case STMT_scope:
            {
                enter_scope(ctx);
                execute_stmts_in_scope(ctx, &stmt.data.scope_stmts, scope);
                exit_scope(ctx);
            }
            break;

            case STMT_none:
                unreachable();
        }
    }
}

void execute_stmts(Ctx *ctx)
{
    execute_stmts_in_scope(ctx, &ctx->stmts, ctx->scope);
}

void init_scope(Scope *scope)
{
    scope->vars = VarMap_init();
    scope->up   = NULL;
}

void free_scope(Scope *scope)
{
    if (scope == NULL)
        return;
    for (c_each(vit, VarMap, scope->vars))
    {
        free_var_value(&vit.ref->second);
    }
    VarMap_drop(&scope->vars);
    free(scope);
}

static void exit_scope(Ctx *ctx)
{
    Scope *up = ctx->scope->up;
    free_scope(ctx->scope);
    ctx->scope = up;
}

static void enter_scope(Ctx *ctx)
{
    Scope *save = ctx->scope;
    ctx->scope  = calloc(1, sizeof(Scope));
    init_scope(ctx->scope);
    ctx->scope->up = save;
}