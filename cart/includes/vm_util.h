#ifndef _VM_UTIL_H
#define _VM_UTIL_H
#include "arena_math.h"

static Element find(Table *t, Arena tmp);
static void close_upvalues(Stack *local);
static void define_native(Arena ar, NativeFn native);
static inline Element clock_native(int argc, Stack *args);
static inline Element file_native(int argc, Stack *argv);
static inline Element square_native(int argc, Stack *args);
static inline Element prime_native(int argc, Stack *args);
#endif
