#include "arena_table.h"

void insert_entry(Table **t, Table entry)
{
    Table *tmp = *t;
    size_t index = entry.key.as.hash & ((tmp - 1)->len - 1);
    Table e = tmp[index];
    Table *ptr = e.next;

    if (e.key.type == ARENA_NULL)
    {
        tmp[index] = entry;
        return;
    }

    if (e.key.as.hash == entry.key.as.hash)
    {

        tmp[index] = new_entry(entry);
        return;
    }

    Element el = find_entry(t, &entry.key);

    if (el.type == NULL_OBJ)
    {
        ALLOC_ENTRY(&tmp[index].next, entry);
        return;
    }

    for (; ptr; ptr = ptr->next)
        if (ptr->key.as.hash == entry.key.as.hash)
            goto END;

    return;
END:
    FREE_ENTRY(ptr->val);
    ptr->val = entry.val;
}

void free_entry(Element el)
{

    switch (el.type)
    {
    case ARENA:
        ARENA_FREE(&el.arena);
        break;
    case NATIVE:
        FREE_NATIVE(el.native);
        break;
    case CLOSURE:
        FREE_CLOSURE(&el.closure);
        break;
    case CLASS:
        FREE_CLASS(el.classc);
        break;
    case INSTANCE:
        FREE_INSTANCE(el.instance);
        break;
    case VECTOR:
        FREE_ARENA(el.arena_vector);
        break;
    case TABLE:
        FREE_TABLE(el.table);
        break;
    case STACK:
        FREE_STACK(&el.stack);
        break;
    default:
        break;
    }
}

void delete_entry(Table **t, Arena key)
{
    Table *a = NULL;
    a = *t;
    size_t index = key.as.hash & ((a - 1)->len - 1);
    Table e = a[index];

    if (e.key.type == ARENA_NULL || key.type == ARENA_NULL)
        return;

    if (e.next && (e.key.as.hash == key.as.hash))
    {
        Table *t = a[index].next;
        FREE_TABLE_ENTRY(&a[index]);
        a[index] = arena_entry(Null(), Null());
        t->prev = NULL;
        a[index] = new_entry(*t);

        return;
    }
    if (!e.next && (e.key.as.hash == key.as.hash))
    {
        FREE_TABLE_ENTRY(&a[index]);
        a[index] = arena_entry(Null(), Null());
        return;
    }

    Table *tmp = e.next;
    Table *del = NULL;

    if (!tmp->next)
    {
        FREE_TABLE_ENTRY(tmp);
        return;
    }

    for (; tmp->next; tmp = tmp->next)
        if (tmp->key.as.hash == key.as.hash)
            goto DEL;

    if (tmp->key.as.hash == key.as.hash)
        goto DEL_LAST;

    return;
DEL:
    del = tmp->prev;
    del->next = tmp->next;
    tmp->next->prev = del;
    FREE_TABLE_ENTRY(tmp);
    return;

DEL_LAST:
    del = tmp->prev;
    del->next = NULL;
    FREE_TABLE_ENTRY(tmp);
}

Element find_entry(Table **t, Arena *hash)
{
    Table *a = *t;
    size_t index = hash->as.hash & ((a - 1)->len - 1);
    Table entry = a[index];

    Element null_ = null_obj();
    if (entry.key.type == ARENA_NULL)
        return null_;

    if (entry.key.as.hash == hash->as.hash)
        switch (entry.type)
        {
        case ARENA:
            return OBJ(entry.val.arena);
        case NATIVE:
            return NATIVE(entry.val.native);
        case CLOSURE:
            return CLOSURE(entry.val.closure);
        case CLASS:
            return CLASS(entry.val.classc);
        case INSTANCE:
            return INSTANCE(entry.val.instance);
        case TABLE:
            return TABLE(entry.val.table);
        case VECTOR:
            return VECT(entry.val.arena_vector);
        case STACK:
            return STK(entry.val.stack);
        default:
            return null_;
        }

    Table *tmp = entry.next;

    for (; tmp; tmp = tmp->next)
        if (tmp->key.as.hash == hash->as.hash)
            switch (entry.type)
            {
            case ARENA:
                return OBJ(entry.val.arena);
            case NATIVE:
                return NATIVE(entry.val.native);
            case CLOSURE:
                return CLOSURE(entry.val.closure);
            case CLASS:
                return CLASS(entry.val.classc);
            case INSTANCE:
                return INSTANCE(entry.val.instance);
            case TABLE:
                return TABLE(entry.val.table);
            case VECTOR:
                return VECT(entry.val.arena_vector);
            case STACK:
                return STK(entry.val.stack);
            default:
                return null_;
            }

    return null_;
}

void alloc_entry(Table **e, Table el)
{
    Table *tmp = NULL;
    tmp = *e;

    if (!tmp)
    {
        *e = NULL;
        *e = ALLOC(sizeof(Table));
        **e = el;
        return;
    }
    for (; tmp->next; tmp = tmp->next)
        ;
    tmp->next = NULL;
    tmp->next = ALLOC(sizeof(Table));
    *tmp->next = el;
    tmp->next->prev = tmp;
}

