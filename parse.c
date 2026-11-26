#include "arena.h"
#include "clox.h"
#include "error.h"
#include "expr.h"
#include "stmt.h"
#include "token.h"
#include "var.h"

#define T StrViewList, CGS_StrView
#define i_implement
#define i_body
#include "grow_list.h"

static Expr *expression(Ctx *ctx);
static Expr *or_expr(Ctx *ctx);
static Expr *and_expr(Ctx *ctx);
static Expr *assignment(Ctx *ctx);
static Expr *equality(Ctx *ctx);
static Expr *comparison(Ctx *ctx);
static Expr *additive(Ctx *ctx);
static Expr *multitive(Ctx *ctx);
static Expr *unary(Ctx *ctx);
static Expr *call(Ctx *ctx);
static Expr *primary(Ctx *ctx);

static Stmt parse_scope_stmt(Ctx *ctx);
static Stmt print_stmt(Ctx *ctx);
static Stmt parse_expr_stmt(Ctx *ctx);
static Stmt parse_var_decl(Ctx *ctx);
static Stmt parse_if_stmt(Ctx *ctx);
static Stmt parse_while_stmt(Ctx *ctx);
static Stmt parse_for_stmt(Ctx *ctx);
static Stmt parse_function(Ctx *ctx);
static Stmt parse_stmt(Ctx *ctx);
void parse_stmts(Ctx *ctx);

static void synchronize(Ctx *ctx, TokenKind to);

void parse(Ctx *ctx)
{
    ctx->tok = TokenList_begin(TokensChain_back(&ctx->toks));

    parse_stmts(ctx);
}

void parse_stmts(Ctx *ctx)
{
    while (curtok(ctx) && curtok(ctx)->kind != TK_eof)
    {
        Stmt stmt = parse_stmt(ctx);
        if (stmt.kind != STMT_none)
        {
            StmtList_iter it = StmtList_push(&ctx->stmts);
            *it.ref          = stmt;
        }
    }
}

static Stmt parse_stmt(Ctx *ctx)
{
    Stmt stmt = {};
    switch (curtok(ctx)->kind)
    {
        case TK_print:
            stmt = print_stmt(ctx);
            break;
        case TK_var:
            stmt = parse_var_decl(ctx);
            break;
        case TK_lcurly:
            stmt = parse_scope_stmt(ctx);
            break;
        case TK_if:
            stmt = parse_if_stmt(ctx);
            break;
        case TK_while:
            stmt = parse_while_stmt(ctx);
            break;
        case TK_for:
            stmt = parse_for_stmt(ctx);
            break;
        case TK_break:
        {
            if (!ctx->parser_state.in_loop)
            {
                error_at_tok(ctx, *curtok(ctx), "break statement cannot appear outside a loop");
                synchronize(ctx, TK_semicolon);
                advance(ctx);
                break;
            }
            stmt = (Stmt) {.kind = STMT_break, .begin = curtok(ctx)};
            advance(ctx);
            if (!skip_tok(ctx, TK_semicolon))
            {
                synchronize(ctx, TK_semicolon);
                advance(ctx);
            }
        }
        break;
        case TK_return:
        {
            Token *begin = curtok(ctx);
            advance(ctx);
            Expr *return_expr = expression(ctx);

            if (!skip_tok(ctx, TK_semicolon))
            {
                synchronize(ctx, TK_semicolon);
                advance(ctx);
                break;
            }

            if (return_expr != NULL)
            {
                stmt = (Stmt) {
                    .kind             = STMT_return,
                    .begin            = begin,
                    .data.return_expr = return_expr,
                };
            }
        }
        break;
        case TK_fun:
            stmt = parse_function(ctx);
            break;
        default:
            stmt = parse_expr_stmt(ctx);
            break;
    }
    return stmt;
}

