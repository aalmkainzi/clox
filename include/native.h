#ifndef CLOX_NATIVE_H_INCLUDED
#define CLOX_NATIVE_H_INCLUDED

#include "expr.h"

Value native_clock(struct Ctx *ctx, ExprList *args);
Value native_sin(struct Ctx *ctx, ExprList *args);

#endif