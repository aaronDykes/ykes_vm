#include "arena_memory.h"
#include <stdio.h>
#include <stdarg.h>

void log_err(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
}

void str_swap(char *from, char *to)
{
    char tmp = *from;
    *from = *to;
    *to = tmp;
}
void string_rev(char *c)
{
    char *to = c + strlen(c) - 1;
    char *from = c;

    for (; from < to; from++, to--)
        str_swap(from, to);
}

int intlen(int n)
{
    int count = 0;
    do
    {
        count++;
    } while (n /= 10);
    return count;
}
int longlen(long long int n)
{
    int count = 0;
    do
    {
        count++;
    } while (n /= 10);
    return count;
}

char *itoa(char *c, int n)
{
    int tmp = n;
    char *ch = c;

    if (n < 0)
        tmp = -n;
    do
    {
        *ch++ = tmp % 10 + '0';
    } while (tmp /= 10);

    if (n < 0)
        *ch++ = '-';
    *ch = '\0';
    string_rev(c);
    return c;
}
char *lltoa(char *c, long long int n)
{
    long long tmp = n;
    char *ch = c;

    if (n < 0)
        tmp = -n;
    do
    {
        *ch++ = tmp % 10 + '0';
    } while (tmp /= 10);

    if (n < 0)
        *ch++ = '-';
    *ch = '\0';
    string_rev(c);
    return c;
}

void str_cat(char *d, char *s)
{

    char *dst = d,
         *src = s;

    while (*src)
        *dst++ = *src++;
}

Arena prepend_int_to_str(Arena s, Arena a)
{
    int len = intlen(s.as.Int);
    int ival = s.as.Int;
    if (ival < 0)
        ++len;
    s = GROW_ARRAY(NULL, sizeof(char) * (len + 1 + a.as.len), ARENA_STR);
    s.as.String = itoa(s.as.String, ival);
    strcat(s.as.String, a.as.String);
    ARENA_FREE(&a);
    return s;
}
Arena prepend_char_to_str(Arena s, Arena a)
{
    char c = s.as.Char;
    s = GROW_ARRAY(NULL, sizeof(char) * (a.as.len + 1), ARENA_STR);
    s.as.String[0] = c;
    strcat(s.as.String, a.as.String);
    ARENA_FREE(&a);
    return s;
}
Arena prepend_long_to_str(Arena s, Arena a)
{
    int len = longlen(s.as.Long);
    long long int llint = s.as.Long;
    if (llint < 0)
        len++;
    s = GROW_ARRAY(NULL, sizeof(char) * (len + 1 + a.as.len), ARENA_STR);
    s.as.String = lltoa(s.as.String, llint);
    strcat(s.as.String, a.as.String);
    ARENA_FREE(&a);
    return s;
}

static Arena append_int_to_str(Arena s, Arena i)
{
    int len = intlen(i.as.Int);
    int ival = i.as.Int;
    if (ival < 0)
        ++len;
    int new = len + s.as.len + 1;

    i = GROW_ARRAY(NULL, sizeof(char) * len, ARENA_STR);
    itoa(i.as.String, ival);
    int tmp = s.size;
    s = GROW_ARRAY(&s, sizeof(char) * new, ARENA_STR);
    strcat(s.as.String + tmp, i.as.String);
    ARENA_FREE(&i);

    return s;
}
static Arena append_str_to_str(Arena s, Arena str)
{
    int new = s.as.len + str.as.len + 1;
    s = GROW_ARRAY(&s, new * sizeof(char), ARENA_STR);
    strcat(s.as.String, str.as.String);
    s.as.String[new] = '\0';
    ARENA_FREE(&str);
    return s;
}
static Arena append_char_to_str(Arena s, Arena c)
{

    int size = s.as.len + 1;
    s = GROW_ARRAY(&s, sizeof(char) * size, ARENA_STR);
    s.as.String[s.as.len - 1] = c.as.Char;
    s.as.String[s.as.len] = '\0';
    return s;
}
static Arena append_long_to_str(Arena s, Arena i)
{
    int len = longlen(i.as.Long);
    long long int llint = i.as.Long;
    int new = len + 1 + s.as.len;

    if (llint < 0)
        ++len;

    i = GROW_ARRAY(NULL, sizeof(char) * (len + 1), ARENA_STR);
    i.as.String = lltoa(i.as.String, llint);
    s = GROW_ARRAY(&s, new * sizeof(char), ARENA_STR);
    strcat(s.as.String, i.as.String);
    ARENA_FREE(&i);
    return s;
}
Arena append(Arena s, Arena ar)
{
    switch (ar.type)
    {
    case ARENA_STR:
    case ARENA_CSTR:
        return append_str_to_str(s, ar);
    case ARENA_CHAR:
        return append_char_to_str(s, ar);
    case ARENA_INT:
        return append_int_to_str(s, ar);
    case ARENA_LONG:
        return append_long_to_str(s, ar);
    default:
        return ar;
    }
}

