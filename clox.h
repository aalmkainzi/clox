#ifndef CLOX_H
#define CLOX_H

#include "cgs/cgs.h"
#include "tokenize.h"

TokenList tokenize(CGS_StrView file_name, CGS_DStr src);
// Token scan_one_token(TokenizeCtx *ctx, const char *tok_start);
void report_error(CGS_StrView file_name, CGS_StrView src, const CGS_StrView error_loc, CGS_StrView message);

#endif