Table new_entry(Table t)
{
    Table el;
    el.key = t.key;
    switch (t.type)
    {
    case ARENA:
        el.val.arena = t.val.arena;
        break;
    case NATIVE:
        el.val.native = t.val.native;
        break;
    case CLOSURE:
        el.val.closure = t.val.closure;
        break;
    case TABLE:
        el.val.table = t.val.table;
        break;
    case VECTOR:
        el.val.arena_vector = t.val.arena_vector;
        break;
    default:
        break;
    }
    el.next = NULL;
    el.prev = NULL;
    el.type = t.type;
    return el;
}

Table Entry(Arena key, Element val)
{
    switch (val.type)
    {
    case ARENA:
        return arena_entry(key, val.arena);
    case NATIVE:
        return native_entry(val.native);
    case CLOSURE:
        return func_entry(val.closure);
    case CLASS:
        return class_entry(val.classc);
    case INSTANCE:
        return instance_entry(key, val.instance);
    case TABLE:
        return table_entry(key, val.table);
    case VECTOR:
        return vector_entry(key, val.arena_vector);
    case STACK:
        return stack_entry(key, val.stack);
    default:
        return func_entry(NULL);
    }
}

Table arena_entry(Arena key, Arena val)
{
    Table el;
    el.key = key;
    el.val.arena = val;
    el.next = NULL;
    el.prev = NULL;
    el.size = key.size + val.size;
    el.type = ARENA;
    return el;
}
Table func_entry(Closure *clos)
{
    Table el;
    el.key = clos->func->name;
    el.val.closure = clos;
    el.next = NULL;
    el.prev = NULL;
    el.size = el.key.size;
    el.type = CLOSURE;
    return el;
}
Table native_entry(Native *func)
{
    Table el;
    el.key = func->obj;
    el.val.native = func;
    el.next = NULL;
    el.prev = NULL;
    el.size = el.key.size;
    el.type = NATIVE;
    return el;
}
Table class_entry(Class *c)
{
    Table el;
    el.key = c->name;
    el.val.classc = c;
    el.next = NULL;
    el.prev = NULL;
    el.size = el.key.size;
    el.type = CLASS;
    return el;
}
Table instance_entry(Arena ar, Instance *c)
{
    Table el;
    el.key = ar;
    el.val.instance = c;
    el.next = NULL;
    el.prev = NULL;
    el.size = el.key.size;
    el.type = INSTANCE;
    return el;
}

Table table_entry(Arena ar, Table *t)
{
    Table el;
    el.key = ar;
    el.val.table = t;
    el.next = NULL;
    el.prev = NULL;
    el.size = el.key.size;
    el.type = TABLE;
    return el;
}
Table vector_entry(Arena ar, Arena *arena_vector)
{
    Table el;
    el.key = ar;
    el.val.arena_vector = arena_vector;
    el.next = NULL;
    el.prev = NULL;
    el.size = el.key.size;
    el.type = VECTOR;
    return el;
}
Table stack_entry(Arena ar, Stack *s)
{
    Table el;
    el.key = ar;
    el.val.stack = s;
    el.next = NULL;
    el.prev = NULL;
    el.size = el.key.size;
    el.type = STACK;
    return el;
}

Table *arena_realloc_table(Table *t, size_t size)
{

    Table *ptr = NULL;

    if (!t && size != 0)
    {
        ptr = arena_alloc_table(size);
        return ptr;
    }
    if (size == 0)
    {
        arena_free_table(t);
        --t;
        t = NULL;
        return NULL;
    }
    size_t new_size = 0;

    if (size > (t - 1)->size)
        new_size = (t - 1)->size;
    else
        new_size = size;

    ptr = arena_alloc_table(size);

    for (size_t i = 0; i < new_size; i++)
    {
        ptr[i] = new_entry(t[i]);

        for (Table *tab = t[i].next; tab; tab = tab->next)
            insert_entry(&ptr, new_entry(*tab));
    }

    FREE(PTR((t - 1)));
    --t;
    t = NULL;
    return ptr;
}

void write_table(Table *t, Arena a, Element b)
{

    if (b.type == CLOSURE)
    {
        if (find_entry(&t, &b.closure->func->name).type != NULL_OBJ)
            goto OVERWRITE;
    }
    else if (b.type == NATIVE)
    {
        if (find_entry(&t, &b.native->obj).type != NULL_OBJ)
            goto OVERWRITE;
    }
    else if (b.type == CLASS)
    {
        if (find_entry(&t, &b.classc->name).type != NULL_OBJ)
            goto OVERWRITE;
    }
    else
    {
        if (find_entry(&t, &a).type != NULL_OBJ)
            goto OVERWRITE;
    }

    int load_capacity = (int)((t - 1)->len * LOAD_FACTOR);

    if (load_capacity < (t - 1)->count + 1)
    {
        (t - 1)->len *= INC;
        t = GROW_TABLE(t, t->len);
    }
    (t - 1)->count++;

OVERWRITE:
    insert_entry(&t, Entry(a, b));
}

Table *arena_alloc_table(size_t size)
{
    Table *t = NULL;
    t = ALLOC((size * sizeof(Table)) + OFFSET);

    size_t n = (size_t)size + 1;

    for (size_t i = 1; i < n; i++)
        t[i] = arena_entry(Null(), Null());

    t->size = n;
    t->len = (int)size;
    t->count = 0;
    return t + 1;
}
