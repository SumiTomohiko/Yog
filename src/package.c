#include <stdarg.h>
#include "yog/env.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/gc.h"
#include "yog/package.h"
#include "yog/table.h"
#include "yog/thread.h"
#include "yog/yog.h"

void
YogPackage_define_function(YogEnv* env, YogVal pkg, const char* name, void* f)
{
    SAVE_ARG(env, pkg);

    YogVal func = YUNDEF;
    PUSH_LOCAL(env, func);

    func = YogNativeFunction_new(env, INVALID_ID, name, f);
    YogObj_set_attr(env, pkg, name, func);

    RETURN_VOID(env);
}

static void
YogPackage_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogObj_keep_children(env, ptr, keeper, heap);
}

static void
YogPackage_init(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);

    YogObj_init(env, pkg, 0, env->vm->cPackage);
    PTR_AS(YogObj, pkg)->attrs = YUNDEF;

    YogVal attrs = YogTable_new_symbol_table(env);
    PTR_AS(YogObj, pkg)->attrs = attrs;

    RETURN_VOID(env);
}

static YogVal
allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal pkg = ALLOC_OBJ(env, YogPackage_keep_children, NULL, YogPackage);
    PUSH_LOCAL(env, pkg);
    YogPackage_init(env, pkg);

    RETURN(env, pkg);
}

static YogVal
call_get_attr(YogEnv* env, YogVal self, ID name)
{
    SAVE_ARG(env, self);
    YogVal klass = YUNDEF;
    YogVal attr = YUNDEF;
    PUSH_LOCALS2(env, klass, attr);

    attr = YogObj_get_attr(env, self, name);
    if (!IS_UNDEF(attr)) {
        RETURN(env, attr);
    }

    klass = YogVal_get_class(env, self);
    attr = YogClass_get_attr(env, klass, name);
    if (!IS_UNDEF(attr)) {
        attr = YogVal_get_descr(env, attr, self, klass);
        RETURN(env, attr);
    }

    RETURN(env, YUNDEF);
}

static void
exec_get_attr(YogEnv* env, YogVal self, ID name)
{
    SAVE_ARG(env, self);
    YogVal klass = YUNDEF;
    YogVal attr = YUNDEF;
    PUSH_LOCALS2(env, klass, attr);

    attr = YogObj_get_attr(env, self, name);
    if (!IS_UNDEF(attr)) {
        FRAME_PUSH(env, attr);
        RETURN_VOID(env);
    }

    klass = YogVal_get_class(env, self);
    attr = YogClass_get_attr(env, klass, name);
    if (!IS_UNDEF(attr)) {
        attr = YogVal_get_descr(env, attr, self, klass);
        FRAME_PUSH(env, attr);
        RETURN_VOID(env);
    }

    YOG_BUG(env, "attribute not found");

    /* NOTREACHED */
}

YogVal
YogPackage_define_class(YogEnv* env)
{
    YogVal klass = YogClass_new(env, "Package", env->vm->cObject);
    PUSH_LOCAL(env, klass);

    YogClass_define_allocator(env, klass, allocate);
    YogClass_define_get_attr_caller(env, klass, call_get_attr);
    YogClass_define_get_attr_executor(env, klass, exec_get_attr);

    POP_LOCALS(env);
    return klass;
}

YogVal
YogPackage_new(YogEnv* env)
{
    return allocate(env, env->vm->cPackage);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