static Stmt parse_function_like(Ctx *ctx)
{
    Stmt stmt = {.kind = STMT_fun, .begin = curtok(ctx), .data.function.args = StrViewList_init(ctx->parse_arena)};

    ArenaCheckpoint checkpoint = arena_checkpoint(ctx->parse_arena);

    stmt.data.function.name = curtok(ctx);

    if (!skip_tok(ctx, TK_ident))
    {
        synchronize(ctx, TK_rcurly);
        advance(ctx);
        return (Stmt) {};
    }

    if (!skip_tok(ctx, TK_lparen))
    {
        synchronize(ctx, TK_rcurly);
        advance(ctx);
        return (Stmt) {};
    }

    bool first = true;
    while (curtok(ctx)->kind != TK_rparen)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            if (!skip_tok(ctx, TK_comma))
            {
                synchronize(ctx, TK_rcurly);
                advance(ctx);
                return (Stmt) {};
            }
        }

        Token *arg_name = curtok(ctx);

        *StrViewList_push(&stmt.data.function.args).ref = arg_name->lexeme;

        if (!skip_tok(ctx, TK_ident))
        {
            synchronize(ctx, TK_rcurly);
            advance(ctx);
            arena_rollback(ctx->parse_arena, checkpoint);
            return (Stmt) {};
        }
    }

    if (stmt.data.function.args.size > 255)
    {
        error_at(ctx, range_view(stmt.begin->lexeme, curtok(ctx)->lexeme), "maximum argument count is 255");
    }

    advance(ctx);

    if (curtok(ctx)->kind == TK_semicolon)
    {
        error_at_tok(ctx, *curtok(ctx), "functions don't need forward declarations. This is not C");
        advance(ctx);

        arena_rollback(ctx->parse_arena, checkpoint);
        return (Stmt) {};
    }

    if (!expect_tok(ctx, TK_lcurly))
    {
        synchronize(ctx, TK_rcurly);
        advance(ctx);
        arena_rollback(ctx->parse_arena, checkpoint);
        return (Stmt) {};
    }

    Stmt body = parse_scope_stmt(ctx);

    if (body.kind == STMT_none)
    {
        arena_rollback(ctx->parse_arena, checkpoint);
        return (Stmt) {};
    }

    stmt.data.function.body = body.data.scope_stmts;
    return stmt;
}

static Stmt parse_function(Ctx *ctx)
{
    Token *begin = curtok(ctx);
    advance(ctx);
    Stmt stmt = parse_function_like(ctx);
    if (stmt.kind != STMT_none)
        stmt.begin = begin;
    return stmt;
}

static Stmt parse_scope_stmt(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(ctx->parse_arena);

    Stmt stmt = {.kind = STMT_scope, .data.scope_stmts = StmtList_init(ctx->parse_arena)};
    advance(ctx);
    StmtList *list = &stmt.data.scope_stmts;

    while (curtok(ctx)->kind != TK_rcurly && curtok(ctx)->kind != TK_eof)
    {
        Stmt scope_stmt = parse_stmt(ctx);
        if (scope_stmt.kind != STMT_none)
        {
            StmtList_iter it = StmtList_push(list);
            *it.ref          = scope_stmt;
        }
    }

    if (!skip_tok(ctx, TK_rcurly))
    {
        arena_rollback(ctx->parse_arena, checkpoint);
        return (Stmt) {};
    }

    return stmt;
}

