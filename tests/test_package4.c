#include <stdio.h>
#include "yog/package.h"
#include "yog/yog.h"

#if defined(__cplusplus)
extern "C"
#endif
#if defined(_MSC_VER) || defined(__MINGW32__)
__declspec(dllexport)
#endif
YogVal
YogInit_test_package4(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    pkg = YogPackage_new(env);

    printf("42\n");

    RETURN(env, pkg);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
