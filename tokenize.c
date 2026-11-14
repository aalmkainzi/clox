#include "clox.h"
#include "error.h"
#include "token.h"
#include <ctype.h>
#include <math.h>

#define i_implement
#define i_no_clone
#define i_keydrop free_token
#define T TokenList, Token
#include "grow_list.h"

#define i_implement
#define i_body
#define i_no_clone
#define i_keydrop TokenList_drop
#define i_aux Arena *
#define i_malloc(n) arena_alloc(self->aux, n)
#define i_free(p, n) ((void)0)
#define i_realloc(p, old_n, new_n) assert(0)
#define T TokensChain, TokenList
#include "stc/list.h"

static bool is_ident_char(char c)
{
    return isalnum(c) || c == '_';
}

static Token scan_keyword(Ctx *ctx, const char *tok_start, bool *err)
{
    (void)ctx;
    (void)err;

    Token ret = {.kind = TK_none};

#define else_if_is_kw(name, str)                                                                   \
    else if (memcmp(tok_start, str, strlen(str)) == 0 && !(is_ident_char(tok_start[strlen(str)]))) \
    {                                                                                              \
        ret.kind   = TK_##name;                                                                    \
        ret.lexeme = cgs_strv(tok_start, 0, strlen(str));                                          \
    }

    if (0)
    {
        ;
    }
    KEYWORD_TOKEN_LIST(else_if_is_kw)

#undef else_if_is_kw

    if (ret.kind == TK_true || ret.kind == TK_false)
    {
        ret.data.boolean = ret.kind == TK_true;
    }

    return ret;
}

static Token scan_ident(Ctx *ctx, const char *tok_start, bool *err)
{
    (void)ctx;
    (void)err;

    Token ret = {.kind = TK_none};

    if (isalpha(tok_start[0]) || tok_start[0] == '_')
    {
        unsigned int len = 1;
        while (is_ident_char(tok_start[len]))
            len += 1;

        CGS_StrView lexeme = cgs_strv(tok_start, 0, len);

        ret.kind   = TK_ident;
        ret.lexeme = lexeme;
    }

    return ret;
}

static Token scan_punct(Ctx *ctx, const char *tok_start, bool *err)
{
    (void)ctx;
    (void)err;

    Token ret = {.kind = TK_none};

#define else_if_is_punct(tok_kind, str)                   \
    else if (memcmp(tok_start, str, strlen(str)) == 0)    \
    {                                                     \
        ret.kind   = TK_##tok_kind;                       \
        ret.lexeme = cgs_strv(tok_start, 0, strlen(str)); \
    }

    if (0)
    {
        ;
    }
    TWO_CHAR_PUNCT_TOKEN_LIST(else_if_is_punct)
    ONE_CHAR_PUNCT_TOKEN_LIST(else_if_is_punct)

#undef else_if_is_punct

    return ret;
}

static bool isnum(char c)
{
    return c >= '0' && c <= '9';
}

static Token scan_num(Ctx *ctx, const char *tok_start, bool *err)
{
    (void)ctx;
    (void)err;

    Token ret = {.kind = TK_none};

    unsigned int scanned = 0;
    if (isnum(tok_start[0]))
    {
        unsigned int len = 1;
        while (isnum(tok_start[len]))
            len += 1;

        CGS_StrView lexeme = cgs_strv(tok_start, 0, len);

        ret.kind   = TK_num;
        ret.lexeme = lexeme;

        char *end;
        uint64_t integer = strtoull(tok_start, &end, 10);
        ret.data.dbl     = (double)integer;

        assert(end == lexeme.chars + lexeme.len);

        scanned = len;
    }

    if (tok_start[scanned] == '.' && isnum(tok_start[scanned + 1]))
    {
        unsigned int len = 2;
        while (isnum(tok_start[scanned + len]))
            len += 1;

        CGS_StrView lexeme = cgs_strv(tok_start, 0, scanned + len);

        ret.kind   = TK_float;
        ret.lexeme = lexeme;

        char *end;
        double decimal = (double)strtoull(tok_start + scanned + 1, &end, 10);
        double div     = ceil(log10(decimal));

        ret.data.dbl += (decimal / pow(10, div));

        assert(end == lexeme.chars + lexeme.len);
    }

    return ret;
}

static char handle_escape_sequence(char c)
{
    switch (c)
    {
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        case '\"':
            return '\"';
    }
    return -1;
}

