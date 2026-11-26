#ifndef CLOX_NATIVE_H_INCLUDED
#define CLOX_NATIVE_H_INCLUDED

#include "expr.h"

#define i_header
#define T ValueList, Value
#include "grow_list.h"

Value native_clock(struct Ctx *ctx, ValueList *args);
Value native_sin(struct Ctx *ctx, ValueList *args);

#endif