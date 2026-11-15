#ifndef CLOX_TEST_H
#define CLOX_TEST_H

#include "clox.h"

int parse_tests(Ctx *ctx);
bool verify_expected_output(Ctx *ctx, CGS_DStr output);
bool verify_expected_error(Ctx *ctx, CGS_StrView error_loc, CGS_StrView error_message);
void check_remaining_expected_output(Ctx *ctx);
void free_tests(Ctx *ctx);

#endif
