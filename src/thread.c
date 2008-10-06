#include "yog/opcodes.h"
#include "yog/yog.h"

void 
YogThread_eval_code(YogEnv* env, YogThread* th, YogCode* code) 
{
    YogFrame* frame = YogFrame_new(env);
    PKG_VARS(frame) = YogTable_new_symbol_table(env);
    frame->stack = YogValArray_new(env, code->stack_size);

    unsigned int pc = 0;
    while (pc < code->insts->size) {
#define CODE        (code)
#define PC          (pc)
#define STACK       (frame->stack)
#define POP()       (YogValArray_pop(env, STACK))
#define PUSH(val)   (YogValArray_push(env, STACK, val))
        switch (code->insts->items[PC]) {
#include "src/thread.inc"
        default:
            Yog_assert(env, FALSE, "Unknown instruction.");
            break;
        }
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
