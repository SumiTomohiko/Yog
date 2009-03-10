#include <stdio.h>
#include "yog/arg.h"
#include "yog/binary.h"
#include "yog/code.h"
#include "yog/error.h"
#include "yog/inst.h"
#include "yog/opcodes.h"
#include "yog/yog.h"

#include "src/code.inc"

static void 
print_val(YogEnv* env, YogVal val) 
{
    if (IS_UNDEF(val)) {
        printf("undef");
    }
    else if (IS_PTR(val)) {
        printf("%p", VAL2PTR(val));
    }
    else if (IS_OBJ(val)) {
        printf("%p", VAL2OBJ(val));
    }
    else if (IS_INT(val)) {
        printf("%d", VAL2INT(val));
    }
    else if (IS_FLOAT(val)) {
        printf("%f", VAL2FLOAT(val));
    }
    else if (IS_BOOL(val)) {
        if (VAL2BOOL(val)) {
            printf("true");
        }
        else {
            printf("false");
        }
    }
    else if (IS_NIL(val)) {
        printf("nil");
    }
    else if (IS_SYMBOL(val)) {
        printf(" :%s", YogVm_id2name(env, ENV_VM(env), VAL2ID(val)));
    }
    else {
        YOG_ASSERT(env, FALSE, "Unknown value type.");
    }
}

void 
YogCode_dump(YogEnv* env, YogVal code)
{
    printf("=== Constants ===\n");
    printf("index value\n");

    unsigned int consts_size = 0;
    if (PTR_AS(YogCode, code)->consts != NULL) {
        consts_size = PTR_AS(YogCode, code)->consts->size;
    }
    unsigned int i = 0;
    for (i = 0; i < consts_size; i++) {
        printf("%05d ", i);

        YogVal val = PTR_AS(YogCode, code)->consts->items[i];
        print_val(env, val);

        printf("\n");
    }

    printf("=== Exception Table ===\n");
    printf("From To Target\n");

    unsigned int exc_tbl_size = PTR_AS(YogCode, code)->exc_tbl_size;
    for (i = 0; i < exc_tbl_size; i++) {
        YogExceptionTableEntry* entry = &PTR_AS(YogExceptionTable, PTR_AS(YogCode, code)->exc_tbl)->items[i];
        printf("%04d %04d %04d\n", entry->from, entry->to, entry->target);
    }

    printf("=== Code ===\n");
    printf("PC Lineno Instruction\n");

    pc_t pc = 0;
    while (pc < PTR_AS(YogCode, code)->insts->size) {
        printf("%04d", pc);

        unsigned int size = PTR_AS(YogCode, code)->lineno_tbl_size;
        for (i = 0; i < size; i++) {
            YogLinenoTableEntry* entry = &PTR_AS(YogCode, code)->lineno_tbl[i];
            if ((entry->pc_from <= pc) && (pc <= entry->pc_to)) {
                printf(" %05d", entry->lineno);
                break;
            }
        }
        if (i == size) {
            printf("      ");
        }

        OpCode op = PTR_AS(YogCode, code)->insts->items[pc];
        printf(" %s", YogCode_get_op_name(op));

        unsigned int n = pc + sizeof(uint8_t);
#define OPERAND(type, offset) \
        (*((type*)&PTR_AS(YogCode, code)->insts->items[n + (offset)]))
        switch (op) {
        case OP(LOAD_LOCAL):
            {
                uint8_t index = OPERAND(uint8_t, 0);
                printf(" %d", index);
            }
            break;
        case OP(STORE_NAME):
        case OP(LOAD_NAME):
            {
                ID id = OPERAND(ID, 0);
                printf(" %d", id);
            }
            break;
        case OP(PUSH_CONST):
            {
                uint8_t index = OPERAND(uint8_t, 0);
                printf(" %d (", index);
                print_val(env, PTR_AS(YogCode, code)->consts->items[index]);
                printf(")");
            }
            break;
        case OP(CALL_COMMAND):
        case OP(CALL_METHOD):
            {
                unsigned int offset = 0;
                ID id = OPERAND(ID, offset);
                printf(" :%s", YogVm_id2name(env, ENV_VM(env), id));
                offset += sizeof(ID);
                uint8_t argc = OPERAND(uint8_t, offset);
                printf(" %d", argc);
                offset += sizeof(uint8_t);
                uint8_t kwargc = OPERAND(uint8_t, offset);
                printf(" %d", kwargc);
                offset += sizeof(uint8_t);
                uint8_t blockargc = OPERAND(uint8_t, offset);
                printf(" %d", blockargc);
                offset += sizeof(uint8_t);
                uint8_t varargc = OPERAND(uint8_t, offset);
                printf(" %d", varargc);
                offset += sizeof(uint8_t);
                uint8_t varkwargc = OPERAND(uint8_t, offset);
                printf(" %d", varkwargc);
            }
            break;
        case OP(JUMP_IF_FALSE):
        case OP(JUMP):
            {
                unsigned int to = OPERAND(unsigned int, 0);
                printf(" %d", to);
            }
            break;
        default:
            break;
        }
#undef OPERAND

        printf("\n");
        pc += Yog_get_inst_size(op);
    }
}

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogCode* code = ptr;

    code->arg_info = YogVal_keep(env, code->arg_info, keeper);

#define KEEP_MEMBER(member)     do { \
    code->member = (*keeper)(env, (void*)code->member); \
} while (0)
    KEEP_MEMBER(local_vars_names);
    KEEP_MEMBER(consts);
    KEEP_MEMBER(insts);
    KEEP_MEMBER(lineno_tbl);
    KEEP_MEMBER(filename);
#undef KEEP_MEMBER

    code->exc_tbl = YogVal_keep(env, code->exc_tbl, keeper);
}

YogVal 
YogCode_new(YogEnv* env) 
{
    YogVal code = YUNDEF;
    PUSH_LOCAL(env, code);

    code = PTR2VAL(ALLOC_OBJ(env, keep_children, NULL, YogCode));
    CODE(code)->arg_info = YUNDEF;
    CODE(code)->stack_size = 0;
    CODE(code)->local_vars_count = 0;
    CODE(code)->local_vars_names = NULL;
    CODE(code)->consts = NULL;
    CODE(code)->insts = NULL;
    CODE(code)->outer_size = 0;
    CODE(code)->exc_tbl_size = 0;
    CODE(code)->exc_tbl = YUNDEF;
    CODE(code)->lineno_tbl_size = 0;
    CODE(code)->lineno_tbl = NULL;
    CODE(code)->filename = NULL;
    CODE(code)->klass_name = INVALID_ID;
    CODE(code)->func_name = INVALID_ID;

    YogVal arg_info = YogArgInfo_new(env);
    CODE(code)->arg_info = arg_info;

    POP_LOCALS(env);
    return code;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
