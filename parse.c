#include "arena.h"
#include "clox.h"
#include "error.h"
#include "expr.h"
#include "stmt.h"
#include "token.h"
#include "var.h"

static Expr *expression(Ctx *ctx);
static Expr *or_expr(Ctx *ctx);
static Expr *and_expr(Ctx *ctx);
static Expr *assignment(Ctx *ctx);
static Expr *equality(Ctx *ctx);
static Expr *comparison(Ctx *ctx);
static Expr *additive(Ctx *ctx);
static Expr *multitive(Ctx *ctx);
static Expr *unary(Ctx *ctx);
static Expr *primary(Ctx *ctx);

static Stmt parse_scope_stmt(Ctx *ctx);
static Stmt print_stmt(Ctx *ctx);
static Stmt parse_expr_stmt(Ctx *ctx);
static Stmt parse_var_decl(Ctx *ctx);
static Stmt parse_if_stmt(Ctx *ctx);
static Stmt parse_while_stmt(Ctx *ctx);
static Stmt parse_for_stmt(Ctx *ctx);
static Stmt parse_stmt(Ctx *ctx);
void parse_stmts(Ctx *ctx);

static void synchronize(Ctx *ctx, TokenKind to);

void parse(Ctx *ctx)
{
    ctx->tok = TokensChain_back(&ctx->toks)->data;

    parse_stmts(ctx);
}

void parse_stmts(Ctx *ctx)
{
    while (ctx->tok && ctx->tok->kind != TK_eof)
    {
        Stmt stmt = parse_stmt(ctx);
        if (stmt.kind != STMT_none)
        {
            StmtList_iter it = stmt_list_push(&ctx->stmts);
            *it.ptr         = stmt;
        }
    }
}

static Stmt parse_stmt(Ctx *ctx)
{
    Stmt stmt = {};
    switch (ctx->tok->kind)
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
        default:
            stmt = parse_expr_stmt(ctx);
            break;
    }
    return stmt;
}

static Stmt parse_scope_stmt(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(&ctx->parser_arena);

    Stmt stmt = {.kind = STMT_scope, .data.scope_stmts = stmt_list_create(&ctx->parser_arena)};
    advance(ctx);
    StmtList *list = &stmt.data.scope_stmts;

    while (ctx->tok->kind != TK_rcurly && ctx->tok->kind != TK_eof)
    {
        Stmt scope_stmt = parse_stmt(ctx);
        if (scope_stmt.kind != STMT_none)
        {
            StmtList_iter it = stmt_list_push(list);
            *it.ptr         = scope_stmt;
        }
    }

    if (!skip_tok(ctx, TK_rcurly))
    {
        arena_rollback(&ctx->parser_arena, checkpoint);
        return (Stmt) {};
    }

    return stmt;
}

static Stmt parse_if_stmt(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(&ctx->parser_arena);

    bool err  = false;
    Stmt stmt = {.kind = STMT_if, .begin = ctx->tok};
    advance(ctx);
    if (!skip_tok(ctx, TK_lparen))
    {
        synchronize(ctx, TK_rparen);
        return (Stmt) {};
    }

    stmt.data.if_stmt.cond = expression(ctx);

    if (!skip_tok(ctx, TK_rparen))
    {
        err = true; // I think the best recovery is to just pretend ) is there
    }

    stmt.data.if_stmt.if_branch  = arena_alloc(&ctx->parser_arena, sizeof(Stmt));
    *stmt.data.if_stmt.if_branch = parse_stmt(ctx);

    if (stmt.data.if_stmt.if_branch->kind == STMT_none)
    {
        err = true;
    }

    if (ctx->tok->kind == TK_else)
    {
        advance(ctx);
        stmt.data.if_stmt.else_branch  = arena_alloc(&ctx->parser_arena, sizeof(Stmt));
        *stmt.data.if_stmt.else_branch = parse_stmt(ctx);
        if (stmt.data.if_stmt.else_branch->kind == STMT_none)
        {
            err = true;
        }
    }

    if (err)
    {
        arena_rollback(&ctx->parser_arena, checkpoint);
        return (Stmt) {};
    }

    return stmt;
}