static Stmt parse_if_stmt(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(ctx->parse_arena);

    bool err  = false;
    Stmt stmt = {.kind = STMT_if, .begin = curtok(ctx)};
    advance(ctx);
    if (!skip_tok(ctx, TK_lparen))
    {
        synchronize(ctx, TK_rparen);
        advance(ctx);
        return (Stmt) {};
    }

    stmt.data.if_stmt.cond = expression(ctx);

    if (!skip_tok(ctx, TK_rparen))
    {
        err = true; // I think the best recovery is to just pretend ) is there
    }

    stmt.data.if_stmt.if_branch  = arena_alloc(ctx->parse_arena, sizeof(Stmt));
    *stmt.data.if_stmt.if_branch = parse_stmt(ctx);

    if (stmt.data.if_stmt.if_branch->kind == STMT_none)
    {
        err = true;
    }

    if (curtok(ctx)->kind == TK_else)
    {
        advance(ctx);
        stmt.data.if_stmt.else_branch  = arena_alloc(ctx->parse_arena, sizeof(Stmt));
        *stmt.data.if_stmt.else_branch = parse_stmt(ctx);
        if (stmt.data.if_stmt.else_branch->kind == STMT_none)
        {
            err = true;
        }
    }

    if (err)
    {
        arena_rollback(ctx->parse_arena, checkpoint);
        return (Stmt) {};
    }

    return stmt;
}

static Stmt parse_while_stmt(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(ctx->parse_arena);

    bool err  = false;
    Stmt stmt = {.begin = curtok(ctx), .kind = STMT_while};

    advance(ctx);

    StmtWhile *w = &stmt.data.while_stmt;
    w->cond      = expression(ctx);
    if (w->cond == NULL)
    {
        err = true;
    }

    bool in_loop_prev         = ctx->parser_state.in_loop;
    ctx->parser_state.in_loop = true;

    w->body  = arena_alloc(ctx->parse_arena, sizeof(Stmt));
    *w->body = parse_stmt(ctx);

    ctx->parser_state.in_loop = in_loop_prev;

    if (w->body->kind == STMT_none)
    {
        err = true;
    }

    if (err)
    {
        arena_rollback(ctx->parse_arena, checkpoint);
        return (Stmt) {};
    }

    return stmt;
}

static Stmt parse_for_stmt(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(ctx->parse_arena);

    bool err = false;

    advance(ctx);
    if (!skip_tok(ctx, TK_lparen))
    {
        err = true;
    }

    Stmt scope = {
        .begin            = curtok(ctx),
        .kind             = STMT_scope,
        .data.scope_stmts = StmtList_init(ctx->parse_arena),
    };

    Stmt init_stmt = {};
    if (curtok(ctx)->kind != TK_semicolon)
    {
        if (curtok(ctx)->kind == TK_var)
        {
            init_stmt = parse_var_decl(ctx);
        }
        else
        {
            init_stmt = parse_expr_stmt(ctx);
        }

        if (init_stmt.kind == STMT_none)
        {
            err = true;
        }
        else
        {
            StmtList_iter it = StmtList_push(&scope.data.scope_stmts);
            *it.ref          = init_stmt;
        }
    }
    else
    {
        advance(ctx);
    }

    Stmt loop = {.begin = curtok(ctx), .kind = STMT_while};

    StmtWhile *w = &loop.data.while_stmt;
    if (curtok(ctx)->kind != TK_semicolon)
    {
        w->cond = expression(ctx);
        if (w->cond == NULL)
        {
            err = true;
        }
        if (!skip_tok(ctx, TK_semicolon))
        {
            err = true;
        }
    }
    else
    {
        w->cond            = arena_alloc(ctx->parse_arena, sizeof(Expr));
        w->cond->kind      = EXPR_value;
        w->cond->value.v   = (Value) {.type.kind = T_bool, .boolean = true};
        w->cond->value.tok = *curtok(ctx);

        advance(ctx);
    }

    Stmt update = {};
    if (curtok(ctx)->kind != TK_rparen)
    {
        update = (Stmt) {
            .begin     = curtok(ctx),
            .kind      = STMT_expr,
            .data.expr = expression(ctx),
        };
        if (update.data.expr == NULL)
        {
            err = true;
        }
    }

    if (!skip_tok(ctx, TK_rparen))
    {
        err = true;
        synchronize(ctx, TK_rparen);
        advance(ctx);
    }

    w->body  = arena_alloc(ctx->parse_arena, sizeof(Stmt));
    *w->body = (Stmt) {
        .begin            = curtok(ctx),
        .kind             = STMT_scope,
        .data.scope_stmts = StmtList_init(ctx->parse_arena),
    };

    bool in_loop_prev         = ctx->parser_state.in_loop;
    ctx->parser_state.in_loop = true;

    Stmt body_stmt = parse_stmt(ctx);

    ctx->parser_state.in_loop = in_loop_prev;

    if (body_stmt.kind == STMT_none)
    {
        err = true;
    }
    else
    {
        StmtList_iter it = StmtList_push(&w->body->data.scope_stmts);
        *it.ref          = body_stmt;
    }

    if (update.kind != STMT_none)
    {
        StmtList_iter it = StmtList_push(&w->body->data.scope_stmts);
        *it.ref          = update;
    }

    if (w->body->kind == STMT_none)
    {
        err = true;
    }

    StmtList_iter it = StmtList_push(&scope.data.scope_stmts);
    *it.ref          = loop;

    if (err)
    {
        arena_rollback(ctx->parse_arena, checkpoint);
        return (Stmt) {};
    }

    return scope;
}

