#include "compiler.h"
#include "compiler_util.h"
#include "arena_table.h"
#ifdef DEBUG_TRACE_EXECUTION
#include "debug.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <math.h>
#include <unistd.h>

#define CALL_PARAM(c) \
    (c & _FLAG_CALL_PARAM_SET)

static void init_compiler(Compiler *a, Compiler *b, ObjType type, Arena name)
{

    Local *local = NULL;

    a->classc = NULL;
    a->func = NULL;
    a->func = function(name);

    a->count.local = 0;
    a->count.scope_depth = 0;
    a->count.call = 0;
    a->count.class = 0;
    a->count.upvalue = 0;
    a->count.param = 0;
    a->count.native = 0;

    a->flags = 0;

    a->current.array_index = 0;
    a->current.array_set = 0;
    a->current.array_get = 0;

    a->count.upvalue = 0;

    a->lookup.call = NULL;
    a->lookup.class = NULL;
    a->lookup.native = NULL;
    a->lookup.include = NULL;

    a->meta.type = type;
    a->meta.cwd = NULL;

    if (b)
    {
        a->base = b->base;
        a->parser = b->parser;
        a->enclosing = b;
        a->count.scope_depth = b->count.scope_depth;
        a->classc = b->classc;

        local = &b->stack.local[b->count.local++];
    }
    else
    {
        local = &a->stack.local[a->count.local++];
        a->enclosing = NULL;
    }

    local->depth = 0;
    local->captured = false;

    local->name = (type == METHOD)
                      ? String("this")
                      : Null();
}

static void consume(int t, const char *err, Parser *parser)
{
    if (parser->cur.type == t)
    {
        advance_compiler(parser);
        return;
    }
    current_err(err, parser);
}
static void advance_compiler(Parser *parser)
{
    parser->pre = parser->cur;

    for (;;)
    {
        parser->cur = scan_token();

        if (parser->cur.type != TOKEN_ERR)
            break;
        current_err(parser->cur.start, parser);
    }
}
static char *read_file(const char *path)
{

    FILE *file = fopen(path, "rb");

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

static bool resolve_include(Compiler *c, Arena ar)
{

    Element el = find_entry(&c->base->lookup.include, &ar);

    if (el.type != NULL_OBJ)
        return true;

    return false;
}

static void str_cop(char *src, char *dst)
{
    char *tmp = src;

    while ((*tmp++ = *dst++))
        ;
}
static char *get_name(char *path)
{
    int len = strlen(path) - 1;
    char *tmp = path + len;

    int count;
    char c = tmp[0];
    for (count = 0; tmp[-1] != '/' || *tmp == c; --tmp, count++)
        ;

    char *file = NULL;
    file = ALLOC((count + 1) * sizeof(char));

    strcpy(file, tmp);

    return file;
}

static void include_dir(Compiler *c, Arena inc)
{
#define SIZE(x, y) \
    strlen(x) + strlen(y)

    DIR *dir = NULL;

    struct dirent *entry;

    char path[CWD_MAX] = {0};
    inc.as.String[inc.as.len - 1] = '\0';
    str_cop(path, (char *)c->base->meta.cwd);
    strcat(path, inc.as.String);

    dir = opendir(path);

    char *prev_str = NULL;

    Arena result = Null();

    consume(TOKEN_CH_SEMI, "Expect `;` at end of include statement.", &c->parser);

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char f_path[CWD_MAX] = {0};

        str_cop(f_path, (char *)c->base->meta.cwd);
        strcat(f_path, inc.as.String);
        strcat(f_path, entry->d_name);

        Arena e = CString(entry->d_name);
        if (resolve_include(c, e))
        {
            error("Double include.", &c->parser);
            exit(1);
        }
        write_table(c->base->lookup.include, e, OBJ(e));

        char *file = read_file(f_path);

        result = (prev_str)
                     ? GROW_ARRAY(
                           &result,
                           SIZE(file, result.as.String),
                           ARENA_STR)
                     : CString(file);

        if (prev_str)
            strcat(result.as.String, file);

        prev_str = file;
    }
    char *remainder = (char *)c->parser.cur.start;

    result = GROW_ARRAY(
        &result,
        result.as.len + strlen(remainder),
        ARENA_STR);

    strcat(result.as.String, remainder);

    init_scanner(result.as.String);

    c->parser.cur = scan_token();

    if (closedir(dir) != 0)
    {
        perror("Unable to close directory");
        exit(1);
    }

#undef SIZE
}

static void include_file(Compiler *c)
{

#define SIZE(x, y) \
    strlen(x) + strlen(y)

    if (c->meta.type != SCRIPT)
    {
        error("Can only include files at top level.", &c->parser);
        exit(1);
    }

    char *remaining = NULL;

    consume(TOKEN_STR, "Expect file path.", &c->parser);
    Arena inc = CString(parse_string(c));

    char asterisc = inc.as.String[inc.as.len - 1];

    if (asterisc == '*')
    {
        include_dir(c, inc);
        return;
    }

    if (resolve_include(c, inc))
    {
        error("Double include.", &c->parser);
        exit(1);
    }

    write_table(c->base->lookup.include, inc, OBJ(inc));

    consume(TOKEN_CH_SEMI, "Expect `;` at end of include statement.", &c->parser);
    remaining = (char *)c->parser.cur.start;

    char path[CWD_MAX] = {0};

    str_cop(path, (char *)c->base->meta.cwd);
    strcat(path, inc.as.String);

    char *file = read_file(path);

    Arena result = GROW_ARRAY(
        NULL,
        SIZE(file, remaining),
        ARENA_STR);

    str_cop(result.as.String, file);

    strcat(result.as.String, remaining);
    init_scanner(result.as.String);
    c->parser.cur = scan_token();

    c->parser.current_file = get_name(inc.as.String);

#undef SIZE
}

static void declaration(Compiler *c)
{

    if (match(TOKEN_INCLUDE, &c->parser))
        include_file(c);
    else if (match(TOKEN_FUNC, &c->parser))
        func_declaration(c);
    else if (match(TOKEN_CLASS, &c->parser))
        class_declaration(c);
    else if (match(TOKEN_VAR, &c->parser))
        var_dec(c);
    else
        statement(c);

    if (c->parser.panic)
        synchronize(&c->parser);
}

static void class_declaration(Compiler *c)
{
    consume(TOKEN_ID, "ERROR: Expect class name.", &c->parser);

    Arena ar = parse_id(c);
    Class *classc = NULL;

    classc = class(ar);
    classc->closures = GROW_TABLE(NULL, STACK_SIZE);

    ClassCompiler *class = NULL;
    class = ALLOC(sizeof(ClassCompiler));

    write_table(c->base->lookup.class, classc->name, OBJ(Int(c->base->count.class)));
    c->base->stack.instance[c->base->count.class ++] = classc;
    class->instance_name = ar;

    class->enclosing = c->classc;
    c->classc = class;

    emit_bytes(c, OP_CLASS, add_constant(&c->func->ch, CLASS(classc)));

    consume(TOKEN_CH_LCURL, "ERROR: Expect ze `{` curl brace", &c->parser);

    while (!check(TOKEN_CH_RCURL, &c->parser) && !check(TOKEN_EOF, &c->parser))
        method(c, classc);

    consume(TOKEN_CH_RCURL, "ERROR: Expect ze `}` curl brace", &c->parser);
    c->classc = c->classc->enclosing;
}

static void method(Compiler *c, Class *class)
{
    consume(TOKEN_ID, "ERROR: Expect method identifier.", &c->parser);
    Arena ar = parse_func_id(c);

    ObjType type = INIT;

    if (ar.as.hash != c->base->hash.init.as.hash)
        type = METHOD;

    method_body(c, type, ar, &class);
}

static void method_body(Compiler *c, ObjType type, Arena ar, Class **class)
{
    Compiler co;
    init_compiler(&co, c, type, ar);

    c = &co;
    begin_scope(c);
    consume(TOKEN_CH_LPAREN, "Expect `(` after function name.", &c->parser);
    if (!check(TOKEN_CH_RPAREN, &c->parser))
        do
        {
            c->func->arity++;
            if (c->func->arity > 255)
                current_err("Cannot declare more than 255 function parameters", &c->parser);
            func_var(c);

        } while (match(TOKEN_CH_COMMA, &c->parser));
    consume(TOKEN_CH_RPAREN, "Expect `)` after function parameters.", &c->parser);
    consume(TOKEN_CH_LCURL, "Expect `{` prior to function body.", &c->parser);

    parse_block(c);

    Compiler *tmp = c;
    Function *f = end_compile(c);

    Closure *clos = new_closure(f);

    write_table((*class)->closures, ar, CLOSURE(clos));

    end_scope(c);

    if (type == INIT)
        (*class)->init = clos;

    c = c->enclosing;

    emit_bytes(
        c, OP_METHOD,
        add_constant(&c->func->ch, CLOSURE(clos)));

    for (int i = 0; i < tmp->count.upvalue; i++)
    {
        emit_byte(c, tmp->stack.upvalue[i].islocal ? 1 : 0);
        emit_byte(c, tmp->stack.upvalue[i].index);
    }

    mark_compiler_roots(c);
}

