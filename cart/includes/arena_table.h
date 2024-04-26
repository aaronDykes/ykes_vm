#ifndef _ARENA_TABLE_H
#define _ARENA_TABLE_H
#include "stack.h"

#define FREE_ENTRY(e) \
    free_entry(e)
#define FREE_TABLE(ar) \
    arena_realloc_table(ar, 0)

#define GROW_TABLE(ar, size) \
    arena_realloc_table(ar, size)

void insert_entry(Table **t, Table entry);
void delete_entry(Table **t, Arena key);

void free_entry(Element el);
Element find_entry(Table **t, Arena *hash);

Table Entry(Arena key, Element val);
Table arena_entry(Arena key, Arena val);
Table class_entry(Class *c);
Table instance_entry(Arena ar, Instance *c);
Table table_entry(Arena ar, Table *t);
Table func_entry(Closure *c);
Table native_entry(Native *func);
Table vector_entry(Arena ar, Arena *arena_vector);
Table stack_entry(Arena ar, Stack *s);
Table new_entry(Table t);

void write_table(Table *t, Arena a, Element b);
Table *arena_alloc_table(size_t size);
Table *arena_realloc_table(Table *t, size_t size);
void alloc_entry(Table **e, Table el);
#endif
