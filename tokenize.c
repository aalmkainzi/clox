#include "clox.h"
#include <ctype.h>
#include <math.h>

#define error_here(tok_start, msg) \
error_at(ctx->file_name, ctx->src, cgs_strv(tok_start, 0, 1), cgs_strv(msg))

typedef struct TokenizeCtx
{
    CGS_StrView file_name;
    CGS_StrView src;
    int lineno;
} TokenizeCtx;

Token scan_keyword(TokenizeCtx *ctx, const char *tok_start)
{
    Token ret = {.kind = TK_none};
    
#define else_if_is_kw(name, str) \
    else if(memcmp(tok_start, str, strlen(str)) == 0 && !isalnum(tok_start[strlen(str)])) \
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
    
    return ret;
}

Token scan_ident(TokenizeCtx *ctx, const char *tok_start)
{
    Token ret = {.kind = TK_none};
    
    if(isalpha(tok_start[0]) || tok_start[0] == '_')
    {
        int len = 1;
        while(isalnum((unsigned char) tok_start[len]) || tok_start[len] == '_')
            len += 1;
        
        CGS_StrView lexeme = cgs_strv(tok_start, 0, len);
        
        ret.kind = TK_ident;
        ret.lexeme = lexeme;
    }
    
    return ret;
}

Token scan_punct(TokenizeCtx *ctx, const char *tok_start)
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

Token scan_num(TokenizeCtx *ctx, const char *tok_start)
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
        ret.data.integer = strtoull(tok_start, &end, 10);
        
        assert(end == lexeme.chars + lexeme.len);
        
        scanned = len;
    }
    
    if(tok_start[scanned] == '.' && isnum(tok_start[scanned + 1]))
    {
        uint64_t integer = 0;
        if(ret.kind == TK_num)
        {
            integer = ret.data.integer;
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

Token scan_str(TokenizeCtx *ctx, const char *tok_start)
{
    Token ret = {.kind = TK_none};
    
    if(tok_start[0] == '\"')
    {
        int len = 2;
        while(tok_start[len - 1] != '\"')
        {
            if(tok_start[len - 1] == '\0')
            {
                error_here(tok_start, "unterminated string literal");
            }
            len += 1;
        }
        
        ret.lexeme = cgs_strv(tok_start, 0, len);
        ret.kind = TK_str;
        ret.data.string = cgs_dstr_init_from(cgs_strv(ret.lexeme, 1, ret.lexeme.len - 1));
    }
    
    return ret;
}

#define try_scan_token(func)                               \
do {                                                       \
    Token scanned_tok = (func)(ctx, tok_start);            \
    if(scanned_tok.kind != TK_none)                        \
    {                                                      \
        TokenList_push(&ret, scanned_tok);                 \
        src_walk.chars += scanned_tok.lexeme.len;          \
        src_walk.len   -= scanned_tok.lexeme.len;          \
        if((func) == scan_str)                             \
        {                                                  \
            lineno += cgs_count(scanned_tok.lexeme, "\n"); \
        }                                                  \
        goto scan_next_token;                              \
    }                                                      \
} while(0)

TokenList tokenize(CGS_StrView file_name, CGS_DStr src)
{
    TokenList ret = TokenList_init();
    
    cgs_append(&src, "        ");
    TokenizeCtx *ctx = &(TokenizeCtx){
        .file_name = file_name,
        .src = cgs_strv(src)
    };
    
    // appending empty space in case we try to scan a long keyword
    
    CGS_StrView src_walk = cgs_strv(src);
    
    int lineno = 1;
    
    scan_next_token:
    
    while(src_walk.len > 0 && isspace(src_walk.chars[0]))
    {
        if(src_walk.chars[0] == '\n')
            lineno += 1;
        
        src_walk.chars += 1;
        src_walk.len -= 1;
    }
    
    if(src_walk.len == 0)
    {
        return ret;
    }
    
    const char *tok_start = src_walk.chars;
    
    try_scan_token(scan_keyword);
    try_scan_token(scan_ident);
    try_scan_token(scan_punct);
    try_scan_token(scan_num);
    try_scan_token(scan_str);
    
    error:
    error_at(file_name, cgs_strv(src), cgs_strv(src_walk, 0, 1), cgs_strv("unexpected character"));
    
    return ret;
}
