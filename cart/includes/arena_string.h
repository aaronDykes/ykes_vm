#ifndef _ARENA_STRING_H
#define _ARENA_STRING_H
#include "arena_memory.h"

Arena ltoa_eqcmp(long long int llint, Arena ar);
Arena ltoa_neqcmp(long long int llint, Arena ar);
Arena itoa_eqcmp(int ival, Arena ar);
Arena itoa_neqcmp(int ival, Arena ar);

void log_err(const char *format, ...);
void str_swap(char *from, char *to);
void string_rev(char *c);
int intlen(int n);
int longlen(long long int n);
char *itoa(char *c, int n);
char *lltoa(char *c, long long int n);

Arena prepend_int_to_str(Arena s, Arena a);
Arena prepend_char_to_str(Arena s, Arena a);
Arena prepend_long_to_str(Arena s, Arena a);

Arena append(Arena s, Arena ar);
Arena append_to_cstr(Arena s, Arena ar);

Arena string_eq(Arena s, Arena c);
Arena string_ne(Arena s, Arena c);
Arena string_gt(Arena s, Arena c);
Arena string_ge(Arena s, Arena c);
Arena string_lt(Arena s, Arena c);
Arena string_le(Arena s, Arena c);

#endif
