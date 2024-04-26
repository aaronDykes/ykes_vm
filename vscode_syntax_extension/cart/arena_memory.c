#include "arena_memory.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>

static void *request_system_memory(size_t size)
{

    return mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE,

        POSIX_MADV_RANDOM |
            POSIX_MADV_NORMAL |
            POSIX_MADV_WILLNEED |
            MAP_PRIVATE |
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
            MAP_ANON,
#endif
        -1, 0);
}
void initialize_global_memory(void)
{

    mem = request_system_memory(OFFSET + (OFFSET * PAGE));
    mem->size = OFFSET;
    mem->next = mem + OFFSET;
    mem->next->size = PAGE;
    mem->next->next = NULL;
}

void destroy_global_memory(void)
{

    mem = mem->next;
    mem->size += OFFSET;

    while (mem)
    {

        Free *tmp = mem->next;
        munmap(mem, mem->size);
        mem = tmp;
    }
}

Arena arena_init(void *data, size_t size, T type)
{
    Arena ar;

    switch (type)
    {
    case ARENA_BYTES:
        ar.listof.Bytes = data;
        ar.len = (int)size;
        ar.count = 0;
        break;
    case ARENA_STR:
    case ARENA_FUNC:
    case ARENA_VAR:
    case ARENA_NATIVE:
        ar.as.String = data;
        ar.as.len = (int)size;
        ar.as.count = 0;
        break;
    case ARENA_INTS:
        ar.listof.Ints = data;
        ar.len = (int)(size / sizeof(int));
        ar.count = 0;
        break;
    case ARENA_DOUBLES:
        ar.listof.Doubles = data;
        ar.len = ((int)(size / sizeof(double)));
        ar.count = 0;
        break;
    case ARENA_LONGS:
        ar.listof.Longs = data;
        ar.len = ((int)(size / sizeof(long long int)));
        ar.count = 0;
        break;
    case ARENA_STRS:
        ar.listof.Strings = data;
        ar.len = ((int)(size / sizeof(char *)));
        ar.count = 0;
        break;
    default:
        return Null();
    }
    ar.size = size;
    ar.type = type;
    return ar;
}

/**
    TODO:
        free off each String in String arena
*/

void arena_free(Arena *ar)
{

    if (!ar)
        return;

    void *new = NULL;

    switch (ar->type)
    {
    case ARENA_BYTES:

        if (!ar->listof.Bytes)
            return;
        new = (void *)ar->listof.Bytes;
        ar->listof.Bytes = NULL;
        break;
    case ARENA_STR:
    case ARENA_FUNC:
    case ARENA_NATIVE:
    case ARENA_VAR:
        if (!ar->as.String)
            return;
        new = (void *)ar->as.String;
        ar->as.String = NULL;
        break;
    case ARENA_INTS:
        if (!ar->listof.Ints)
            return;
        new = (void *)ar->listof.Ints;
        ar->listof.Ints = NULL;
        break;
    case ARENA_DOUBLES:
        if (!ar->listof.Doubles)
            return;
        new = (void *)ar->listof.Doubles;
        ar->listof.Doubles = NULL;
        break;
    case ARENA_LONGS:
        if (!ar->listof.Longs)
            return;
        new = (void *)ar->listof.Longs;
        ar->listof.Longs = NULL;
        break;
    case ARENA_STRS:
        if (!ar->listof.Strings)
            return;
        new = (void *)ar->listof.Strings;
        ar->listof.Strings = NULL;
        break;
    default:
        return;
    }

    FREE(PTR(new));
    ar = NULL;
}

static void merge_list(void)
{

    Free *prev = NULL;
    for (Free *free = mem->next; free; free = free->next)
    {

        prev = free;
        if (free->next && ((unsigned long)free + OFFSET + free->size) == (unsigned long)free->next)
        {
            prev->size += free->next->size;
            prev->next = free->next->next;
        }
    }
}

void free_ptr(Free *new)
{

    if (!new)
        return;
    if (new->size == 0)
        return;

    Free *free = NULL, *prev = NULL;

    for (free = mem->next; free->next && (((unsigned long)free < (unsigned long)new)); free = free->next)
        prev = free->next;

    if (free && free < new)
    {
        new->next = free->next;
        free->next = new;
    }
    else if (prev && prev < new)
    {
        new->next = prev->next;
        prev->next = new;
    }

    merge_list();
    new = NULL;
    prev = NULL;
    free = NULL;
}

