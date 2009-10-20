#include "yog/classmethod.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/class.h"
#include "yog/object.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal
call_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal method = YUNDEF;
    YogVal f = YUNDEF;
    YogVal class_of_f = YUNDEF;
    YogVal class_of_class = YUNDEF;
    PUSH_LOCALS4(env, method, f, class_of_f, class_of_class);

    f = PTR_AS(YogClassMethod, attr)->f;
    class_of_f = YogVal_get_class(env, f);
    YOG_ASSERT(env, PTR_AS(YogClass, class_of_f)->call_get_descr != NULL, "f is not callable");
    class_of_class = YogVal_get_class(env, klass);
    method = PTR_AS(YogClass, class_of_f)->call_get_descr(env, f, klass, class_of_class);

    RETURN(env, method);
}

static void
exec_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal method = YUNDEF;
    PUSH_LOCAL(env, method);

    method = call_descr_get(env, attr, obj, klass);
    FRAME_PUSH(env, method);

    RETURN_VOID(env);
}

YogVal
YogClassMethod_define_class(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "ClassMethod", env->vm->cClassMethod);
    YogClass_define_descr_get_executor(env, klass, exec_descr_get);
    YogClass_define_descr_get_caller(env, klass, call_descr_get);

    RETURN(env, klass);
}

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogClassMethod* classmethod = PTR_AS(YogClassMethod, ptr);
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
