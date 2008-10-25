#include <setjmp.h>
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
    YogVal attr = YogObj_get_attr(env, YOGVAL_PTR(pkg), command);
    Yog_assert(env, YOGVAL_TYPE(attr) == VAL_FUNC, "Command isn't a function.");
    (YOGVAL_FUNC(attr))(env, pkg, argc, args);
}

void 
YogThread_eval_code(YogEnv* env, YogThread* th, YogCode* code) 
{
#if 0
    YogCode_dump(env, code);
#endif

    YogFrame* frame = YogFrame_new(env);
    PKG_VARS(frame) = YogTable_new_symbol_table(env);
    frame->stack = YogValArray_new(env, code->stack_size);

#define POP_BUF()   th->jmp_buf_list = th->jmp_buf_list->prev
    volatile pc_t pc = 0;
    YogJmpBuf jmpbuf;
    int status = 0;
    if ((status = setjmp(jmpbuf.buf)) == 0) {
        jmpbuf.prev = th->jmp_buf_list;
        th->jmp_buf_list = &jmpbuf;
    }
    else {
        unsigned int i = 0;
        for (i = 0; i < code->exc_tbl_size; i++) {
            YogExcTblEntry* entry = &code->exc_tbl->items[i];
            BOOL found = FALSE;
            if ((entry->from <= pc) && (pc < entry->to)) {
                pc = entry->target;
                found = TRUE;
                break;
            }
            if (!found) {
                POP_BUF();
                Yog_assert(env, th->jmp_buf_list != NULL, "No more jmp_buf.");
                longjmp(th->jmp_buf_list->buf, status);
            }
        }
    }

    while (pc < code->insts->size) {
#if 0
        printf("%s:%d pc=%d\n", __FILE__, __LINE__, pc);
#endif
#define CODE            (code)
#define PC              (pc)
#define STACK           (frame->stack)
#define POP()           (YogValArray_pop(env, STACK))
#define PUSH(val)       (YogValArray_push(env, STACK, val))
#define CONSTS(index)   (YogValArray_at(env, code->consts, index))
#define ENV             (env)
#define FRAME           (frame)
#define JUMP(m)         \
    PC = m; \
    continue
#if 0
        if (0 < STACK->size) {
            YogVal_print(env, STACK->items[STACK->size - 1]);
        }
        else {
            printf("stack is empty.\n");
        }
#endif

        OpCode op = code->insts->items[PC];
        unsigned int n = PC + sizeof(uint8_t);
        switch (op) {
#include "src/thread.inc"
        default:
            Yog_assert(env, FALSE, "Unknown instruction.");
            break;
        }
#undef JUMP
#undef FRAME
#undef ENV
#undef CONSTS
#undef PUSH
#undef POP
#undef STACK
#undef PC
#undef CODE

        pc = n;
    }

    POP_BUF();
#undef POP_BUF
}

static void 
gc_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogThread* th = ptr;
    th->cur_frame = do_gc(env, th->cur_frame);
}

YogThread*
YogThread_new(YogEnv* env) 
{
    YogThread* th = ALLOC_OBJ(env, gc_children, YogThread);
    th->cur_frame = NULL;
    th->jmp_buf_list = NULL;
    th->jmp_val = YogVal_undef();

    return th;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
