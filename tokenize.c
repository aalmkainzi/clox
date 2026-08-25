#include <ctype.h>
#include <math.h>
#include "clox.h"
#include "token.h"

static bool is_ident_char(char c)
{
    return isalnum(c) || c == '_';
}

static Token scan_keyword(Ctx *ctx, const char *tok_start)
{
    Token ret = {.kind = TK_none};
    
#define else_if_is_kw(name, str) \
    else if(memcmp(tok_start, str, strlen(str)) == 0 && !(is_ident_char(tok_start[strlen(str)]))) \
    { \
        ret.kind = TK_##name; \
        ret.lexeme = cgs_strv(tok_start, 0, strlen(str)); \
    }
    
    if(0)
    {
        ;
    }
    KEYWORD_TOKEN_LIST(else_if_is_kw)
    
#undef else_if_is_kw
    
    if(ret.kind == TK_true || ret.kind == TK_false)
    {
        ret.data.boolean = ret.kind == TK_true;
    }
    
    return ret;
}

static Token scan_ident(Ctx *ctx, const char *tok_start)
{
    Token ret = {.kind = TK_none};
    
    if(isalpha(tok_start[0]) || tok_start[0] == '_')
    {
        int len = 1;
        while(is_ident_char(tok_start[len]))
            len += 1;
        
        CGS_StrView lexeme = cgs_strv(tok_start, 0, len);
        
        ret.kind = TK_ident;
        ret.lexeme = lexeme;
    }
    
    return ret;
}

static Token scan_punct(Ctx *ctx, const char *tok_start)
{
    Token ret = {.kind = TK_none};
    
#define else_if_is_punct(tok_kind, str) \
    else if(memcmp(tok_start, str, strlen(str)) == 0) \
    { \
        ret.kind = TK_##tok_kind; \
        ret.lexeme = cgs_strv(tok_start, 0, strlen(str)); \
    }
    
    if(0)
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

static Token scan_num(Ctx *ctx, const char *tok_start)
{
    Token ret = {.kind = TK_none};
    
    int scanned = 0;
    if(isnum(tok_start[0]))
    {
        int len = 1;
        while(isnum(tok_start[len]))
            len += 1;
        
        CGS_StrView lexeme = cgs_strv(tok_start, 0, len);
        
        ret.kind = TK_num;
        ret.lexeme = lexeme;
        
        char *end;
        unsigned long long ull = strtoull(tok_start, &end, 10);
        ret.data.dbl = ull;
        
        assert(end == lexeme.chars + lexeme.len);
        
        scanned = len;
    }
    
    if(tok_start[scanned] == '.' && isnum(tok_start[scanned + 1]))
    {
        uint64_t integer = 0;
        if(ret.kind == TK_num)
        {
            integer = ret.data.dbl;
        }
        
        int len = 2;
        while(isnum(tok_start[scanned + len]))
            len += 1;
        
        CGS_StrView lexeme = cgs_strv(tok_start, 0, scanned + len);
        
        ret.kind = TK_float;
        ret.lexeme = lexeme;
        
        char *end;
        uint64_t decimal = strtoull(tok_start + scanned + 1, &end, 10);
        uint64_t div = ceil(log10(decimal));
        
        ret.data.dbl = integer + (decimal / pow(10, div));
        
        assert(end == lexeme.chars + lexeme.len);
    }
    
    return ret;
}

static Token scan_str(Ctx *ctx, const char *tok_start)
{
    Token ret = {.kind = TK_none};
    
    if(tok_start[0] == '\"')
    {
        int len = 2;
        while(tok_start[len - 1] != '\"')
        {
            if(tok_start[len - 1] == '\0')
            {
                report_error(ctx, tok_start, "unterminated string literal");
            }
            len += 1;
        }
        
        ret.lexeme = cgs_strv(tok_start, 0, len);
        ret.kind = TK_str;
        ret.data.string = cgs_dstr_init_from(cgs_strv(ret.lexeme, 1, ret.lexeme.len - 1));
    }
    
    return ret;
}

#define try_scan_token(func)                                    \
do {                                                            \
    Token scanned_tok = (func)(ctx, tok_start);                 \
    if(scanned_tok.kind != TK_none)                             \
    {                                                           \
        if((func) == scan_str)                                  \
        {                                                       \
            ctx->lineno += cgs_count(scanned_tok.lexeme, "\n"); \
        }                                                       \
        return scanned_tok;                                     \
    }                                                           \
} while(0)

Token scan_one_token(Ctx *ctx, const char *tok_start)
{
    try_scan_token(scan_keyword);
    try_scan_token(scan_ident);
    try_scan_token(scan_punct);
    try_scan_token(scan_num);
    try_scan_token(scan_str);
    
    return (Token){.kind = TK_none};
}

void tokenize(Ctx *ctx)
{
    ctx->toks = TokenList_init();
    TokenList *toks = &ctx->toks;
    
    CGS_StrView src_walk = cgs_strv(ctx->src);
    
    while(true)
    {
        while(src_walk.len > 0 && isspace(src_walk.chars[0]))
        {
            if(src_walk.chars[0] == '\n')
                ctx->lineno += 1;
            
            src_walk.chars += 1;
            src_walk.len -= 1;
        }
        
        if(src_walk.len == 0)
        {
            TokenList_push(toks, (Token){.kind = TK_eof, .lexeme = src_walk});
            return;
        }
        
        const char *tok_start = src_walk.chars;
        
        Token scanned_tok = scan_one_token(ctx, tok_start);
        
        if(scanned_tok.kind == TK_none)
        {
            report_error(ctx, src_walk, "unexpected character %?", src_walk.chars[0]);
            src_walk.chars += 1;
            src_walk.len -= 1;
        }
        else
        {
            TokenList_push(toks, scanned_tok);
            
            src_walk.chars += scanned_tok.lexeme.len;
            src_walk.len   -= scanned_tok.lexeme.len;
        }
    }
}

// utils

void skip_tok(Ctx *ctx, TokenKind kind)
{
    if(ctx->cur->kind != kind)
        report_tok_error(ctx, ctx->cur, "expected %?, but got %?", tok2str[kind], tok2str[ctx->cur->kind]);
    
    ctx->cur += 1;
}

bool check_any_(Ctx *ctx, TokenKind *kinds, int n_kinds)
{
    for(int i = 0 ; i < n_kinds ; i++)
    {
        if(ctx->cur->kind == kinds[i])
            return true;
    }
    return false;
}

Token *advance(Ctx *ctx)
{
    Token *ret = ctx->cur;
    ctx->cur += 1;
    return ret;
}

void free_tokens(TokenList *toks)
{
    for(ptrdiff_t i = 0 ; i < toks->size ; i++)
    {
        Token *tok = &toks->data[i];
        if(tok->kind == TK_str)
        {
            cgs_dstr_deinit(&tok->data.string);
        }
    }
    TokenList_drop(toks);
}

Token *copy_token(Ctx *ctx, Token *token)
{
    Token *copy = malloc(sizeof(Token));
    *copy = *token;
    
    if(copy->kind == TK_str)
    {
        copy->data.string = cgs_dup(copy->data.string);
    }
    
    return copy;
}

// only usable with tokens from copy_token
void free_token(Token *tok)
{
    if(tok->kind == TK_str)
    {
        cgs_dstr_deinit(&tok->data.string);
    }
    free(tok);
}

// "a" "bcdefg" "h"
// -> "abcdefgh"
CGS_StrView range_view(CGS_StrView left, CGS_StrView right)
{
    CGS_StrView ret = left;
    ret.len += (right.chars - left.chars) + right.len;
    return ret;
}