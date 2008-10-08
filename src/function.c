#include "yog/yog.h"

#if 0
YogFunc* 
YogFunc_new(YogEnv* env, YogFuncBody* f) 
{
    YogFunc* func = ALLOC_OBJ(env, GCOBJ_FUNC, YogFunc);
    YOGBASICOBJ(func)->klass = ENV_VM(env)->func_klass;
    func->body = f;

    return func;
}
#endif

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
