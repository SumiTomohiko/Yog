#include <stdio.h>
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
    YogPackage_define_method(env, pkg, "foo", foo, 0, 0, 0, 0, NULL);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
