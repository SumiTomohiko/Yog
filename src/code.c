#include <stdio.h>
#include "yog/arg.h"
#include "yog/array.h"
#include "yog/binary.h"
#include "yog/class.h"
#include "yog/code.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/inst.h"
#include "yog/object.h"
#include "yog/opcodes.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

#include "code.inc"

static void
print_val(YogEnv* env, YogVal val)
{
    if (IS_UNDEF(val)) {
        printf("undef");
    }
    else if (IS_PTR(val)) {
        printf("%p", VAL2PTR(val));
    }
    else if (IS_FIXNUM(val)) {
        printf("%zd", VAL2INT(val));
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
        printf(" :%s", BINARY_CSTR(YogVM_id2bin(env, env->vm, VAL2ID(val))));
    }
    else {
        YOG_BUG(env, "Unknown value type.");
    }
}

BOOL
YogCode_get_lineno(YogEnv* env, YogVal self, uint_t pc, uint_t* lineno)
{
    YOG_ASSERT(env, lineno != NULL, "lineno is NULL");
    SAVE_ARG(env, self);
    YogVal lineno_tbl = YUNDEF;
    PUSH_LOCAL(env, lineno_tbl);

    uint_t size = PTR_AS(YogCode, self)->lineno_tbl_size;
    uint_t i;
    for (i = 0; i < size; i++) {
        lineno_tbl = PTR_AS(YogCode, self)->lineno_tbl;
        YogLinenoTableEntry* ent = &PTR_AS(YogLinenoTableEntry, lineno_tbl)[i];
        if ((ent->pc_from <= pc) && (pc <= ent->pc_to)) {
            *lineno = ent->lineno;
            RETURN(env, TRUE);
        }
    }

    RETURN(env, FALSE);
}

static void
YogCode_dump(YogEnv* env, YogVal code)
{
    SAVE_ARG(env, code);
    YogVal insts = YUNDEF;
    YogVal consts = YUNDEF;
    PUSH_LOCALS2(env, insts, consts);

    printf("stack size: %zu\n", PTR_AS(YogCode, code)->stack_size);
    printf("outer size: %zu\n", PTR_AS(YogCode, code)->outer_size);
    printf("=== Constants ===\n");
    printf("index value\n");

    uint_t consts_size = 0;
    consts = PTR_AS(YogCode, code)->consts;
    if (IS_PTR(consts)) {
        consts_size = PTR_AS(YogValArray, consts)->size;
    }
    uint_t i;
    for (i = 0; i < consts_size; i++) {
        printf("%05zd ", i);

        YogVal val = PTR_AS(YogValArray, consts)->items[i];
        print_val(env, val);

        printf("\n");
    }

    printf("=== Exception Table ===\n");
    printf("From To Target\n");

    uint_t exc_tbl_size = PTR_AS(YogCode, code)->exc_tbl_size;
    for (i = 0; i < exc_tbl_size; i++) {
        YogExceptionTableEntry* entry = &PTR_AS(YogExceptionTable, PTR_AS(YogCode, code)->exc_tbl)->items[i];
        printf("%04zd %04zd %04zd\n", entry->from, entry->to, entry->target);
    }

    printf("=== Code ===\n");
    printf("PC Lineno Instruction\n");

    pc_t pc = 0;
    insts = PTR_AS(YogCode, code)->insts;
    while (pc < PTR_AS(YogByteArray, insts)->size) {
        printf("%04zd", pc);

        uint_t lineno;
        if (YogCode_get_lineno(env, code, pc, &lineno)) {
            printf(" %05zd", lineno);
        }
        else {
            printf("      ");
        }

        OpCode op = (OpCode)PTR_AS(YogByteArray, insts)->items[pc];
        printf(" %s", YogCode_get_op_name(op));

        uint_t n = pc + sizeof(uint8_t);
#define OPERAND(type, offset) \
        (*((type*)&PTR_AS(YogByteArray, insts)->items[n + (offset)]))
        switch (op) {
        case OP(LOAD_NONLOCAL_INDEX):
        case OP(STORE_NONLOCAL_INDEX):
            {
                uint8_t level = OPERAND(uint8_t, 0);
                uint8_t index = OPERAND(uint8_t, 1);
                printf(" %d %d", level, index);
            }
            break;
        case OP(LOAD_LOCAL_INDEX):
        case OP(STORE_LOCAL_INDEX):
            {
                uint8_t index = OPERAND(uint8_t, 0);
                printf(" %d", index);
            }
            break;
        case OP(STORE_NONLOCAL_NAME):
        case OP(LOAD_NONLOCAL_NAME):
            {
                uint8_t index = OPERAND(uint8_t, 0);
                ID id = OPERAND(ID, 1);
                YogVal bin = YogVM_id2bin(env, env->vm, id);
                printf(" %u %zd ('%s)", index, id, BINARY_CSTR(bin));
            }
            break;
        case OP(STORE_LOCAL_NAME):
        case OP(LOAD_LOCAL_NAME):
            {
                ID id = OPERAND(ID, 0);
                YogVal bin = YogVM_id2bin(env, env->vm, id);
                printf(" %zd ('%s)", id, BINARY_CSTR(bin));
            }
            break;
        case OP(PUSH_CONST):
            {
                uint8_t index = OPERAND(uint8_t, 0);
                printf(" %d (", index);
                YogVal consts = PTR_AS(YogCode, code)->consts;
                YogVal c = PTR_AS(YogValArray, consts)->items[index];
                print_val(env, c);
                printf(")");
            }
            break;
        case OP(CALL_FUNCTION):
            {
                uint_t offset = 0;
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
        case OP(LOAD_GLOBAL):
        case OP(LOAD_ATTR):
            {
                YogVal bin = YogVM_id2bin(env, env->vm, OPERAND(ID, 0));
                printf(" '%s", BINARY_CSTR(bin));
            }
            break;
        case OP(JUMP_IF_TRUE):
        case OP(JUMP_IF_FALSE):
        case OP(JUMP):
            {
                uint_t to = OPERAND(uint_t, 0);
                printf(" %zd", to);
            }
            break;
        default:
            break;
        }
#undef OPERAND

        printf("\n");
        pc += Yog_get_inst_size(op);
    }

    RETURN_VOID(env);
}

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogCode* code = PTR_AS(YogCode, ptr);

    /* FIXME: Why not to use KEEP? */
    YogGC_KEEP(env, code, arg_info, keeper, heap);

#define KEEP_MEMBER(member)     do { \
    code->member = PTR_AS(ID, (*keeper)(env, (void*)code->member, heap)); \
} while (0)
    KEEP_MEMBER(local_vars_names);
