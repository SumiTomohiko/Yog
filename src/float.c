#include "yog/env.h"
#include "yog/float.h"
#include "yog/klass.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal 
allocate(YogEnv* env, YogVal klass) 
{
    SAVE_ARG(env, klass);

    YogVal f = ALLOC_OBJ(env, NULL, NULL, YogFloat);
    YogBasicObj_init(env, f, 0, klass);
    PTR_AS(YogFloat, f)->val = 0;

    RETURN(env, f);
}

YogVal 
YogFloat_new(YogEnv* env) 
{
    YogVal f = allocate(env, env->vm->cFloat);
    PTR_AS(YogFloat, f)->val = 0;
    return f;
}

YogVal 
YogFloat_klass_new(YogEnv* env) 
{
    SAVE_LOCALS(env);

    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Float", env->vm->cObject);
    YogKlass_define_allocator(env, klass, allocate);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
