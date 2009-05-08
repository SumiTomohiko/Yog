#include <stdarg.h>
#include "yog/env.h"
#include "yog/function.h"
#include "yog/gc.h"
#include "yog/method.h"
#include "yog/package.h"
#include "yog/st.h"
#include "yog/thread.h"
#include "yog/yog.h"

void 
YogPackage_define_method(YogEnv* env, YogVal pkg, const char* name, void* f, unsigned int blockargc, unsigned int varargc, unsigned int kwargc, unsigned int required_argc, ...)
{
    SAVE_ARG(env, pkg);

    ID func_name = INTERN(name);

    va_list ap;
    va_start(ap, required_argc);
    YogVal builtin_f = YogBuiltinFunction_new(env, f, INVALID_ID, func_name, blockargc, varargc, kwargc, required_argc, ap);
    va_end(ap);
    PUSH_LOCAL(env, builtin_f);

    YogVal method = YogBuiltinBoundMethod_new(env);
    MODIFY(env, PTR_AS(YogBuiltinBoundMethod, method)->self, pkg);
    MODIFY(env, PTR_AS(YogBuiltinBoundMethod, method)->f, builtin_f);
    PUSH_LOCAL(env, method);

    YogObj_set_attr_id(env, pkg, func_name, method);

    RETURN_VOID(env);
}

static void 
YogPackage_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogObj_keep_children(env, ptr, keeper);

    YogPackage* pkg = ptr;
    pkg->code = YogVal_keep(env, pkg->code, keeper);
}

static void 
YogPackage_init(YogEnv* env, YogVal pkg) 
{
    SAVE_ARG(env, pkg);

    YogObj_init(env, pkg, 0, env->vm->cPackage);
    PTR_AS(YogObj, pkg)->attrs = YUNDEF;
    PTR_AS(YogPackage, pkg)->code = YUNDEF;

    YogVal attrs = YogTable_new_symbol_table(env);
    MODIFY(env, PTR_AS(YogObj, pkg)->attrs, attrs);

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

YogVal 
YogPackage_klass_new(YogEnv* env) 
{
    YogVal klass = YogKlass_new(env, "Package", env->vm->cObject);
    PUSH_LOCAL(env, klass);

    YogKlass_define_allocator(env, klass, allocate);

    POP_LOCALS(env);
    return klass;
}

YogVal 
YogPackage_new(YogEnv* env) 
{
    YogVal pkg = allocate(env, env->vm->cPackage);
    PUSH_LOCAL(env, pkg);

    YogPackage_init(env, pkg);

    POP_LOCALS(env);
    return pkg;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