void *alloc_ptr(size_t size)
{

    Free *prev = NULL;
    Free *free = NULL;
    Free *alloced = NULL;

    for (free = mem->next; free && free->size < size; free = free->next)
        prev = free;

    if (free && free->size >= size)
    {

        size_t tmp = free->size - size;
        alloced = free + tmp;
        alloced->size = tmp - OFFSET;
        alloced += 1;

        free->size = tmp;

        if (prev && tmp == 0)
            prev->next = free->next;
        else if (!prev && tmp == 0)
            mem->next = free->next;

        return alloced;
    }

    if (prev)
    {
        size_t tmp = PAGE;
        while (size > tmp)
            tmp *= INC;

        prev->next = request_system_memory(tmp * OFFSET);
        prev->next->size = tmp - size;

        alloced = prev->next + prev->next->size;
        alloced->size = prev->next->size - OFFSET;
        alloced += 1;

        return alloced;
    }

    return NULL;
}
Arena *arena_alloc_arena(size_t size)
{
    Arena *p = NULL;
    p = ALLOC((size * sizeof(Arena)) + sizeof(Arena));

    p->size = size;
    p->count = 0;
    p->size = size;
    p->len = (int)size;

    return p + 1;
}

Arena *arena_realloc_arena(Arena *ar, size_t size)
{
    Arena *ptr = NULL;
    size_t new_size = 0;

    if (!ar && size != 0)
    {
        ptr = arena_alloc_arena(size);
        return ptr;
    }
    if (size == 0)
    {
        arena_free_arena(ar);
        --ar;
        ar = NULL;
        return NULL;
    }

    ptr = arena_alloc_arena(size);

    if (size > (ar - 1)->size)
        new_size = (ar - 1)->size;
    else
        new_size = size;

    for (size_t i = 0; i < new_size; i++)
        ptr[i] = ar[i];

    (ptr - 1)->count = (ar - 1)->count;

    FREE(PTR((ar - 1)));
    --ar;
    ar = NULL;
    return ptr;
}

void arena_free_arena(Arena *ar)
{
    if (!(ar - 1))
        return;

    if ((ar - 1)->count == 0)
    {
        FREE(PTR((ar - 1)));
        --ar;
        ar = NULL;
        return;
    }

    for (size_t i = 0; i < (ar - 1)->size; i++)
        switch (ar[i].type)
        {
        case ARENA_BYTES:
        case ARENA_INTS:
        case ARENA_DOUBLES:
        case ARENA_LONGS:
        case ARENA_BOOLS:
        case ARENA_STR:
        case ARENA_STRS:
        case ARENA_FUNC:
        case ARENA_NATIVE:
        case ARENA_VAR:
            ARENA_FREE(&ar[i]);
            break;
        default:
            return;
        }

    FREE(PTR(((ar - 1))));
    --ar;
    ar = NULL;
}

Arena arena_alloc(size_t size, T type)
{

    void *ptr = NULL;
    ptr = ALLOC(size);
    return arena_init(ptr, size, type);
}

