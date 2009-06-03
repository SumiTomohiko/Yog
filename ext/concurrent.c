#include "yog/vm.h"
#include "yog/env.h"
#include "yog/object.h"
#include "yog/yog.h"

void
YogInit_concurrent(YogEnv* env, YogVal pkg)
{
    YogObj_set_attr(env, pkg, "Thread", env->vm->cThread);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
