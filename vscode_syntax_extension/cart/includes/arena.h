#ifndef _MEMORY_ARENA_H
#define _MEMORY_ARENA_H
#include <stdlib.h>
#include <stdbool.h>

typedef enum
{

    ARENA_BYTE,
    ARENA_SIZE,
    ARENA_INT,
    ARENA_DOUBLE,
    ARENA_LONG,
    ARENA_CHAR,
    ARENA_STR,
    ARENA_CSTR,
    ARENA_BOOL,
    ARENA_NULL,
    ARENA_BYTES,
    ARENA_INTS,
    ARENA_DOUBLES,
    ARENA_LONGS,
    ARENA_BOOLS,
    ARENA_SIZES,
    ARENA_STRS,
    ARENA_FUNC,
    ARENA_NATIVE,
    ARENA_VAR,

} T;

typedef enum
{
    OP_CONSTANT,
    OP_CLOSURE,
    OP_PRINT,

    OP_CLASS,
    OP_GET_INSTANCE,

    OP_EACH_ACCESS,
    OP_GET_ACCESS,
    OP_SET_ACCESS,
    OP_LEN,

    OP_ALLOC_TABLE,

    OP_PUSH_ARRAY_VAL,
    OP_POP__ARRAY_VAL,
    OP_PREPEND_ARRAY_VAL,

    OP_CPY_ARRAY,
    OP_POP,
    OP_POPN,
    OP_PUSH,
    OP_RM,
    OP_CLOSE_UPVAL,

    OP_PUSH_TOP,
    OP_GET_PROP,
    OP_SET_PROP,

    OP_FIND_CLOSURE,
    OP_GET_CLOSURE,
    OP_GET_METHOD,
    OP_GET_CLASS,
    OP_GET_NATIVE,

    OP_GLOBAL_DEF,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,

    OP_SET_FUNC_VAR,

    OP_RESET_ARGC,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_SET_LOCAL_PARAM,

    OP_GET_UPVALUE,
    OP_SET_UPVALUE,

    OP_ASSIGN,
    OP_ADD_ASSIGN,
    OP_SUB_ASSIGN,
    OP_MUL_ASSIGN,
    OP_DIV_ASSIGN,
    OP_MOD_ASSIGN,
    OP_AND_ASSIGN,
    OP__OR_ASSIGN,

    OP_NEG,

    OP_INC_LOC,
    OP_INC_GLO,
    OP_DEC_LOC,
    OP_DEC_GLO,

    OP_INC,
    OP_DEC,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_MOD,
    OP_DIV,

    OP_BIT_AND,
    OP_BIT_OR,

    OP_AND,
    OP_OR,

    OP_SEQ,
    OP_SNE,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,

    OP_JMP_NIL,
    OP_JMP_NOT_NIL,
    OP_JMPL,
    OP_JMPC,
    OP_JMPF,
    OP_JMPT,
    OP_JMP,
    OP_LOOP,

    OP_CALL,
    OP_METHOD,

    OP_NULL,

    OP_RETURN

} opcode;

typedef enum
{
    ARENA,
    NATIVE,
    CLASS,
    INSTANCE,
    CLOSURE,
    FUNCTION,
    VECTOR,

    METHOD,
    STACK,
    TABLE,
    INIT,
    UPVAL,
    SCRIPT,
    NULL_OBJ
} ObjType;

typedef enum
{
    OBJECT_TYPE,
    ARENA_TYPE,
    WILD_CARD
} Typed;

typedef union Vector Vector;
typedef struct Value Value;
typedef struct Arena Arena;
typedef struct Data Data;

typedef struct ExprType ExprType;
typedef struct Chunk Chunk;
typedef struct Function Function;
typedef struct Closure Closure;
typedef struct Upval Upval;
typedef struct Native Native;
typedef struct Element Element;
typedef struct Stack Stack;
typedef struct Class Class;
typedef struct BoundClosure BoundClosure;
typedef struct Instance Instance;
typedef struct Table Table;
typedef Element (*NativeFn)(int argc, Stack *argv);

struct ExprType
{
    Typed type;
    union
    {
        ObjType object;
        T arena;
    };
};

union Vector
{
    uint8_t *Bytes;
    int *Ints;
    double *Doubles;
    long long int *Longs;
    char **Strings;
    bool *Bools;
    void *Void;
    size_t Sizes;
};

struct Value
{

    long long int hash;
    union
    {
        struct
        {
            int len;
            int count;
            char *String;
        };

        size_t Size;
        uint8_t Byte;
        int Int;
        double Double;
        long long int Long;
        char Char;
        bool Bool;
        void *Void;
    };
};

struct Arena
{
    size_t size;
    T type;

    union
    {
        struct
        {
            int count;
            int len;
            Vector listof;
        };

        Value as;
    };
};

struct Chunk
{
    Arena cases;
    Arena op_codes;
    Arena lines;
    Stack *constants;
};

struct Function
{
    int arity;
    int upvalue_count;
    Arena name;
    Chunk ch;
};

struct Closure
{
    Function *func;
    Upval **upvals;
    int upval_count;
};

struct Native
{
    int arity;
    Arena obj;
    NativeFn fn;
};

struct Element
{
    ObjType type;

    union
    {
        Arena arena;
        Arena *arena_vector;
        Native *native;
        Closure *closure;
        Class *classc;
        Instance *instance;
        Table *table;
        Stack *stack;
        void *null;
    };
};

struct Class
{
    Closure *init;
    Arena name;
    Table *fields;
};

struct Instance
{
    Class *classc;
};

struct Stack
{
    int count;
    int len;
    size_t size;
    Element as;
    Stack *top;
};

struct Upval
{
    int len;
    int count;
    size_t size;
    Stack *index;
    Stack closed;
    Upval *next;
};

struct Table
{
    size_t size;
    Arena key;
    ObjType type;
    int count;
    int len;

    Element val;
    Table *next;
    Table *prev;
};

#endif