Arena arena_realloc(Arena *ar, size_t size, T type)
{

    if (size == 0)
    {
        ARENA_FREE(ar);
        return Null();
    }

    void *ptr = NULL;
    ptr = ALLOC(size);

    if (!ar && size != 0)
        return arena_init(ptr, size, type);

    size_t new_size = 0;

    if (size > ar->size)
        new_size = ar->size;
    else
        new_size = size;

    switch (type)
    {
    case ARENA_BYTES:
        if (!ar->listof.Bytes)
            return arena_init(ptr, size, type);
        memcpy(ptr, ar->listof.Bytes, new_size);
        break;
    case ARENA_STR:
    case ARENA_CSTR:
    case ARENA_VAR:
    case ARENA_FUNC:
    case ARENA_NATIVE:
        if (!ar->as.String)
            return arena_init(ptr, size, type);
        memcpy(ptr, ar->as.String, new_size);
        break;
    case ARENA_INTS:
        if (!ar->listof.Ints)
            return arena_init(ptr, size, type);
        memcpy(ptr, ar->listof.Ints, new_size);
        break;
    case ARENA_DOUBLES:
        if (!ar->listof.Doubles)
            return arena_init(ptr, size, type);
        memcpy(ptr, ar->listof.Doubles, new_size);
        break;
    case ARENA_LONGS:
        if (!ar->listof.Doubles)
            return arena_init(ptr, size, type);
        memcpy(ptr, ar->listof.Longs, new_size);
        break;

    case ARENA_STRS:
    {

        if (!ar->listof.Strings || ar->type == ARENA_NULL)
            return arena_init(ptr, size, type);
        Arena str = arena_init(ptr, size, type);
        for (size_t i = 0; i < new_size; i++)
            str.listof.Strings[i] = CString(ar->listof.Strings[i]).as.String;
    }
    break;

    default:
        return Null();
    }

    Arena a = arena_init(ptr, size, type);
    a.count = (ar->count > a.len)
                  ? a.len
                  : ar->count;
    ARENA_FREE(ar);
    return a;
}
Element cpy_array(Element el)
{

    Arena ar = el.arena;
    T type = ar.type;
    size_t size = ar.size;
    int len = ar.count;

    void *ptr = NULL;
    ptr = ALLOC(size);

    switch (type)
    {
    case ARENA_BYTES:
        if (!ar.listof.Bytes)
            return OBJ(arena_init(ptr, size, type));
        memcpy(ptr, ar.listof.Bytes, size);
        break;
    case ARENA_STR:
    case ARENA_CSTR:
    case ARENA_VAR:
    case ARENA_FUNC:
    case ARENA_NATIVE:
        if (!ar.as.String)
            return OBJ(arena_init(ptr, size, type));
        memcpy(ptr, ar.as.String, size);
        break;
    case ARENA_INTS:
        if (!ar.listof.Ints)
            return OBJ(arena_init(ptr, size, type));
        memcpy(ptr, ar.listof.Ints, size);
        break;
    case ARENA_DOUBLES:
        if (!ar.listof.Doubles)
            return OBJ(arena_init(ptr, size, type));
        memcpy(ptr, ar.listof.Doubles, size);
        break;
    case ARENA_LONGS:
        if (!ar.listof.Doubles)
            return OBJ(arena_init(ptr, size, type));
        memcpy(ptr, ar.listof.Longs, size);
        break;

    case ARENA_STRS:
    {

        if (!ar.listof.Strings || ar.type == ARENA_NULL)
            return OBJ(arena_init(ptr, size, type));
        Arena str = arena_init(ptr, size, type);
        for (int i = 0; i < len; i++)
            str.listof.Strings[i] = CString(ar.listof.Strings[i]).as.String;
    }
    break;

    default:
        return null_obj();
    }

    Arena a = arena_init(ptr, size, type);
    a.count = (ar.count > a.len)
                  ? a.len
                  : ar.count;
    return OBJ(a);
}

Arena Char(char Char)
{
    Arena ar;
    ar.type = ARENA_CHAR;
    ar.as.Char = Char;
    ar.size = sizeof(char);
    long long int k = hash(ar);
    ar.as.hash = k;
    return ar;
}
Arena Int(int Int)
{
    Arena ar;
    ar.type = ARENA_INT;
    ar.as.Int = Int;
    ar.size = sizeof(int);
    ar.as.hash = Int;
    return ar;
}
Arena Byte(uint8_t Byte)
{
    Arena ar;
    ar.type = ARENA_BYTE;
    ar.as.Byte = Byte;
    ar.size = sizeof(uint8_t);
    return ar;
}
Arena Long(long long int Long)
{
    Arena ar;
    ar.type = ARENA_LONG;
    ar.as.Long = Long;
    ar.size = sizeof(long long int);
    long long int k = hash(ar);
    ar.as.hash = k;
    return ar;
}
Arena Double(double Double)
{
    Arena ar;
    ar.type = ARENA_DOUBLE;
    ar.as.Double = Double;
    ar.size = sizeof(double);
    long long int h = hash(ar);
    ar.as.hash = h;
    return ar;
}
Arena String(const char *str)
{
    size_t size = strlen(str);
    Arena ar = arena_alloc(size, ARENA_STR);
    memcpy(ar.as.String, str, size);
    ar.as.String[size] = '\0';
    ar.size = size;
    long long int k = hash(ar);
    ar.as.hash = k;
    return ar;
}
Arena CString(const char *str)
{
    size_t size = strlen(str);
    Arena ar;
    ar.as.String = (char *)str;
    ar.size = size;
    ar.type = ARENA_CSTR;
    ar.as.len = (int)size;
    long long int k = hash(ar);
    ar.as.hash = k;
    return ar;
}