static void _call(Compiler *c)
{
    uint8_t argc = argument_list(c);
    emit_bytes(c, (c->count.scope_depth > 0) ? OP_CALL_LOCAL : OP_CALL, argc);
}

static int argument_list(Compiler *c)
{
    uint8_t argc = 0;

    if (match(TOKEN_CH_RPAREN, &c->parser))
        return 0;

    c->flags |= _FLAG_CALL_PARAM_SET;
    do
    {
        expression(c);
        if (argc == 255)
            current_err("Cannot pass more than 255 function parameters", &c->parser);
        argc++;

    } while (match(TOKEN_CH_COMMA, &c->parser));

    c->flags &= _FLAG_CALL_PARAM_RST;
    consume(TOKEN_CH_RPAREN, "Expect `)` after function args", &c->parser);
    return argc;
}

static void func_declaration(Compiler *c)
{
    consume(TOKEN_ID, "Expect function name.", &c->parser);
    Arena ar = parse_func_id(c);

    write_table(c->base->lookup.call, ar, OBJ(Int(c->base->count.call++)));
    func_body(c, CLOSURE, ar);
}

static void func_body(Compiler *c, ObjType type, Arena ar)
{
    Compiler co;
    init_compiler(&co, c, type, ar);

    c = &co;
    begin_scope(c);
    consume(TOKEN_CH_LPAREN, "Expect `(` after function name.", &c->parser);
    if (!check(TOKEN_CH_RPAREN, &c->parser))
        do
        {
            c->func->arity++;
            if (c->func->arity > 255)
                current_err("Cannot declare more than 255 function parameters", &c->parser);
            func_var(c);

        } while (match(TOKEN_CH_COMMA, &c->parser));
    consume(TOKEN_CH_RPAREN, "Expect `)` after function parameters.", &c->parser);
    consume(TOKEN_CH_LCURL, "Expect `{` prior to function body.", &c->parser);

    parse_block(c);

    Compiler *tmp = c;
    Function *f = end_compile(c);

    Closure *clos = new_closure(f);
    end_scope(c);

    c = c->enclosing;

    emit_bytes(
        c, OP_CLOSURE,
        add_constant(&c->func->ch, CLOSURE(clos)));

    for (int i = 0; i < tmp->count.upvalue; i++)
    {
        emit_byte(c, tmp->stack.upvalue[i].islocal ? 1 : 0);
        emit_byte(c, tmp->stack.upvalue[i].index);
    }
    mark_compiler_roots(c);
}

static void func_var(Compiler *c)
{
    consume(TOKEN_ID, "Expect variable name.", &c->parser);

    Arena ar = parse_id(c);

    int glob = parse_var(c, ar);

    uint8_t set = OP_SET_FUNC_VAR;

    if (glob == -1)
    {
        glob = resolve_local(c, &ar);
        set = OP_SET_LOCAL_PARAM;
    }
    emit_bytes(c, set, glob);
}

static void var_dec(Compiler *c)
{
    consume(TOKEN_ID, "Expect variable name.", &c->parser);

    Arena ar = parse_id(c);
    int glob = parse_var(c, ar);

    uint8_t set = 0;

    if (glob != -1)
        set = OP_GLOBAL_DEF;
    else
    {
        glob = resolve_local(c, &ar);
        set = OP_SET_LOCAL;
    }

    if (match(TOKEN_OP_ASSIGN, &c->parser))
    {
        emit_byte(c, OP_ZERO_E2);
        expression(c);

        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        c->flags &= _FLAG_FIRST_EXPR_RST;

        emit_bytes(c, set, glob);
    }
    else
    {

        if (c->count.scope_depth > 0)
            emit_byte(c, OP_PUSH_NULL_OBJ);
        emit_bytes(c, set, glob);
    }

    consume(TOKEN_CH_SEMI, "Expect ';' after variable declaration.", &c->parser);
}

static void synchronize(Parser *parser)
{
    parser->panic = false;

    while (parser->cur.type != TOKEN_EOF)
    {
        if (parser->pre.type == TOKEN_CH_SEMI)
            return;

        switch (parser->pre.type)
        {
        case TOKEN_FUNC:
        case TOKEN_CLASS:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;
        }
        advance_compiler(parser);
    }
}

static void statement(Compiler *c)
{

    if (match(TOKEN_PRINT, &c->parser))
        print_statement(c);
    else if (match(TOKEN_OP_REM, &c->parser))
        rm_statement(c);
    else if (match(TOKEN_IF, &c->parser))
        if_statement(c);
    else if (match(TOKEN_WHILE, &c->parser))
        while_statement(c);
    else if (match(TOKEN_FOR, &c->parser))
        for_statement(c);
    else if (match(TOKEN_SWITCH, &c->parser))
        switch_statement(c);
    else if (match(TOKEN_CH_LCURL, &c->parser))
        block(c);
    else if (match(TOKEN_EACH, &c->parser))
        each_statement(c);
    else if (match(TOKEN_RETURN, &c->parser))
        return_statement(c);
    else if (is_comment(&c->parser))
        advance_compiler(&c->parser);
    else
        default_expression(c);
}

static void rm_statement(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` prior to rm expression.", &c->parser);

    if (match(TOKEN_THIS, &c->parser))
        _this(c);
    else
        id(c);

    emit_byte(c, (c->count.scope_depth > 0) ? OP_RM_LOCAL : OP_RM);
    consume(TOKEN_CH_RPAREN, "Expect `)` after rm statement", &c->parser);
    consume(TOKEN_CH_SEMI, "Expect `;` at end of statement", &c->parser);
}

static inline bool is_comment(Parser *parser)
{
    return check(TOKEN_LINE_COMMENT, parser) || check(TOKEN_NLINE_COMMENT, parser);
}

static void for_statement(Compiler *c)
{
    begin_scope(c);
    consume(TOKEN_CH_LPAREN, "Expect `(` after to 'for'.", &c->parser);
    if (!match(TOKEN_CH_SEMI, &c->parser))
    {
        if (match(TOKEN_VAR, &c->parser))
            var_dec(c);
        else
            id(c);
    }

    int start = c->func->ch.op_codes.count;
    int exit = -1;
    if (!match(TOKEN_CH_SEMI, &c->parser))
    {
        expression(c);
        c->flags &= _FLAG_FIRST_EXPR_RST;
        consume(TOKEN_CH_SEMI, "Expect `;` after 'for' condition.", &c->parser);
        exit = emit_jump(c, OP_JMPF);
        emit_byte(c, OP_POP);
    }
    if (!match(TOKEN_CH_RPAREN, &c->parser))
    {
        int body_jump = emit_jump(c, OP_JMP);
        int inc_start = c->func->ch.op_codes.count;

        expression(c);
        c->flags &= _FLAG_FIRST_EXPR_RST;
        emit_byte(c, OP_POP);
        consume(TOKEN_CH_RPAREN, "Expect `)` after 'for' statement.", &c->parser);

        emit_loop(c, start);
        start = inc_start;
        patch_jump(c, body_jump);
    }

    statement(c);
    emit_loop(c, start);

    if (exit != -1)
    {
        patch_jump(c, exit);
        emit_byte(c, OP_POP);
    }

    end_scope(c);
}

static void while_statement(Compiler *c)
{
    int start = c->func->ch.op_codes.count;

    consume(TOKEN_CH_LPAREN, "Expect `(` after 'while'.", &c->parser);
    expression(c);
    if (c->count.scope_depth > 0)
        emit_bytes(c, OP_POPN, add_constant(&c->func->ch, OBJ(Int(2))));
    c->flags &= _FLAG_FIRST_EXPR_RST;
    consume(TOKEN_CH_RPAREN, "Expect `)` after 'while' condition.", &c->parser);

    int exit_jmp = emit_jump(c, OP_JMPF);

    statement(c);

    // If falsey, loop
    emit_loop(c, start);

    // If true, exit
    patch_jump(c, exit_jmp);
}
static void each_statement(Compiler *c)
{
    emit_byte(c, OP_RESET_ARGC);

    consume(TOKEN_CH_LPAREN, "Expect `(` prior to each expression.", &c->parser);

    Arena ar;
    int glob = 0;
    uint8_t set = 0;

    if (match(TOKEN_VAR, &c->parser))
    {
        consume(TOKEN_ID, "Expect identifier after var keyword", &c->parser);
        ar = parse_id(c);
        glob = parse_var(c, ar);
    }
    else
        error("Expect variable declaration at start of each expression", &c->parser);

    if (glob != -1)
        set = OP_SET_GLOBAL;
    else if ((glob = resolve_upvalue(c, &ar)) != -1)
        set = OP_SET_UPVALUE;
    else
    {
        glob = resolve_local(c, &ar);
        set = OP_SET_LOCAL;
    }
    consume(TOKEN_CH_COLON, "Expect `:` between variable declaration and identifier", &c->parser);

    int start = c->func->ch.op_codes.count;

    if (match(TOKEN_THIS, &c->parser))
        _this(c);
    else
        id(c);

    if (match(TOKEN_CH_LSQUARE, &c->parser))
        _access(c);

    emit_byte(c, (c->count.scope_depth > 0) ? OP_EACH_LOCAL_ACCESS : OP_EACH_ACCESS);
    int exit = emit_jump(c, (c->count.scope_depth > 0) ? OP_JMP_NIL_LOCAL : OP_JMP_NIL);

    emit_bytes(c, set, glob);

    consume(TOKEN_CH_RPAREN, "Expect `)` following an each expression.", &c->parser);

    statement(c);
    emit_loop(c, start);

    patch_jump(c, exit);
}

static void consume_if(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` after an 'if'.", &c->parser);
    expression(c);
    c->flags &= _FLAG_FIRST_EXPR_RST;
    consume(TOKEN_CH_RPAREN, "Expect `)` after an 'if' condtion.", &c->parser);
}
static void consume_elif(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` after an 'elif'.", &c->parser);
    expression(c);
    c->flags &= _FLAG_FIRST_EXPR_RST;

    consume(TOKEN_CH_RPAREN, "Expect `)` after an 'elif' condtion.", &c->parser);
}

static Arena consume_switch(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` after a 'switch'.", &c->parser);
    Arena args = get_id(c);
    consume(TOKEN_CH_RPAREN, "Expect `)` after a 'switch' condtion.", &c->parser);
    return args;
}

