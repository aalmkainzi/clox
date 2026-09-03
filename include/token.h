#ifndef TOKENS_H
#define TOKENS_H

#include "cgs/cgs.h"
#include "expr.h"

// clang-format off

#define ONE_CHAR_PUNCT_TOKEN_LIST(macro) \
macro(comma      , "," )                 \
macro(dot        , "." )                 \
macro(semicolon  , ";" )                 \
macro(plus       , "+" )                 \
macro(minus      , "-" )                 \
macro(equal      , "=" )                 \
macro(star       , "*" )                 \
macro(slash      , "/" )                 \
macro(not        , "!" )                 \
macro(lparen     , "(" )                 \
macro(rparen     , ")" )                 \
macro(lsquare    , "[" )                 \
macro(rsquare    , "]" )                 \
macro(lcurly     , "{" )                 \
macro(rcurly     , "}" )                 \
macro(lt         , "<" )                 \
macro(gt         , ">" )

#define TWO_CHAR_PUNCT_TOKEN_LIST(macro) \
macro(lte        , "<=")                 \
macro(gte        , ">=")                 \
macro(not_equal  , "!=")                 \
macro(equal_equal, "==")

#define PUNCT_TOKEN_LIST(macro)  \
ONE_CHAR_PUNCT_TOKEN_LIST(macro) \
TWO_CHAR_PUNCT_TOKEN_LIST(macro)

#define KEYWORD_TOKEN_LIST(macro) \
macro(if      , "if"   )          \
macro(while   , "while")          \
macro(for     , "for"  )          \
macro(and     , "and"  )          \
macro(or      , "or"   )          \
macro(true    , "true" )          \
macro(false   , "false")          \
macro(function, "fun"  )          \
macro(var     , "var"  )          \
macro(class   , "class")          \
macro(print   , "print")          \
macro(nil     , "nil"  )

#define LITERAL_TOKEN_LIST(macro) \
macro(str  , "string literal")    \
macro(num  , "number literal")    \
macro(float, "float literal" )    \

#define TOKEN_LIST(macro)  \
macro(ident, "identifier") \
LITERAL_TOKEN_LIST(macro)  \
KEYWORD_TOKEN_LIST(macro)  \
PUNCT_TOKEN_LIST(macro)    \
macro(eof, "EOF")

// clang-format on

#define make_enum_variant(kind, str) TK_##kind,

typedef enum TokenKind
{
    TK_none = 0,
    TK_ident,

    TK_LIT_BEGIN,
    LITERAL_TOKEN_LIST(make_enum_variant) TK_LIT_END,

    TK_KW_BEGIN,
    KEYWORD_TOKEN_LIST(make_enum_variant) TK_KW_END,

    TK_PUNCT_BEGIN,
    PUNCT_TOKEN_LIST(make_enum_variant) TK_PUNCT_END,

    TK_eof,
} TokenKind;

#undef make_enum_variant

#define make_tok2str_entry(kind, str) [TK_##kind] = (CGS_StrView) {.chars = str, .len = sizeof(str) - 1},

static const CGS_StrView tok2str[] = {TOKEN_LIST(make_tok2str_entry)};

// clang-format off

#undef make_tok2str_entry

typedef union TokenData
{
    bool boolean;
    double dbl;
    CGS_DStr string;
} TokenData;

typedef struct Token
{
    TokenKind kind;
    CGS_StrView lexeme;
    bool seperate : 1;

    TokenData data;
} Token;

Token *copy_token(Token *token);
void free_token(Token *tok);

// write functions so Token can be printable
static inline CGS_Error stringify_token(CGS_Writer *writer, Token tok, CGS_StrView fmt_arg)
{
    (void)fmt_arg;
    return cgs_append(writer, tok.lexeme);
}

static inline CGS_Error stringify_token_ptr(CGS_Writer *writer, Token *tok, CGS_StrView fmt_arg)
{
    (void)fmt_arg;
    return cgs_append(writer, tok->lexeme);
}

#define ADD_TOSTR (Token, stringify_token)
#include "cgs/cgs.h"

#define ADD_TOSTR (Token *, stringify_token_ptr)
#include "cgs/cgs.h"

#define i_no_clone
#define i_keydrop free_token
#define T TokenList, Token
#include "stc/vec.h"

#define i_no_clone
#define i_keydrop TokenList_drop
#define T TokensChain, TokenList
#include "stc/list.h"

#endif
