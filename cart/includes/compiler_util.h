#ifndef _COMPILER_UTIL_H
#define _COMPILER_UTIL_H
#include "scanner.h"

#define LOCAL_COUNT 500
#define CALL_COUNT 255
#define CWD_MAX 1024

#define _FLAG_CALL_PARAM_SET 0x01 /* 0001 */
#define _FLAG_CALL_PARAM_RST 0x0E /* 1110 */

#define _FLAG_FIRST_EXPR_SET 0x02 /* 0010*/
#define _FLAG_FIRST_EXPR_RST 0x0D /* 1101*/

#define _FLAG_INSTANCE_CALL_SET 0x04 /* 0100 */
#define _FLAG_INSTANCE_CALL_RST 0x0B /* 1011 */

struct Parser
{
    token cur;
    token pre;
    bool err;
    bool panic;
    const char *current_file;
};

typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY

} Precedence;

typedef struct Parser Parser;
typedef struct Local Local;
typedef struct Upvalue Upvalue;
typedef struct Compiler Compiler;
typedef struct ClassCompiler ClassCompiler;
typedef void (*parse_fn)(Compiler *);
typedef struct parse_rule PRule;
typedef struct Counter Counter;
typedef struct Lookup Lookup;
typedef struct Hashes Hashes;
typedef struct CurrentConstant CurrentConstant;
typedef struct Meta Meta;
typedef struct CompilerStacks CompilerStacks;

struct Local
{
    Arena name;
    int depth;
    bool captured;
};

struct Upvalue
{
    uint8_t index;
    bool islocal;
};

struct ClassCompiler
{
    ClassCompiler *enclosing;
    Arena instance_name;
};

struct Counter
{
    int local;
    int scope_depth;
    int upvalue;
    int call;
    int param;
    int class;
    int native;
};

struct Lookup
{
    Table *call;
    Table *class;
    Table *include;
    Table *native;
};

struct Hashes
{
    Arena init;
    Arena len;
    Arena push;
    Arena pop;
    Arena reverse;
    Arena remove;
    Arena sort;
    Arena bin_search;
};

struct CurrentConstant
{
    uint16_t array_index;
    uint16_t array_set;
    uint16_t array_get;
};

struct Meta
{
    ObjType type;
    const char *cwd;
    const char *current_file;
};

struct CompilerStacks
{
    Class *instance[CALL_COUNT];
    Local local[LOCAL_COUNT];
    Upvalue upvalue[LOCAL_COUNT];
};

struct Compiler
{
    Counter count;
    uint8_t flags;

    Lookup lookup;
    Hashes hash;
    CurrentConstant current;
    Meta meta;

    Compiler *base;
    Compiler *enclosing;
    CompilerStacks stack;
    ClassCompiler *classc;

    Function *func;
    Parser parser;
};

struct parse_rule
{
    parse_fn prefix;
    parse_fn infix;
    Precedence prec;
};

static void consume(int t, const char *str, Parser *parser);
static void advance_compiler(Parser *parser);

static void declaration(Compiler *c);
static void _call(Compiler *c);

static int argument_list(Compiler *c);

static void class_declaration(Compiler *c);
static void method(Compiler *c, Class *class);
static void method_body(Compiler *c, ObjType type, Arena ar, Class **class);

static void func_declaration(Compiler *c);
static void func_body(Compiler *c, ObjType type, Arena ar);
static void func_var(Compiler *c);

static void var_dec(Compiler *c);
static void synchronize(Parser *parser);

static void statement(Compiler *c);
static void print_statement(Compiler *c);

static void begin_scope(Compiler *c);
static void end_scope(Compiler *c);

static void parse_block(Compiler *c);
static void block(Compiler *c);

// static void comment(Compiler *c);

static void emit_loop(Compiler *c, int byte);
static int emit_jump_long(Compiler *c, int byte);
static int emit_jump(Compiler *c, int byte);

static void patch_jump_long(Compiler *c, int begin, int byte);
static void patch_jump(Compiler *c, int byte);

static void for_statement(Compiler *c);
static void while_statement(Compiler *c);
static void each_statement(Compiler *c);

static void rm_statement(Compiler *c);

