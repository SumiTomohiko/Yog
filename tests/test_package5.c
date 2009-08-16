#include <stdio.h>
#include "yog/package.h"
#include "yog/yog.h"

static YogVal
foo(YogEnv* env)
{
    printf("42\n");
    return YNIL;
}

void
YogInit_test_package5(YogEnv* env, YogVal pkg)
{
    YogPackage_define_function(env, pkg, "foo", foo);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
