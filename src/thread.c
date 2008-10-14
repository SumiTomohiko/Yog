#include <stdio.h>
#include "yog/opcodes.h"
#include "yog/yog.h"

YogVal 
YogThread_call_method(YogEnv* env, YogVal receiver, const char* method, unsigned int argc, YogVal* args) 
{
    ID id = YogVm_intern(env, ENV_VM(env), method);
    return YogThread_call_method_id(env, receiver, id, argc, args);
}

YogVal 
YogThread_call_method_id(YogEnv* env, YogVal receiver, ID method, unsigned int argc, YogVal* args) 
{
    YogKlass* klass = YogVal_get_klass(env, receiver);
    YogVal attr = YogObj_get_attr(env, YOGOBJ(klass), method);
    Yog_assert(env, YOGVAL_TYPE(attr) == VAL_FUNC, "Attribute isn't a function.");
    YogVal ret = (YOGVAL_FUNC(attr))(env, receiver, argc, args);
    return ret;
}

void 
YogThread_call_command(YogEnv* env, ID command, unsigned int argc, YogVal* args)
{
    YogVm* vm = ENV_VM(env);
    ID bltins = YogVm_intern(env, vm, "builtins");
    YogVal pkg = YogVal_undef();
    if (!YogTable_lookup(env, vm->pkgs, YogVal_symbol(bltins), &pkg)) {
        Yog_assert(env, FALSE, "Can't find builtins package.");
    }
    YogVal attr = YogObj_get_attr(env, YOGOBJ(YOGVAL_GCOBJ(pkg)), command);
    Yog_assert(env, YOGVAL_TYPE(attr) == VAL_FUNC, "Command isn't a function.");
    (YOGVAL_FUNC(attr))(env, pkg, argc, args);
}

void 
YogThread_eval_code(YogEnv* env, YogThread* th, YogCode* code) 
{
    YogFrame* frame = YogFrame_new(env);
    PKG_VARS(frame) = YogTable_new_symbol_table(env);
    frame->stack = YogValArray_new(env, code->stack_size);

    pc_t pc = 0;
    while (pc < code->insts->size) {
#define CODE            (code)
#define PC              (pc)
#define STACK           (frame->stack)
#define POP()           (YogValArray_pop(env, STACK))
#define PUSH(val)       (YogValArray_push(env, STACK, val))
#define CONSTS(index)   (YogValArray_at(env, code->consts, index))
#define ENV             (env)
#define FRAME           (frame)
#if 0
        if (0 < STACK->size) {
            YogVal_print(env, STACK->items[STACK->size - 1]);
        }
        else {
            printf("stack is empty.\n");
        }
#endif

        OpCode op = code->insts->items[PC];
        PC += sizeof(uint8_t);
        switch (op) {
#include "src/thread.inc"
        default:
            Yog_assert(env, FALSE, "Unknown instruction.");
            break;
        }
#undef FRAME
#undef ENV
#undef CONSTS
#undef PUSH
#undef POP
#undef STACK
#undef PC
#undef CODE
    }
}

YogThread*
YogThread_new(YogEnv* env) 
{
    YogThread* th = ALLOC_OBJ(env, GCOBJ_THREAD, YogThread);
    th->cur_frame = NULL;

    return th;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
