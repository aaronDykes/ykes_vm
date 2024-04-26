#include "virtual_machine.h"
#include "compiler.h"
#include "vm_util.h"
#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

void initVM(void)
{

    initialize_global_memory();

    machine.stack = NULL;
    machine.call_stack = NULL;
    machine.class_stack = NULL;
    machine.native_calls = NULL;
    machine.glob = NULL;

    machine.stack = GROW_STACK(NULL, STACK_SIZE);
    machine.call_stack = GROW_STACK(NULL, STACK_SIZE);
    machine.class_stack = GROW_STACK(NULL, STACK_SIZE);
    machine.native_calls = GROW_STACK(NULL, STACK_SIZE);
    machine.glob = GROW_TABLE(NULL, TABLE_SIZE);

    machine.argc = 0;
    machine.cargc = 0;

    define_native(native_name("clock"), clock_native);
    define_native(native_name("square"), square_native);
    define_native(native_name("prime"), prime_native);
    define_native(native_name("file"), file_native);
}
void freeVM(void)
{
    FREE_TABLE(machine.glob);
    FREE_STACK(&machine.stack);
    FREE_STACK(&machine.call_stack);
    FREE_STACK(&machine.class_stack);
    FREE_STACK(&machine.native_calls);

    machine.glob = NULL;
    machine.stack = NULL;
    machine.call_stack = NULL;
    machine.class_stack = NULL;
    machine.native_calls = NULL;

    destroy_global_memory();
}

static void reset_vm_stack(void)
{
    reset_stack(machine.stack);
    machine.frame_count = 0;
}

static void runtime_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = machine.frame_count - 1; i >= 0; i--)
    {

        CallFrame *frame = &machine.frames[i];
        Function *func = frame->closure->func;
        int line = frame->closure->func->ch.lines.listof.Ints[i];

        if (!func->name.as.String)
            fprintf(stderr, "script\n");
        else
            fprintf(stderr, "%s()\n", func->name.as.String);
        fprintf(stderr, "[line %d] in script\n", line);
    }

    reset_vm_stack();
}

static void define_native(Arena ar, NativeFn n)
{
    Element el = native_fn(native(n, ar));
    push(&machine.native_calls, el);
}

static inline Element prime_native(int argc, Stack *args)
{
    return OBJ(_prime(args->as.arena));
}

