#include "stmt.h"
#include "arena.h"
#include "clox.h"
#include "error.h"
#include "expr.h"
#include "var.h"

#define GL_TYPE Stmt
#define GL_NAME StmtList
#define GL_IMPL
#include "grow_list.h"

static bool execute_stmt_list(Ctx *ctx, StmtList *stmts);

static bool execute_stmt(Ctx *ctx, Stmt stmt)
{
    switch (stmt.kind)
    {
        case STMT_print:
        {
            Value val = eval(ctx, stmt.data.print_expr);
            if (ctx->error)
            {
                return false;
            }
            cgs_writeln_tostr(val);
            if (ctx->test && (ctx->test_data.verified_outputs != (unsigned int)-1))
            {
                CGS_DStr dstr = cgs_dstr_init();
                cgs_tostr(&dstr, val);

                if (ctx->test_data.expected_outputs.size == ctx->test_data.verified_outputs)
                {
                    error("unexpected output '%?'", dstr);
                    ctx->test_data.verified_outputs = (unsigned int)-1;
                    ctx->passed_test                = false;
                }
                else if (!cgs_equal(dstr, ctx->test_data.expected_outputs.data[ctx->test_data.verified_outputs]))
                {
                    error(
                        "expected output '%?' but got '%?'",
                        ctx->test_data.expected_outputs.data[ctx->test_data.verified_outputs],
                        dstr
                    );

                    ctx->test_data.verified_outputs = (unsigned int)-1;
                    ctx->passed_test                = false;
                }
                else
                {
                    passed("expected output '%?' has been printed", val);
                    ctx->test_data.verified_outputs++;
                }
                cgs_dstr_deinit(&dstr);
            }
        }
        break;
        case STMT_var:
        {
            CGS_StrView name = stmt.data.var.name->lexeme;
            Value val        = {};
            if (stmt.data.var.rhs)
            {
                val = eval(ctx, stmt.data.var.rhs);
                if (ctx->error)
                {
                    return false;
                }
            }
            else
            {
                val = (Value) {.type = T_nil};
            }

            val = var_value(val);

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
            Value val = eval(ctx, stmt.data.expr);
            if (ctx->error)
            {
                return false;
            }

            if (ctx->repl)
                cgs_writeln_tostr(val);
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
            if (ctx->error)
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
            return ret;
        }
        break;
        case STMT_while:
        {
            StmtWhile *while_stmt = &stmt.data.while_stmt;

            ArenaCheckpoint before_eval = arena_checkpoint(&ctx->eval_arena);
            Value cond                  = eval(ctx, while_stmt->cond);
            while (is_truthy(cond))
            {
                if (ctx->error || !execute_stmt(ctx, *while_stmt->body))
                {
                    return false;
                }

                arena_rollback(&ctx->eval_arena, before_eval);
                cond = eval(ctx, while_stmt->cond);
            }
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

void init_scope(Ctx *ctx, Scope *scope)
{
    scope->vars     = (VarMap) {};
    scope->vars.aux = &ctx->eval_arena;
    scope->up       = NULL;
}

void exit_scope(Ctx *ctx)
{
    for (c_each(it, VarMap, ctx->scope->vars))
    {
        free_var_value(it.ref->second);
    }

    ArenaCheckpoint checkpoint = ctx->scope->exit_checkpoint;
    Scope *up                  = ctx->scope->up;
    arena_rollback(&ctx->eval_arena, checkpoint);
    ctx->scope = up;
}

void enter_scope(Ctx *ctx)
{
    Scope *up = ctx->scope;

    ArenaCheckpoint checkpoint  = arena_checkpoint(&ctx->eval_arena);
    ctx->scope                  = arena_alloc(&ctx->eval_arena, sizeof(Scope));
    ctx->scope->exit_checkpoint = checkpoint;

    init_scope(ctx, ctx->scope);
    ctx->scope->up = up;
}
