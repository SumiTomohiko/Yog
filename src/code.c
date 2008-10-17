#include "yog/yog.h"

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
    code->argc = 0;
    code->stack_size = 0;
    code->local_vars_count = 0;
    code->consts = NULL;
    code->insts = NULL;
    code->exc_tbl_size = 0;
    code->exc_tbl = NULL;

    return code;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