Arena Bool(bool Bool)
{
    Arena ar;
    ar.type = ARENA_BOOL;
    ar.as.Bool = Bool;
    ar.size = sizeof(bool);
    return ar;
}
Arena Size(size_t Size)
{
    Arena ar;
    ar.type = ARENA_SIZE;
    ar.as.Size = Size;
    ar.size = 1;
    long long int k = hash(ar);
    ar.as.hash = k;
    return ar;
}
Arena Null(void)
{
    Arena ar;
    ar.type = ARENA_NULL;
    ar.as.Void = NULL;
    return ar;
}

Arena Ints(int *Ints, int len)
{
    return arena_init(Ints, sizeof(int) * len, ARENA_INTS);
}
Arena Doubles(double *Doubles, int len)
{
    return arena_init(Doubles, sizeof(double) * len, ARENA_DOUBLES);
}

Arena Longs(long long int *Longs, int len)
{
    return arena_init(Longs, sizeof(long long int) * len, ARENA_LONGS);
}

Arena Strings(void)
{
    return arena_init(NULL, 0, ARENA_STRS);
}

static void int_push(int **Ints, int index, int Int)
{
    (*Ints)[index] = Int;
}
static void double_push(double **Doubles, int index, double Double)
{
    (*Doubles)[index] = Double;
}
static void long_push(long long int **Longs, int index, long long int Long)
{
    (*Longs)[index] = Long;
}

void push_arena(Element *el, Arena ar)
{
    if ((el->arena_vector - 1)->len < (el->arena_vector - 1)->count + 1)
    {
        (el->arena_vector - 1)->len = GROW_CAPACITY((el->arena_vector - 1)->len);
        el->arena_vector = GROW_ARENA(el->arena_vector, (el->arena_vector - 1)->len);
    }

    el->arena_vector[(el->arena_vector - 1)->count++] = ar;
}

Element pop_arena(Element *el)
{
    Element tmp = OBJ(el->arena_vector[(el->arena_vector - 1)->count - 1]);
    el->arena_vector[--(el->arena_vector - 1)->count] = Null();
    return tmp;
}

void push_int(Element *el, int Int)
{
    if (el->arena.len < el->arena.count + 1)
    {
        el->arena.len = GROW_CAPACITY(el->arena.len);

        el->arena.size = el->arena.len * sizeof(int);
        el->arena = GROW_ARRAY(&el->arena, el->arena.size, ARENA_INTS);
    }

    int_push(&el->arena.listof.Ints, el->arena.count++, Int);
}
Element pop_int(Element *el)
{

    Element tmp = OBJ(Int(el->arena.listof.Ints[el->arena.count - 1]));
    el->arena.listof.Ints[--el->arena.count] = 0;
    return tmp;
}
void push_double(Element *el, double Double)
{
    if (el->arena.len < el->arena.count + 1)
    {
        el->arena.len = GROW_CAPACITY(el->arena.len);
        el->arena.size = el->arena.len * sizeof(double);
        el->arena = GROW_ARRAY(&el->arena, el->arena.size, ARENA_DOUBLES);
    }
    double_push(&el->arena.listof.Doubles, el->arena.count++, Double);
}
Element pop_double(Element *el)
{

    Element tmp = OBJ(Double(el->arena.listof.Doubles[el->arena.count - 1]));
    el->arena.listof.Doubles[--el->arena.count] = 0;
    return tmp;
}
void push_long(Element *el, long long int Long)
{
    if (el->arena.len < el->arena.count + 1)
    {
        el->arena.len = GROW_CAPACITY(el->arena.len);
        el->arena.size = el->arena.len * sizeof(long long int);
        el->arena = GROW_ARRAY(&el->arena, el->arena.size, ARENA_LONGS);
    }
    // el->arena.listof
    long_push(&el->arena.listof.Longs, el->arena.count++, Long);
}
Element pop_long(Element *el)
{

    Element tmp = OBJ(Long(el->arena.listof.Longs[el->arena.count - 1]));
    el->arena.listof.Longs[--el->arena.count] = 0;
    return tmp;
}

void push_string(Element *el, const char *String)
{
    if (el->arena.len < el->arena.count + 1)
    {
        el->arena.len = GROW_CAPACITY(el->arena.len);
        el->arena.size = el->arena.len * sizeof(char *);
        el->arena = GROW_ARRAY(&el->arena, el->arena.size, ARENA_STRS);
    }
    el->arena.listof.Strings[el->arena.count++] = (char *)String;
}
Element pop_string(Element *el)
{

    Element tmp = OBJ(CString(el->arena.listof.Strings[el->arena.count - 1]));
    el->arena.listof.Strings[--el->arena.count] = NULL;
    return tmp;
}

