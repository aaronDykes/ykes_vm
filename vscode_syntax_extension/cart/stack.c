#include "stack.h"
#include <stdio.h>

void write_chunk(Chunk *c, uint8_t byte, int line)
{

    if (c->op_codes.len < c->op_codes.count + 1)
    {
        c->op_codes.len = GROW_CAPACITY(c->op_codes.len);
        c->op_codes = GROW_ARRAY(&c->op_codes, c->op_codes.len * sizeof(uint8_t), ARENA_BYTES);
    }

    if (c->lines.len < c->lines.count + 1)
    {
        c->lines.len = GROW_CAPACITY(c->lines.len);
        c->lines = GROW_ARRAY(&c->lines, c->lines.len * sizeof(int), ARENA_INTS);
    }

    c->lines.listof.Ints[c->lines.count++] = line;
    c->op_codes.listof.Bytes[c->op_codes.count++] = byte;
}

int add_constant(Chunk *c, Element ar)
{
    push(&c->constants, ar);
    return c->constants->count - 1;
}

void reset_stack(Stack *s)
{
    s->top = s;
}
void check_stack_size(Stack *s)
{

    if (!s)
        return;
    if (s->count + 1 > s->len)
    {
        s->len = GROW_CAPACITY(s->len);
        s = GROW_STACK(s, s->len);
        reset_stack(s);
    }
}

void popn(Stack **s, int ival)
{
    for (int i = 0; i < ival; i++)
        --(*s)->count, --(*s)->top;
}

void push(Stack **s, Element e)
{
    Stack *st = *s;
    check_stack_size(st);

    if (!st)
    {
        st = GROW_STACK(NULL, STACK_SIZE);
        *s = st;
        reset_stack(*s);
    }

    (st->top++)->as = e;
    st->count++;
}
