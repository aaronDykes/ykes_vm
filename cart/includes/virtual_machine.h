#ifndef _VIRTUAL_MACHINE_H
#define _VIRTUAL_MACHINE_H

#include "debug.h"
#include "arena_table.h"
#include <limits.h>

typedef enum
{
    INTERPRET_SUCCESS,
    INTERPRET_COMPILE_ERR,
    INTERPRET_RUNTIME_ERR

} Interpretation;

struct CallFrame
{
    Closure *closure;
    uint8_t *ip;
    uint8_t *ip_start;
    Stack *slots;
};

struct vm
{
    int frame_count;
    int argc;
    int cargc;

    CallFrame frames[FRAMES_MAX];
    Stack *stack;
    Stack *call_stack;
    Stack *method_call_stack;
    Stack *class_stack;
    Stack *native_calls;
    Element pop_val;
    Upval *open_upvals;
    Table *glob;
};

vm machine;

void initVM(void);
void freeVM(void);

Interpretation run(void);
Interpretation interpret(const char *source);
Interpretation interpret_path(const char *source, const char *path, const char *name);

#endif
