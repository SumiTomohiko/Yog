#include "yog/yog.h"

YogFunc* 
YogFunc_new(YogEnv* env) 
{
    YogFunc* f = ALLOC_OBJ(env, GCOBJ_FUNC, YogFunc);
    YOGBASICOBJ(f)->klass = ENV_VM(env)->func_klass;
    f->f = NULL;

    return f;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
