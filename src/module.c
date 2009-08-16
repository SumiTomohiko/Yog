#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/module.h"
#include "yog/object.h"
#include "yog/table.h"
#include "yog/thread.h"
#include "yog/yog.h"

static YogVal
allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal module = YUNDEF;
    PUSH_LOCAL(env, module);

    module = ALLOC_OBJ(env, YogObj_keep_children, NULL, YogModule);
    YogObj_init(env, module, 0, klass);
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

YogVal
YogModule_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Module", env->vm->cObject);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */