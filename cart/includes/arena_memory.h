#ifndef _ARENA_MEMORY_H
#define _ARENA_MEMORY_H

#include <string.h>
#include "arena.h"
#include "common.h"

#define LOAD_FACTOR 0.75
#define FRAMES_MAX 500
#define CAPACITY 64
#define INC 2
#define PAGE 16384
#define PAGE_COUNT 16
#define INIT_GLOBAL \
    (PAGE * PAGE_COUNT)
#define STACK_SIZE 64
#define MIN_SIZE 8
#define NATIVE_STACK_SIZE 32
#define TABLE_SIZE 128
#define MACHINE_STACK 256
#define IP_SIZE 100
#define MEM_OFFSET 1
#define OFFSET sizeof(Free)

#define ALLOC(size) \
    alloc_ptr(size + OFFSET)

#define PTR(ptr) \
    (((Free *)ptr) - 1)

#define FREE(ptr) \
    free_ptr(ptr)

#define GROW_CAPACITY(capacity) \
    ((capacity) < CAPACITY ? CAPACITY : capacity * INC)

#define GROW_ARRAY(ar, size, type) \
    arena_realloc(ar, size, type)
#define GROW_ARENA(ar, size) \
    arena_realloc_arena(ar, size)
#define FREE_ARENA(ar) \
    arena_realloc_arena(ar, 0)
#define FREE_ARRAY(ar) \
    arena_realloc(ar, 0, ARENA_NULL)
#define ARENA_FREE(ar) \
    arena_free(ar)

#define ALLOC_ENTRY(a, b) \
    alloc_entry(a, b)
#define FREE_TABLE_ENTRY(ar) \
    arena_free_entry(ar)

#define GROW_STACK(st, size) \
    realloc_stack(st, size)
#define FREE_STACK(st) \
    free_stack(st)

#define FREE_FUNCTION(func) \
    free_function(func)
#define FREE_NATIVE(nat) \
    free_native(nat)
#define FREE_CLOSURE(clos) \
    free_closure(clos)
#define FREE_UPVALS(up) \
    free_upvals(up)

#define FREE_UPVAL(up) \
    free_upval(up)
#define NEW_STACK(size) \
    stack(size)

#define OBJ(o) \
    Obj(o)
#define VECT(o) \
    vector(o)
#define FUNC(ar) \
    Func(ar)
#define NATIVE(n) \
    native_fn(n)
#define CLOSURE(c) \
    closure(c)
#define UPVAL(c) \
    upval_el(c)
#define CLASS(c) \
    new_class(c)
#define BOUND(c) \
    bound_closure_el(c)
#define INSTANCE(c) \
    new_instance(c)
#define TABLE(t) \
    table_el(t)

#define STK(stk) \
    stack_el(stk)

#define FREE_CLASS(c) \
    free_class(c)
#define FREE_INSTANCE(c) \
    free_instance(c)

typedef union Free Free;
typedef struct CallFrame CallFrame;
typedef struct vm vm;
typedef long long int Align;

union Free
{

    struct
    {
        size_t size;
        Free *next;
    };
    Align align;
};

static Free *mem;

void initialize_global_memory(void);
void destroy_global_memory(void);

Arena *arena_alloc_arena(size_t size);
Arena *arena_realloc_arena(Arena *ar, size_t size);
void arena_free_arena(Arena *ar);

void *alloc_ptr(size_t size);
void free_ptr(Free *new);
void free_garbage(Free **new);

Arena arena_init(void *data, size_t size, T type);
Arena arena_alloc(size_t size, T type);
Arena arena_realloc(Arena *ar, size_t size, T type);

Element cpy_array(Element el);

Arena Ints(int *ints, int len);
Arena Doubles(double *doubles, int len);
Arena Longs(long long int *longs, int len);
Arena Strings(void);

void push_arena(Element *el, Arena ar);
Element pop_arena(Element *el);

void push_int(Element *el, int Int);
Element pop_int(Element *el);

void push_double(Element *el, double Double);
Element pop_double(Element *el);

void push_long(Element *el, long long int Long);
Element pop_long(Element *el);

void push_string(Element *el, const char *String);
Element pop_string(Element *el);

Arena Char(char ch);
Arena Int(int ival);
Arena Byte(uint8_t byte);
Arena Long(long long int llint);
Arena Double(double dval);
Arena String(const char *str);
Arena CString(const char *str);
Arena Bool(bool boolean);
Arena Size(size_t Size);
Arena Null(void);

void arena_free(Arena *ar);

void init_chunk(Chunk *c);
void free_chunk(Chunk *c);

Stack *stack(size_t size);
Stack *realloc_stack(Stack *stack, size_t size);
void free_stack(Stack **stack);

Upval **upvals(size_t size);
void free_upvals(Upval **up);

Stack value(Element el);
Element stack_el(Stack *el);
Element Obj(Arena ar);
Element native_fn(Native *native);
Element closure(Closure *clos);
Element new_class(Class *classc);
Element new_instance(Instance *ci);
Element table_el(Table *t);
Element vector(Arena *vect);
Element null_obj(void);

Function *function(Arena name);
void free_function(Function *func);

Closure *new_closure(Function *func);
void free_closure(Closure **closure);

Upval *upval(Stack *index);
void free_upval(Upval *up);

Native *native(NativeFn native, Arena ar);
void free_native(Native *native);

void print(Element ar);
void print_line(Element ar);

long long int hash(Arena key);
void alloc_entry(Table **e, Table el);

void arena_free_table(Table *t);
void arena_free_entry(Table *entry);

Arena Var(const char *str);
Arena func_name(const char *str);
Arena native_name(const char *str);

Class *class(Arena name);
void free_class(Class *c);

Instance *instance(Class *c);
void free_instance(Instance *ic);

#endif