Class *class(Arena name)
{
    Class *c = NULL;
    c = ALLOC(sizeof(Class));
    c->name = name;
    c->init = NULL;
    c->fields = NULL;
    return c;
}

void free_class(Class *c)
{

    ARENA_FREE(&c->name);
    arena_free_table(c->fields);
    FREE(PTR(c));
}

Instance *instance(Class *classc)
{
    Instance *ic = NULL;
    ic = ALLOC(sizeof(Instance));
    ic->classc = classc;
    return ic;
}
void free_instance(Instance *ic)
{
    FREE(PTR(ic));
    ic = NULL;
}

Stack *stack(size_t size)
{
    Stack *s = NULL;
    s = ALLOC((size * sizeof(Stack)) + sizeof(Stack));
    s->size = size;

    (s + 1)->len = (int)size;
    (s + 1)->count = 0;
    (s + 1)->top = s + 1;
    return s + 1;
}
Stack *realloc_stack(Stack *st, size_t size)
{

    if (size == 0)
    {
        FREE_STACK(&st);
        --st;
        st = NULL;
        return NULL;
    }
    Stack *s = NULL;
    s = NEW_STACK(size);

    if (!st)
        return s;

    size_t new_size = 0;
    if (size > (st - 1)->size)
        new_size = (st - 1)->size;
    else
        new_size = size;

    for (size_t i = 0; i < new_size; i++)
        s[i].as = st[i].as;

    s->count = st->count;
    s->top = s;
    s->top += s->count;
    FREE(PTR((st - 1)));
    --st;
    st = NULL;
    return s;
}
void free_stack(Stack **stack)
{
    if (!stack)
        return;
    if (!(*stack) - 1)
        return;

    if (((*stack) - 1)->count == 0)
    {
        FREE(PTR(((*stack) - 1)));
        stack = NULL;
        return;
    }

    for (size_t i = 0; i < (*stack - 1)->size; i++)
        switch ((*stack)[i].as.type)
        {
        case ARENA:
            ARENA_FREE(&(*stack)[i].as.arena);
            break;
        case NATIVE:
            FREE_NATIVE((*stack)[i].as.native);
            break;
        case CLASS:
            FREE_CLASS((*stack)[i].as.classc);
            break;
        case CLOSURE:
            FREE_CLOSURE(&(*stack)[i].as.closure);
            break;
        case INSTANCE:
            FREE_INSTANCE((*stack)[i].as.instance);
            break;
        case VECTOR:
            FREE_ARENA((*stack)[i].as.arena_vector);
            break;
        case STACK:
            FREE_STACK(&(*stack)[i].as.stack);
            break;
        case TABLE:
            arena_free_table((*stack)[i].as.table);
            break;
        default:
            break;
        }
    FREE(PTR(((*stack) - 1)));
    stack = NULL;
}

Upval **upvals(size_t size)
{
    Upval **up = NULL;
    ALLOC((sizeof(Upval *) * size) + sizeof(Upval *));

    *up = NULL;
    *up = ALLOC(sizeof(Upval));

    (*up)->size = size;

    for (size_t i = 1; i < size; i++)
        up[i] = NULL;

    return up + 1;
}

void free_upvals(Upval **up)
{
    if (!up)
        return;
    if (!((*up) - 1))
        return;
    for (size_t i = 0; i < ((*up) - 1)->size; i++)
        (*up)[i].index = NULL;

    FREE(PTR(((*up) - 1)));
    FREE(PTR((up - 1)));
    --up;
    up = NULL;
}

Stack value(Element e)
{
    Stack s;
    s.as = e;
    return s;
}

Element stack_el(Stack *el)
{
    Element e;
    e.stack = el;
    e.type = STACK;
    return e;
}

Element Obj(Arena ar)
{
    Element s;
    s.arena = ar;
    s.type = ARENA;

    return s;
}

Element native_fn(Native *native)
{
    Element s;

    s.native = native;
    s.type = NATIVE;

    return s;
}

Element closure(Closure *closure)
{
    Element el;
    el.closure = closure;
    el.type = CLOSURE;

    return el;
}

