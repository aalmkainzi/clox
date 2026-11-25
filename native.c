#include "native.h"
#include "clox.h"
#include <time.h>

Value clock_native(Ctx *ctx, ExprList *args)
{
    (void)ctx;
    (void)args;
    Value now = {
        .type.kind = T_num,
        .number    = (double)clock() / CLOCKS_PER_SEC,
    };
    return now;
}
