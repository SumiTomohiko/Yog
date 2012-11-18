#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/module.h"
#include "yog/vm.h"
#include "yog/yog.h"

void
YogEnumerable_eval_builtin_script(YogEnv* env, YogVal klass)
{
    const char* src =
#include "enumerable.inc"
    ;
    YogMisc_eval_source(env, VAL2HDL(env, klass), src);
}

void
YogEnumerable_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal mEnumerable = YUNDEF;
    PUSH_LOCAL(env, mEnumerable);
    YogVM* vm = env->vm;

    mEnumerable = YogModule_of_name(env, "Enumerable");
    vm->mEnumerable = mEnumerable;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
