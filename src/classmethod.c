#include "yog/classmethod.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/thread.h"
#include "yog/yog.h"

YogVal
YogClassMethod_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "ClassMethod", env->vm->cClassMethod);

    RETURN(env, klass);
}

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogClassMethod* classmethod = ptr;
    YogGC_keep(env, &classmethod->f, keeper, heap);
}

YogVal
YogClassMethod_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal classmethod = YUNDEF;
    PUSH_LOCAL(env, classmethod);

    classmethod = ALLOC_OBJ(env, keep_children, NULL, YogClassMethod);
    YogBasicObj_init(env, classmethod, 0, env->vm->cClassMethod);
    PTR_AS(YogClassMethod, classmethod)->f = YUNDEF;

    RETURN(env, classmethod);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
