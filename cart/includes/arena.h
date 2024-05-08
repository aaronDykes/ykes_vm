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
    ARENA_SHORTS,

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
    OP_PRINT_LOCAL,

    OP_CLASS,
    OP_GET_INSTANCE,

    OP_EACH_ACCESS,
    OP_EACH_LOCAL_ACCESS,
    OP_GET_GLOB_ACCESS,
    OP_SET_GLOB_ACCESS,
    OP_GET_LOCAL_ACCESS,
    OP_SET_LOCAL_ACCESS,
    OP_LEN,
    OP_LEN_LOCAL,

    OP_ALLOC_TABLE,
    OP_ALLOC_STACK,
    OP_ALLOC_VECTOR,

    OP_PUSH_GLOB_ARRAY_VAL,
    OP_PUSH_LOCAL_ARRAY_VAL,
    OP_POP_LOCAL_ARRAY_VAL,
    OP_POP_GLOB_ARRAY_VAL,
    OP_PREPEND_ARRAY_VAL,

    OP_CPY_ARRAY,
    OP_POP,
    OP_POPN,
    OP_PUSH,
    OP_RM,
    OP_RM_LOCAL,
    OP_CLOSE_UPVAL,

    OP_PUSH_TOP,
    OP_GET_PROP,
    OP_SET_PROP,

    OP_FIND_CLOSURE,
    OP_GET_CLOSURE,
    OP_GET_METHOD,
    OP_GET_CLASS,
    OP_GET_NATIVE,
    OP_GET_NATIVE_LOCAL,

    OP_GLOBAL_DEF,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,

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

    OP_ACC,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_MOD,
    OP_DIV,

    OP_ADD_LOCAL,
    OP_SUB_LOCAL,
    OP_MUL_LOCAL,
    OP_MOD_LOCAL,
    OP_DIV_LOCAL,
    OP_SEQ_LOCAL,
    OP_SNE_LOCAL,
    OP_EQ_LOCAL,
    OP_NE_LOCAL,
    OP_LT_LOCAL,
    OP_LE_LOCAL,
    OP_GT_LOCAL,
    OP_GE_LOCAL,

    OP_BIT_AND,
    OP_BIT_OR,

    OP_AND,
    OP_OR,
    OP_AND_LOCAL,
    OP_OR_LOCAL,

    OP_SEQ,
    OP_SNE,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,

    OP_JMP_NIL,
    OP_JMP_NIL_LOCAL,
    OP_JMP_NOT_NIL,
    OP_JMPL,
    OP_JMPC,
    OP_JMPF,
    OP_JMPT,
    OP_JMP,
    OP_LOOP,

    OP_MOV_PEEK_R1,
    OP_MOV_PEEK_R2,
    OP_MOV_PEEK_R3,
    OP_MOV_R1,
    OP_MOV_R2,
    OP_MOV_R3,
    OP_MOV_CNT_R1,
    OP_MOV_CNT_R2,
    OP_MOV_CNT_R3,

    OP_MOV_R1_R2,
    OP_MOV_R1_R3,

    OP_MOV_R2_R1,

    OP_MOV_R3_R1,
    OP_MOV_R3_R2,

    OP_MOV_R1_R4,
    OP_MOV_R1_E2,
    OP_MOV_R1_E1,
    OP_MOV_R2_E1,
    OP_MOV_R2_E2,

    OP_MOV_E2_E1,
    OP_MOV_E2_E3,
    OP_MOV_E3_E2,
    OP_ZERO_E1,
    OP_ZERO_E2,
    OP_ZERO_EL_REGISTERS,

    OP_MOV_E1_E2,
    OP_MOV_E1_E3,
    OP_MOV_E3_E1,

    OP_MOV_E1,
    OP_MOV_E2,
    OP_MOV_E3,
    OP_MOV_CNT_E1,
    OP_MOV_CNT_E2,
    OP_MOV_CNT_E3,
    OP_MOV_PEEK_E1,
    OP_MOV_PEEK_E2,

    OP_STR_R1,
    OP_STR_R2,
    OP_STR_R3,
    OP_STR_R4,
    OP_STR_R5,

    OP_STR_E1,
    OP_STR_E2,
    OP_STR_E3,

    OP_ZERO_ARENA_REGISTERS,
    OP_ZERO_R1,
    OP_ZERO_R2,
    OP_ZERO_R3,
    OP_ZERO_R4,
    OP_ZERO_R5,

    OP_CALL,
    OP_CALL_LOCAL,
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

union Vector
{
    uint8_t *Bytes;
    uint16_t *Shorts;
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
};

struct Instance
{
    Class *classc;
    Table *fields;
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