static void consume_if(Compiler *c);
static void consume_elif(Compiler *c);
static Arena consume_switch(Compiler *c);

static void switch_statement(Compiler *c);
static void case_statement(Compiler *c, Arena ar);

static void if_statement(Compiler *c);
static void elif_statement(Compiler *c);
static void ternary_statement(Compiler *c);
static void null_coalescing_statement(Compiler *c);

static void return_statement(Compiler *c);

static void default_expression(Compiler *c);
static void expression(Compiler *c);

static bool match(int t, Parser *parser);
static bool check(int t, Parser *parser);
static bool is_comment(Parser *parser);

static void grouping(Compiler *c);
static PRule *get_rule(int t);
static void parse_precedence(Precedence precedence, Compiler *c);

static void _and_(Compiler *c);
static void _or_(Compiler *c);

static void binary(Compiler *c);
static void unary(Compiler *c);

static void current_err(const char *err, Parser *parser);
static void error(const char *err, Parser *parser);
static void error_at(Token t, Parser *parser, const char *err);

static void emit_byte(Compiler *c, int byte);
static void emit_bytes(Compiler *c, int b1, int b2);
static void emit_return(Compiler *c);

static void array(Compiler *c);
static void _access(Compiler *c);
static void dval(Compiler *c);
static void pi(Compiler *c);

static void euler(Compiler *c);

static void ival(Compiler *c);
static void llint(Compiler *c);
static void ch(Compiler *c);
static void boolean(Compiler *c);
static void cstr(Compiler *c);

static const char *parse_string(Compiler *c);
static void string(Compiler *c);
static void array_alloc(Compiler *c);
static void vector_alloc(Compiler *c);
static void stack_alloc(Compiler *c);

static void table(Compiler *c);

static int resolve_local(Compiler *c, Arena *name);
static int resolve_upvalue(Compiler *c, Arena *name);
static int add_upvalue(Compiler *c, int upvalue, bool t);

static Arena parse_func_id(Compiler *c);
static void push_array_val(Compiler *c);

static void parse_native_var_arg(Compiler *c);

static void dot(Compiler *c);
static void _this(Compiler *c);

static Arena parse_id(Compiler *c);

static int parse_var(Compiler *c, Arena ar);
static void id(Compiler *c);
static Arena get_id(Compiler *c);

static bool idcmp(Arena a, Arena b);
static void declare_var(Compiler *c, Arena ar);
static void add_local(Compiler *c, Arena *ar);

