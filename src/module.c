#include "yog/function.h"
#include "yog/gc.h"
#include "yog/class.h"
#include "yog/module.h"
#include "yog/object.h"
#include "yog/table.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal
allocate(YogEnv* env, YogVal klass)
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
YogModule_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal module = YUNDEF;
    YogVal attrs = YUNDEF;
    PUSH_LOCALS2(env, module, attrs);

    module = allocate(env, env->vm->cModule);

    attrs = YogTable_new_symbol_table(env);
    PTR_AS(YogObj, module)->attrs = attrs;

    RETURN(env, module);
}

void
YogModule_define_function(YogEnv* env, YogVal self, const char* name, void* f)
{
    SAVE_ARG(env, self);
    YogVal func = YUNDEF;
    PUSH_LOCAL(env, func);

    func = YogNativeFunction_new(env, INVALID_ID, name, f);
    YogObj_set_attr(env, self, name, func);

    RETURN_VOID(env);
}

YogVal
YogModule_define_class(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "Module", env->vm->cObject);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