static void switch_statement(Compiler *c)
{
    Arena args = consume_switch(c);
    bool expect = match(TOKEN_CH_LCURL, &c->parser);

    begin_scope(c);
    emit_byte(c, OP_ZERO_E1);

    case_statement(c, args);

    if (match(TOKEN_DEFAULT, &c->parser))
    {
        consume(TOKEN_CH_COLON, "Expect `:` prior to case body.", &c->parser);
        statement(c);
    }
    if (expect)
        consume(TOKEN_CH_RCURL, "Expect `}` after switch body", &c->parser);

    c->flags &= _FLAG_CALL_PARAM_RST;

    Element el = OBJ(c->func->ch.cases);
    push_int(&el, c->func->ch.op_codes.count);

    end_scope(c);
}

static void case_statement(Compiler *c, Arena args)
{
    uint8_t arg = args.listof.Ints[0];
    uint8_t get = args.listof.Ints[1];

    while (match(TOKEN_CASE, &c->parser))
    {
        expression(c);
        consume(TOKEN_CH_COLON, "Expect `:` prior to case body.", &c->parser);
        emit_bytes(c, get, arg);
        if (get == OP_GET_GLOBAL)
            emit_byte(c, 1);

        emit_byte(c, OP_SEQ_LOCAL);

        int tr = emit_jump_long(c, OP_JMPC);
        int begin = c->func->ch.op_codes.count;
        statement(c);
        emit_bytes(c, OP_JMPL, c->func->ch.cases.count);
        patch_jump_long(c, begin, tr);
    }

    FREE_ARRAY(&args);
}

static void if_statement(Compiler *c)
{

    emit_byte(c, OP_ZERO_E1);
    consume_if(c);

    int fi = emit_jump(c, OP_JMPF);
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_POP);

    statement(c);

    int exit = emit_jump(c, OP_JMP);
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_POP);
    patch_jump(c, fi);

    elif_statement(c);

    if (match(TOKEN_ELSE, &c->parser))
        statement(c);

    Element el = OBJ(c->func->ch.cases);
    push_int(&el, c->func->ch.op_codes.count);
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_POP);
    patch_jump(c, exit);
}

static void elif_statement(Compiler *c)
{
    while (match(TOKEN_ELIF, &c->parser))
    {

        consume_elif(c);
        int tr = emit_jump_long(c, OP_JMPC);
        if (c->count.scope_depth > 0)
            emit_byte(c, OP_POP);
        int begin = c->func->ch.op_codes.count;
        statement(c);
        emit_bytes(c, OP_JMPL, c->func->ch.cases.count);

        patch_jump_long(c, begin, tr);
        if (c->count.scope_depth > 0)
            emit_byte(c, OP_POP);
    }
}

static void ternary_statement(Compiler *c)
{
    int exit = emit_jump(c, OP_JMPF);
    c->flags &= _FLAG_FIRST_EXPR_RST;
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_POP);

    emit_byte(c, OP_ZERO_R5);
    expression(c);
    consume(TOKEN_CH_COLON, "Expect `:` between ternary expressions.", &c->parser);

    int tr = emit_jump(c, OP_JMP);

    patch_jump(c, exit);
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_POP);
    c->flags &= _FLAG_FIRST_EXPR_RST;
    emit_byte(c, OP_ZERO_R5);
    expression(c);
    patch_jump(c, tr);
}
static void null_coalescing_statement(Compiler *c)
{
    emit_byte(c, OP_ZERO_E1);
    int exit = emit_jump(c, c->count.scope_depth > 0
                                ? OP_JMP_LOCAL_NOT_NIL
                                : OP_JMP_GLOB_NOT_NIL);
    c->flags &= _FLAG_FIRST_EXPR_RST;

    if (c->count.scope_depth > 0 || CALL_PARAM(c->flags))
        emit_byte(c, OP_POP);

    expression(c);
    // if (c->count.scope_depth > 0 || CALL_PARAM(c->flags))
    emit_bytes(c, c->current.array_set, c->current.array_index);
    patch_jump(c, exit);
}

static void return_statement(Compiler *c)
{
    if (c->meta.type == SCRIPT)
        error("ERROR: Unable to return from top of script.", &c->parser);

    else if (match(TOKEN_CH_SEMI, &c->parser))
        emit_return(c);
    else
    {
        if (c->meta.type == INIT)
            error("ERROR: Unable to return value from initializer.", &c->parser);
        expression(c);
        c->flags &= _FLAG_FIRST_EXPR_RST;
        consume(TOKEN_CH_SEMI, "ERROR: Expect semi colon after return statement.", &c->parser);
        // if (c->count.scope_depth > 0)

        emit_byte(c, OP_RETURN);
    }
}

static void patch_jump_long(Compiler *c, int count, int offset)
{

    int j1 = count - offset - 2;
    int j2 = (c->func->ch.op_codes.count) - offset - 2;

    if (j1 >= INT16_MAX)
        error("ERROR: To great a distance ", &c->parser);

    c->func->ch.op_codes.listof.Shorts[offset] = j2;
    c->func->ch.op_codes.listof.Shorts[offset + 1] = j1;
}

static void patch_jump(Compiler *c, int offset)
{

    int jump = c->func->ch.op_codes.count - offset - 1;

    if (jump >= INT16_MAX)
        error("ERROR: To great a distance ", &c->parser);

    c->func->ch.op_codes.listof.Shorts[offset] = jump;
}

static void emit_loop(Compiler *c, int byte)
{
    emit_byte(c, OP_LOOP);

    int offset = c->func->ch.op_codes.count - byte + 1;

    if (offset > UINT16_MAX)
        error("ERROR: big boi loop", &c->parser);

    emit_byte(c, offset);
}
static int emit_jump_long(Compiler *c, int byte)
{
    emit_byte(c, byte);
    emit_bytes(c, 0xFFFF, 0xFFFF);

    return c->func->ch.op_codes.count - 2;
}
static int emit_jump(Compiler *c, int byte)
{
    emit_bytes(c, byte, 0xFFFF);

    return c->func->ch.op_codes.count - 1;
}

static void default_expression(Compiler *c)
{
    expression(c);
    c->flags &= _FLAG_FIRST_EXPR_RST;
    consume(TOKEN_CH_SEMI, "Expect `;` after expression.", &c->parser);
    emit_byte(c, OP_POP);
}