Element new_class(Class *classc)
{
    Element el;
    el.classc = classc;
    el.type = CLASS;
    return el;
}

Element new_instance(Instance *ci)
{
    Element el;
    el.instance = ci;
    el.type = INSTANCE;
    return el;
}
Element table_el(Table *t)
{
    Element el;
    el.table = t;
    el.type = TABLE;
    return el;
}
Element vector(Arena *vect)
{
    Element el;
    el.arena_vector = vect;
    el.type = VECTOR;
    return el;
}
Element null_obj(void)
{
    Element el;
    el.null = NULL;
    el.type = NULL_OBJ;

    return el;
}

Function *function(Arena name)
{
    Function *func = ALLOC(sizeof(Function));
    func->arity = 0;
    func->upvalue_count = 0;
    func->name = name;
    init_chunk(&func->ch);

    return func;
}
void free_function(Function *func)
{
    if (!func)
        return;
    if (func->name.type != ARENA_NULL)
        FREE_ARRAY(&func->name);
    free_chunk(&func->ch);

    FREE(PTR(func));
    func = NULL;
}

Native *native(NativeFn func, Arena ar)
{
    Native *native = NULL;
    native = ALLOC(sizeof(Native));
    native->fn = func;
    native->obj = ar;
    return native;
}

void free_native(Native *native)
{

    if (!native)
        return;

    ARENA_FREE(&native->obj);
    FREE(PTR(native));
    native = NULL;
}
Closure *new_closure(Function *func)
{
    Closure *closure = NULL;
    closure = ALLOC(sizeof(Closure));
    closure->func = func;
    if (!func)
    {
        closure->upval_count = 0;
        return closure;
    }
    if (func->upvalue_count > 0)
        closure->upvals = upvals(func->upvalue_count);
    else
        closure->upvals = NULL;
    closure->upval_count = func->upvalue_count;

    return closure;
}

void free_closure(Closure **closure)
{
    if (!(*closure))
        return;

    FREE_UPVALS((*closure)->upvals);
    FREE(PTR(closure));
    (*closure) = NULL;
    closure = NULL;
}

Upval *upval(Stack *index)
{
    Upval *up = NULL;
    up = ALLOC(sizeof(Upval));
    up->index = index;
    up->closed = *index;
    up->next = NULL;
    return up;
}
void free_upval(Upval *up)
{
    if (!up)
        return;
    up->index = NULL;
    up->next = NULL;
}

void init_chunk(Chunk *c)
{
    c->op_codes.len = 0;
    c->op_codes.count = 0;
    c->op_codes.listof.Bytes = NULL;
    c->lines.len = 0;
    c->lines.count = 0;
    c->lines.listof.Ints = NULL;
    c->cases.len = 0;
    c->cases.count = 0;
    c->cases = GROW_ARRAY(NULL, MIN_SIZE, ARENA_INTS);
    c->cases.len = PAGE_COUNT;
    c->constants = GROW_STACK(NULL, STACK_SIZE);
}

void free_chunk(Chunk *c)
{
    if (!c)
    {
        init_chunk(c);
        return;
    }
    if (c->op_codes.listof.Bytes)
        FREE_ARRAY(&c->op_codes);
    if (c->cases.listof.Ints)
        FREE_ARRAY(&c->cases);
    if (c->lines.listof.Ints)
        FREE_ARRAY(&c->lines);
    c->constants = NULL;
    init_chunk(c);
}

void arena_free_table(Table *t)
{
    if (!(t - 1))
        return;
    if (!t)
        return;

    size_t size = (t - 1)->size;

    if ((t - 1)->count == 0)
    {
        FREE(PTR((t - 1)));
        --t;
        t = NULL;
        return;
    }
    for (size_t i = 0; i < size; i++)
        FREE_TABLE_ENTRY(&t[i]);

    FREE(PTR((t - 1)));
    --t;
    t = NULL;
}

void arena_free_entry(Table *entry)
{
    if (entry->type == ARENA)
        FREE_ARRAY(&entry->val.arena);
    else if (entry->type == NATIVE)
        FREE_NATIVE(entry->val.native);
    else if (entry->type == CLASS)
        FREE_CLASS(entry->val.classc);
    else if (entry->type == INSTANCE)
        FREE_INSTANCE(entry->val.instance);
    else if (entry->type == TABLE)
        arena_free_table(entry->val.table);
    else if (entry->type == VECTOR)
        FREE_ARENA(entry->val.arena_vector);
    else
        FREE_CLOSURE(&entry->val.closure);

    FREE_ARRAY(&entry->key);

    entry->next = NULL;
    entry->prev = NULL;
    entry = NULL;
}

