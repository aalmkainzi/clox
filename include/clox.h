#ifndef CLOX_H
#define CLOX_H

#include "cgs/cgs.h"
#include "expr.h"
#include "linemap.h"
#include "stmt.h"
#include "token.h"
#include "var.h"

c_declare_vec(StrViews, CGS_StrView);
c_declare_hashmap(ExpectedErrors, int, StrViews);

typedef struct Scope
{
    struct Scope *up;
    VarMap vars;
    Arenas_iter arena;
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

    Arena parser_arena;

    struct
    {
        ExpectedErrors expected_errors;
        StrViews expected_outputs;
        unsigned int verified_outputs;
    } test_data;

    LineMap linemap;

    bool verbose     : 1;
    bool test        : 1;
    bool passed_test : 1;
    bool error       : 1;
    bool repl        : 1;
} Ctx;
// clang-format on

int parse_tests(Ctx *ctx);

// "abc" ... "h"
// -> "abcdefgh"
CGS_StrView range_view(CGS_StrView begin, CGS_StrView end);

void tokenize(Ctx *ctx);
void parse(Ctx *ctx);

void free_tokens(TokenList *toks);

[[nodiscard("syncronize if this returns false")]]
bool skip_tok(Ctx *ctx, TokenKind kind); // skip `kind` token
void advance(Ctx *ctx);                  // dequeue
Token *peek(Ctx *ctx);
bool check_any_(Ctx *ctx, TokenKind *kinds, int n_kinds);
bool consume_any_(Ctx *ctx, TokenKind *kinds, int n_kinds);

void init_scope(Scope *scope);
void free_scope(Scope *scope);

#define check_any(ctx, ...) \
    check_any_(ctx, (TokenKind[]) {__VA_ARGS__}, sizeof((TokenKind[]) {__VA_ARGS__}) / sizeof(TokenKind))

#define check_tok(ctx, kind) check_any_(ctx, (TokenKind[1]) {kind}, 1)

#define consume_any(ctx, ...) \
    consume_any_(ctx, (TokenKind[]) {__VA_ARGS__}, sizeof((TokenKind[]) {__VA_ARGS__}) / sizeof(TokenKind))

#define consume_tok(ctx, kind) consume_any_(ctx, (TokenKind[1]) {kind}, 1)

#define T StrViews, CGS_StrView
#define i_eq(a, b) cgs_equal(*a, *b)
#define i_declared
#include "stc/vec.h"

#define T ExpectedErrors, int, StrViews
#define i_valdrop StrViews_drop
#define i_valclone StrViews_clone
#define i_no_clone
#define i_declared
#include "stc/hashmap.h"

#endif
