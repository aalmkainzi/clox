#ifndef CLOX_NATIVE_H_INCLUDED
#define CLOX_NATIVE_H_INCLUDED

#include "expr.h"

Value clock_native(struct Ctx *ctx, ExprList *args);

#endif