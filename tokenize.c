#include "clox.h"
#include <ctype.h>

Token scan_keyword(const char *tok_start)
{
    Token ret = {.kind = TK_none};
    
#define else_if_is_kw(name, str) \
    else if(memcmp(tok_start, str, strlen(str)) == 0) \
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

Token scan_ident(const char *tok_start)
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

Token scan_punct(const char *tok_start)
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
    PUNCT_TOKEN_LIST(else_if_is_punct)
    
#undef else_if_is_punct
    
    return ret;
}

static bool isnum(char c)
{
    return c >= '0' && c <= '9';
}

Token scan_num(const char *tok_start)
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
        
        scanned = len;
    }
    
    if(tok_start[scanned] == '.' && isnum(tok_start[scanned + 1]))
    {
        int len = 2;
        while(isnum(tok_start[scanned + len]))
            len += 1;
        
        CGS_StrView lexeme = cgs_strv(tok_start, 0, scanned + len);
        
        ret.kind = TK_float;
        ret.lexeme = lexeme;
    }
    
    return ret;
}

Token scan_str(const char *tok_start)
{
    Token ret = {.kind = TK_none};
    
    if(tok_start[0] == '\"')
    {
        
    }
    
    return ret;
}

#define try_scan_token(func) \
do { \
    Token scanned_tok = (func)(tok_start); \
    if(scanned_tok.kind != TK_none) \
    { \
        TokenList_push(&ret, scanned_tok); \
        src_walk.chars -= scanned_tok.lexeme.len; \
        src_walk.len += scanned_tok.lexeme.len; \
        goto scan_next_token; \
    } \
} while(0)

TokenList tokenize(CGS_StrView file_name, CGS_DStr src)
{
    TokenList ret = TokenList_init();
    
    // appending empty space in case we try to scan a long keyword
    cgs_append(&src, "        ");
    
    CGS_StrView src_walk = cgs_strv(src);
    // TODO should probably cgs_split(src, "\n") so we get lineno on each token
    
    scan_next_token:
    
    CGS_Result(CGS_StrView) res = cgs_next_tok_any(&src_walk, " \f\n\r\t\v");
    
    if(res.err.ec != CGS_OK)
    {
        if(res.err.ec == CGS_NOT_FOUND)
            return ret; // empty string remaining, return scanned tokens
    }
    
    const char *tok_start = res.val.chars;
    
    try_scan_token(scan_keyword);
    try_scan_token(scan_ident);
    try_scan_token(scan_punct);
    try_scan_token(scan_num);
    
    error:
    error_at(file_name, 0, 0, cgs_strv(src), cgs_strv("unexpected character"));
    
    return ret;
}