static void print_statement(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` prior to print expression", &c->parser);
    // c->flags &= _FLAG_FIRST_EXPR_RST;
    do
    {
        expression(c);

        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        c->flags &= _FLAG_FIRST_EXPR_RST;
        if (check(TOKEN_CH_COMMA, &c->parser))
            emit_byte(c, (c->count.scope_depth > 0) ? OP_PRINT_LOCAL : OP_PRINT);

    } while (match(TOKEN_CH_COMMA, &c->parser));

    emit_byte(c, (c->count.scope_depth > 0) ? OP_PRINT_LOCAL : OP_PRINT);

    consume(TOKEN_CH_RPAREN, "Expect `)` after print expression", &c->parser);
    consume(TOKEN_CH_SEMI, "Expect ';' after value.", &c->parser);
}

static void begin_scope(Compiler *c)
{
    c->count.scope_depth++;
}
static void end_scope(Compiler *c)
{

    c->count.scope_depth--;
    if (c->count.local > 0 && (c->stack.local[c->count.local - 1].depth > c->count.scope_depth))
        emit_bytes(c, OP_POPN, add_constant(&c->func->ch, OBJ(Int(c->count.local - 1))));

    while (c->count.local > 0 && (c->stack.local[c->count.local - 1].depth > c->count.scope_depth))
    {
        if (c->stack.local[c->count.local - 1].captured)
            emit_byte(c, OP_CLOSE_UPVAL);
        else
            ARENA_FREE(&c->stack.local[c->count.local - 1].name);
        --c->count.local;
    }
}

static void parse_block(Compiler *c)
{
    while (!check(TOKEN_CH_RCURL, &c->parser) && !check(TOKEN_EOF, &c->parser))
        declaration(c);
    consume(TOKEN_CH_RCURL, "Expect `}` after block statement", &c->parser);
}
static void block(Compiler *c)
{
    begin_scope(c);
    parse_block(c);
    end_scope(c);
}

static bool match(int t, Parser *parser)
{
    if (!check(t, parser))
        return false;
    advance_compiler(parser);
    return true;
}
static bool check(int t, Parser *parser)
{
    return parser->cur.type == t;
}

static void expression(Compiler *c)
{
    parse_precedence(PREC_ASSIGNMENT, c);
}
static void grouping(Compiler *c)
{
    emit_byte(c, OP_MOV_R1_R3);
    c->flags &= _FLAG_FIRST_EXPR_RST;
    expression(c);
    consume(TOKEN_CH_RPAREN, "Expect `)` after expression", &c->parser);
    emit_byte(c, OP_MOV_R3_R2);
}
static PRule *get_rule(int t)
{
    return &rules[t];
}

static void parse_precedence(Precedence prec, Compiler *c)
{
    advance_compiler(&c->parser);
    parse_fn prefix_rule = get_rule(c->parser.pre.type)->prefix;

    if (!prefix_rule)
    {
        error("ERROR: Expect expression.", &c->parser);
        return;
    }

    prefix_rule(c);

    while (prec <= get_rule(c->parser.cur.type)->prec)
    {
        advance_compiler(&c->parser);
        parse_fn infix = get_rule(c->parser.pre.type)->infix;
        infix(c);
    }
}

static void _and_(Compiler *c)
{
    int end = emit_jump(c, OP_JMPF);

    emit_byte(c, OP_POP);
    parse_precedence(PREC_AND, c);

    patch_jump(c, end);
}
static void _or_(Compiler *c)
{
    int else_jmp = emit_jump(c, OP_JMPT);

    emit_byte(c, OP_POP);
    parse_precedence(PREC_OR, c);

    patch_jump(c, else_jmp);
}

static void binary(Compiler *c)
{

    int t = c->parser.pre.type;

    PRule *rule = get_rule(t);
    parse_precedence((Precedence)rule->prec + 1, c);

    if (c->count.scope_depth > 0)
        switch (t)
        {
        case TOKEN_OP_ADD:
            emit_byte(c, OP_ADD_LOCAL);
            break;
        case TOKEN_OP_SUB:
            emit_byte(c, OP_SUB_LOCAL);
            break;
        case TOKEN_OP_MUL:
            emit_byte(c, OP_MUL_LOCAL);
            break;
        case TOKEN_OP_DIV:
            emit_byte(c, OP_DIV_LOCAL);
            break;
        case TOKEN_OP_MOD:
            emit_byte(c, OP_MOD_LOCAL);
            break;
        case TOKEN_OP_NE:
            emit_byte(c, OP_NE_LOCAL);
            break;
        case TOKEN_OP_EQ:
            emit_byte(c, OP_EQ_LOCAL);
            break;
        case TOKEN_OP_SNE:
            emit_byte(c, OP_SNE_LOCAL);
            break;
        case TOKEN_OP_SEQ:
            emit_byte(c, OP_SEQ_LOCAL);
            break;
        case TOKEN_OP_GT:
            emit_byte(c, OP_GT_LOCAL);
            break;
        case TOKEN_OP_GE:
            emit_byte(c, OP_GE_LOCAL);
            break;
        case TOKEN_OP_LT:
            emit_byte(c, OP_LT_LOCAL);
            break;
        case TOKEN_OP_LE:
            emit_byte(c, OP_LE_LOCAL);
            break;
        case TOKEN_OP_OR:
            emit_byte(c, OP_OR_LOCAL);
            break;
        case TOKEN_OP_AND:
            emit_byte(c, OP_AND_LOCAL);
            break;
        default:
            return;
        }
    else
        switch (t)
        {
        case TOKEN_OP_ADD:
            emit_byte(c, OP_ADD);
            break;
        case TOKEN_OP_SUB:
            emit_byte(c, OP_SUB);
            break;
        case TOKEN_OP_MUL:
            emit_byte(c, OP_MUL);
            break;
        case TOKEN_OP_DIV:
            emit_byte(c, OP_DIV);
            break;
        case TOKEN_OP_MOD:
            emit_byte(c, OP_MOD);
            break;
        case TOKEN_OP_NE:
            emit_byte(c, OP_NE);
            break;
        case TOKEN_OP_EQ:
            emit_byte(c, OP_EQ);
            break;
        case TOKEN_OP_SNE:
            emit_byte(c, OP_SNE);
            break;
        case TOKEN_OP_SEQ:
            emit_byte(c, OP_SEQ);
            break;
        case TOKEN_OP_GT:
            emit_byte(c, OP_GT);
            break;
        case TOKEN_OP_GE:
            emit_byte(c, OP_GE);
            break;
        case TOKEN_OP_LT:
            emit_byte(c, OP_LT);
            break;
        case TOKEN_OP_LE:
            emit_byte(c, OP_LE);
            break;
        case TOKEN_OP_OR:
            emit_byte(c, OP_OR);
            break;
        case TOKEN_OP_AND:
            emit_byte(c, OP_AND);
            break;
        default:
            return;
        }
}
static void unary(Compiler *c)
{
    int op = c->parser.pre.type;

    parse_precedence(PREC_UNARY, c);

    switch (op)
    {

    case TOKEN_OP_SUB:
    case TOKEN_OP_BANG:
        emit_byte(c, OP_NEG);
        break;
    default:
        return;
    }
}

static void current_err(const char *err, Parser *parser)
{
    error_at(&parser->cur, parser, err);
}
static void error(const char *err, Parser *parser)
{
    error_at(&parser->pre, parser, err);
}
static void error_at(Token toke, Parser *parser, const char *err)
{
    if (parser->panic)
        return;
    parser->panic = true;
    parser->err = true;

    fprintf(stderr, "[file: %s, line: %d] Error", parser->current_file, toke->line - 1);

    if (toke->type == TOKEN_EOF)
        fprintf(stderr, " at end");
    else if (toke->type != TOKEN_ERR)
        fprintf(stderr, " at '%.*s'", toke->size, toke->start);

    fprintf(stderr, ": %s\n", err);
}

static void emit_return(Compiler *c)
{
    if (c->meta.type == INIT)
        emit_bytes(c, OP_GET_LOCAL, 0);
    // else
    // emit_byte(c, OP_NULL);
    emit_byte(c, OP_RETURN);
}
static void emit_byte(Compiler *c, int byte)
{
    write_chunk(&c->func->ch, (uint16_t)(byte & 0xFFFF), c->parser.pre.line);
}
static void emit_bytes(Compiler *c, int b1, int b2)
{
    write_chunk(&c->func->ch, (uint16_t)(b1 & 0xFFFF), c->parser.pre.line);
    write_chunk(&c->func->ch, (uint16_t)(b2 & 0xFFFF), c->parser.pre.line);
}

static void pi(Compiler *c)
{
    int arg = add_constant(&c->func->ch, OBJ(Double(M_PI)));
    if (!c->flags)
    {
        emit_bytes(c, OP_MOV_CNT_R1, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R1);
        c->flags |= _FLAG_FIRST_EXPR_SET;
    }
    else
    {
        emit_bytes(c, OP_MOV_CNT_R2, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R2);
    }
}

static void euler(Compiler *c)
{
    int arg = add_constant(&c->func->ch, OBJ(Double(M_E)));
    if (!c->flags)
    {
        emit_bytes(c, OP_MOV_CNT_R1, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R1);
        c->flags |= _FLAG_FIRST_EXPR_SET;
    }
    else
    {
        emit_bytes(c, OP_MOV_CNT_R2, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R2);
    }
}

static void dval(Compiler *c)
{
    double val = strtod(c->parser.pre.start, NULL);
    int arg = add_constant(&c->func->ch, OBJ(Double(val)));

    if (!c->flags)
    {
        emit_bytes(c, OP_MOV_CNT_R1, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R1);
        c->flags |= _FLAG_FIRST_EXPR_SET;
    }
    else
    {

        emit_bytes(c, OP_MOV_CNT_R2, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R2);
    }
}
static void ival(Compiler *c)
{

    int arg = add_constant(&c->func->ch, OBJ(Int((atoi(c->parser.pre.start)))));

    if (!c->flags)
    {
        emit_bytes(c, OP_MOV_CNT_R1, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R1);
        c->flags |= _FLAG_FIRST_EXPR_SET;
    }
    else
    {

        emit_bytes(c, OP_MOV_CNT_R2, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R2);
    }
}
static void llint(Compiler *c)
{
    int arg = add_constant(&c->func->ch, OBJ(Long(atoll(c->parser.pre.start))));

    if (!c->flags)
    {
        emit_bytes(c, OP_MOV_CNT_R1, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R1);
        c->flags |= _FLAG_FIRST_EXPR_SET;
    }
    else
    {

        emit_bytes(c, OP_MOV_CNT_R2, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R2);
    }
}
static void ch(Compiler *c)
{
    int arg = add_constant(&c->func->ch, OBJ(Char(*++c->parser.pre.start)));

    if (!c->flags)
    {
        emit_bytes(c, OP_MOV_CNT_R1, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R1);
        c->flags |= _FLAG_FIRST_EXPR_SET;
    }
    else
    {

        emit_bytes(c, OP_MOV_CNT_R2, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R2);
    }
}

static void boolean(Compiler *c)
{
    int arg = 0;
    if (*c->parser.pre.start == 'n')
        arg = add_constant(&c->func->ch, OBJ(Null()));
    else
        arg = add_constant(&c->func->ch, OBJ(Bool(*c->parser.pre.start == 't' ? true : false)));
    if (!c->flags)
    {
        emit_bytes(c, OP_MOV_CNT_R1, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R1);
        c->flags |= _FLAG_FIRST_EXPR_SET;
    }
    else
    {

        emit_bytes(c, OP_MOV_CNT_R2, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R2);
    }
}
static const char *parse_string(Compiler *c)
{
    char *ch = (char *)++c->parser.pre.start;
    ch[c->parser.pre.size - 2] = '\0';
    return ch;
}
static void cstr(Compiler *c)
{

    int arg = add_constant(&c->func->ch, OBJ(CString(parse_string(c))));

    if (!c->flags)
    {
        emit_bytes(c, OP_MOV_CNT_R1, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R1);
        c->flags |= _FLAG_FIRST_EXPR_SET;
    }
    else
    {

        emit_bytes(c, OP_MOV_CNT_R2, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R2);
    }
}
static void string(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` prior to string declaration.", &c->parser);
    consume(TOKEN_STR, "Expect string declaration", &c->parser);
    int arg = add_constant(&c->func->ch, OBJ(String(parse_string(c))));
    consume(TOKEN_CH_RPAREN, "Expect `)` after string declaration.", &c->parser);

    if (!c->flags)
    {
        emit_bytes(c, OP_MOV_CNT_R1, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R1);
        c->flags |= _FLAG_FIRST_EXPR_SET;
    }
    else
    {

        emit_bytes(c, OP_MOV_CNT_R2, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R2);
    }
}