static inline Element clock_native(int argc, Stack *args)
{
    return OBJ(Double((double)clock() / CLOCKS_PER_SEC));
}
static char *get_file(const char *path)
{

    char rest[PATH_MAX] = {0};
    char *tmp = NULL;
    tmp = realpath(path, rest);
    FILE *file = NULL;
    file = fopen(tmp, "r");

    if (!file)
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = NULL;
    buffer = ALLOC(fileSize + 1);

    if (!buffer)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void append_file(const char *path, const char *data)
{
    FILE *f = NULL;

    char *tmp = NULL;
    tmp = realpath(path, NULL);
    if (!tmp)
    {
        fprintf(stderr, "Unable to open file: %s\n", tmp);
        exit(1);
    }

    f = fopen(tmp, "a");

    if (!f)
    {
        fprintf(stderr, "Unable to open file: %s\n", path);
        exit(1);
    }

    fprintf(f, "\n%s", data);
    fclose(f);

    tmp = NULL;
    f = NULL;
}
static void write_file(const char *path, const char *data)
{
    FILE *f = NULL;

    char *tmp = NULL;
    tmp = realpath(path, NULL);
    if (!tmp)
    {
        fprintf(stderr, "Unable to open file: %s\n", tmp);
        exit(1);
    }

    f = fopen(tmp, "w");

    if (!f)
    {
        fprintf(stderr, "Unable to open file: %s\n", path);
        exit(1);
    }

    fprintf(f, "%s", data);
    fclose(f);

    tmp = NULL;
    f = NULL;
}

static inline Element file_native(int argc, Stack *argv)
{
    switch (*argv->as.arena.as.String)
    {
    case 'r':
        return OBJ(CString(get_file(argv[1].as.arena.as.String)));
    case 'w':
        write_file(argv[1].as.arena.as.String, argv[2].as.arena.as.String);
        return null_obj();
    case 'a':
        append_file(argv[1].as.arena.as.String, argv[2].as.arena.as.String);
        return null_obj();
    default:
        return null_obj();
    }
}

static inline Element square_native(int argc, Stack *args)
{
    return OBJ(_sqr(args->as.arena));
}

static bool call(Closure *c, uint8_t argc)
{

    if (c->func->arity != argc)
    {
        runtime_error("ERROR: Expected `%d` args, but got `%d`.", c->func->arity, argc);
        return false;
    }

    if (machine.frame_count == FRAMES_MAX)
    {
        runtime_error("ERROR: Stack overflow.");
        return false;
    }

    CallFrame *frame = &machine.frames[machine.frame_count++];
    frame->closure = c;
    frame->closure->upvals = c->upvals;
    frame->ip = c->func->ch.op_codes.listof.Bytes;
    frame->ip_start = c->func->ch.op_codes.listof.Bytes;
    frame->slots = machine.stack->top - argc - 1;
    return true;
}

Interpretation interpret(const char *src)
{

    Function *func = NULL;

    if (!(func = compile(src)))
        return INTERPRET_RUNTIME_ERR;

    Closure *clos = new_closure(func);
    call(clos, 0);

    push(&machine.stack, closure(clos));

    close_upvalues(machine.stack->top - 1);
    Interpretation res = run();
    return res;
}
Interpretation interpret_path(const char *src, const char *path, const char *name)
{

    Function *func = NULL;

    if (!(func = compile_path(src, path, name)))
        return INTERPRET_RUNTIME_ERR;

    Closure *clos = new_closure(func);
    call(clos, 0);

    push(&machine.stack, closure(clos));

    close_upvalues(machine.stack->top - 1);
    Interpretation res = run();
    return res;
}

static Element find(Table *t, Arena ar)
{
    return find_entry(&t, &ar);
}
static bool call_value(Element el, uint8_t argc)
{
    switch (el.type)
    {

    case CLOSURE:
        return call(el.closure, argc);
    case NATIVE:
    {
        Element res = el.native->fn(argc, machine.stack->top - argc);
        machine.stack->count -= (argc + 1);
        machine.stack->top -= (argc + 1);
        push(&machine.stack, res);
        return true;
    }
    case CLASS:
    case INSTANCE:
        machine.stack->top[-1 - argc].as = el;
        return true;
    default:
        break;
    }
    runtime_error("ERROR: Can only call functions and classes.");
    return false;
}

static Upval *capture_upvalue(Stack *s)
{
    Upval *prev = NULL;
    Upval *curr = machine.open_upvals;

    for (; curr && curr->index && curr->index < s; curr = curr->next)
        prev = curr;

    if (curr && curr->index == s)
        return curr;

    Upval *new = upval(s);
    new->next = curr;

    if (prev)
        prev->next = new;
    else
        machine.open_upvals = new;
    return new;
}

static void close_upvalues(Stack *local)
{
    while (machine.open_upvals && machine.open_upvals->index >= local)
    {
        Upval *up = machine.open_upvals;
        up->closed = *machine.open_upvals->index;
        up->index = &up->closed;
        machine.open_upvals = up->next;
    }
}
Closure *traverse_stack_closure(Arena *ar)
{
    for (Stack *s = machine.call_stack->top - 1; s >= machine.call_stack; s--)
        if (s->as.closure->func->name.as.hash == ar->as.hash)
            return s->as.closure;
    return NULL;
}

Native *traverse_stack_native(Arena *ar)
{
    for (Stack *s = machine.native_calls->top - 1; s >= machine.native_calls; s--)
        if (s->as.native->obj.as.hash == ar->as.hash)
            return s->as.native;
    return NULL;
}

static void free_asterisk(Element el)
{
    switch (el.type)
    {
    case ARENA:
        if (el.arena.type == ARENA_VAR)
            delete_entry(&machine.glob, el.arena);
        else
            ARENA_FREE(&el.arena);
        break;
    case SCRIPT:
    case CLOSURE:
        FREE_CLOSURE(&el.closure);
        break;
    case CLASS:
        FREE_CLASS(el.classc);
        break;
    case INSTANCE:
        FREE_INSTANCE(el.instance);
        break;
    case TABLE:
        FREE_TABLE(el.table);
        break;
    default:
        return;
    }
}

static bool not_null(Element el)
{
    switch (el.type)
    {
    case ARENA:
    {
        Arena ar = el.arena;
        switch (ar.type)
        {
        case ARENA_BYTE:
        case ARENA_SIZE:
        case ARENA_INT:
        case ARENA_DOUBLE:
        case ARENA_CHAR:
        case ARENA_BOOL:
        case ARENA_LONG:
            return true;
        case ARENA_NULL:
            return false;
        case ARENA_STR:
        case ARENA_CSTR:
        case ARENA_VAR:
        case ARENA_NATIVE:
        case ARENA_FUNC:
            return ar.as.String ? true : false;
        case ARENA_BYTES:
            return ar.listof.Bytes ? true : false;
        case ARENA_INTS:
            return ar.listof.Ints ? true : false;
        case ARENA_DOUBLES:
            return ar.listof.Doubles ? true : false;
        case ARENA_LONGS:
            return ar.listof.Longs ? true : false;
        case ARENA_BOOLS:
            return ar.listof.Bools ? true : false;
        case ARENA_SIZES:
            return ar.listof.Sizes ? true : false;
        case ARENA_STRS:
            return ar.listof.Strings ? true : false;
        default:
            return false;
        }
    }
    case TABLE:
        return el.table ? true : false;
    case VECTOR:
        return el.arena_vector ? true : false;
    case CLOSURE:
        return el.closure ? true : false;
    case CLASS:
        return el.classc ? true : false;
    case INSTANCE:
        return el.instance ? true : false;
    case STACK:
        return el.stack ? true : false;
    default:
        return false;
    }
}

Interpretation run(void)
{

    CallFrame *frame = &machine.frames[machine.frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (((READ_BYTE() << 8) & 0xFF) | READ_BYTE() & 0xFF)
#define READ_CONSTANT() ((frame->closure->func->ch.constants + READ_BYTE())->as)
#define GET_FUNC(ar) \
    (traverse_stack_closure(&ar))
#define GET_NATIVE(ar) \
    (NATIVE(traverse_stack_native(&ar)))
#define PEEK() ((machine.stack->top - 1)->as)
#define NPEEK(N) ((machine.stack->top + (-1 - N))->as)
#define CPEEK(N) ((machine.call_stack->top + (-1 + -N))->as)
#define FALSEY() (!PEEK().arena.as.Bool)
#define POPN(n) (popn(&machine.stack, n))
#define LOCAL() ((frame->slots + READ_BYTE())->as)
#define JUMP() (*(frame->closure->func->ch.cases.listof.Ints + READ_SHORT()))
#define PUSH(ar) (push(&machine.stack, ar))
#define CPUSH(ar) (push(&machine.call_stack, ar))
#define PPUSH(ar) (push(&machine.class_stack, ar))
#define FIND_GLOB(ar) (find(machine.glob, ar))
#define FIND_PARAM(ar) (find(frame->closure->func->params, ar))
#define WRITE_GLOB(a, b) (write_table(machine.glob, a, b))
#define WRITE_PARAM(a, b) (write_table(frame->closure->func->params, a, b))
#define RM() \
    free_asterisk(POP())
#define POP() \
    (--machine.stack->count, (--machine.stack->top)->as)
#define CPOP() \
    (--machine.call_stack->count, (--machine.call_stack->top)->as)

    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        for (Stack *v = machine.stack; v < machine.stack->top; v++)
            print_line(v->as);
        disassemble_instruction(&frame->closure->func->ch,
                                (int)(frame->ip - frame->closure->func->ch.op_codes.listof.Bytes));
#endif

        switch (READ_BYTE())
        {
        case OP_CONSTANT:
            PUSH(READ_CONSTANT());
            break;

        case OP_CLOSURE:
        {
            Element e = READ_CONSTANT();
            CPUSH(e);

            for (int i = 0; i < e.closure->upval_count; i++)
                e.closure->upvals[i] =
                    (READ_BYTE())
                        ? capture_upvalue(frame->slots + READ_BYTE())
                        : frame->closure->upvals[READ_BYTE()];
        }
        break;

        case OP_GET_UPVALUE:
            PUSH((*frame->closure->upvals + READ_BYTE())->closed.as);
            break;
        case OP_SET_UPVALUE:
            ((*frame->closure->upvals + READ_BYTE()))->closed = *(machine.stack->top - 1);
            break;

        case OP_NEG:
            (--machine.stack->top)->as = OBJ(_neg((machine.stack->top++)->as.arena));
            break;

        case OP_INC_GLO:
        {
            Element key = READ_CONSTANT();
            Element ar = OBJ(_inc(FIND_GLOB(key.arena).arena));
            WRITE_GLOB(key.arena, ar);
            PUSH(ar);
            break;
        }
        case OP_DEC_GLO:
        {
            Element key = READ_CONSTANT();
            Element ar = OBJ(_dec(FIND_GLOB(key.arena).arena));
            WRITE_GLOB(key.arena, ar);
            PUSH(ar);
            break;
        }
        case OP_INC_LOC:
        {
            uint8_t index = READ_BYTE();
            Element el = OBJ(_inc((frame->slots + index)->as.arena));
            (frame->slots + index)->as = el;
            PUSH(el);
            break;
        }
        case OP_DEC_LOC:
        {
            uint8_t index = READ_BYTE();
            Element el = OBJ(_dec((frame->slots + index)->as.arena));
            (frame->slots + index)->as = el;
            PUSH(el);
            break;
        }
        case OP_INC:
            (--machine.stack->top)->as = OBJ(_inc((machine.stack->top++)->as.arena));
            break;
        case OP_DEC:
            (--machine.stack->top)->as = OBJ(_dec((machine.stack->top++)->as.arena));
            break;
        case OP_ADD:
            PUSH(OBJ(_add(POP().arena, POP().arena)));
            break;
        case OP_POPN:
            POPN(READ_CONSTANT().arena.as.Int);
            break;
        case OP_POP:
            POP();
            break;
        case OP_SUB:
            PUSH(OBJ(_sub(POP().arena, POP().arena)));
            break;
        case OP_MUL:
            PUSH(OBJ(_mul(POP().arena, POP().arena)));
            break;
        case OP_MOD:
            PUSH(OBJ(_mod(POP().arena, POP().arena)));
            break;
        case OP_DIV:
            PUSH(OBJ(_div(POP().arena, POP().arena)));
            break;
        case OP_EQ:
            PUSH(OBJ(_eq(POP().arena, POP().arena)));
            break;
        case OP_NE:
            PUSH(OBJ(_ne(POP().arena, POP().arena)));
            break;
        case OP_SEQ:
            PUSH(OBJ(_seq(POP().arena, POP().arena)));
            break;
        case OP_SNE:
            PUSH(OBJ(_sne(POP().arena, POP().arena)));
            break;
        case OP_LT:
            PUSH(OBJ(_lt(POP().arena, POP().arena)));
            break;
        case OP_LE:
            PUSH(OBJ(_le(POP().arena, POP().arena)));
            break;
        case OP_GT:
            PUSH(OBJ(_gt(POP().arena, POP().arena)));
            break;
        case OP_GE:
            PUSH(OBJ(_ge(POP().arena, POP().arena)));
            break;
        case OP_OR:
            PUSH(OBJ(_or(POP().arena, POP().arena)));
            break;
        case OP_AND:
            PUSH(OBJ(_and(POP().arena, POP().arena)));
            break;
        case OP_GET_ACCESS:
        {

            Element el = _get_access(POP(), POP());
            if (el.type != NULL_OBJ)
            {
                PUSH(el);
                break;
            }
            return INTERPRET_RUNTIME_ERR;
        }
        case OP_RESET_ARGC:
            machine.cargc = 0;
            machine.argc = 0;
            break;
        case OP_EACH_ACCESS:
        {
            Element el = _get_each_access(POP(), machine.cargc++);
            PEEK() = el;
        }
        break;
        case OP_SET_ACCESS:
            _set_access(POP(), POP().arena, PEEK());
            break;
        case OP_PUSH_ARRAY_VAL:
        {

            Element e1 = POP();
            Element e2 = POP();
            Element res = _push_array_val(e1, e2);
            if (res.type != NULL_OBJ)
            {
                PUSH(res);
                PUSH(OBJ(Bool(e2.type == VECTOR || e2.type == STACK)));
                break;
            }
            return INTERPRET_RUNTIME_ERR;
        }
        case OP_POP__ARRAY_VAL:
        {
            Element p = PEEK();
            if (p.type == ARENA)
                --p.arena.count;
            else
                --p.arena_vector->count;
            machine.pop_val = _pop_array_val(PEEK());
            PEEK() = p;
            break;
        }
        case OP_PUSH:
            PUSH(machine.pop_val);
            break;
        case OP_CPY_ARRAY:
            PUSH(cpy_array(POP()));
            break;
        case OP_LEN:
            PUSH(OBJ(_len(POP())));
            break;
        case OP_NULL:
            break;
        case OP_JMPF:
            frame->ip += (READ_SHORT() * FALSEY());
            break;
        case OP_JMPC:
        {
            uint16_t jump = READ_SHORT();
            uint16_t offset = READ_SHORT();

            if (FALSEY())
            {
                POP();
                frame->ip += jump;
                break;
            }
            frame->ip += offset;
            break;
        }
        case OP_JMPL:
            frame->ip = frame->ip_start + JUMP();
            break;
        case OP_SET_PROP:
        {
            Element el = PEEK();
            Element inst = NPEEK(1);
            if (inst.type != INSTANCE)
            {
                runtime_error("ERROR: Can only set properties of an instance.");
                return INTERPRET_RUNTIME_ERR;
            }
            write_table(inst.instance->classc->fields, READ_CONSTANT().arena, el);
            POP();
            break;
        }
        case OP_PUSH_TOP:
            PUSH(PEEK());
            break;
        case OP_GET_PROP:
        {
            if (PEEK().type != INSTANCE)
            {
                runtime_error("ERROR: Only instances contain properties.");
                return INTERPRET_RUNTIME_ERR;
            }
            Instance *inst = POP().instance;
            Arena name = READ_CONSTANT().arena;

            Element n = find_entry(&inst->classc->fields, &name);

            if (n.type != NULL_OBJ)
            {
                PUSH(n);
                break;
            }

            runtime_error("ERROR: Undefined property '%s'.", name.as.String);
            return INTERPRET_RUNTIME_ERR;
        }
        case OP_CALL:
        {
            uint8_t argc = READ_BYTE();
            if (!call_value(NPEEK(argc), argc))
                return INTERPRET_RUNTIME_ERR;

            frame = (machine.frames + (machine.frame_count - 1));
            machine.argc = (argc == 0) ? 1 : argc;
            machine.cargc = 1;

            break;
        }
        case OP_JMPT:
            frame->ip += (READ_SHORT() * !FALSEY());
            break;
        case OP_JMP_NIL:
        {
            uint16_t offset = READ_SHORT();
            if (!not_null(PEEK()))
                frame->ip += offset;
            break;
        }
        case OP_JMP_NOT_NIL:
        {
            uint16_t offset = READ_SHORT();
            if (not_null(PEEK()))
                frame->ip += offset;

            break;
        }
        break;
        case OP_JMP:
            frame->ip += READ_SHORT();
            break;
        case OP_LOOP:
            frame->ip -= READ_SHORT();
            break;
        case OP_CLOSE_UPVAL:
            close_upvalues(machine.stack->top - 1);
            POP();
            break;
        case OP_GET_LOCAL:
            PUSH(LOCAL());
            break;
        case OP_SET_LOCAL:
            LOCAL() = PEEK();
            break;
        case OP_SET_LOCAL_PARAM:
            LOCAL() = (machine.cargc < machine.argc)
                          ? (frame->slots + machine.cargc++)->as
                          : PEEK();
            break;
        case OP_GET_CLOSURE:
            PUSH((machine.call_stack + READ_BYTE())->as);
            break;
        case OP_GET_NATIVE:
            PUSH((machine.native_calls + READ_BYTE())->as);
            break;
        case OP_CLASS:
            PPUSH(INSTANCE(READ_CONSTANT().instance));
            break;
        case OP_GET_CLASS:
            PUSH((machine.class_stack + READ_BYTE())->as);
            break;
        case OP_RM:
            RM();
            break;
        case OP_ALLOC_TABLE:
            if (PEEK().type != ARENA && PEEK().arena.type != ARENA_INT)
            {
                runtime_error("ERROR: Table argument must be a numeric value.");
                return INTERPRET_RUNTIME_ERR;
            }
            PUSH(TABLE(GROW_TABLE(NULL, POP().arena.as.Int)));
            break;
        case OP_GET_GLOBAL:
        {

            Arena var = READ_CONSTANT().arena;
            Element el = FIND_GLOB(var);

            if (el.type != NULL_OBJ)
            {
                PUSH(el);
                break;
            }

            runtime_error("ERROR: Undefined property '%s'.", var.as.String);
            return INTERPRET_RUNTIME_ERR;
        }
        case OP_SET_GLOBAL:
        case OP_GLOBAL_DEF:
        {
            Element el = READ_CONSTANT();
            Element res = POP();

            if (res.type == CLOSURE)
                res.closure->func->name = el.arena;
            WRITE_GLOB(el.arena, res);
        }
        break;
        case OP_SET_FUNC_VAR:
        {
            Element el = READ_CONSTANT();
            Element res = (machine.cargc < machine.argc)
                              ? (frame->slots + machine.cargc++)->as
                              : POP();

            if (res.type == CLOSURE)
                res.closure->func->name = el.arena;
            WRITE_GLOB(el.arena, res);
        }
        break;

        case OP_PRINT:
            print_line(POP());
            break;
        case OP_RETURN:
        {
            Element el = POP();
            --machine.frame_count;

            if (machine.frame_count == 0)
            {
                POP();
                return INTERPRET_SUCCESS;
            }
            for (Stack *s = machine.stack; s < machine.stack->top; s++)
                POP();

            machine.stack->top = frame->slots;
            PUSH(el);

            frame = &machine.frames[machine.frame_count - 1];
            break;
        }
        }
    }

#undef RM
#undef CPOP
#undef POP
#undef WRITE_GLOB
#undef WRITE_PARAM
#undef FIND_GLOB
#undef FIND_PARAM
#undef PPUSH
#undef CPUSH
#undef PUSH
#undef JUMP
#undef LOCAL
#undef POPN
#undef POP
#undef FALSEY
#undef NPEEK
#undef PEEK
#undef GET_NATIVE
#undef GET_FUNC
#undef READ_CONSTANT
#undef READ_SHORT
#undef READ_BYTE
}