static Stmt parse_while_stmt(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(&ctx->parser_arena);

    bool err  = false;
    Stmt stmt = {.begin = ctx->tok, .kind = STMT_while};

    advance(ctx);

    StmtWhile *w = &stmt.data.while_stmt;
    w->cond      = expression(ctx);
    if (w->cond == NULL)
    {
        err = true;
    }

    w->body  = arena_alloc(&ctx->parser_arena, sizeof(Stmt));
    *w->body = parse_stmt(ctx);

    if (w->body->kind == STMT_none)
    {
        err = true;
    }

    if (err)
    {
        arena_rollback(&ctx->parser_arena, checkpoint);
        return (Stmt) {};
    }

    return stmt;
}

static Stmt parse_for_stmt(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(&ctx->parser_arena);

    bool err = false;

    advance(ctx);
    if (!skip_tok(ctx, TK_lparen))
    {
        err = true;
    }

    Stmt scope = {
        .begin            = ctx->tok,
        .kind             = STMT_scope,
        .data.scope_stmts = stmt_list_create(&ctx->parser_arena),
    };

    Stmt init_stmt = {};
    if (ctx->tok->kind != TK_semicolon)
    {
        if (ctx->tok->kind == TK_var)
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
            StmtList_iter it = stmt_list_push(&scope.data.scope_stmts);
            *it.ptr         = init_stmt;
        }
    }
    else
    {
        advance(ctx);
    }

    Stmt loop = {.begin = ctx->tok, .kind = STMT_while};

    StmtWhile *w = &loop.data.while_stmt;
    if (ctx->tok->kind != TK_semicolon)
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
        w->cond        = arena_alloc(&ctx->parser_arena, sizeof(Expr));
        w->cond->kind  = EXPR_value;
        w->cond->value = (Value) {.type = T_bool, .boolean = true};
        advance(ctx);
    }

    Stmt update = {};
    if (ctx->tok->kind != TK_rparen)
    {
        update = (Stmt) {
            .begin     = ctx->tok,
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
    }

    w->body  = arena_alloc(&ctx->parser_arena, sizeof(Stmt));
    *w->body = (Stmt) {
        .begin            = ctx->tok,
        .kind             = STMT_scope,
        .data.scope_stmts = stmt_list_create(&ctx->parser_arena),
    };

    Stmt body_stmt = parse_stmt(ctx);
    if (body_stmt.kind == STMT_none)
    {
        err = true;
    }
    else
    {
        StmtList_iter it = stmt_list_push(&w->body->data.scope_stmts);
        *it.ptr         = body_stmt;
    }

    if (update.kind != STMT_none)
    {
        StmtList_iter it = stmt_list_push(&w->body->data.scope_stmts);
        *it.ptr         = update;
    }

    if (w->body->kind == STMT_none)
    {
        err = true;
    }

    StmtList_iter it = stmt_list_push(&scope.data.scope_stmts);
    *it.ptr         = loop;

    if (err)
    {
        arena_rollback(&ctx->parser_arena, checkpoint);
        return (Stmt) {};
    }

    return scope;
}

static Stmt parse_var_decl(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(&ctx->parser_arena);

    Stmt stmt = {.kind = STMT_var, .begin = ctx->tok};
    advance(ctx);
    stmt.data.var.name = ctx->tok;
    if (!skip_tok(ctx, TK_ident))
    {
        synchronize(ctx, TK_semicolon);
        return (Stmt) {};
    }

    if (ctx->tok->kind == TK_equal)
    {
        advance(ctx);
        stmt.data.var.rhs = expression(ctx);
    }

    if (!skip_tok(ctx, TK_semicolon))
    {
        arena_rollback(&ctx->parser_arena, checkpoint);
        return (Stmt) {};
    }
    return stmt;
}

static Stmt parse_expr_stmt(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(&ctx->parser_arena);

    bool err  = false;
    Stmt stmt = {.kind = STMT_expr, .begin = ctx->tok};

    stmt.data.expr = expression(ctx);
    stmt.end       = ctx->tok;

    if (stmt.data.expr == NULL)
    {
        err = true;
    }

    if (!skip_tok(ctx, TK_semicolon))
    {
        err = true;
        advance(ctx);
    }

    if (err)
    {
        arena_rollback(&ctx->parser_arena, checkpoint);
        return (Stmt) {};
    }

    return stmt;
}

