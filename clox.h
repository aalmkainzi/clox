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
    
    bool error;
} Ctx;

void tokenize(Ctx *ctx);
Expr *parse(Ctx *ctx);
void free_expr(Expr *expr);
void free_tokens(TokenList *toks);

void report_error(CGS_StrView file_name, CGS_StrView src, const CGS_StrView error_loc, CGS_DStr message);
void report_tok_error(CGS_StrView file_name, CGS_StrView src, Token *tok, CGS_DStr message);

void skip_(Ctx *ctx, TokenKind kind); // skip `kind` token
Token *advance_(Ctx *ctx); // returns cur, advances cur
bool check_any_(Ctx *ctx, TokenKind *kinds, int n_kinds);

#define error_at(loc, msg, ...) \
(ctx->error = true, report_error(ctx->file_name, ctx->src, cgs_strv(loc, 0, 1), cgs_asprintf(msg, __VA_ARGS__)))

#define error_at_tok(tok, msg, ...) \
(ctx->error = true, report_tok_error(ctx->file_name, ctx->src, tok, cgs_asprintf(msg, __VA_ARGS__)))

#define skip(kind) \
skip_(ctx, kind)

#define advance() \
advance_(ctx)

#define check_any(...) \
check_any_(ctx, (TokenKind[]){__VA_ARGS__}, sizeof((TokenKind[]){__VA_ARGS__})/sizeof(TokenKind))

#endif