static Stmt parse_var_decl(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(ctx->parse_arena);

    Stmt stmt = {.kind = STMT_var, .begin = curtok(ctx)};
    advance(ctx);
    stmt.data.var.name = curtok(ctx);
    if (!skip_tok(ctx, TK_ident))
    {
        synchronize(ctx, TK_semicolon);
        advance(ctx);
        return (Stmt) {};
    }

    if (curtok(ctx)->kind == TK_equal)
    {
        advance(ctx);
        stmt.data.var.rhs = expression(ctx);
    }

    if (!skip_tok(ctx, TK_semicolon))
    {
        arena_rollback(ctx->parse_arena, checkpoint);
        return (Stmt) {};
    }
    return stmt;
}

static Stmt parse_expr_stmt(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(ctx->parse_arena);

    Stmt stmt = {.kind = STMT_expr, .begin = curtok(ctx)};

    stmt.data.expr = expression(ctx);

    if (stmt.data.expr == NULL || !skip_tok(ctx, TK_semicolon))
    {
        synchronize(ctx, TK_semicolon);
        advance(ctx);
        arena_rollback(ctx->parse_arena, checkpoint);
        return (Stmt) {};
    }

    return stmt;
}

static Stmt print_stmt(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(ctx->parse_arena);

    Stmt stmt = {.kind = STMT_print, .begin = curtok(ctx)};
    advance(ctx);

    stmt.data.print_expr = expression(ctx);

    if (!skip_tok(ctx, TK_semicolon))
    {
        arena_rollback(ctx->parse_arena, checkpoint);
        return (Stmt) {};
    }
    return stmt;
}

static Expr *expression(Ctx *ctx)
{
    return assignment(ctx);
}

static Expr *assignment(Ctx *ctx)
{
    Token *lhs_begin = curtok(ctx);
    Expr *left       = or_expr(ctx);

    if (curtok(ctx)->kind == TK_equal)
    {
        TokenList_iter lhs_end = ctx->tok;
        TokenList_prev(&lhs_end);

        if (left->kind != EXPR_var)
        {
            error_at_tok_range(ctx, *lhs_begin, *lhs_end.ref, "not a variable");
        }
        Token *op = curtok(ctx);
        advance(ctx);
        Expr *right = expression(ctx);
        return new_binary_expr(ctx, left, right, *op);
    }

    return left;
}

static Expr *or_expr(Ctx *ctx)
{
    Expr *expr = and_expr(ctx);

    while (curtok(ctx)->kind == TK_or)
    {
        Token *op = curtok(ctx);
        advance(ctx);
        Expr *right = and_expr(ctx);

        expr = new_binary_expr(ctx, expr, right, *op);
    }

    return expr;
}

