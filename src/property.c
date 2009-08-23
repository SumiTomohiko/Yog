#include "yog/error.h"
#include "yog/gc.h"
#include "yog/class.h"
#include "yog/property.h"
#include "yog/thread.h"
#include "yog/yog.h"

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogProperty* prop = ptr;
#define KEEP(member)    YogGC_keep(env, &prop->member, keeper, heap)
    KEEP(getter);
    KEEP(setter);
#undef KEEP
}

YogVal
YogProperty_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal prop = YUNDEF;
    PUSH_LOCAL(env, prop);

    prop = ALLOC_OBJ(env, keep_children, NULL, YogProperty);
    YogBasicObj_init(env, prop, 0, env->vm->cProperty);
    PTR_AS(YogProperty, prop)->getter = YUNDEF;
    PTR_AS(YogProperty, prop)->setter = YUNDEF;

    RETURN(env, prop);
}

static void
exec_get_descr(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal getter = YUNDEF;
    YogVal class_of_getter = YUNDEF;
    YogVal method = YUNDEF;
    YogVal class_of_method = YUNDEF;
    PUSH_LOCALS4(env, getter, class_of_getter, method, class_of_method);

    getter = PTR_AS(YogProperty, attr)->getter;
    class_of_getter = YogVal_get_class(env, getter);
    if (!IS_PTR(getter)) {
        ID id = PTR_AS(YogClass, class_of_getter)->name;
        const char* name = YogVM_id2name(env, env->vm, id);
        YogError_raise_TypeError(env, "'%s' object is not callable", name);
    }
    YOG_ASSERT(env, PTR_AS(YogClass, class_of_getter)->call_get_descr != NULL, "can't make instance method");
    method = PTR_AS(YogClass, class_of_getter)->call_get_descr(env, getter, obj, klass);
    YOG_ASSERT(env, IS_PTR(method), "method isn't pointer");
    class_of_method = YogVal_get_class(env, method);
    Executor exec = PTR_AS(YogClass, class_of_method)->exec;
    YOG_ASSERT(env, exec != NULL, "method isn't callable");
    exec(env, method, 0, NULL, YNIL, 0, NULL, YNIL, YNIL);

    RETURN_VOID(env);
}

static YogVal
call_get_descr(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal getter = YUNDEF;
    YogVal class_of_getter = YUNDEF;
    YogVal method = YUNDEF;
    YogVal class_of_method = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS5(env, getter, class_of_getter, method, class_of_method, retval);

    getter = PTR_AS(YogProperty, attr)->getter;
    class_of_getter = YogVal_get_class(env, getter);
    if (!IS_PTR(getter)) {
        ID id = PTR_AS(YogClass, class_of_getter)->name;
        const char* name = YogVM_id2name(env, env->vm, id);
        YogError_raise_TypeError(env, "'%s' object is not callable", name);
    }
    YOG_ASSERT(env, PTR_AS(YogClass, class_of_getter)->call_get_descr != NULL, "can't make instance method");
    method = PTR_AS(YogClass, class_of_getter)->call_get_descr(env, getter, obj, klass);
    YOG_ASSERT(env, IS_PTR(method), "method isn't pointer");
    class_of_method = YogVal_get_class(env, method);
    Caller call = PTR_AS(YogClass, class_of_method)->call;
    YOG_ASSERT(env, call != NULL, "method isn't callable");
    retval = call(env, method, 0, NULL, YNIL, 0, NULL, YNIL, YNIL);

    RETURN(env, retval);
}

static void
exec_set_descr(YogEnv* env, YogVal attr, YogVal obj, YogVal val)
{
    SAVE_ARGS3(env, attr, obj, val);
    YogVal setter = YUNDEF;
    YogVal class_of_setter = YUNDEF;
    YogVal method = YUNDEF;
    YogVal class_of_method = YUNDEF;
    YogVal class_of_obj = YUNDEF;
    PUSH_LOCALS5(env, setter, class_of_setter, method, class_of_method, class_of_obj);
    YogVal args[] = { val };
    PUSH_LOCALSX(env, array_sizeof(args), args);

    setter = PTR_AS(YogProperty, attr)->setter;
    class_of_setter = YogVal_get_class(env, setter);
    if (!IS_PTR(setter)) {
        ID id = PTR_AS(YogClass, class_of_setter)->name;
        const char* name = YogVM_id2name(env, env->vm, id);
        YogError_raise_TypeError(env, "'%s' object is not callable", name);
    }
    class_of_obj = YogVal_get_class(env, obj);
    YOG_ASSERT(env, PTR_AS(YogClass, class_of_setter)->call_get_descr != NULL, "can't make instance method");
    method = PTR_AS(YogClass, class_of_setter)->call_get_descr(env, setter, obj, class_of_obj);
    YOG_ASSERT(env, IS_PTR(method), "method isn't pointer");
    class_of_method = YogVal_get_class(env, method);
    Executor exec = PTR_AS(YogClass, class_of_method)->exec;
    YOG_ASSERT(env, exec != NULL, "method isn't callable");
    exec(env, method, array_sizeof(args), args, YNIL, 0, NULL, YNIL, YNIL);

    RETURN_VOID(env);
}

YogVal
YogProperty_define_class(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "Property", env->vm->cObject);
    YogClass_define_descr_get_executor(env, klass, exec_get_descr);
    YogClass_define_descr_get_caller(env, klass, call_get_descr);
    YogClass_define_descr_set_executor(env, klass, exec_set_descr);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
