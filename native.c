#include "native.h"
#include "clox.h"
#include <math.h>
#include <time.h>

#define i_implement
#define i_body
#define T ValueList, Value
#include "grow_list.h"

Value native_clock(Ctx *ctx, ValueList *args)
{
    (void)ctx;
    (void)args;
    return (Value) {
        .type.kind = T_num,
        .number    = (double)clock() / CLOCKS_PER_SEC,
    };
}

Value native_sin(Ctx *ctx, ValueList *args)
{
    Value v = *ValueList_begin(args).ref;
    if (!expect_number(ctx, &v))
    {
        return error_value();
    }
    return (Value) {
        .type.kind = T_num,
        .number    = sin(v.number),
    };
}
