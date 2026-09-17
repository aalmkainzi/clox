#include "clox.h"
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

static Stmt print_stmt(Ctx *ctx);
static Stmt expr_stmt(Ctx *ctx);
static Stmt var_decl(Ctx *ctx);
static Stmt if_stmt(Ctx *ctx);
static Stmt while_stmt(Ctx *ctx);
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
            StmtList_push(&ctx->stmts, stmt);
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
            stmt = var_decl(ctx);
            break;
        case TK_lcurly:
            advance(ctx);
            stmt.kind      = STMT_scope;
            StmtList *list = &stmt.data.scope_stmts;
            while (ctx->tok->kind != TK_rcurly && ctx->tok->kind != TK_eof)
            {
                StmtList_push(list, parse_stmt(ctx));
            }
            if (!skip_tok(ctx, TK_rcurly))
            {
                ; // nothing to do. eof reached, error reported
            }
            break;
        case TK_if:
            stmt = if_stmt(ctx);
            break;
        case TK_while:
            stmt = while_stmt(ctx);
            break;
        default:
            stmt = expr_stmt(ctx);
            break;
    }
    return stmt;
}

static Stmt if_stmt(Ctx *ctx)
{
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
        ; // I think the best recovery is to just pretend ) is there
    }

    stmt.data.if_stmt.if_branch  = calloc(1, sizeof(Stmt));
    *stmt.data.if_stmt.if_branch = parse_stmt(ctx);

    if (stmt.data.if_stmt.if_branch->kind == STMT_none)
    {
        free_stmt(stmt.data.if_stmt.if_branch);
        free(stmt.data.if_stmt.if_branch);
        return (Stmt){};
    }
    
    if (ctx->tok->kind == TK_else)
    {
        advance(ctx);
        stmt.data.if_stmt.else_branch  = calloc(1, sizeof(Stmt));
        *stmt.data.if_stmt.else_branch = parse_stmt(ctx);
        if (stmt.data.if_stmt.else_branch->kind == STMT_none)
        {
            free_stmt(stmt.data.if_stmt.if_branch);
            free(stmt.data.if_stmt.if_branch);
            return (Stmt){};
        }
    }
    return stmt;
}

static Stmt while_stmt(Ctx *ctx)
{
    Stmt stmt = {.begin = ctx->tok, .kind = STMT_while};
    advance(ctx);
    
    stmt.data.while_stmt.cond = expression(ctx);
    stmt.data.while_stmt.body = calloc(1, sizeof(Stmt));
    *stmt.data.while_stmt.body = parse_stmt(ctx);
    
    if (stmt.data.while_stmt.body->kind == STMT_none)
    {
        
    }
    
    return stmt;
}

static Stmt var_decl(Ctx *ctx)
{
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
        return (Stmt) {};
    }
    return stmt;
}

static Stmt expr_stmt(Ctx *ctx)
{
    Stmt stmt = {.kind = STMT_expr, .begin = ctx->tok};

    stmt.data.expr = expression(ctx);
    stmt.end       = ctx->tok;

    if (!skip_tok(ctx, TK_semicolon))
    {
        free_expr(stmt.data.expr);
        return (Stmt) {};
    }

    return stmt;
}

static Stmt print_stmt(Ctx *ctx)
{
    Stmt stmt = {.kind = STMT_print, .begin = ctx->tok};
    advance(ctx);

    stmt.data.print_expr = expression(ctx);
    stmt.end             = ctx->tok;

    if (!skip_tok(ctx, TK_semicolon))
    {
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
            report_tok_range_error(ctx, lhs_begin, lhs_end, "not a variable");
        }
        Token *op = ctx->tok;
        advance(ctx);
        Expr *right = expression(ctx);
        return new_binary(left, right, op);
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

        expr = new_binary(expr, right, op);
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

        expr = new_binary(expr, right, op);
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

        expr = new_binary(expr, right, op);
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

        expr = new_binary(expr, right, op);
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

        expr = new_binary(expr, right, op);
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

        expr = new_binary(expr, right, op);
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
        Expr *new = new_unary(exp, op);
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
        expr = new_var(ctx->tok);
    }
    else
    {
        expr = new_literal(ctx->tok);

        if (expr == NULL)
        {
            report_tok_error(ctx, ctx->tok, "expected expression, but got '%?'", tok2str[ctx->tok->kind]);

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
}
