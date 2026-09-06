#include "clox.h"
#include "expr.h"
#include "stmt.h"
#include "token.h"
#include "var.h"

static Expr *expression(Ctx *ctx);
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
void parse_stmts(Ctx *ctx);

void init_scope(Scope *scope);
void synchronize(Ctx *ctx, TokenKind to);

void parse(Ctx *ctx)
{
    ctx->scope = calloc(1, sizeof(Scope));
    init_scope(ctx->scope);

    ctx->tok = TokensChain_back(&ctx->toks)->data;

    parse_stmts(ctx);
}

void parse_repl(Ctx *ctx)
{
    assert(ctx->scope != NULL);
    ctx->tok = TokensChain_back(&ctx->toks)->data;
    parse_stmts(ctx);
}

void parse_stmts(Ctx *ctx)
{
    while (ctx->tok && ctx->tok->kind != TK_eof)
    {
        Stmt stmt;
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
                Scope *new_scope = calloc(1, sizeof(Scope));
                init_scope(new_scope);
                new_scope->up = ctx->scope;
                ctx->scope    = new_scope;
                break;
            case TK_rcurly:
                if (ctx->scope->up == NULL)
                {
                    report_tok_error(ctx, ctx->tok, "unexpected }");
                }
                else
                {
                    free_scope(ctx->scope);
                    ctx->scope = ctx->scope->up;
                }
                break;
            default:
                stmt = expr_stmt(ctx);
                break;
        }
        StmtList_push(&ctx->scope->stmts, stmt);
    }
}

void init_scope(Scope *scope)
{
    scope->stmts = StmtList_init();
    scope->vars  = VarMap_init();
    scope->up    = NULL;
}

static Stmt var_decl(Ctx *ctx)
{
    Stmt stmt = {.kind = STMT_var, .begin = ctx->tok};
    advance(ctx);
    stmt.var.name = ctx->tok;
    if (!skip_tok(ctx, TK_ident))
    {
        synchronize(ctx, TK_semicolon);
        return (Stmt){};
    }

    if (ctx->tok->kind == TK_equal)
    {
        advance(ctx);
        stmt.var.rhs = expression(ctx);
    }

    if (!skip_tok(ctx, TK_semicolon))
    {
        return (Stmt){};
    }
    return stmt;
}

static Stmt expr_stmt(Ctx *ctx)
{
    Stmt stmt = {.kind = STMT_expr, .begin = ctx->tok};

    stmt.expr = expression(ctx);
    stmt.end  = ctx->tok;

    if (!skip_tok(ctx, TK_semicolon))
    {
        return (Stmt){};
    }

    return stmt;
}

static Stmt print_stmt(Ctx *ctx)
{
    Stmt stmt = {.kind = STMT_print, .begin = ctx->tok};
    advance(ctx);

    stmt.print_expr = expression(ctx);
    stmt.end        = ctx->tok;

    if (!skip_tok(ctx, TK_semicolon))
    {
        return (Stmt){};
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
    Expr *left       = equality(ctx);

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
        skip_tok(ctx, TK_rparen);
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

void synchronize(Ctx *ctx, TokenKind to)
{
    while(ctx->tok->kind != to && ctx->tok->kind != TK_eof)
    {
        advance(ctx);
    }
}
