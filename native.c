#include "native.h"
#include "clox.h"
#include <math.h>
#include <time.h>

Value native_clock(Ctx *ctx, ExprList *args)
{
    (void)ctx;
    (void)args;
    return (Value) {
        .type.kind = T_num,
        .number    = (double)clock() / CLOCKS_PER_SEC,
    };
}

Value native_sin(Ctx *ctx, ExprList *args)
{
    Expr *expr = *ExprList_begin(args).ref;
    Value v    = eval(ctx, expr);
    if (!expect_number(ctx, &v))
    {
        return error_value();
    }
    return (Value) {
        .type.kind = T_num,
        .number    = sin(v.number),
    };
}