static void array_alloc(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` prior to allocation.", &c->parser);

    if (match(TOKEN_INT, &c->parser))
    {
        int arg = add_constant(&c->func->ch, OBJ(GROW_ARRAY(NULL, atoi(c->parser.pre.start), ARENA_INTS)));
        emit_bytes(c, OP_MOV_CNT_R1, arg);
        if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
            emit_byte(c, OP_STR_R1);
    }
    else if (match(TOKEN_THIS, &c->parser))
    {
        _this(c);
        emit_byte(c, OP_CPY_ARRAY);
    }
    else if (match(TOKEN_ID, &c->parser))
    {
        id(c);
        emit_byte(c, OP_CPY_ARRAY);
    }
    else if (match(TOKEN_CH_LSQUARE, &c->parser))
        array(c);
    else
        error("ERROR: Invalid expression inside of array allocation", &c->parser);

    consume(TOKEN_CH_RPAREN, "Expect `)` after allocation.", &c->parser);
}
static void vector_alloc(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` prior to vector allocation", &c->parser);
    int arg = 0;

    if (match(TOKEN_CH_RPAREN, &c->parser))
        arg = add_constant(&c->func->ch, OBJ(Int(STACK_SIZE)));

    else if (match(TOKEN_INT, &c->parser))
    {
        int n = atoi(c->parser.pre.start);
        arg = add_constant(&c->func->ch, OBJ(Int(n)));
        consume(TOKEN_CH_RPAREN, "Expect `)` after vector allocation.", &c->parser);
    }
    else
        error("ERROR: Invalid vector allocation", &c->parser);

    emit_bytes(c, OP_MOV_CNT_R1, arg);
    emit_byte(c, OP_ALLOC_VECTOR);
    if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
        emit_byte(c, OP_STR_E2);
}

static void stack_alloc(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` prior to stack allocation", &c->parser);

    int arg = 0;

    if (match(TOKEN_CH_RPAREN, &c->parser))
        arg = add_constant(&c->func->ch, OBJ(Int(STACK_SIZE)));

    else if (match(TOKEN_INT, &c->parser))
    {
        int n = atoi(c->parser.pre.start);
        arg = add_constant(&c->func->ch, OBJ(Int(n)));

        consume(TOKEN_CH_RPAREN, "Expect `)` after Stack allocation", &c->parser);
    }
    else
        error("ERROR: Invalid stack allocation", &c->parser);

    emit_bytes(c, OP_MOV_CNT_R1, arg);
    emit_byte(c, OP_ALLOC_STACK);
    if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
        emit_byte(c, OP_STR_E2);
}

static void table(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` prior to table allocation", &c->parser);

    int cst = 0;

    if (match(TOKEN_CH_RPAREN, &c->parser))
        cst = add_constant(&c->func->ch, OBJ(Int(STACK_SIZE)));
    else if (match(TOKEN_INT, &c->parser))
    {
        int n = atoi(c->parser.pre.start);
        cst = add_constant(&c->func->ch, OBJ(Int(n)));
        consume(TOKEN_CH_RPAREN, "Expect `)` after table declaration", &c->parser);
    }
    else
        error("ERROR: Invalid table allocation", &c->parser);

    emit_bytes(c, OP_MOV_CNT_R1, cst);
    emit_byte(c, OP_ALLOC_TABLE);

    if (c->count.scope_depth > 0 || (CALL_PARAM(c->flags)))
        emit_byte(c, OP_STR_E2);
}

static int resolve_native(Compiler *c, Arena *ar)
{

    Element el = find_entry(&c->base->lookup.native, ar);

    if (el.type == ARENA && el.arena.type == ARENA_INT)
        return el.arena.as.Int;

    return -1;
}
static void parse_native_var_arg(Compiler *c)
{

    char *ch = (char *)c->parser.pre.start;
    ch[c->parser.pre.size] = '\0';

    Arena ar = native_name(ch);
    int arg = resolve_native(c, &ar);
    emit_bytes(c, OP_GET_NATIVE, arg);
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_STR_E2);
    consume(TOKEN_CH_LPAREN, "Expect `(` prior to function call", &c->parser);
    _call(c);
}

static Arena parse_func_id(Compiler *c)
{
    char *ch = (char *)c->parser.pre.start;
    ch[c->parser.pre.size] = '\0';
    return func_name(ch);
}

static Arena parse_id(Compiler *c)
{
    char *ch = (char *)c->parser.pre.start;
    ch[c->parser.pre.size] = '\0';
    return Var(ch);
}

static Arena get_id(Compiler *c)
{
    bool pre_inc = (c->parser.pre.type == TOKEN_OP_INC);
    bool pre_dec = (c->parser.pre.type == TOKEN_OP_DEC);

    if (match(TOKEN_ID, &c->parser))
        ;

    Arena ar = parse_id(c);

    uint8_t get, set;

    Arena args = GROW_ARRAY(NULL, 3 * sizeof(int), ARENA_INTS);
    int arg = resolve_local(c, &ar);

    if (arg != -1)
    {
        get = OP_GET_LOCAL;
        set = OP_SET_LOCAL;
    }
    else if ((arg = resolve_upvalue(c, &ar)) != -1)
    {
        get = OP_GET_UPVALUE;
        set = OP_SET_UPVALUE;
    }
    else
    {
        arg = add_constant(&c->func->ch, OBJ(ar));
        get = OP_GET_GLOBAL;
        set = OP_SET_GLOBAL;
    }
    args.listof.Ints[0] = arg;
    args.listof.Ints[1] = get;

    if (pre_inc)
        emit_bytes(c, get == OP_GET_LOCAL ? OP_INC_LOC : OP_INC_GLO, arg);
    else if (pre_dec)
        emit_bytes(c, get == OP_GET_LOCAL ? OP_DEC_LOC : OP_DEC_GLO, arg);
    else if (match(TOKEN_OP_DEC, &c->parser))
        emit_bytes(c, get == OP_GET_LOCAL ? OP_DEC_LOC : OP_DEC_GLO, arg);
    else if (match(TOKEN_OP_INC, &c->parser))
        emit_bytes(c, get == OP_GET_LOCAL ? OP_INC_LOC : OP_INC_GLO, arg);
    else if (match(TOKEN_OP_ASSIGN, &c->parser))
    {
        expression(c);

        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_bytes(c, set, arg);
    }
    else
    {
        emit_bytes(c, get, arg);
        if (get == OP_GET_GLOBAL && (c->count.scope_depth > 0 || (CALL_PARAM(c->flags))))
            emit_byte(c, 1);
        else if (get == OP_GET_GLOBAL)
            emit_byte(c, 0);

        if (check(TOKEN_CH_DOT, &c->parser))
            c->current.array_set = set,
            c->current.array_index = arg;
    }
    c->flags &= _FLAG_FIRST_EXPR_RST;

    return args;
}

static int resolve_call(Compiler *c, Arena *ar)
{

    Element el = find_entry(&c->base->lookup.call, ar);

    if (el.type == ARENA && el.arena.type == ARENA_INT)
        return el.arena.as.Int;

    return -1;
}

static int resolve_instance(Compiler *c, Arena ar)
{
    Element el = find_entry(&c->base->lookup.class, &ar);

    if (el.type == ARENA && el.arena.type == ARENA_INT)
        return el.arena.as.Int;

    return -1;
}

