#ifndef CLOX_H
#define CLOX_H

#include "cgs/cgs.h"
#include "tokens.h"

TokenList tokenize(CGS_StrView file_name, CGS_DStr src);
void error_at(CGS_StrView file_name, CGS_StrView src, const CGS_StrView error_loc, CGS_StrView message);

#endif
