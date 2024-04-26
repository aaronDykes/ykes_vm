#ifndef _ARENA_MATH_UTIL_H
#define _ARENA_MATH_UTIL_H
#include "arena_memory.h"

static Arena add_arena_char(char ch, Arena ar);
static Arena sub_arena_char(char ch, Arena ar);
static Arena mul_arena_char(char ch, Arena ar);
static Arena div_arena_char(char ch, Arena ar);
static Arena mod_arena_char(char ch, Arena ar);

static Arena add_arena_size(size_t size, Arena ar);
static Arena sub_arena_size(size_t size, Arena ar);
static Arena mul_arena_size(size_t size, Arena ar);
static Arena div_arena_size(size_t size, Arena ar);
static Arena mod_arena_size(size_t size, Arena ar);

static Arena add_arena_int(int ival, Arena ar);
static Arena sub_arena_int(int ival, Arena ar);
static Arena mul_arena_int(int ival, Arena ar);
static Arena div_arena_int(int ival, Arena ar);
static Arena mod_arena_int(int ival, Arena ar);

static Arena add_arena_long(long long int llint, Arena ar);
static Arena sub_arena_long(long long int llint, Arena ar);
static Arena mul_arena_long(long long int llint, Arena ar);
static Arena div_arena_long(long long int llint, Arena ar);
static Arena mod_arena_long(long long int llint, Arena ar);

static Arena add_arena_double(double dval, Arena ar);
static Arena sub_arena_double(double dval, Arena ar);
static Arena mul_arena_double(double dval, Arena ar);
static Arena div_arena_double(double dval, Arena ar);

static Arena char_eq(char ch, Arena ar);
static Arena char_ne(char ch, Arena ar);
static Arena char_lt(char ch, Arena ar);
static Arena char_le(char ch, Arena ar);
static Arena char_gt(char ch, Arena ar);
static Arena char_ge(char ch, Arena ar);

static Arena int_eq(int ival, Arena ar);
static Arena int_ne(int ival, Arena ar);
static Arena int_lt(int ival, Arena ar);
static Arena int_le(int ival, Arena ar);
static Arena int_gt(int ival, Arena ar);
static Arena int_ge(int ival, Arena ar);

static Arena long_eq(long long int llint, Arena ar);
static Arena long_ne(long long int llint, Arena ar);
static Arena long_lt(long long int llint, Arena ar);
static Arena long_le(long long int llint, Arena ar);
static Arena long_gt(long long int llint, Arena ar);
static Arena long_ge(long long int llint, Arena ar);

static Arena double_eq(double dval, Arena ar);
static Arena double_ne(double dval, Arena ar);
static Arena double_lt(double dval, Arena ar);
static Arena double_le(double dval, Arena ar);
static Arena double_gt(double dval, Arena ar);
static Arena double_ge(double dval, Arena ar);

static Arena size_eq(size_t size, Arena ar);
static Arena size_ne(size_t size, Arena ar);
static Arena size_lt(size_t size, Arena ar);
static Arena size_le(size_t size, Arena ar);
static Arena size_gt(size_t size, Arena ar);
static Arena size_ge(size_t size, Arena ar);

static Arena _access_ints(int *Ints, Arena a, int len);
static Arena _access_longs(long long int *Longs, Arena a, int len);
static Arena _access_doubles(double *Doubles, Arena a, int len);
static Arena _access_string(char *String, Arena a, int len);
static Arena _access_strings(char **Strings, Arena a, int len);

#endif