Arena append_to_cstr(Arena s, Arena ar)
{
    s = String(s.as.String);
    switch (ar.type)
    {
    case ARENA_STR:
    case ARENA_CSTR:
        return append_str_to_str(s, ar);
    case ARENA_CHAR:
        return append_char_to_str(s, ar);
    case ARENA_INT:
        return append_int_to_str(s, ar);
    case ARENA_LONG:
        return append_long_to_str(s, ar);
    default:
        return ar;
    }
}

Arena ltoa_eqcmp(long long int llint, Arena ar)
{
    int len = longlen(llint);
    Arena a = GROW_ARRAY(NULL, sizeof(char) * len + 1, ARENA_STR);
    Arena res = Bool(strcmp(lltoa(a.as.String, llint), ar.as.String) == 0);
    ARENA_FREE(&a);
    return res;
}
Arena ltoa_neqcmp(long long int llint, Arena ar)
{
    int len = longlen(llint);
    Arena a = GROW_ARRAY(NULL, sizeof(char) * len + 1, ARENA_STR);
    Arena res = Bool(strcmp(lltoa(a.as.String, llint), ar.as.String) != 0);
    ARENA_FREE(&a);
    return res;
}
Arena itoa_eqcmp(int ival, Arena ar)
{
    int len = intlen(ival);
    Arena a = GROW_ARRAY(NULL, sizeof(char) * len + 1, ARENA_STR);
    Arena res = Bool(strcmp(itoa(a.as.String, ival), ar.as.String) == 0);
    ARENA_FREE(&a);
    return res;
}
Arena itoa_neqcmp(int ival, Arena ar)
{
    int len = intlen(ival);
    Arena a = GROW_ARRAY(NULL, sizeof(char) * len + 1, ARENA_STR);
    Arena res = Bool(strcmp(itoa(a.as.String, ival), ar.as.String) != 0);
    ARENA_FREE(&a);
    return res;
}

Arena string_eq(Arena s, Arena c)
{

    switch (c.type)
    {
    case ARENA_NULL:
        return Bool(*s.as.String == '\0');
    case ARENA_CSTR:
    case ARENA_STR:
        return Bool(strcmp(s.as.String, c.as.String) == 0);
    case ARENA_INT:
        return itoa_eqcmp(c.as.Int, s);
    case ARENA_LONG:
        return ltoa_eqcmp(c.as.Int, s);
    default:
        return Bool(false);
    }
}
Arena string_ne(Arena s, Arena c)
{
    switch (c.type)
    {
    case ARENA_NULL:
        return Bool(*s.as.String != '\0');
    case ARENA_CSTR:
    case ARENA_STR:
        return Bool(strcmp(s.as.String, c.as.String) != 0);
    case ARENA_INT:
        return itoa_eqcmp(c.as.Int, s);
    case ARENA_LONG:
        return ltoa_eqcmp(c.as.Int, s);
    default:
        return Bool(true);
    }
}
Arena string_gt(Arena s, Arena c)
{
    if (c.type != ARENA_STR)
    {
        log_err("ERROR: string comparison type mismatch\n");
        return Bool(false);
    }
    return Bool(strcmp(s.as.String, c.as.String) > 0);
}
Arena string_ge(Arena s, Arena c)
{
    if (c.type != ARENA_STR)
    {
        log_err("ERROR: string comparison type mismatch\n");
        return Bool(false);
    }
    return Bool(strcmp(s.as.String, c.as.String) >= 0);
}
Arena string_lt(Arena s, Arena c)
{
    if (c.type != ARENA_STR)
    {
        log_err("ERROR: string comparison type mismatch\n");
        return Bool(false);
    }
    return Bool(strcmp(s.as.String, c.as.String) < 0);
}
Arena string_le(Arena s, Arena c)
{
    if (c.type != ARENA_STR)
    {
        log_err("ERROR: string comparison type mismatch\n");
        return Bool(false);
    }
    return Bool(strcmp(s.as.String, c.as.String) <= 0);
}