static Stmt print_stmt(Ctx *ctx)
{
    ArenaCheckpoint checkpoint = arena_checkpoint(&ctx->parser_arena);

    Stmt stmt = {.kind = STMT_print, .begin = ctx->tok};
    advance(ctx);

    stmt.data.print_expr = expression(ctx);
    stmt.end             = ctx->tok;

    if (!skip_tok(ctx, TK_semicolon))
    {
        arena_rollback(&ctx->parser_arena, checkpoint);
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
    Token *lhs_begin = ctx->tok;
    Expr *left       = or_expr(ctx);

    if (ctx->tok->kind == TK_equal)
    {
        Token *lhs_end = ctx->tok - 1;

        if (left->kind != EXPR_var)
        {
            error_at_tok_range(ctx, *lhs_begin, *lhs_end, "not a variable");
        }
        Token *op = ctx->tok;
        advance(ctx);
        Expr *right = expression(ctx);
        return new_binary(ctx, left, right, *op);
    }

    return left;
}

static Expr *or_expr(Ctx *ctx)
{
    Expr *expr = and_expr(ctx);

    while (ctx->tok->kind == TK_or)
    {
        Token *op = ctx->tok;
        advance(ctx);
        Expr *right = and_expr(ctx);

        expr = new_binary(ctx, expr, right, *op);
    }

    return expr;
}

static Expr *and_expr(Ctx *ctx)
{
    Expr *expr = equality(ctx);

    while (ctx->tok->kind == TK_and)
    {
        Token *op = ctx->tok;
        advance(ctx);
        Expr *right = equality(ctx);

        expr = new_binary(ctx, expr, right, *op);
    }

    return expr;
}

static Expr *equality(Ctx *ctx)
{
    Expr *expr = comparison(ctx);

    while (check_any(ctx, TK_equal_equal, TK_not_equal))
    {
        Token *op = ctx->tok;
        advance(ctx);
        Expr *right = comparison(ctx);

        expr = new_binary(ctx, expr, right, *op);
    }

    return expr;
}

static Expr *comparison(Ctx *ctx)
{
    Expr *expr = additive(ctx);

    while (check_any(ctx, TK_lt, TK_lte, TK_gt, TK_gte))
    {
        Token *op = ctx->tok;
        advance(ctx);
        Expr *right = additive(ctx);

        expr = new_binary(ctx, expr, right, *op);
    }

    return expr;
}

static Expr *additive(Ctx *ctx)
{
    Expr *expr = multitive(ctx);

    while (check_any(ctx, TK_plus, TK_minus))
    {
        Token *op = ctx->tok;
        advance(ctx);
        Expr *right = multitive(ctx);

        expr = new_binary(ctx, expr, right, *op);
    }

    return expr;
}

static Expr *multitive(Ctx *ctx)
{
    Expr *expr = unary(ctx);

    while (check_any(ctx, TK_star, TK_slash))
    {
        Token *op = ctx->tok;
        advance(ctx);
        Expr *right = unary(ctx);

        expr = new_binary(ctx, expr, right, *op);
    }

    return expr;
}

static Expr *unary(Ctx *ctx)
{
    if (check_any(ctx, TK_not, TK_minus))
    {
        Token *op = ctx->tok;
        advance(ctx);
        Expr *exp = unary(ctx);
        Expr *new = new_unary(ctx, exp, *op);
        return new;
    }
    else
    {
        return primary(ctx);
    }
}

static Expr *primary(Ctx *ctx)
{
    if (ctx->tok->kind == TK_lparen)
    {
        advance(ctx);
        Expr *inside = expression(ctx);
        if (!skip_tok(ctx, TK_rparen))
            synchronize(ctx, TK_semicolon);

        return inside;
    }

    Expr *expr = NULL;

    if (ctx->tok->kind == TK_ident)
    {
        expr = new_var(ctx, *ctx->tok);
    }
    else
    {
        expr = new_literal(ctx, *ctx->tok);

        if (expr == NULL)
        {
            error_at_tok(ctx, *ctx->tok, "expected expression, but got %?", tok2str[ctx->tok->kind]);

            while (ctx->tok->kind != TK_semicolon && ctx->tok->kind != TK_eof)
            {
                advance(ctx);
            }
            return NULL;
        }
    }

    advance(ctx);
    return expr;
}

static void synchronize(Ctx *ctx, TokenKind to)
{
    while (ctx->tok->kind != to && ctx->tok->kind != TK_eof)
    {
        advance(ctx);
    }
    if (ctx->tok->kind == to)
        advance(ctx);
}
