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
        case STMT_if:
            free_expr(stmt->data.if_stmt.cond);
            free_stmt(stmt->data.if_stmt.if_branch);
            free(stmt->data.if_stmt.if_branch);
            if (stmt->data.if_stmt.else_branch)
            {
                free_stmt(stmt->data.if_stmt.else_branch);
                free(stmt->data.if_stmt.else_branch);
            }
            break;
        case STMT_while:
            free_expr(stmt->data.while_stmt.cond);
            free_stmt(stmt->data.while_stmt.body);
            free(stmt->data.while_stmt.body);
            break;

        case STMT_none:;
    }
}

static bool execute_stmt_list(Ctx *ctx, StmtList *stmts);

static bool execute_stmt(Ctx *ctx, Stmt stmt)
{
    switch (stmt.kind)
    {
        case STMT_print:
        {
            Value val = eval(ctx, stmt.data.print_expr);
            if (val.type == T_none)
                return false;
            cgs_writeln_tostr(val);
            free_value(val);
        }
        break;
        case STMT_var:
        {
            CGS_StrView name = stmt.data.var.name->lexeme;
            Value val        = {};
            if (stmt.data.var.rhs)
            {
                val = eval(ctx, stmt.data.var.rhs);
                if (val.type == T_none)
                    return false;
            }
            else
            {
                val = (Value) {.type = T_nil};
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
            {
                ctx->error = true;
                return false;
            }

            if (ctx->repl)
                cgs_writeln_tostr(val);
            free_value(val);
        }
        break;
        case STMT_scope:
        {
            enter_scope(ctx);
            bool ret = execute_stmt_list(ctx, &stmt.data.scope_stmts);
            exit_scope(ctx);
            return ret;
        }
        break;
        case STMT_if:
        {
            StmtIf *if_stmt = &stmt.data.if_stmt;
            Value cond      = eval(ctx, if_stmt->cond);
            if (cond.type == T_none)
            {
                return false;
            }

            bool ret = true;
            if (is_truthy(cond))
            {
                ret = execute_stmt(ctx, *if_stmt->if_branch);
            }
            else if (if_stmt->else_branch)
            {
                ret = execute_stmt(ctx, *if_stmt->else_branch);
            }
            free_value(cond);
            return ret;
        }
        break;
        case STMT_while:
        {
            StmtWhile *while_stmt = &stmt.data.while_stmt;
            Value cond;
            for (cond = eval(ctx, while_stmt->cond); is_truthy(cond);
                 free_value(cond), cond = eval(ctx, while_stmt->cond))
            {
                if (!execute_stmt(ctx, *while_stmt->body))
                {
                    free_value(cond);
                    return false;
                }
            };
            free_value(cond); // free the false value
        }
        break;
        case STMT_none:
        default:
            unreachable();
            return false;
        
    }
    return true;
}

static bool execute_stmt_list(Ctx *ctx, StmtList *stmts)
{
    for (c_each(it, StmtList, *stmts))
    {
        Stmt stmt = *it.ref;
        if (!execute_stmt(ctx, stmt))
            return false;
    }
    return true;
}

void execute_stmts(Ctx *ctx)
{
    execute_stmt_list(ctx, &ctx->stmts);
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