#include "cgs/cgs.h"
#include "expr.h"
#include "stc/common.h"

static uint32_t strv_hash(const CGS_StrView *sv)
{
    return c_hash_n(sv->chars, sv->len);
}

#define i_hash strv_hash
#define i_eq(a, b) cgs_equal(*a, *b)
#define i_implement
#define T VarMap, CGS_StrView, Value
#include "stc/hashmap.h"