static void push_array_val(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` after push.", &c->parser);
    expression(c);
    c->flags &= _FLAG_FIRST_EXPR_RST;
    emit_byte(c, (c->count.scope_depth > 0)
                     ? OP_PUSH_LOCAL_ARRAY_VAL
                     : OP_PUSH_GLOB_ARRAY_VAL);
    emit_bytes(c, c->current.array_set, c->current.array_index);
    consume(TOKEN_CH_RPAREN, "Expect `)` after push expression.", &c->parser);
}
static void pop_array_val(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` after push.", &c->parser);
    emit_byte(c, (c->count.scope_depth > 0)
                     ? OP_POP_LOCAL_ARRAY_VAL
                     : OP_POP_GLOB_ARRAY_VAL);
    emit_bytes(c, c->current.array_set, c->current.array_index);
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_POP);

    consume(TOKEN_CH_RPAREN, "Expect `)` after push expression.", &c->parser);
}

static void reverse_array(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` prior to calling reverse.", &c->parser);
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_REVERSE_LOCAL_ARRAY);
    else
    {
        emit_byte(c, OP_REVERSE_GLOB_ARRAY);
        emit_bytes(c, c->current.array_set, c->current.array_index);
    }
    consume(TOKEN_CH_RPAREN, "Expect `)` after call to reverse.", &c->parser);
}
static void sort_array(Compiler *c)
{
    consume(TOKEN_CH_LPAREN, "Expect `(` prior to calling reverse.", &c->parser);
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_SORT_LOCAL_ARRAY);
    else
        emit_bytes(c, OP_ZERO_E2, OP_SORT_GLOB_ARRAY);
    consume(TOKEN_CH_RPAREN, "Expect `)` after call to reverse.", &c->parser);
    emit_bytes(c, c->current.array_set, c->current.array_index);
}

static void search_array(Compiler *c)
{
    emit_byte(c, OP_MOV_R1_R4);
    consume(TOKEN_CH_LPAREN, "Expect `(` prior to calling reverse.", &c->parser);

    expression(c);

    if (c->count.scope_depth > 0)
        emit_byte(c, OP_BIN_SEARCH_LOCAL_ARRAY);
    else
        emit_byte(c, OP_BIN_SEARCH_GLOB_ARRAY);

    consume(TOKEN_CH_RPAREN, "Expect `)` after call to reverse.", &c->parser);
}

static void _this(Compiler *c)
{
    if (!c->classc)
    {
        error("ERROR: can't use `this` keyword outside of a class body.", &c->parser);
        return;
    }

    // c->flags |= _FLAG_INSTANCE_SET;

    if (match(TOKEN_CH_DOT, &c->parser))
        dot(c);

    // c->flags &= _FLAG_INSTANCE_RST;
}

static void dot(Compiler *c)
{
    match(TOKEN_ID, &c->parser);

    Arena ar = parse_id(c);

    if (ar.as.hash == c->base->hash.bin_search.as.hash)
    {
        emit_byte(c, OP_MOV_E1_E3);
        search_array(c);
        return;
    }

    if (ar.as.hash == c->base->hash.reverse.as.hash)
    {
        emit_byte(c, OP_MOV_E1_E3);
        reverse_array(c);
        return;
    }
    if (ar.as.hash == c->base->hash.sort.as.hash)
    {
        emit_byte(c, OP_MOV_E1_E3);
        sort_array(c);
        return;
    }
    if (ar.as.hash == c->base->hash.len.as.hash)
    {
        emit_byte(c, OP_MOV_E1_E3);
        emit_bytes(c, (c->count.scope_depth > 0) ? OP_LEN_LOCAL : OP_LEN, OP_ZERO_E1);

        return;
    }
    if (ar.as.hash == c->base->hash.push.as.hash)
    {

        emit_byte(c, OP_CONDITIONAL_MOV_R1_E1);
        emit_bytes(c, OP_MOV_E1_E3, OP_ZERO_E1);
        push_array_val(c);

        return;
    }

    if (ar.as.hash == c->base->hash.pop.as.hash)
    {

        emit_bytes(c, OP_MOV_E1_E3, OP_ZERO_E1);
        pop_array_val(c);

        return;
    }

    if (match(TOKEN_OP_ASSIGN, &c->parser))
    {

        expression(c);
        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        int cst = add_constant(&c->func->ch, OBJ(ar));
        emit_bytes(c, OP_SET_PROP, cst);
    }
    else if (match(TOKEN_ADD_ASSIGN, &c->parser))
    {

        int cst = add_constant(&c->func->ch, OBJ(ar));
        emit_bytes(c, OP_GET_PROP, cst);

        c->flags |= _FLAG_FIRST_EXPR_SET;
        expression(c);
        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_byte(c, (c->count.scope_depth > 0) ? OP_ADD_LOCAL : OP_ADD);

        emit_bytes(c, OP_SET_PROP, cst);
    }
    else if (match(TOKEN_SUB_ASSIGN, &c->parser))
    {

        int cst = add_constant(&c->func->ch, OBJ(ar));
        emit_bytes(c, OP_GET_PROP, cst);

        c->flags |= _FLAG_FIRST_EXPR_SET;
        expression(c);
        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_byte(c, (c->count.scope_depth > 0) ? OP_SUB_LOCAL : OP_SUB);

        emit_bytes(c, OP_SET_PROP, cst);
    }
    else if (match(TOKEN_MUL_ASSIGN, &c->parser))
    {

        int cst = add_constant(&c->func->ch, OBJ(ar));
        emit_bytes(c, OP_GET_PROP, cst);

        c->flags |= _FLAG_FIRST_EXPR_SET;
        expression(c);
        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_byte(c, (c->count.scope_depth > 0) ? OP_MUL_LOCAL : OP_MUL);

        emit_bytes(c, OP_SET_PROP, cst);
    }
    else if (match(TOKEN_DIV_ASSIGN, &c->parser))
    {

        int cst = add_constant(&c->func->ch, OBJ(ar));
        emit_bytes(c, OP_GET_PROP, cst);

        c->flags |= _FLAG_FIRST_EXPR_SET;
        expression(c);
        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_byte(c, (c->count.scope_depth > 0) ? OP_DIV_LOCAL : OP_DIV);

        emit_bytes(c, OP_SET_PROP, cst);
    }
    else if (match(TOKEN_MOD_ASSIGN, &c->parser))
    {
        int cst = add_constant(&c->func->ch, OBJ(ar));
        emit_bytes(c, OP_GET_PROP, cst);

        c->flags |= _FLAG_FIRST_EXPR_SET;
        expression(c);
        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_byte(c, (c->count.scope_depth > 0) ? OP_MOD_LOCAL : OP_MOD);

        emit_bytes(c, OP_SET_PROP, cst);
    }
    else if (match(TOKEN_AND_ASSIGN, &c->parser))
    {

        int cst = add_constant(&c->func->ch, OBJ(ar));
        emit_bytes(c, OP_GET_PROP, cst);

        c->flags |= _FLAG_FIRST_EXPR_SET;
        expression(c);
        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_byte(c, (c->count.scope_depth > 0) ? OP_AND_LOCAL : OP_AND);

        emit_bytes(c, OP_SET_PROP, cst);
    }
    else if (match(TOKEN_OR__ASSIGN, &c->parser))
    {
        int cst = add_constant(&c->func->ch, OBJ(ar));
        emit_bytes(c, OP_GET_PROP, cst);

        c->flags |= _FLAG_FIRST_EXPR_SET;
        expression(c);
        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_byte(c, (c->count.scope_depth > 0) ? OP_OR_LOCAL : OP_OR);

        emit_bytes(c, OP_SET_PROP, cst);
    }
    else
    {
        int cst = add_constant(&c->func->ch, OBJ(ar));

        if (check(TOKEN_CH_LPAREN, &c->parser))
            emit_bytes(c, OP_GET_METHOD, cst);
        else
            emit_bytes(c, OP_GET_PROP, cst);
    }

    c->flags &= _FLAG_FIRST_EXPR_RST;
}

static void int_array(Compiler *c)
{
    Element el = OBJ(Ints(NULL, 0));

    do
    {
        if (match(TOKEN_INT, &c->parser))
            push_int(&el, atoi(c->parser.pre.start));
        else
        {
            error("Unexpected type in array declaration", &c->parser);
            return;
        }
    } while (match(TOKEN_CH_COMMA, &c->parser));

    int arg = add_constant(&c->func->ch, el);
    emit_bytes(c, OP_MOV_CNT_R1, arg);
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_STR_R1);
}
static void double_array(Compiler *c)
{
    Element el = OBJ(Doubles(NULL, 0));

    do
    {
        if (match(TOKEN_DOUBLE, &c->parser))
            push_double(&el, strtod(c->parser.pre.start, NULL));
        else
        {
            error("Unexpected type in array declaration", &c->parser);
            return;
        }
    } while (match(TOKEN_CH_COMMA, &c->parser));

    int arg = add_constant(&c->func->ch, el);
    emit_bytes(c, OP_MOV_CNT_R1, arg);
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_STR_R1);
}
static void string_array(Compiler *c)
{
    Element el = OBJ(Strings());

    do
    {
        if (match(TOKEN_STR, &c->parser))
            push_string(&el, parse_string(c));
        else
        {
            error("Unexpected type in array declaration", &c->parser);
            return;
        }
    } while (match(TOKEN_CH_COMMA, &c->parser));

    int arg = add_constant(&c->func->ch, el);
    emit_bytes(c, OP_MOV_CNT_R1, arg);
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_STR_R1);
}
static void long_array(Compiler *c)
{
    Element el = OBJ(Longs(NULL, 0));
    do
    {
        if (match(TOKEN_LLINT, &c->parser))
            push_long(&el, atoll(c->parser.pre.start));
        else
        {
            error("Unexpected type in array declaration", &c->parser);
            return;
        }
    } while (match(TOKEN_CH_COMMA, &c->parser));

    int arg = add_constant(&c->func->ch, el);
    emit_bytes(c, OP_MOV_CNT_R1, arg);
    if (c->count.scope_depth > 0)
        emit_byte(c, OP_STR_R1);
}

static void array(Compiler *c)
{

    emit_byte(c, OP_ZERO_E1);
    switch (c->parser.cur.type)
    {
    case TOKEN_INT:
        int_array(c);
        break;
    case TOKEN_STR:
        string_array(c);
        break;
    case TOKEN_DOUBLE:
        double_array(c);
        break;
    case TOKEN_LLINT:
        long_array(c);
        break;
    default:
        error("Unexpected type in array declaration", &c->parser);
    }

    consume(TOKEN_CH_RSQUARE, "Expect closing brace after array declaration.", &c->parser);
}

static void _access(Compiler *c)
{

    expression(c);
    c->flags &= _FLAG_FIRST_EXPR_RST;

    consume(TOKEN_CH_RSQUARE, "Expect closing brace after array access.", &c->parser);

    emit_byte(c, OP_MOV_E1_E3);
    emit_byte(c, OP_ZERO_E1);

    if (match(TOKEN_OP_ASSIGN, &c->parser))
    {

        emit_byte(c, OP_MOV_R1_R4);
        expression(c);
        c->flags &= _FLAG_FIRST_EXPR_RST;

        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_byte(c, (c->count.scope_depth > 0)
                         ? OP_SET_LOCAL_ACCESS
                         : OP_SET_GLOB_ACCESS);
    }
    else
        emit_byte(c, (c->count.scope_depth > 0)
                         ? OP_GET_LOCAL_ACCESS
                         : OP_GET_GLOB_ACCESS);
}

static void id(Compiler *c)
{
    bool pre_inc = (c->parser.pre.type == TOKEN_OP_INC);
    bool pre_dec = (c->parser.pre.type == TOKEN_OP_DEC);

    match(TOKEN_ID, &c->parser);

    Arena ar = parse_func_id(c);
    uint8_t get, set;
    int arg = resolve_call(c, &ar);

    if (arg != -1)
    {
        emit_bytes(c, OP_GET_CLOSURE, arg);

        if (c->count.scope_depth > 0 || CALL_PARAM(c->flags))
            emit_byte(c, OP_STR_E2);
        return;
    }

    if ((arg = resolve_instance(c, ar)) != -1)
    {

        emit_bytes(c, OP_MOV_CLASS_R4, arg);

        consume(TOKEN_CH_LPAREN, "Expect `(` prior to method call.", &c->parser);

        if (c->base->stack.instance[arg]->init)
        {
            Closure *clos = c->base->stack.instance[arg]->init;
            int cst = add_constant(&c->func->ch, CLOSURE(clos));
            emit_bytes(c, OP_MOV_CNT_E2, cst);
            if (c->count.scope_depth > 0 || CALL_PARAM(c->flags))
                emit_byte(c, OP_STR_E2);
            _call(c);
        }
        else
            consume(TOKEN_CH_RPAREN, "Expect `)` after method call.", &c->parser);

        emit_byte(c, OP_MOV_E4_E2);

        if (c->count.scope_depth > 0 || CALL_PARAM(c->flags))
            emit_byte(c, OP_STR_E4);

        return;
    }

    ar.type = ARENA_VAR;
    arg = resolve_local(c, &ar);

    if (arg != -1)
    {

        get = OP_GET_LOCAL;
        set = OP_SET_LOCAL;
    }
    else if ((arg = resolve_upvalue(c, &ar)) != -1)
    {
        get = OP_GET_UPVALUE;
        set = OP_SET_UPVALUE;
    }

    else
    {
        arg = add_constant(&c->func->ch, OBJ(ar));
        get = OP_GET_GLOBAL;
        set = OP_SET_GLOBAL;
    }

    emit_bytes(c, OP_ZERO_E1, OP_ZERO_R5);

    if (pre_inc)
        emit_bytes(c, get == OP_GET_LOCAL ? OP_INC_LOC : OP_INC_GLO, arg);
    else if (pre_dec)
        emit_bytes(c, get == OP_GET_LOCAL ? OP_DEC_LOC : OP_DEC_GLO, arg);
    else if (match(TOKEN_OP_DEC, &c->parser))
        emit_bytes(c, get == OP_GET_LOCAL ? OP_DEC_LOC : OP_DEC_GLO, arg);
    else if (match(TOKEN_OP_INC, &c->parser))
        emit_bytes(c, get == OP_GET_LOCAL ? OP_INC_LOC : OP_INC_GLO, arg);
    else if (match(TOKEN_OP_ASSIGN, &c->parser))
    {
        emit_byte(c, OP_ZERO_E2);
        expression(c);

        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_bytes(c, set, arg);
    }
    else if (match(TOKEN_ADD_ASSIGN, &c->parser))
    {
        emit_bytes(c, get, arg);
        c->flags |= _FLAG_FIRST_EXPR_SET;

        if (get == OP_GET_GLOBAL)
            emit_byte(c, (c->count.scope_depth > 0 || CALL_PARAM(c->flags)) ? 1 : 0);
        expression(c);

        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_byte(c, (c->count.scope_depth > 0) ? OP_ADD_LOCAL : OP_ADD);
        emit_bytes(c, set, arg);
    }
    else if (match(TOKEN_SUB_ASSIGN, &c->parser))
    {
        emit_bytes(c, get, arg);

        c->flags |= _FLAG_FIRST_EXPR_SET;

        if (get == OP_GET_GLOBAL)
            emit_byte(c, (c->count.scope_depth > 0 || CALL_PARAM(c->flags)) ? 1 : 0);

        expression(c);

        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);
        emit_byte(c, (c->count.scope_depth > 0) ? OP_SUB_LOCAL : OP_SUB);
        emit_bytes(c, set, arg);
    }
    else if (match(TOKEN_MUL_ASSIGN, &c->parser))
    {

        emit_bytes(c, get, arg);
        c->flags |= _FLAG_FIRST_EXPR_SET;

        if (get == OP_GET_GLOBAL)
            emit_byte(c, (c->count.scope_depth > 0 || CALL_PARAM(c->flags)) ? 1 : 0);

        expression(c);

        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);
        emit_byte(c, (c->count.scope_depth > 0) ? OP_MUL_LOCAL : OP_MUL);
        emit_bytes(c, set, arg);
    }
    else if (match(TOKEN_DIV_ASSIGN, &c->parser))
    {
        emit_bytes(c, get, arg);
        c->flags |= _FLAG_FIRST_EXPR_SET;

        if (get == OP_GET_GLOBAL)
            emit_byte(c, (c->count.scope_depth > 0 || CALL_PARAM(c->flags)) ? 1 : 0);

        expression(c);
        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);
        emit_byte(c, (c->count.scope_depth > 0) ? OP_DIV_LOCAL : OP_DIV);
        emit_bytes(c, set, arg);
    }
    else if (match(TOKEN_MOD_ASSIGN, &c->parser))
    {
        emit_bytes(c, get, arg);
        c->flags |= _FLAG_FIRST_EXPR_SET;

        if (get == OP_GET_GLOBAL)
            emit_byte(c, (c->count.scope_depth > 0 || CALL_PARAM(c->flags)) ? 1 : 0);

        expression(c);
        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_byte(c, (c->count.scope_depth > 0) ? OP_MOD_LOCAL : OP_MOD);
        emit_bytes(c, set, arg);
    }
    else if (match(TOKEN_AND_ASSIGN, &c->parser))
    {
        emit_bytes(c, get, arg);
        c->flags |= _FLAG_FIRST_EXPR_SET;

        if (get == OP_GET_GLOBAL)
            emit_byte(c, (c->count.scope_depth > 0 || CALL_PARAM(c->flags)) ? 1 : 0);

        expression(c);
        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_byte(c, (c->count.scope_depth > 0) ? OP_AND_LOCAL : OP_AND);
        emit_bytes(c, set, arg);
    }
    else if (match(TOKEN_OR__ASSIGN, &c->parser))
    {
        emit_bytes(c, get, arg);
        c->flags |= _FLAG_FIRST_EXPR_SET;

        if (get == OP_GET_GLOBAL)
            emit_byte(c, (c->count.scope_depth > 0 || CALL_PARAM(c->flags)) ? 1 : 0);

        expression(c);
        if (match(TOKEN_CH_TERNARY, &c->parser))
            ternary_statement(c);
        else if (match(TOKEN_CH_NULL_COALESCING, &c->parser))
            null_coalescing_statement(c);

        emit_byte(c, (c->count.scope_depth > 0) ? OP_OR_LOCAL : OP_OR);
        emit_bytes(c, set, arg);
    }
    else
    {
        emit_bytes(c, get, arg);

        if (get == OP_GET_GLOBAL)
        {
            if (c->count.scope_depth > 0 || CALL_PARAM(c->flags))
                emit_byte(c, (uint16_t)0x0001);
            else
                emit_byte(c, (uint16_t)0x0000);
        }

        if (check(TOKEN_CH_DOT, &c->parser))
            c->current.array_set = set,
            c->current.array_index = arg;
    }

    c->flags &= _FLAG_FIRST_EXPR_RST;
}

static int parse_var(Compiler *c, Arena ar)
{
    declare_var(c, ar);
    if (c->count.scope_depth > 0)
        return -1;
    return add_constant(&c->func->ch, OBJ(ar));
}

static bool idcmp(Arena a, Arena b)
{
    if (a.as.len != b.as.len)
        return false;

    return a.as.hash == b.as.hash;
}

static int resolve_local(Compiler *c, Arena *name)
{
    for (int i = c->count.local - 1; i >= 0; i--)
        if (idcmp(*name, c->stack.local[i].name))
            return i;
    return -1;
}
static int resolve_upvalue(Compiler *c, Arena *name)
{

    if (!c->enclosing)
        return -1;

    int local = resolve_local(c->enclosing, name);

    if (local != -1)
    {
        c->enclosing->stack.local[local].captured = true;
        return add_upvalue(c, (uint8_t)local, true);
    }

    int upvalue = resolve_upvalue(c->enclosing, name);
    if (upvalue != -1)
    {
        return add_upvalue(c, (uint8_t)upvalue, false);
    }

    return -1;
}

static int add_upvalue(Compiler *c, int index, bool islocal)
{
    int count = c->func->upvalue_count;

    for (int i = 0; i < count; i++)
    {
        Upvalue *upval = &c->stack.upvalue[i];

        if (upval->index == index && upval->islocal == islocal)
            return i;
    }

    if (count > UINT16_MAX)
    {
        error("ERROR: To many closure variables in function.", &c->parser);
        return 0;
    }

    c->stack.upvalue[c->func->upvalue_count].islocal = islocal;
    c->stack.upvalue[c->func->upvalue_count].index = (uint16_t)index;
    c->count.upvalue++;
    return c->func->upvalue_count++;
}

static void declare_var(Compiler *c, Arena ar)
{
    if (c->count.scope_depth == 0)
        return;

    for (int i = c->count.local - 1; i >= 0; i--)
    {
        Local *local = &c->stack.local[i];

        if (local->depth != 0 && local->depth < c->count.scope_depth)
            break;

        else if (idcmp(ar, local->name))
            error("ERROR: Duplicate variable identifiers in scope", &c->parser);
    }

    add_local(c, &ar);
}

static void add_local(Compiler *c, Arena *ar)
{
    if (c->count.local == LOCAL_COUNT)
    {
        error("ERROR: Too many local variables in function.", &c->parser);
        return;
    }
    c->stack.local[c->count.local].name = *ar;
    c->stack.local[c->count.local].depth = c->count.scope_depth;
    c->stack.local[c->count.local].captured = false;
    c->count.local++;
}
static Function *end_compile(Compiler *a)
{
    Function *f = a->func;

    emit_return(a);
#ifdef DEBUG_PRINT_CODE
    if (!a->parser.err)
        disassemble_chunk(
            &a->func->ch,
            a->func->name.as.String);
#endif
    if (a->enclosing)
    {

        Parser tmp = a->parser;
        a = a->enclosing;
        a->parser = tmp;
    }

    return f;
}

void mark_compiler_roots(Compiler *c)
{
    Compiler *compiler = NULL;
    for (compiler = c; compiler; compiler = compiler->enclosing)
    {
        mark_obj(OBJ(compiler->func->name));
        mark_obj(OBJ(compiler->func->ch.cases));
        mark_obj(OBJ(compiler->func->ch.lines));
        mark_obj(OBJ(compiler->func->ch.op_codes));
        mark_obj(STK(compiler->func->ch.constants));
        mark_obj(FUNC(compiler->func));
    }
}

Function *compile(const char *src)
{
    Compiler c;

    init_scanner(src);

    init_compiler(&c, NULL, SCRIPT, func_name("SCRIPT"));

    c.base = &c;
    c.base->meta.cwd = NULL;
    c.base->meta.current_file = NULL;

    c.base->lookup.call = GROW_TABLE(NULL, TABLE_SIZE);
    c.base->lookup.class = GROW_TABLE(NULL, TABLE_SIZE);
    c.base->lookup.include = GROW_TABLE(NULL, TABLE_SIZE);
    c.base->lookup.native = GROW_TABLE(NULL, TABLE_SIZE);

    // mark_obj(c.base->func)

    mark_obj(TABLE(c.base->lookup.call));
    mark_obj(TABLE(c.base->lookup.class));
    mark_obj(TABLE(c.base->lookup.include));
    mark_obj(TABLE(c.base->lookup.native));

    c.base->hash.len = CString("len");
    c.base->hash.init = String("init");
    c.base->hash.push = CString("push");
    c.base->hash.pop = CString("pop");
    c.base->hash.reverse = CString("reverse");

    c.parser.panic = false;
    c.parser.err = false;
    c.parser.current_file = NULL;

    write_table(c.base->lookup.native, CString("clock"), OBJ(Int(c.base->count.native++)));
    write_table(c.base->lookup.native, CString("square"), OBJ(Int(c.base->count.native++)));
    write_table(c.base->lookup.native, CString("prime"), OBJ(Int(c.base->count.native++)));
    write_table(c.base->lookup.native, CString("file"), OBJ(Int(c.base->count.native++)));
    write_table(c.base->lookup.native, CString("strstr"), OBJ(Int(c.base->count.native++)));
    // write_table(c.base->lookup.native, CString("reverse"), OBJ(Int(c.base->count.native++)));

    advance_compiler(&c.parser);

    while (!match(TOKEN_EOF, &c.parser))
        declaration(&c);
    consume(TOKEN_EOF, "Expect end of expression", &c.parser);

    Function *f = end_compile(&c);

    FREE(PTR((c.base->lookup.call - 1)));
    FREE(PTR((c.base->lookup.native - 1)));
    FREE(PTR((c.base->lookup.class - 1)));
    FREE(PTR((c.base->lookup.include - 1)));
    FREE(PTR((c.base->hash.init.as.String)));

    return c.parser.err ? NULL : f;
}
Function *compile_path(const char *src, const char *path, const char *name)
{
    Compiler c;

    init_scanner(src);

    init_compiler(&c, NULL, SCRIPT, func_name("SCRIPT"));

    c.base = &c;
    c.base->meta.cwd = path;
    c.base->meta.current_file = name;

    c.base->lookup.call = GROW_TABLE(NULL, TABLE_SIZE);
    c.base->lookup.class = GROW_TABLE(NULL, TABLE_SIZE);
    c.base->lookup.include = GROW_TABLE(NULL, TABLE_SIZE);
    c.base->lookup.native = GROW_TABLE(NULL, TABLE_SIZE);

    c.base->hash.len = CString("len");
    c.base->hash.init = String("init");
    c.base->hash.push = CString("push");
    c.base->hash.pop = CString("pop");
    c.base->hash.reverse = CString("reverse");
    c.base->hash.remove = CString("remove");
    c.base->hash.sort = CString("sort");
    c.base->hash.bin_search = CString("search");

    c.parser.panic = false;
    c.parser.err = false;
    c.parser.current_file = name;

    mark_obj(TABLE(c.base->lookup.call));
    mark_obj(TABLE(c.base->lookup.class));
    mark_obj(TABLE(c.base->lookup.include));
    mark_obj(TABLE(c.base->lookup.native));

    write_table(c.base->lookup.native, CString("clock"), OBJ(Int(c.base->count.native++)));
    write_table(c.base->lookup.native, CString("square"), OBJ(Int(c.base->count.native++)));
    write_table(c.base->lookup.native, CString("prime"), OBJ(Int(c.base->count.native++)));
    write_table(c.base->lookup.native, CString("file"), OBJ(Int(c.base->count.native++)));
    write_table(c.base->lookup.native, CString("strstr"), OBJ(Int(c.base->count.native++)));

    advance_compiler(&c.parser);

    while (!match(TOKEN_EOF, &c.parser))
        declaration(&c);
    consume(TOKEN_EOF, "Expect end of expression", &c.parser);

    Function *f = end_compile(&c);

    FREE(PTR(c.parser.current_file));
    FREE(PTR((c.base->lookup.call - 1)));
    FREE(PTR((c.base->lookup.native - 1)));
    FREE(PTR((c.base->lookup.class - 1)));
    FREE(PTR((c.base->lookup.include - 1)));
    FREE(PTR((c.base->hash.init.as.String)));

    return c.parser.err ? NULL : f;
}