Token scan_str(Ctx *ctx, const char *tok_start, bool *err)
{
    Token ret = {.kind = TK_none};

    if (tok_start[0] != '\"')
    {
        return ret;
    }

    CGS_DStr string_data = cgs_dstr_init();

    unsigned int it = 2;

    while (tok_start[it - 1] != '\"')
    {
        if (tok_start[it - 1] == '\0')
        {
            error_at(ctx, cgs_strv(tok_start, 0, 1), "unterminated string literal");
            *err = true;
            cgs_dstr_deinit(&string_data);
            return ret;
        }

        if (tok_start[it - 1] == '\\')
        {
            it += 1;
            char esc_code = handle_escape_sequence(tok_start[it - 1]);
            if (esc_code != -1)
            {
                cgs_putc(&string_data, esc_code);
                it += 1;
                continue;
            }
        }

        cgs_putc(&string_data, tok_start[it - 1]);
        it += 1;
    }

    ret.lexeme      = cgs_strv(tok_start, 0, it);
    ret.kind        = TK_str;
    ret.data.string = string_data;

    return ret;
}

#define try_scan_token(func)                             \
    do                                                   \
    {                                                    \
        Token scanned_tok = (func)(ctx, tok_start, err); \
        if (scanned_tok.kind != TK_none)                 \
        {                                                \
            return scanned_tok;                          \
        }                                                \
        else if (*err)                                   \
        {                                                \
            return scanned_tok;                          \
        }                                                \
    } while (0)

Token scan_one_token(Ctx *ctx, const char *tok_start, bool *err)
{
    try_scan_token(scan_keyword);
    try_scan_token(scan_ident);
    try_scan_token(scan_punct);
    try_scan_token(scan_num);
    try_scan_token(scan_str);

    return (Token) {.kind = TK_none};
}

void tokenize(Ctx *ctx)
{
    TokenList *toks = TokensChain_push(&ctx->toks, TokenList_init(ctx->parse_arena));

    CGS_StrView src_walk = cgs_strv(ctx->src);

    while (true)
    {
        while (src_walk.len > 0 && isspace(src_walk.chars[0]))
        {
            src_walk.chars += 1;
            src_walk.len -= 1;
        }
        if (cgs_starts_with(src_walk, "//"))
        {
            src_walk.chars += 2;
            src_walk.len -= 2;
            while (src_walk.len > 0 && src_walk.chars[0] != '\n')
            {
                src_walk.chars += 1;
                src_walk.len -= 1;
            }
            continue;
        }

        if (src_walk.len == 0)
        {
            TokenList_iter it = TokenList_push(toks);
            *it.ref           = (Token) {.kind = TK_eof, .lexeme = src_walk};
            return;
        }

        const char *tok_start = src_walk.chars;

        bool err          = false;
        Token scanned_tok = scan_one_token(ctx, tok_start, &err);

        if (err)
        {
            src_walk.chars += 1;
            src_walk.len -= 1;
        }
        else if (scanned_tok.kind == TK_none)
        {
            error_at(ctx, cgs_strv(src_walk, 0, 1), "unexpected character %?", src_walk.chars[0]);
            src_walk.chars += 1;
            src_walk.len -= 1;
        }

        else
        {
            *TokenList_push(toks).ref = scanned_tok;

            src_walk.chars += scanned_tok.lexeme.len;
            src_walk.len -= scanned_tok.lexeme.len;
        }
    }
}

// utils

[[nodiscard("synchronize if this returns false")]]
bool skip_tok(Ctx *ctx, TokenKind kind)
{
    if (curtok(ctx)->kind != kind)
    {
        error_at_tok(ctx, *curtok(ctx), "expected %?, but got %?", tok2str[kind], tok2str[curtok(ctx)->kind]);
        return false;
    }

    advance(ctx);
    return true;
}

bool check_any_(Ctx *ctx, TokenKind *kinds, int n_kinds)
{
    for (int i = 0; i < n_kinds; i++)
    {
        if (curtok(ctx)->kind == kinds[i])
            return true;
    }
    return false;
}

void advance(Ctx *ctx)
{
    if (curtok(ctx)->kind == TK_eof)
        return;

    TokenList_next(&ctx->tok);
}

bool consume_any_(Ctx *ctx, TokenKind *kinds, int n_kinds)
{
    if (check_any_(ctx, kinds, n_kinds))
    {
        advance(ctx);
        return true;
    }
    return false;
}

void free_token(Token *tok)
{
    assert(!tok->seperate);
    if (tok->kind == TK_str)
    {
        cgs_dstr_deinit(&tok->data.string);
    }
}

CGS_StrView range_view(CGS_StrView begin, CGS_StrView end)
{
    CGS_StrView ret = begin;
    ret.len         = (unsigned int)(end.chars - begin.chars) + end.len;
    return ret;
}
