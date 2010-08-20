#include <stdarg.h>
#include "yog/callable.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/package.h"
#include "yog/table.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

void
YogPackage_define_function2(YogEnv* env, YogHandle* pkg, const char* name, void* f, ...)
{
    YogHandle* h_class_name = YogHandle_REGISTER(env, YNIL);
    YogVM* vm = env->vm;
    ID id_func_name = YogVM_intern(env, vm, name);
    YogVal func_name = YogVM_id2name(env, vm, id_func_name);
    YogHandle* h_func_name = YogHandle_REGISTER(env, func_name);
    va_list ap;
    va_start(ap, f);
    YogHandle* func = YogNativeFunction2_new(env, pkg, h_class_name, h_func_name, f, ap);
    va_end(ap);
    YogObj_set_attr(env, HDL2VAL(pkg), name, HDL2VAL(func));
}

void
YogPackage_define_function(YogEnv* env, YogVal pkg, const char* name, YogAPI f)
{
    SAVE_ARG(env, pkg);

    YogVal func = YUNDEF;
    PUSH_LOCAL(env, func);

    func = YogNativeFunction_new(env, INVALID_ID, pkg, name, f);
    YogObj_set_attr(env, pkg, name, func);

    RETURN_VOID(env);
}

void
YogPackage_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogObj_keep_children(env, ptr, keeper, heap);
}

void
YogPackage_init(YogEnv* env, YogVal pkg, type_t type)
{
    SAVE_ARG(env, pkg);
    YogVal attrs = YUNDEF;
    PUSH_LOCAL(env, attrs);

    YogObj_init(env, pkg, type, FLAG_PKG, env->vm->cPackage);
    attrs = YogTable_new_symbol_table(env);
    YogGC_UPDATE_PTR(env, PTR_AS(YogObj, pkg), attrs, attrs);

    RETURN_VOID(env);
}

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    pkg = ALLOC_OBJ(env, YogPackage_keep_children, NULL, YogPackage);
    YogPackage_init(env, pkg, TYPE_PACKAGE);

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
        YogScriptFrame_push_stack(env, env->frame, attr);
        RETURN_VOID(env);
    }

    klass = YogVal_get_class(env, self);
    attr = YogClass_get_attr(env, klass, name);
    if (!IS_UNDEF(attr)) {
        attr = YogVal_get_descr(env, attr, self, klass);
        YogScriptFrame_push_stack(env, env->frame, attr);
        RETURN_VOID(env);
    }

    YogError_raise_AttributeError(env, "%C object has no attribute \"%I\"", self, name);

    /* NOTREACHED */
    RETURN_VOID(env);
}

void
YogPackage_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cPackage = YUNDEF;
    PUSH_LOCAL(env, cPackage);
    YogVM* vm = env->vm;

    cPackage = YogClass_new(env, "Package", vm->cObject);
    YogClass_define_allocator(env, cPackage, alloc);
    YogClass_define_get_attr_caller(env, cPackage, call_get_attr);
    YogClass_define_get_attr_executor(env, cPackage, exec_get_attr);
    vm->cPackage = cPackage;

    RETURN_VOID(env);
}

YogVal
YogPackage_new(YogEnv* env)
{
    return alloc(env, env->vm->cPackage);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
