#ifndef CLOX_H
#define CLOX_H

#include "cgs/cgs.h"
#include "expr.h"
#include "token.h"

typedef struct Ctx
{
    TokenList toks;
    CGS_StrView file_name;
    CGS_StrView src;
    int lineno;

    Token *cur;

    bool error;
} Ctx;

Token *copy_token(Token *token);
void free_token(Token *tok);

CGS_StrView range_view(CGS_StrView left, CGS_StrView right);

void tokenize(Ctx *ctx);
Expr *parse(Ctx *ctx);

void free_expr(Expr *expr);
void free_tokens(TokenList *toks);

void report_error_(Ctx *ctx, const CGS_StrView error_loc, CGS_DStr message);
void report_tok_error_(Ctx *ctx, Token *tok, CGS_DStr message);
void report_tok_range_error_(Ctx *ctx, Token *begin_tok, Token *end_tok, CGS_DStr message);

void skip_tok(Ctx *ctx, TokenKind kind); // skip `kind` token
Token *advance(Ctx *ctx);                // dequeue
bool check_any_(Ctx *ctx, TokenKind *kinds, int n_kinds);
bool consume_any_(Ctx *ctx, TokenKind *kinds, int n_kinds);

extern char error_message_buf[256];
extern CGS_OneUseAllocator error_message_allocator;

#define report_error(ctx, loc, msg, ...) report_error_(ctx, cgs_strv(loc, 0, 1), cgs_asprintf(msg, __VA_ARGS__))

#define report_tok_error(ctx, tok, msg, ...) report_tok_error_(ctx, tok, cgs_asprintf(msg, __VA_ARGS__))

#define report_tok_range_error(ctx, begin_tok, end_tok, msg, ...) report_tok_range_error_(ctx, begin_tok, end_tok, cgs_asprintf(msg, __VA_ARGS__))

#define check_any(ctx, ...) check_any_(ctx, (TokenKind[]) {__VA_ARGS__}, sizeof((TokenKind[]) {__VA_ARGS__}) / sizeof(TokenKind))

#define check_tok(ctx, kind) check_any_(ctx, (TokenKind[1]) {kind}, 1)

#define consume_any(ctx, ...) consume_any_(ctx, (TokenKind[]) {__VA_ARGS__}, sizeof((TokenKind[]) {__VA_ARGS__}) / sizeof(TokenKind))

#define consume_tok(ctx, kind) consume_any_(ctx, (TokenKind[1]) {kind}, 1)

#endif
