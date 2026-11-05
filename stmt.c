#include "stmt.h"
#include "arena.h"
#include "clox.h"
#include "error.h"
#include "expr.h"
#include "var.h"

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
            val.var   = true;
            val.scope = ctx->scope;

            VarMap_result res = VarMap_insert(&ctx->scope->vars, name, val);
            if (!res.inserted)
            {
                res.ref->second = val;
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
            Arenas_iter arena     = begin_arena();
            Value cond;
            for (cond = eval(ctx, while_stmt->cond); is_truthy(cond); cond = eval(ctx, while_stmt->cond))
            {
                arena_reset(arena.ref);
                if (ctx->error || !execute_stmt(ctx, *while_stmt->body))
                {
                    return false;
                }
            };
            end_arena(arena);
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
    for (stmt_list_each(it, stmts))
    {
        Stmt stmt = *it.ptr;
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

    VarMap_drop(&scope->vars);
}

void exit_scope(Ctx *ctx)
{
    free_scope(ctx->scope);
    Scope *up = ctx->scope->up;
    end_arena(ctx->scope->arena);
    ctx->scope = up;
}

void enter_scope(Ctx *ctx)
{
    Arenas_iter scope_arena = begin_arena();
    Scope *save             = ctx->scope;

    // the first allocation in the new arena is the Scope itself
    ctx->scope = cgs_alloc(arena_stack_allocator, Scope, 1).ptr;
    init_scope(ctx->scope);
    ctx->scope->up    = save;
    ctx->scope->arena = scope_arena;
}
