#include "yog/yog.h"

YogFunc* 
YogFunc_new(YogEnv* env)
{
    YogFunc* func = ALLOC_OBJ(env, GCOBJ_FUNC, YogFunc);
    YOGBASICOBJ(func)->klass = ENV_VM(env)->func_klass;
    func->code = NULL;

    return func;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
