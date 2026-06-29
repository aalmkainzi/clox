#ifndef CLOX_H
#define CLOX_H

#include "cgs/cgs.h"
#include "token.h"
#include "expr.h"

typedef struct Ctx
{
    TokenList toks;
    CGS_StrView file_name;
    CGS_StrView src;
    int lineno;
    
    Token *cur;
} Ctx;

void tokenize(Ctx *ctx);
Expr *parse_expr(Ctx *ctx);

void report_error(CGS_StrView file_name, CGS_StrView src, const CGS_StrView error_loc, CGS_StrView message);

void skip_(Ctx *ctx, TokenKind kind); // skip `kind` token
Token *advance_(Ctx *ctx); // returns cur, advances cur
bool check_any_(Ctx *ctx, TokenKind *kinds, int n_kinds);

#define error_at(tok_start, msg) \
report_error(ctx->file_name, ctx->src, cgs_strv(tok_start, 0, 1), cgs_strv(msg))

#define skip(kind) \
skip_(ctx, kind)

#define advance() \
advance_(ctx)

#define check_any(...) \
check_any_(ctx, (TokenKind[]){__VA_ARGS__}, sizeof((TokenKind[]){__VA_ARGS__})/sizeof(TokenKind))

#endif