static PRule rules[] = {
    [TOKEN_CH_LPAREN] = {grouping, _call, PREC_CALL},
    [TOKEN_CH_RPAREN] = {NULL, NULL, PREC_NONE},

    [TOKEN_CH_LCURL] = {NULL, NULL, PREC_NONE},
    [TOKEN_CH_RCURL] = {NULL, NULL, PREC_NONE},

    [TOKEN_CH_LSQUARE] = {array, _access, PREC_CALL},
    [TOKEN_CH_RSQUARE] = {NULL, NULL, PREC_NONE},

    [TOKEN_CH_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_CH_SEMI] = {NULL, NULL, PREC_NONE},
    [TOKEN_CH_DOT] = {NULL, dot, PREC_CALL},

    [TOKEN_OP_INC] = {id, NULL, PREC_TERM},
    [TOKEN_OP_DEC] = {NULL, NULL, PREC_TERM},

    [TOKEN_OP_SUB] = {unary, binary, PREC_TERM},
    [TOKEN_OP_ADD] = {NULL, binary, PREC_TERM},

    [TOKEN_OP_DIV] = {NULL, binary, PREC_FACTOR},
    [TOKEN_OP_MOD] = {NULL, binary, PREC_FACTOR},
    [TOKEN_OP_MUL] = {NULL, binary, PREC_FACTOR},

    [TOKEN_OP_ASSIGN] = {NULL, NULL, PREC_ASSIGNMENT},
    [TOKEN_ADD_ASSIGN] = {NULL, NULL, PREC_ASSIGNMENT},
    [TOKEN_SUB_ASSIGN] = {NULL, NULL, PREC_ASSIGNMENT},
    [TOKEN_MUL_ASSIGN] = {NULL, NULL, PREC_ASSIGNMENT},
    [TOKEN_DIV_ASSIGN] = {NULL, NULL, PREC_ASSIGNMENT},
    [TOKEN_MOD_ASSIGN] = {NULL, NULL, PREC_ASSIGNMENT},
    [TOKEN_AND_ASSIGN] = {NULL, NULL, PREC_ASSIGNMENT},
    [TOKEN_OR__ASSIGN] = {NULL, NULL, PREC_ASSIGNMENT},

    [TOKEN_OP_BANG] = {unary, NULL, PREC_TERM},
    [TOKEN_OP_SNE] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_OP_SEQ] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_OP_NE] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_OP_EQ] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_OP_GT] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_OP_GE] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_OP_LT] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_OP_LE] = {NULL, binary, PREC_COMPARISON},

    [TOKEN_SC_AND] = {NULL, _and_, PREC_AND},
    [TOKEN_SC_OR] = {NULL, _or_, PREC_OR},

    [TOKEN_OP_AND] = {NULL, binary, PREC_AND},
    [TOKEN_OP_OR] = {NULL, binary, PREC_OR},

    [TOKEN_FALSE] = {boolean, NULL, PREC_NONE},
    [TOKEN_TRUE] = {boolean, NULL, PREC_NONE},
    [TOKEN_EACH] = {NULL, NULL, PREC_NONE},

    [TOKEN_ID] = {id, NULL, PREC_NONE},
    [TOKEN_STR] = {cstr, NULL, PREC_NONE},
    [TOKEN_ALLOC_STR] = {string, NULL, PREC_NONE},
    [TOKEN_ALLOC_ARRAY] = {array_alloc, NULL, PREC_NONE},
    [TOKEN_ALLOC_VECTOR] = {vector_alloc, NULL, PREC_NONE},
    [TOKEN_ALLOC_STACK] = {stack_alloc, NULL, PREC_NONE},
    [TOKEN_TABLE] = {table, NULL, PREC_CALL},
    [TOKEN_CH_TERNARY] = {NULL, NULL, PREC_NONE},
    [TOKEN_CH_NULL_COALESCING] = {NULL, NULL, PREC_NONE},

    [TOKEN_CHAR] = {ch, NULL, PREC_NONE},
    [TOKEN_INT] = {ival, NULL, PREC_NONE},
    [TOKEN_DOUBLE] = {dval, NULL, PREC_NONE},
    [TOKEN_LLINT] = {llint, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},

    [TOKEN_LINE_COMMENT] = {NULL, NULL, PREC_NONE},
    [TOKEN_NLINE_COMMENT] = {NULL, NULL, PREC_NONE},

    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELIF] = {NULL, NULL, PREC_OR},
    [TOKEN_NULL] = {boolean, NULL, PREC_NONE},

    [TOKEN_CLOCK] = {parse_native_var_arg, NULL, PREC_CALL},
    [TOKEN_STR_STR] = {parse_native_var_arg, NULL, PREC_CALL},
    [TOKEN_SQRT] = {parse_native_var_arg, NULL, PREC_CALL},
    [TOKEN_PRIME] = {parse_native_var_arg, NULL, PREC_CALL},
    [TOKEN_FILE] = {parse_native_var_arg, NULL, PREC_CALL},
    // [TOKEN_REVERSE] = {parse_native_var_arg, NULL, PREC_CALL},

    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {_this, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_TYPE_ARRAY] = {NULL, NULL, PREC_NONE},
    [TOKEN_TYPE_INT] = {NULL, NULL, PREC_NONE},
    [TOKEN_TYPE_DOUBLE] = {NULL, NULL, PREC_NONE},
    [TOKEN_TYPE_LONG] = {NULL, NULL, PREC_NONE},
    [TOKEN_TYPE_STRING] = {NULL, NULL, PREC_NONE},
    [TOKEN_TYPE_CHAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_TYPE_BYTE] = {NULL, NULL, PREC_NONE},

    [TOKEN_PI] = {pi, NULL, PREC_NONE},
    [TOKEN_EULER] = {euler, NULL, PREC_NONE},

    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static void mark_compiler_roots(Compiler *c);
static Function *end_compile(Compiler *a);
static void init_compiler(Compiler *a, Compiler *b, ObjType type, Arena ar);

#endif
