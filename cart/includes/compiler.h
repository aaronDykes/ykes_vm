#ifndef _YKES_COMPILER_H
#define _YKES_COMPILER_H
#include "stack.h"
// #include "compiler_util.h"

Function *compile(const char *src);
Function *compile_path(const char *src, const char *path, const char *name);
#endif
