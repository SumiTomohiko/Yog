#include <stdio.h>
#include <stdlib.h>
#include "yog/yog.h"

void 
Yog_assert(YogEnv* env, BOOL result, const char* msg) 
{
    if (!result) {
        printf("%s\n", msg);
        abort();
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
