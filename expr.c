#include "expr.h"
#include "clox.h"

Expr *new_binary(Ctx *ctx, Expr *left, Expr *right, Token op)
{
    Expr *ret         = arena_calloc(&ctx->parser_arena, sizeof(Expr));
    ret->kind         = EXPR_binary;
    ret->binary.left  = left;
    ret->binary.right = right;
    ret->binary.op    = op;

    return ret;
}

Expr *new_unary(Ctx *ctx, Expr *left, Token op)
{
    Expr *ret       = arena_calloc(&ctx->parser_arena, sizeof(Expr));
    ret->kind       = EXPR_unary;
    ret->unary.expr = left;
    ret->unary.op   = op;

    return ret;
}

static Expr *alloc_literal_expr(Ctx *ctx, LiteralKind litKind, Token tok)
{
    constexpr TypeKind map[] = {
        [LIT_bool] = T_bool,
        [LIT_num]  = T_num,
        [LIT_str]  = T_str,
        [LIT_nil]  = T_nil,
    };

    TypeKind type = map[litKind];

    Expr *ret       = arena_calloc(&ctx->parser_arena, sizeof(Expr));
    ret->kind       = EXPR_value;
    ret->value.tok  = tok;
    ret->value.type = type;

    switch (type)
    {
        case T_bool:
            ret->value.boolean = tok.data.boolean;
            break;
        case T_num:
            ret->value.number = tok.data.dbl;
            break;
        case T_str:
            ret->value.string = cgs_dup(tok.data.string, arena_allocator(&ctx->parser_arena));
            break;
        case T_nil:
            break;
        case T_none:
            unreachable();
    }

    return ret;
}

Expr *new_literal(Ctx *ctx, Token tok)
{
    switch (tok.kind)
    {
        case TK_nil:
            return alloc_literal_expr(ctx, LIT_nil, tok);
        case TK_str:
            return alloc_literal_expr(ctx, LIT_str, tok);
        case TK_num:
            return alloc_literal_expr(ctx, LIT_num, tok);
        case TK_float:
            return alloc_literal_expr(ctx, LIT_num, tok);
        case TK_true:
            return alloc_literal_expr(ctx, LIT_bool, tok);
        case TK_false:
            return alloc_literal_expr(ctx, LIT_bool, tok);
        default:
            return NULL;
    }
}

Expr *new_var(Ctx *ctx, Token tok)
{
    Expr *ret  = arena_calloc(&ctx->parser_arena, sizeof(Expr));
    ret->kind  = EXPR_var;
    ret->ident = tok;
    return ret;
}

Token expr_tok(Expr *expr)
{
    switch (expr->kind)
    {
        case EXPR_unary:
            return expr->unary.op;
        case EXPR_binary:
            return expr->binary.op;
        case EXPR_value:
            return expr->value.tok;
        case EXPR_var:
            return expr->ident;
    }
    unreachable();
}

CGS_StrView expr_view(Expr *expr)
{
    switch (expr->kind)
    {
        case EXPR_unary:
            return range_view(expr->unary.op.lexeme, expr_view(expr->unary.expr));
        case EXPR_binary:
            return range_view(expr_view(expr->binary.left), expr_view(expr->binary.right));
        case EXPR_value:
            return expr->value.tok.lexeme;
        case EXPR_var:
            return expr->ident.lexeme;
    }
    unreachable();
}
