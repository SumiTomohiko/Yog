#include <stdio.h>
#include "yog/yog.h"

#include "src/code.inc"

static void 
print_val(YogEnv* env, YogVal val) 
{
    if (IS_UNDEF(val)) {
        printf("undef");
    }
    else if (IS_PTR(val)) {
        printf("%p", YOGVAL_PTR(val));
    }
    else if (IS_OBJ(val)) {
        printf("%p", YOGVAL_OBJ(val));
    }
    else if (IS_INT(val)) {
        printf("%d", YOGVAL_INT(val));
    }
    else if (IS_FLOAT(val)) {
        printf("%f", YOGVAL_FLOAT(val));
    }
    else if (IS_BOOL(val)) {
        if (YOGVAL_BOOL(val)) {
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
        printf(" :%s", YogVm_id2name(env, ENV_VM(env), YOGVAL_SYMBOL(val)));
    }
    else {
        Yog_assert(env, FALSE, "Unknown value type.");
    }
}

void 
YogCode_dump(YogEnv* env, YogCode* code) 
{
    printf("=== Constants ===\n");
    printf("index value\n");

    unsigned int consts_size = 0;
    if (code->consts != NULL) {
        consts_size = code->consts->size;
    }
    unsigned int i = 0;
    for (i = 0; i < consts_size; i++) {
        printf("%05d ", i);

        YogVal val = code->consts->items[i];
        print_val(env, val);

        printf("\n");
    }

    printf("=== Exception Table ===\n");
    printf("From To Target\n");

    unsigned int exc_tbl_size = code->exc_tbl_size;
    for (i = 0; i < exc_tbl_size; i++) {
        YogExcTblEntry* entry = &code->exc_tbl->items[i];
        printf("%04d %04d %04d\n", entry->from, entry->to, entry->target);
    }

    printf("=== Code ===\n");
    printf("PC Lineno Instruction\n");

    pc_t pc = 0;
    while (pc < code->insts->size) {
        printf("%04d", pc);

        unsigned int size = code->lineno_tbl_size;
        for (i = 0; i < size; i++) {
            LinenoTableEntry* entry = &code->lineno_tbl[i];
            if ((entry->pc_from <= pc) && (pc <= entry->pc_to)) {
                printf(" %05d", entry->lineno);
                break;
            }
        }
        if (i == size) {
            printf("      ");
        }

        OpCode op = code->insts->items[pc];
        printf(" %s", get_op_name(op));

        unsigned int n = pc + sizeof(uint8_t);
#define OPERAND(type)   (*((type*)&code->insts->items[n]))
        switch (op) {
            case OP(STORE_PKG):
            case OP(LOAD_PKG):
                {
                    ID id = OPERAND(ID);
                    printf(" %d", id);
                }
                break;
            case OP(PUSH_CONST):
                {
                    uint8_t index = OPERAND(uint8_t);
                    printf(" %d (", index);
                    print_val(env, code->consts->items[index]);
                    printf(")");
                }
                break;
            case OP(CALL_COMMAND):
            case OP(CALL_METHOD):
                {
                    ID id = OPERAND(ID);
                    printf(" :%s", YogVm_id2name(env, ENV_VM(env), id));
                }
                break;
            case OP(JUMP_IF_FALSE):
            case OP(JUMP):
                {
                    unsigned int to = OPERAND(unsigned int);
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
gc_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogCode* code = ptr;
    code->consts = do_gc(env, code->consts);
    code->insts = do_gc(env, code->insts);
    code->exc_tbl = do_gc(env, code->exc_tbl);
}

YogCode* 
YogCode_new(YogEnv* env) 
{
    YogCode* code = ALLOC_OBJ(env, gc_children, YogCode);
    YogArgInfo* arg_info = &code->arg_info;
    arg_info->argc = 0;
    arg_info->argnames = NULL;
    arg_info->blockargc = 0;
    arg_info->blockargname = 0;
    arg_info->varargc = 0;
    arg_info->kwargc = 0;

    code->stack_size = 0;
    code->local_vars_count = 0;
    code->consts = NULL;
    code->insts = NULL;
    code->exc_tbl_size = 0;
    code->exc_tbl = NULL;
    code->lineno_tbl_size = 0;
    code->lineno_tbl = NULL;

    return code;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
