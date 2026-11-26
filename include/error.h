#ifndef ERROR_H
#define ERROR_H

#include "clox.h"

void error_at_(Ctx *ctx, CGS_StrView error_loc, CGS_DStr messsage);
void error_at_tok_(Ctx *ctx, Token tok, CGS_DStr messsage);
void error_at_tok_range_(Ctx *ctx, Token begin_tok, Token end_tok, CGS_DStr messsage);
void error_(Ctx *ctx, CGS_DStr messsage);

void passed_(Ctx *ctx, CGS_DStr messsage);

#if 0 && !defined(NDEBUG)
    #define error_at(ctx, loc, msg, ...) \
        error_at_(ctx, cgs_strv(loc), cgs_asprintf("%?:%?: " msg, __FILE__, __LINE__, __VA_ARGS__))
#else
    #define error_at(ctx, loc, msg, ...) error_at_(ctx, cgs_strv(loc), cgs_asprintf(msg, __VA_ARGS__))
#endif

#define error_at_tok(ctx, tok, msg, ...) error_at_tok_(ctx, tok, cgs_asprintf(msg, __VA_ARGS__))

#define error_at_tok_range(ctx, begin_tok, end_tok, msg, ...) \
    error_at_tok_range_(ctx, begin_tok, end_tok, cgs_asprintf(msg, __VA_ARGS__))

#define error(ctx, msg, ...) error_(ctx, cgs_asprintf(msg, __VA_ARGS__))

#define passed(ctx, msg, ...) passed_(ctx, cgs_asprintf(msg, __VA_ARGS__))

#endif
