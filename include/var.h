#ifndef VAR_H
#define VAR_H

#include "expr.h"
#include "stc/common.h"
#include "token.h"

#define i_no_clone
#define i_no_clone
#define i_valdrop free_var_value
#define i_header
#define T VarMap, CGS_StrView, Value
#include "stc/hashmap.h"

#endif