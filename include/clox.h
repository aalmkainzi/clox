#ifndef CLOX_H
#define CLOX_H

#include "cgs/cgs.h"
#include "expr.h"
#include "linemap.h"
#include "stmt.h"
#include "token.h"
#include "var.h"

declare_hashmap(ExpectedErrors, int, CGS_StrView);

typedef struct Scope
{
    struct Scope *up;
    VarMap vars;
} Scope;

// clang-format off
typedef struct Ctx
{
    CGS_StrView file_name;
    CGS_StrView src;

    TokensChain toks;

    Token *tok;

    Scope *scope;
    StmtList stmts;

    ExpectedErrors expected_errors;
    LineMap linemap;

    bool verbose : 1;
    bool test    : 1;
    bool error   : 1;
    bool repl    : 1;
} Ctx;
// clang-format on

void clox_init();
int parse_tests(Ctx *ctx);

// "abc" ... "h"
// -> "abcdefgh"
CGS_StrView range_view(CGS_StrView begin, CGS_StrView end);

void tokenize(Ctx *ctx);
void parse(Ctx *ctx);

void free_expr(Expr *expr);
void free_tokens(TokenList *toks);

void report_error_(Ctx *ctx, const CGS_StrView error_loc);
void report_tok_error_(Ctx *ctx, Token *tok);
void report_tok_range_error_(Ctx *ctx, Token *begin_tok, Token *end_tok);
void error_message_();

[[nodiscard("syncronize if this returns false")]]
bool skip_tok(Ctx *ctx, TokenKind kind); // skip `kind` token
void advance(Ctx *ctx);                  // dequeue
Token *peek(Ctx *ctx);
bool check_any_(Ctx *ctx, TokenKind *kinds, int n_kinds);
bool consume_any_(Ctx *ctx, TokenKind *kinds, int n_kinds);

void init_scope(Scope *scope);
void free_scope(Scope *scope);

extern _Thread_local char error_message_buf[1024];

#define report_error(ctx, loc, msg, ...) \
    (cgs_sprintf(error_message_buf, msg, __VA_ARGS__), report_error_(ctx, cgs_strv(loc)))

#define report_tok_error(ctx, tok, msg, ...) \
    (cgs_sprintf(error_message_buf, msg, __VA_ARGS__), report_tok_error_(ctx, tok))

#define report_tok_range_error(ctx, begin_tok, end_tok, msg, ...) \
    (cgs_sprintf(error_message_buf, msg, __VA_ARGS__), report_tok_range_error_(ctx, begin_tok, end_tok))

#define check_any(ctx, ...) \
    check_any_(ctx, (TokenKind[]) {__VA_ARGS__}, sizeof((TokenKind[]) {__VA_ARGS__}) / sizeof(TokenKind))

#define check_tok(ctx, kind) check_any_(ctx, (TokenKind[1]) {kind}, 1)

#define consume_any(ctx, ...) \
    consume_any_(ctx, (TokenKind[]) {__VA_ARGS__}, sizeof((TokenKind[]) {__VA_ARGS__}) / sizeof(TokenKind))

#define consume_tok(ctx, kind) consume_any_(ctx, (TokenKind[1]) {kind}, 1)

#define error_message(msg, ...) (cgs_sprintf(error_message_buf, msg, __VA_ARGS__), error_message_())

#define i_declared
#define T ExpectedErrors, int, CGS_StrView
#include "stc/hashmap.h"

#endif
