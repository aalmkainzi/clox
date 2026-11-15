#include "stmt.h"
#include "arena.h"
#include "clox.h"
#include "error.h"
#include "expr.h"
#include "test.h"
#include "var.h"

#define T StmtList, Stmt
#define i_implement
#include "grow_list.h"

typedef enum : unsigned char
{
    ES_FAIL,
    ES_OK,
    ES_BREAK
} ExecuteStmtResult;

static ExecuteStmtResult execute_stmt_list(Ctx *ctx, const StmtList *stmts);

static ExecuteStmtResult execute_stmt(Ctx *ctx, const Stmt *stmt)
{
    switch (stmt->kind)
    {
        case STMT_print:
        {
            ArenaCheckpoint before_eval = arena_checkpoint(ctx->eval_arena);
            Value val                   = eval(ctx, stmt->data.print_expr);

            if (ctx->error)
            {
                arena_rollback(ctx->eval_arena, before_eval);
                return ES_FAIL;
            }
            cgs_writeln_tostr(val);
            if (ctx->opts.test && !ctx->test_data.failed_test)
            {
                CGS_DStr as_str = cgs_asprintf("%?", val);

                verify_expected_output(ctx, as_str);

                cgs_dstr_deinit(&as_str);
            }
            arena_rollback(ctx->eval_arena, before_eval);
        }
        break;
        case STMT_var:
        {
            CGS_StrView name            = stmt->data.var.name->lexeme;
            Value val                   = {};
            ArenaCheckpoint before_eval = arena_checkpoint(ctx->eval_arena);
            if (stmt->data.var.rhs)
            {
                val = eval(ctx, stmt->data.var.rhs);
                if (ctx->error)
                {
                    arena_rollback(ctx->eval_arena, before_eval);
                    return ES_FAIL;
                }
            }
            else
            {
                val = (Value) {.type = T_nil};
            }

            val = var_value(val); // copies the val from the eval_arena to malloc heap
            arena_rollback(ctx->eval_arena, before_eval);

            VarMap_value *v = VarMap_get_mut(&ctx->scope->vars, name);
            if (v)
            {
                free_var_value(v->second);
                v->second = val;
            }
            else
            {
                VarMap_result res = VarMap_insert(&ctx->scope->vars, name, val);
                assert(res.inserted); // we already confirmed !v, so there's no way insertion fails
            }
        }
        break;
        case STMT_expr:
        {
            ArenaCheckpoint before_eval = arena_checkpoint(ctx->eval_arena);
            Value val                   = eval(ctx, stmt->data.expr);
            (void)val;

            if (ctx->error)
            {
                arena_rollback(ctx->eval_arena, before_eval);
                return ES_FAIL;
            }

            arena_rollback(ctx->eval_arena, before_eval);
        }
        break;
        case STMT_scope:
        {
            enter_scope(ctx);
            ExecuteStmtResult result = execute_stmt_list(ctx, &stmt->data.scope_stmts);
            exit_scope(ctx);
            return result;
        }
        break;
        case STMT_if:
        {
            const StmtIf *if_stmt = &stmt->data.if_stmt;

            ArenaCheckpoint before_eval = arena_checkpoint(ctx->eval_arena);
            Value cond                  = eval(ctx, if_stmt->cond);

            if (ctx->error)
            {
                arena_rollback(ctx->eval_arena, before_eval);
                return ES_FAIL;
            }

            bool truthy = is_truthy(cond);
            arena_rollback(ctx->eval_arena, before_eval);

            ExecuteStmtResult result = ES_OK;
            if (truthy)
            {
                result = execute_stmt(ctx, if_stmt->if_branch);
            }
            else if (if_stmt->else_branch)
            {
                result = execute_stmt(ctx, if_stmt->else_branch);
            }

            return result;
        }
        break;
        case STMT_while:
        {
            const StmtWhile *while_stmt = &stmt->data.while_stmt;
            ArenaCheckpoint before_eval = arena_checkpoint(ctx->eval_arena);

            while (1)
            {
                Value cond = eval(ctx, while_stmt->cond);
                if (ctx->error)
                {
                    arena_rollback(ctx->eval_arena, before_eval);
                    return ES_FAIL;
                }

                bool truthy = is_truthy(cond);
                arena_rollback(ctx->eval_arena, before_eval);

                if (!truthy)
                    break;

                ExecuteStmtResult result = execute_stmt(ctx, while_stmt->body);

                if (result == ES_BREAK)
                    return ES_OK;
                if (result != ES_OK)
                    return result;
            }
        }
        break;
        case STMT_break:
            return ES_BREAK;

        case STMT_none:
        default:
            unreachable();
            return ES_FAIL;
    }
    return ES_OK;
}

static ExecuteStmtResult execute_stmt_list(Ctx *ctx, const StmtList *stmts)
{
    for (c_each(it, StmtList, *stmts))
    {
        ExecuteStmtResult result = execute_stmt(ctx, it.ref);
        if (result == ES_FAIL || result == ES_BREAK)
            return result;
    }
    return ES_OK;
}

void execute_stmts(Ctx *ctx)
{
    execute_stmt_list(ctx, &ctx->stmts);
}

void init_scope(Ctx *ctx, Scope *scope)
{
    scope->vars     = (VarMap) {};
    scope->vars.aux = ctx->eval_arena;
    VarMap_reserve(&scope->vars, 8);
    scope->up = NULL;
}

void exit_scope(Ctx *ctx)
{
    for (c_each(it, VarMap, ctx->scope->vars))
    {
        free_var_value(it.ref->second);
    }

    ArenaCheckpoint checkpoint = ctx->scope->exit_checkpoint;
    Scope *up                  = ctx->scope->up;
    arena_rollback(ctx->eval_arena, checkpoint);
    ctx->scope = up;
}

void enter_scope(Ctx *ctx)
{
    Scope *up = ctx->scope;

    ArenaCheckpoint checkpoint  = arena_checkpoint(ctx->eval_arena);
    ctx->scope                  = arena_alloc(ctx->eval_arena, sizeof(Scope));
    ctx->scope->exit_checkpoint = checkpoint;

    init_scope(ctx, ctx->scope);
    ctx->scope->up = up;
}