Arena Var(const char *str)
{
    size_t size = strlen(str);
    Arena ar = GROW_ARRAY(NULL, size, ARENA_VAR);
    memcpy(ar.as.String, str, size);
    ar.as.String[size] = '\0';
    size_t h = hash(ar);
    ar.as.hash = h;
    ar.type = ARENA_VAR;
    return ar;
}

Arena func_name(const char *str)
{
    size_t size = strlen(str);
    Arena ar = GROW_ARRAY(NULL, size, ARENA_FUNC);
    memcpy(ar.as.String, str, size);
    ar.as.String[size] = '\0';
    size_t h = hash(ar);
    ar.as.hash = h;
    ar.type = ARENA_FUNC;
    return ar;
}
Arena native_name(const char *str)
{
    size_t size = strlen(str);
    Arena ar = GROW_ARRAY(NULL, size, ARENA_NATIVE);
    memcpy(ar.as.String, str, size);
    ar.as.String[size] = '\0';
    size_t h = hash(ar);
    ar.as.hash = h;
    ar.type = ARENA_NATIVE;
    return ar;
}

long long int hash(Arena key)
{
    long long int index = 2166136261u;

    switch (key.type)
    {
    case ARENA_VAR:
    case ARENA_FUNC:
    case ARENA_STR:
    case ARENA_CSTR:
    case ARENA_NATIVE:
        for (char *s = key.as.String; *s; s++)
        {
            index ^= (int)*s;
            index *= 16777619;
        }
        break;
    case ARENA_INT:
        index ^= key.as.Int;
        index = (index * 16777669);
        break;
    case ARENA_DOUBLE:
        index ^= ((long long int)key.as.Double);
        index = (index * 16777420);
        break;
    case ARENA_LONG:
        index ^= key.as.Long;
        index = (index * 16776969);
        break;
    case ARENA_CHAR:
        index ^= key.as.Char;
        index = (index * 16742069);
        break;
    default:
        return 0;
    }
    return index;
}

static void parse_str(const char *str)
{
    char *s = (char *)str;

    for (; *s; s++)
        if (*s == '\\' && s[1] == 'n')
            printf("\n"), s++;
        else if (*s == '\\' && s[1] == 't')
            printf("\t"), s++;
        else
            printf("%c", *s);
}

