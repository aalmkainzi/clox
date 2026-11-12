#include "var.h"
#include "cgs/cgs.h"
#include "clox.h"
#include "stc/common.h"

static uint32_t strv_hash(const CGS_StrView *sv)
{
    return c_hash_n(sv->chars, sv->len);
}

#define i_hash strv_hash
#define i_eq(a, b) cgs_equal(*a, *b)
#define i_implement
#define T VarMap, CGS_StrView, Value
#define i_aux Arena *
#define i_malloc(N) arena_alloc(self->aux, (size_t)N)
#define i_body
#include "stc/hashmap.h"
