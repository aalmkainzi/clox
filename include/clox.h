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
    ArenaCheckpoint exit_checkpoint;
} Scope;

// clang-format off
typedef struct Ctx
{
    CGS_StrView file_name;
    CGS_StrView src;
    LineMap linemap; // mapping from line number to file offset

    TokensChain toks;
    TokenList_iter tok;

    Scope *scope;
    StmtList stmts;

    Arena *parse_arena;
    Arena *eval_arena;
    CGS_Allocator *parse_allocator;
    CGS_Allocator *eval_allocator;

    struct
    {
        ExpectedErrors expected_errors;
        StrViews expected_outputs;
        unsigned int verified_outputs;
    } test_data;

    bool verbose     : 1;
    bool test        : 1;
    bool failed_test : 1;
    bool error       : 1;
    bool repl        : 1;
} Ctx;
// clang-format on

#define curtok(ctx) (ctx)->tok.ref

// "abc" ... "h"
// -> "abcdefgh"
CGS_StrView range_view(CGS_StrView begin, CGS_StrView end);

void tokenize(Ctx *ctx);
void parse(Ctx *ctx);

void free_tokens(TokenList *toks);

[[nodiscard("syncronize if this returns false")]]
bool skip_tok(Ctx *ctx, TokenKind kind); // skip `kind` token. reports error if current token is not `kind`
void advance(Ctx *ctx);
bool check_any_(Ctx *ctx, TokenKind *kinds, int n_kinds);
bool consume_any_(Ctx *ctx, TokenKind *kinds, int n_kinds);

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
