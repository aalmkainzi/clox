#ifndef CLOX_H
#define CLOX_H

#include "cgs/cgs.h"
#include "stc/types.h"

typedef struct Token Token;
declare_vec(TokenList, Token);

#define PUNCT_TOKEN_LIST(macro) \
macro(comma       , "," ) \
macro(dot         , "." ) \
macro(semicolon   , ";" ) \
macro(lt          , "<" ) \
macro(gt          , ">" ) \
macro(lte         , "<=") \
macro(gte         , ">=") \
macro(plus        , "+" ) \
macro(minus       , "-" ) \
macro(equal       , "=" ) \
macro(star        , "*" ) \
macro(slash       , "/" ) \
macro(exclamation , "!" ) \
macro(open_paren  , "(" ) \
macro(close_paren , ")" ) \
macro(open_square , "[" ) \
macro(close_square, "]" ) \
macro(open_curly  , "{" ) \
macro(close_curly , "}" )

#define KEYWORD_TOKEN_LIST(macro) \
macro(if      , "if"   ) \
macro(while   , "while") \
macro(for     , "for"  ) \
macro(and     , "and"  ) \
macro(or      , "or"   ) \
macro(true    , "true" ) \
macro(false   , "false") \
macro(function, "fun"  ) \
macro(variable, "var"  ) \
macro(class   , "class")

#define LITERAL_TOKEN_LIST(macro) \
macro(str  , "string literal"  )  \
macro(num  , "number literal"  )  \
macro(float, "floating literal")  \

#define TOKEN_LIST(macro)  \
macro(ident, "identifier") \
LITERAL_TOKEN_LIST(macro)  \
KEYWORD_TOKEN_LIST(macro)  \
PUNCT_TOKEN_LIST  (macro)

#define make_enum_variant(kind, str) TK_##kind,

typedef enum TokenKind
{
    TK_none = 0,
    
    TOKEN_LIST(make_enum_variant)
} TokenKind;

#undef make_enum_variant

struct Token
{
    TokenKind kind;
    CGS_StrView lexeme;
    
    union
    {
        uint64_t integer;
        double floating;
        CGS_DStr string;
    } data;
};

#define i_declared
#define T TokenList, Token
#include "stc/vec.h"

TokenList tokenize(CGS_StrView file_name, CGS_DStr src);
void error_at(CGS_StrView file_name, int lineno, int offset, CGS_StrView src, CGS_StrView message);

#endif