#undef KEEP_MEMBER

#define KEEP(member)    YogGC_KEEP(env, code, member, keeper, heap)
    KEEP(lineno_tbl);
    KEEP(insts);
    KEEP(consts);
    KEEP(exc_tbl);
    KEEP(filename);
#undef KEEP
}

static void
check_self_Code(YogEnv* env, YogHandle* self)
{
    YogVal val = HDL2VAL(self);
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_CODE)) {
        return;
    }
    YogError_raise_TypeError(env, "self must be Code, not %C", val);
}

static YogVal
dump(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    check_self_Code(env, self);
    YogCode_dump(env, HDL2VAL(self));
    return HDL2VAL(self);
}

void
YogCode_define_classes(YogEnv* env, YogHandle* pkg)
{
    YogVM* vm = env->vm;
    YogHandle* cCode = VAL2HDL(env, YogClass_new(env, "Code", vm->cObject));
#define DEFINE_METHOD(name, ...) do { \
    YogVal o = HDL2VAL(cCode); \
    YogClass_define_method2(env, o, HDL2VAL(pkg), (name), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD("dump", dump, NULL);
#undef DEFINE_METHOD
    vm->cCode = HDL2VAL(cCode);
}

YogVal
YogCode_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal code = YUNDEF;
    PUSH_LOCAL(env, code);

    code = ALLOC_OBJ(env, keep_children, NULL, YogCode);
    YogBasicObj_init(env, code, TYPE_CODE, 0, env->vm->cCode);
    CODE(code)->arg_info = YUNDEF;
    CODE(code)->stack_size = 0;
    CODE(code)->local_vars_count = 0;
    CODE(code)->local_vars_names = NULL;
    CODE(code)->consts = YUNDEF;
    CODE(code)->insts = YUNDEF;
    CODE(code)->outer_size = 0;
    CODE(code)->exc_tbl_size = 0;
    CODE(code)->exc_tbl = YUNDEF;
    CODE(code)->lineno_tbl_size = 0;
    CODE(code)->lineno_tbl = YUNDEF;
    CODE(code)->filename = YUNDEF;
    CODE(code)->class_name = INVALID_ID;
    CODE(code)->func_name = INVALID_ID;

    RETURN(env, code);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
