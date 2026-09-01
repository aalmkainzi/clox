#include "cgs/cgs.h"
#include "expr.h"
#include "stc/common.h"

static uint32_t strv_hash(const CGS_StrView *sv)
{
    return c_hash_n(sv->chars, sv->len);
}

static bool strv_eq(const CGS_StrView *a, const CGS_StrView *b)
{
    return cgs_equal(*a, *b);
}

#define i_hash strv_hash
#define i_eq strv_eq
#define i_implement
#define T VarMap, CGS_StrView, Value
#include "stc/hashmap.h"