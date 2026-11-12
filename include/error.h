#ifndef ERROR_H
#define ERROR_H

#include "clox.h"

void error_at_(Ctx *ctx, CGS_StrView error_loc);
void error_at_tok_(Ctx *ctx, Token tok);
void error_at_tok_range_(Ctx *ctx, Token begin_tok, Token end_tok);
void error_();

void passed_();

extern _Thread_local char error_message_buf[1024];
extern _Thread_local char passed_message_buf[1024];

#ifdef NDEBUG
    #define error_at(ctx, loc, msg, ...) \
        (cgs_sprintf(error_message_buf, msg, __VA_ARGS__), error_at_(ctx, cgs_strv(loc)))
#else
    #define error_at(ctx, loc, msg, ...)                                                 \
        (cgs_sprintf(error_message_buf, "%?:%?: " msg, __FILE__, __LINE__, __VA_ARGS__), \
         error_at_(ctx, cgs_strv(loc)))
#endif
#define error_at_tok(ctx, tok, msg, ...) \
    (cgs_sprintf(error_message_buf, msg, __VA_ARGS__), error_at_tok_(ctx, tok))

#define error_at_tok_range(ctx, begin_tok, end_tok, msg, ...) \
    (cgs_sprintf(error_message_buf, msg, __VA_ARGS__), error_at_tok_range_(ctx, begin_tok, end_tok))

#define error(msg, ...) (cgs_sprintf(error_message_buf, msg, __VA_ARGS__), error_())

#define passed(msg, ...) (cgs_sprintf(passed_message_buf, msg, __VA_ARGS__), passed_())

#endif
