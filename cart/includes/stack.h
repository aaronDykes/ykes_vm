#ifndef _STACK_H
#define _STACK_H

#include "arena_memory.h"

void write_chunk(Chunk *c, uint8_t byte, int line);
int add_constant(Chunk *c, Element ar);

void push(Stack **s, Element e);
void popn(Stack **s, int ival);
void reset_stack(Stack *s);
void check_stack_size(Stack *s);

#endif
