#ifndef VAR_H
#define VAR_H

#include "expr.h"
#include "stc/common.h"
#include "token.h"

#define i_header
#define i_aux Arena *
#define T VarMap, CGS_StrView, Value
#include "stc/hashmap.h"

#endif
