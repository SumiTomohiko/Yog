#include <stdio.h>
#include "yog/package.h"
#include "yog/yog.h"

static YogVal
foo(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    printf("42\n");
    RETURN(env, YNIL);
}

YogVal
YogInit_test_package5(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    pkg = YogPackage_new(env);
    YogPackage_define_function(env, pkg, "foo", foo);

    RETURN(env, pkg);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
