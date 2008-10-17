#include "yog/yog.h"

static void 
gc_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogFunc* f = ptr;
    YOGBASICOBJ(f)->klass = do_gc(env, YOGBASICOBJ(f)->klass);
    f->code = do_gc(env, f->code);
}

YogFunc* 
YogFunc_new(YogEnv* env)
{
    YogFunc* func = ALLOC_OBJ(env, gc_children, YogFunc);
    YOGBASICOBJ(func)->klass = ENV_VM(env)->func_klass;
    func->code = NULL;

    return func;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
