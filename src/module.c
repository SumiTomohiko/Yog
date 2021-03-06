#include "yog/callable.h"
#include "yog/class.h"
#include "yog/gc.h"
#include "yog/module.h"
#include "yog/object.h"
#include "yog/table.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal module = YUNDEF;
    PUSH_LOCAL(env, module);

    module = ALLOC_OBJ(env, YogObj_keep_children, NULL, YogModule);
    YogObj_init(env, module, TYPE_MODULE, 0, klass);
    PTR_AS(YogModule, module)->name = INVALID_ID;

    RETURN(env, module);
}

YogVal
YogModule_new(YogEnv* env, ID name)
{
    SAVE_LOCALS(env);
    YogVal module = YUNDEF;
    YogVal attrs = YUNDEF;
    PUSH_LOCALS2(env, module, attrs);

    module = alloc(env, env->vm->cModule);
    PTR_AS(YogModule, module)->name = name;

    attrs = YogTable_create_symbol_table(env);
    YogGC_UPDATE_PTR(env, PTR_AS(YogObj, module), attrs, attrs);

    RETURN(env, module);
}

YogVal
YogModule_of_name(YogEnv* env, const char* name)
{
    ID id = YogVM_intern(env, env->vm, name);
    return YogModule_new(env, id);
}

void
YogModule_define_function(YogEnv* env, YogVal self, YogVal pkg, const char* name, YogAPI f)
{
    SAVE_ARGS2(env, self, pkg);
    YogVal func = YUNDEF;
    PUSH_LOCAL(env, func);

    func = YogNativeFunction_new(env, INVALID_ID, pkg, name, f);
    YogObj_set_attr(env, self, name, func);

    RETURN_VOID(env);
}

void
YogModule_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cModule = YUNDEF;
    PUSH_LOCAL(env, cModule);
    YogVM* vm = env->vm;

    cModule = YogClass_new(env, "Module", vm->cObject);
    vm->cModule = cModule;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
