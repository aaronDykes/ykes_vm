#ifndef _VIRTUAL_MACHINE_H
#define _VIRTUAL_MACHINE_H

#include "debug.h"
#include "arena_table.h"
#include <limits.h>

#define _FLAG_INSTANCE_CALL_SET 0x01 /* 0001 */
#define _FLAG_INSTANCE_CALL_RST 0x0E /* 1110 */

#define INSTANCE_CALL(flag) \
    (flag & _FLAG_INSTANCE_CALL_SET)

typedef enum
{
    INTERPRET_SUCCESS,
    INTERPRET_COMPILE_ERR,
    INTERPRET_RUNTIME_ERR

} Interpretation;

struct CallFrame
{
    Closure *closure;
    uint16_t *ip;
    uint16_t *ip_start;
    Stack *slots;
};

struct vm
{
    int frame_count;
    int argc;
    int cargc;
    uint8_t flags;

    size_t bytes_allocated;
    size_t next_gc;

    Element
        e1,
        e2,
        e3,
        e4,
        e5; /* functions, instances and data structures */

    Arena
        r1,
        r2,
        r3,
        r4,
        r5; /* Everything else */

    CallFrame frames[FRAMES_MAX];
    Stack *stack;

    // Stack *reg_stack;
    Stack *gray_stack;

    Free *gc_work_list;
    Free *gc_work_list_head;

    bool collect;
    Stack *call_stack;
    Stack *class_stack;
    Stack *native_calls;

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