void print(Element ar)
{
    Arena a = ar.arena;

    if (ar.type == NATIVE)
    {
        printf("<native: %s>", ar.native->obj.as.String);
        return;
    }
    if (ar.type == CLOSURE)
    {
        printf("<fn: %s>", ar.closure->func->name.as.String);
        return;
    }
    if (ar.type == CLASS)
    {
        printf("<class: %s>", ar.classc->name.as.String);
        return;
    }
    if (ar.type == INSTANCE)
    {
        if (!ar.instance->classc)
            return;
        printf("<instance: %s>", ar.instance->classc->name.as.String);
        return;
    }
    if (ar.type == VECTOR)
    {
        printf("[\n");

        for (int i = 0; i < (ar.arena_vector - 1)->count; i++)
        {
            print(OBJ(ar.arena_vector[i]));
            if (i != (ar.arena_vector - 1)->count - 1)
                printf(", ");
        }

        printf("\n]\n");
        return;
    }

    if (ar.type == STACK)
    {
        printf("{ ");

        if (ar.stack->count == 0)
        {
            printf(" }");
            return;
        }
        for (int i = 0; i < ar.stack->count; i++)
        {
            print((ar.stack + i)->as);
            if (i != ar.stack->count - 1)
                printf(",\n");
        }

        printf(" }");
        return;
    }

    switch (a.type)
    {
    case ARENA_BYTE:
        printf("%d", a.as.Byte);
        break;
    case ARENA_CHAR:
        printf("%c", a.as.Char);
        break;
    case ARENA_DOUBLE:
        printf("%f", a.as.Double);
        break;
    case ARENA_INT:
        printf("%d", a.as.Int);
        break;
    case ARENA_LONG:
        printf("%lld", a.as.Long);
        break;
    case ARENA_BOOL:
        printf("%s", (a.as.Bool == true) ? "true" : "false");
        break;
    case ARENA_STR:
    case ARENA_VAR:
    case ARENA_FUNC:
    case ARENA_CSTR:
        parse_str(a.as.String);
        break;
    case ARENA_INTS:
        printf("[ ");
        for (int i = 0; i < a.count; i++)
            if (i == a.count - 1)
                printf("%d ]", a.listof.Ints[i]);
            else
                printf("%d, ", a.listof.Ints[i]);
        break;
    case ARENA_DOUBLES:
        printf("[ ");
        for (int i = 0; i < a.count; i++)
            if (i == a.count - 1)
                printf("%f ]", a.listof.Doubles[i]);
            else
                printf("%f, ", a.listof.Doubles[i]);
        break;
    case ARENA_STRS:
        printf("[ ");
        for (int i = 0; i < a.count; i++)
            if (i == a.count - 1)
                printf("%s ]", a.listof.Strings[i]);
            else
                printf("%s, ", a.listof.Strings[i]);
        break;
    case ARENA_NULL:
        printf("[ null ]");
        break;

    default:
        return;
    }
}
void print_line(Element ar)
{
    Arena a = ar.arena;

    if (ar.type == NATIVE)
    {
        printf("<native: %s>\n", ar.native->obj.as.String);
        return;
    }
    if (ar.type == CLOSURE)
    {
        printf("<fn: %s>\n", ar.closure->func->name.as.String);
        return;
    }
    if (ar.type == CLASS)
    {
        if (!ar.classc)
            return;
        printf("<class: %s>\n", ar.classc->name.as.String);
        return;
    }
    if (ar.type == INSTANCE)
    {
        if (!ar.instance->classc)
            return;
        printf("<instance: %s>\n", ar.instance->classc->name.as.String);
        return;
    }
    if (ar.type == VECTOR)
    {
        printf("[");

        int count = (ar.arena_vector - 1)->count;
        if (count == 0)
        {
            printf(" ]\n");
            return;
        }

        printf("\n");
        for (int i = 0; i < (ar.arena_vector - 1)->count; i++)
        {
            print(OBJ(ar.arena_vector[i]));
            if (i != (ar.arena_vector - 1)->count - 1)
                printf(", ");
        }

        printf("\n]\n");
        return;
    }

    if (ar.type == STACK)
    {
        if (!ar.stack)
            return;
        printf("{");

        if (ar.stack->count == 0)
        {
            printf(" }\n");
            return;
        }
        printf("\n");
        for (int i = 0; i < ar.stack->count; i++)
        {
            print((ar.stack + i)->as);
            if (i != ar.stack->count - 1)
                printf(",\n");
        }

        printf("\n}\n");
        return;
    }

    switch (a.type)
    {
    case ARENA_BYTE:
        printf("%d\n", a.as.Byte);
        break;
    case ARENA_CHAR:
        printf("%c\n", a.as.Char);
        break;
    case ARENA_DOUBLE:
        printf("%f\n", a.as.Double);
        break;
    case ARENA_INT:
        printf("%d\n", a.as.Int);
        break;
    case ARENA_LONG:
        printf("%lld\n", a.as.Long);
        break;
    case ARENA_BOOL:
        printf("%s\n", (a.as.Bool == true) ? "true" : "false");
        break;
    case ARENA_STR:
    case ARENA_VAR:
    case ARENA_FUNC:
    case ARENA_CSTR:
        parse_str(a.as.String);
        printf("\n");
        break;
    case ARENA_INTS:
        printf("[ ");
        for (int i = 0; i < a.count; i++)
            if (i == a.count - 1)
                printf("%d ]\n", a.listof.Ints[i]);
            else
                printf("%d, ", a.listof.Ints[i]);
        break;
    case ARENA_DOUBLES:
        printf("[ ");
        for (int i = 0; i < a.count; i++)
            if (i == a.count - 1)
                printf("%f ]\n", a.listof.Doubles[i]);
            else
                printf("%f, ", a.listof.Doubles[i]);
        break;
    case ARENA_STRS:
        printf("[ ");
        for (int i = 0; i < a.count; i++)
            if (i == a.count - 1)
            {

                parse_str(a.listof.Strings[i]);
                printf(" ]\n");
            }
            else
            {
                parse_str(a.listof.Strings[i]);
                printf(", ");
            }
        break;
    case ARENA_NULL:
        printf("[ null ]\n");
        break;

    default:
        return;
    }
}
