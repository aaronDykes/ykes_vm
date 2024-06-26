#include <stdio.h>
#include "debug.h"

static int byte_instruction(const char *name, Chunk *chunk,
                            int offset)
{
    uint16_t slot = chunk->op_codes.listof.Shorts[offset];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int simple_instruction(const char *name, int offset)
{
    printf("%s\n", name);
    return ++offset;
}

static int constant_instruction(const char *name, Chunk *c, int offset)
{
    uint16_t constant = c->op_codes.listof.Shorts[offset];

    printf("%-16s %4d '", name, constant);
    print_line(c->constants[constant].as);
    printf("\n");
    return offset + 2;
}

static int jump_instruction(const char *name, int sign,
                            Chunk *chunk, int offset)
{
    uint16_t jump = chunk->op_codes.listof.Shorts[offset + 1];

    printf("%-16s %4d -> %d\n",
           name, offset,
           offset + 2 + sign * jump);

    return offset + 2;
}

void disassemble_chunk(Chunk *c, const char *name)
{

    printf("==== chunk: `%s` ====\n", name);

    for (int i = 0; i < c->op_codes.len;)
        i = disassemble_instruction(c, i);
}

int disassemble_instruction(Chunk *c, int offset)
{

    printf("%d: %04d ", c->lines.listof.Ints[offset], offset);

    switch (c->op_codes.listof.Shorts[offset])
    {
    case OP_CONSTANT:
        return constant_instruction("OP_CONSTANT", c, offset);
    case OP_CLOSURE:
    {
        offset++;
        uint16_t constant = c->op_codes.listof.Shorts[offset++];
        printf("%-16s %4d ", "OP_CLOSURE", constant);
        print_line(c->constants[constant].as);

        Closure *clos = c->constants[constant].as.closure;
        if (!clos)
            return offset;
        for (int j = 0; j < clos->upval_count; j++)
        {
            int isLocal = c->op_codes.listof.Shorts[offset++];
            int index = c->op_codes.listof.Shorts[offset++];
            printf("%04d      |                     %s %d\n",
                   offset - 2, isLocal ? "local" : "upvalue", index);
        }

        return offset;
    }
    case OP_PUSH_GLOB_ARRAY_VAL:
        return simple_instruction("OP_PUSH_GLOB_ARRAY_VAL", offset);
    case OP_PUSH_LOCAL_ARRAY_VAL:
        return simple_instruction("OP_PUSH_LOCAL_ARRAY_VAL", offset);
    case OP_POP_GLOB_ARRAY_VAL:
        return simple_instruction("OP_POP_GLOB_ARRAY_VAL", offset);
    case OP_POP_LOCAL_ARRAY_VAL:
        return simple_instruction("OP_POP_LOCAL_ARRAY_VAL", offset);

    case OP_RESET_ARGC:
        return simple_instruction("OP_RESET_ARGC", offset);
    case OP_SET_GLOB_ACCESS:
        return constant_instruction("OP_SET_GLOB_ACCESS", c, offset);
    case OP_GET_GLOB_ACCESS:
        return constant_instruction("OP_GET_GLOB_ACCESS", c, offset);
    case OP_SET_LOCAL_ACCESS:
        return constant_instruction("OP_SET_LOCAL_ACCESS", c, offset);
    case OP_GET_LOCAL_ACCESS:
        return constant_instruction("OP_GET_LOCAL_ACCESS", c, offset);
    case OP_SET_LOCAL_PARAM:
        return constant_instruction("OP_SET_LOCAL_PARAM", c, offset);
    case OP_EACH_ACCESS:
        return constant_instruction("OP_EACH_ACCESS", c, offset);
    case OP_EACH_LOCAL_ACCESS:
        return constant_instruction("OP_EACH_LOCAL_ACCESS", c, offset);
    case OP_METHOD:
        return constant_instruction("OP_METHOD", c, offset);
    case OP_CLASS:
        return constant_instruction("OP_CLASS", c, offset);
    case OP_GET_INSTANCE:
        return constant_instruction("OP_GET_INSTANCE", c, offset);
    case OP_CLOSE_UPVAL:
        return simple_instruction("OP_CLOSE_UPVAL", offset);
    case OP_GET_UPVALUE:
        return byte_instruction("OP_GET_UPVALUE", c, offset);
    case OP_SET_UPVALUE:
        return byte_instruction("OP_SET_UPVALUE", c, offset);
    case OP_PUSH_TOP:
        return byte_instruction("OP_PUSH_TOP", c, offset);
    case OP_GET_NATIVE:
        return simple_instruction("OP_GET_NATIVE", offset);
    case OP_GET_CLOSURE:
        return simple_instruction("OP_GET_CLOSURE", offset);
    case OP_GET_METHOD:
        return simple_instruction("OP_GET_METHOD", offset);
    case OP_MOV_CLASS_R4:
        return simple_instruction("OP_MOV_CLASS_R4", offset);
    case OP_MOV_CLASS_R5:
        return simple_instruction("OP_MOV_CLASS_R4", offset);
    case OP_NEG:
        return simple_instruction("OP_NEG", offset);
    case OP_LEN:
        return simple_instruction("OP_LEN", offset);
    case OP_LEN_LOCAL:
        return simple_instruction("OP_LEN_LOCAL", offset);
    case OP_INC:
        return simple_instruction("OP_DEC", offset);
    case OP_DEC:
        return simple_instruction("OP_DEC", offset);
    case OP_DEC_LOC:
        return simple_instruction("OP_DEC_LOC", offset);
    case OP_DEC_GLO:
        return simple_instruction("OP_DEC_GLO", offset);
    case OP_INC_LOC:
        return simple_instruction("OP_INC_LOC", offset);
    case OP_INC_GLO:
        return simple_instruction("OP_INC_GLO", offset);
    case OP_ADD:
        return simple_instruction("OP_ADD", offset);
    case OP_SUB:
        return simple_instruction("OP_SUB", offset);
    case OP_MUL:
        return simple_instruction("OP_MUL", offset);
    case OP_DIV:
        return simple_instruction("OP_DIV", offset);
    case OP_MOD:
        return simple_instruction("OP_MOD", offset);

    case OP_LT:
        return simple_instruction("OP_LT", offset);
    case OP_LE:
        return simple_instruction("OP_LE", offset);
    case OP_GT:
        return simple_instruction("OP_GT", offset);
    case OP_GE:
        return simple_instruction("OP_GE", offset);
    case OP_EQ:
        return simple_instruction("OP_EQ", offset);
    case OP_NE:
        return simple_instruction("OP_NE", offset);
    case OP_SEQ:
        return simple_instruction("OP_SEQ", offset);
    case OP_SNE:
        return simple_instruction("OP_SNE", offset);
    case OP_AND:
        return simple_instruction("OP_AND", offset);
    case OP_OR:
        return simple_instruction("OP_OR", offset);

    case OP_ADD_LOCAL:
        return simple_instruction("OP_ADD_LOCAL", offset);
    case OP_SUB_LOCAL:
        return simple_instruction("OP_SUB_LOCAL", offset);
    case OP_MUL_LOCAL:
        return simple_instruction("OP_MUL_LOCAL", offset);
    case OP_MOD_LOCAL:
        return simple_instruction("OP_MOD_LOCAL", offset);
    case OP_DIV_LOCAL:
        return simple_instruction("OP_DIV_LOCAL", offset);
    case OP_SEQ_LOCAL:
        return simple_instruction("OP_SEQ_LOCAL", offset);
    case OP_SNE_LOCAL:
        return simple_instruction("OP_SNE_LOCAL", offset);
    case OP_EQ_LOCAL:
        return simple_instruction("OP_EQ_LOCAL", offset);
    case OP_NE_LOCAL:
        return simple_instruction("OP_NE_LOCAL", offset);
    case OP_LT_LOCAL:
        return simple_instruction("OP_LT_LOCAL", offset);
    case OP_LE_LOCAL:
        return simple_instruction("OP_LE_LOCAL", offset);
    case OP_GT_LOCAL:
        return simple_instruction("OP_GT_LOCAL", offset);
    case OP_GE_LOCAL:
        return simple_instruction("OP_GE_LOCAL", offset);

    case OP_ALLOC_TABLE:
        return simple_instruction("OP_ALLOC_TABLE", offset);

    case OP_ALLOC_STACK:
        return simple_instruction("OP_ALLOC_STACK", offset);
    case OP_ALLOC_VECTOR:
        return simple_instruction("OP_ALLOC_VECTOR", offset);
    case OP_NULL:
        return simple_instruction("OP_NULL", offset);
    case OP_GET_LOCAL:
        return byte_instruction("OP_GET_LOCAL", c, offset);
    case OP_SET_LOCAL:
        return byte_instruction("OP_SET_LOCAL", c, offset);

    case OP_RM:
        return byte_instruction("OP_RM", c, offset);
    case OP_RM_LOCAL:
        return byte_instruction("OP_RM_LOCAL", c, offset);
    case OP_GET_GLOBAL:
        return simple_instruction("OP_GET_GLOBAL", offset);

    case OP_SET_GLOBAL:
        return simple_instruction("OP_SET_GLOBAL", offset);
    case OP_GLOBAL_DEF:
        return simple_instruction("OP_GLOBAL_DEF", offset);

    case OP_JMP_NIL:
        return jump_instruction("OP_JMP_NIL", 1, c, offset);
    case OP_JMP_NIL_LOCAL:
        return jump_instruction("OP_JMP_NIL_LOCAL", 1, c, offset);
    case OP_JMPF:
        return jump_instruction("OP_JMPF", 1, c, offset);
    case OP_JMPT:
        return jump_instruction("OP_JMPT", 1, c, offset);
    case OP_JMPL:
        return jump_instruction("OP_JMPL", 1, c, offset);
    case OP_JMPC:
        return jump_instruction("OP_JMPC", 1, c, offset);
    case OP_JMP:
        return jump_instruction("OP_JMP", 1, c, offset);
    case OP_LOOP:
        return jump_instruction("OP_LOOP", -1, c, offset);
    case OP_POPN:
        return simple_instruction("OP_POPN", offset);
    case OP_POP:
        return simple_instruction("OP_POP", offset);
    case OP_CALL:
        return byte_instruction("OP_CALL", c, offset);
    case OP_CALL_LOCAL:
        return byte_instruction("OP_CALL_LOCAL", c, offset);
    case OP_PRINT:
        return simple_instruction("OP_PRINT", offset);
    case OP_PRINT_LOCAL:
        return simple_instruction("OP_PRINT_LOCAL", offset);
    case OP_RETURN:
        return simple_instruction("OP_RETURN", offset);
    case OP_GET_PROP:
        return byte_instruction("OP_GET_PROP", c, offset);
    case OP_SET_PROP:
        return byte_instruction("OP_SET_PROP", c, offset);

    case OP_ZERO_ARENA_REGISTERS:
        return simple_instruction("OP_ZERO_ARENA_REGISTERS", offset);
    case OP_ZERO_R1:
        return simple_instruction("OP_ZERO_R1", offset);
    case OP_ZERO_R2:
        return simple_instruction("OP_ZERO_R2", offset);
    case OP_ZERO_R3:
        return simple_instruction("OP_ZERO_R3", offset);
    case OP_ZERO_R4:
        return simple_instruction("OP_ZERO_R4", offset);
    case OP_ZERO_R5:
        return simple_instruction("OP_ZERO_R5", offset);

    case OP_CONDITIONAL_MOV_R1_E1:
        return byte_instruction("OP_CONDITIONAL_MOV_R1_E1", c, offset);
    case OP_MOV_PEEK_R1:
        return byte_instruction("OP_MOV_PEEK_R1", c, offset);
    case OP_MOV_PEEK_R2:
        return byte_instruction("OP_MOV_PEEK_R2", c, offset);
    case OP_MOV_PEEK_R3:
        return byte_instruction("OP_MOV_PEEK_R3", c, offset);
    case OP_MOV_R1:
        return byte_instruction("OP_MOV_R1", c, offset);
    case OP_MOV_R2:
        return byte_instruction("OP_MOV_R2", c, offset);
    case OP_MOV_R3:
        return byte_instruction("OP_MOV_R3", c, offset);

    case OP_STR_R1:
        return simple_instruction("OP_STR_R1", offset);
    case OP_STR_R2:
        return simple_instruction("OP_STR_R2", offset);
    case OP_STR_R3:
        return simple_instruction("OP_STR_R3", offset);
    case OP_STR_R4:
        return simple_instruction("OP_STR_R4", offset);
    case OP_STR_R5:
        return simple_instruction("OP_STR_R5", offset);

    case OP_STR_E1:
        return simple_instruction("OP_STR_E1", offset);
    case OP_STR_E2:
        return simple_instruction("OP_STR_E2", offset);
    case OP_STR_E3:
        return simple_instruction("OP_STR_E3", offset);

    case OP_MOV_CNT_R1:
        return byte_instruction("OP_MOV_CNT_R1", c, offset);
    case OP_MOV_CNT_R2:
        return byte_instruction("OP_MOV_CNT_R2", c, offset);
    case OP_MOV_CNT_R3:
        return byte_instruction("OP_MOV_CNT_R3", c, offset);
    case OP_MOV_R1_R2:
        return byte_instruction("OP_MOV_R1_R2", c, offset);
    case OP_MOV_R1_R3:
        return byte_instruction("OP_MOV_R1_R3", c, offset);
    case OP_MOV_R2_R1:
        return byte_instruction("OP_MOV_R2_R1", c, offset);
    case OP_MOV_R3_R1:
        return byte_instruction("OP_MOV_R3_R1", c, offset);
    case OP_MOV_R3_R2:
        return byte_instruction("OP_MOV_R3_R2", c, offset);
        break;
    case OP_MOV_R1_R4:
        return byte_instruction("OP_MOV_R1_R4", c, offset);
    case OP_MOV_R1_E2:
        return byte_instruction("OP_MOV_R1_E2", c, offset);
    case OP_MOV_R1_E1:
        return byte_instruction("OP_MOV_R1_E1", c, offset);
    case OP_MOV_R2_E1:
        return byte_instruction("OP_MOV_R2_E1", c, offset);
    case OP_MOV_R2_E2:
        return byte_instruction("OP_MOV_R2_E2", c, offset);
    case OP_MOV_E2_E1:
        return byte_instruction("OP_MOV_E2_E1", c, offset);
    case OP_MOV_E2_E3:
        return byte_instruction("OP_MOV_E2_E3", c, offset);
    case OP_MOV_E3_E2:
        return byte_instruction("OP_MOV_E3_E2", c, offset);
    case OP_MOV_E4_E2:
        return byte_instruction("OP_MOV_E4_E2", c, offset);
    case OP_ZERO_E1:
        return byte_instruction("OP_ZERO_E1", c, offset);

    case OP_ZERO_E2:
        return byte_instruction("OP_ZERO_E2", c, offset);
    case OP_ZERO_EL_REGISTERS:
        return byte_instruction("OP_ZERO_EL_REGISTERS", c, offset);
    case OP_MOV_E1_E2:
        return byte_instruction("OP_MOV_E1_E2", c, offset);
    case OP_MOV_E1_E3:
        return byte_instruction("OP_MOV_E1_E3", c, offset);
    case OP_MOV_E1:
        return byte_instruction("OP_MOV_E1", c, offset);
    case OP_MOV_E2:
        return byte_instruction("OP_MOV_E2", c, offset);
    case OP_MOV_E3:
        return byte_instruction("OP_MOV_E3", c, offset);

    case OP_MOV_CNT_E1:
        return byte_instruction("OP_MOV_CNT_E1", c, offset);
    case OP_MOV_CNT_E2:
        return byte_instruction("OP_MOV_CNT_E2", c, offset);
    case OP_MOV_CNT_E3:
        return byte_instruction("OP_MOV_CNT_E3", c, offset);
    case OP_MOV_PEEK_E1:
        return byte_instruction("OP_MOV_PEEK_E1", c, offset);
    case OP_MOV_PEEK_E2:
        return byte_instruction("OP_MOV_PEEK_E2", c, offset);
    case OP_REVERSE_GLOB_ARRAY:
        return simple_instruction("OP_REVERSE_GLOB_ARRAY", offset);
    case OP_REVERSE_LOCAL_ARRAY:
        return simple_instruction("OP_REVERSE_LOCAL_ARRAY", offset);

    case OP_SORT_GLOB_ARRAY:
        return simple_instruction("OP_SORT_GLOB_ARRAY", offset);
    case OP_SORT_LOCAL_ARRAY:
        return simple_instruction("OP_SORT_LOCAL_ARRAY", offset);

    case OP_BIN_SEARCH_GLOB_ARRAY:
        return simple_instruction("OP_BIN_SEARCH_GLOB_ARRAY", offset);
    case OP_BIN_SEARCH_LOCAL_ARRAY:
        return simple_instruction("OP_BIN_SEARCH_LOCAL_ARRAY", offset);

    case OP_JMP_GLOB_NOT_NIL:
        return simple_instruction("OP_JMP_GLOB_NOT_NIL", offset);
    case OP_JMP_LOCAL_NOT_NIL:
        return simple_instruction("OP_JMP_LOCAL_NOT_NIL", offset);

    default:
        printf("Unkown opcode: %d\n", offset);
        return ++offset;
    }
}