static Expr *and_expr(Ctx *ctx)
{
    Expr *expr = equality(ctx);

    while (curtok(ctx)->kind == TK_and)
    {
        Token *op = curtok(ctx);
        advance(ctx);
        Expr *right = equality(ctx);

        expr = new_binary_expr(ctx, expr, right, *op);
    }

    return expr;
}

static Expr *equality(Ctx *ctx)
{
    Expr *expr = comparison(ctx);

    while (check_any(ctx, TK_equal_equal, TK_not_equal))
    {
        Token *op = curtok(ctx);
        advance(ctx);
        Expr *right = comparison(ctx);

        expr = new_binary_expr(ctx, expr, right, *op);
    }

    return expr;
}

static Expr *comparison(Ctx *ctx)
{
    Expr *expr = additive(ctx);

    while (check_any(ctx, TK_lt, TK_lte, TK_gt, TK_gte))
    {
        Token *op = curtok(ctx);
        advance(ctx);
        Expr *right = additive(ctx);

        expr = new_binary_expr(ctx, expr, right, *op);
    }

    return expr;
}

static Expr *additive(Ctx *ctx)
{
    Expr *expr = multitive(ctx);

    while (check_any(ctx, TK_plus, TK_minus))
    {
        Token *op = curtok(ctx);
        advance(ctx);
        Expr *right = multitive(ctx);

        expr = new_binary_expr(ctx, expr, right, *op);
    }

    return expr;
}

static Expr *multitive(Ctx *ctx)
{
    Expr *expr = unary(ctx);

    while (check_any(ctx, TK_star, TK_slash))
    {
        Token *op = curtok(ctx);
        advance(ctx);
        Expr *right = unary(ctx);

        expr = new_binary_expr(ctx, expr, right, *op);
    }

    return expr;
}

static Expr *unary(Ctx *ctx)
{
    if (check_any(ctx, TK_not, TK_minus))
    {
        Token *op = curtok(ctx);
        advance(ctx);
        Expr *exp = unary(ctx);
        Expr *new = new_unary_expr(ctx, exp, *op);
        return new;
    }
    else
    {
        return call(ctx);
    }
}

static Expr *call(Ctx *ctx)
{
    Expr *expr = primary(ctx);

    while (curtok(ctx)->kind == TK_lparen)
    {
        Token *lparen = curtok(ctx);
        advance(ctx);
        ExprList args = ExprList_init(ctx->parse_arena);

        bool first = true;
        while (curtok(ctx)->kind != TK_rparen)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                if (!skip_tok(ctx, TK_comma))
                {
                    synchronize(ctx, TK_semicolon);
                    return NULL;
                }
            }

            Expr *arg                 = expression(ctx);
            *ExprList_push(&args).ref = arg;
        }
        expr = new_call_expr(ctx, expr, args, *lparen, *curtok(ctx));
        advance(ctx);

        if (args.size > 255)
        {
            error_at(ctx, expr_view(expr), "maximum argument count is 255");
        }
    }

    return expr;
}

static Expr *primary(Ctx *ctx)
{
    if (curtok(ctx)->kind == TK_lparen)
    {
        advance(ctx);
        Expr *inside = expression(ctx);
        if (!skip_tok(ctx, TK_rparen))
            synchronize(ctx, TK_semicolon);

        return inside;
    }

    Expr *expr = NULL;

    if (curtok(ctx)->kind == TK_ident)
    {
        expr = new_var_expr(ctx, *curtok(ctx));
    }
    else
    {
        expr = new_literal_expr(ctx, *curtok(ctx));

        if (expr == NULL)
        {
            error_at_tok(ctx, *curtok(ctx), "expected expression, but got %?", tok2str[curtok(ctx)->kind]);

            synchronize(ctx, TK_semicolon);
            return NULL;
        }
    }

    advance(ctx);
    return expr;
}

// TODO synchronize should be bracket aware
static void synchronize(Ctx *ctx, TokenKind to)
{
    while (curtok(ctx)->kind != to && curtok(ctx)->kind != TK_eof)
    {
        advance(ctx);
    }
}
