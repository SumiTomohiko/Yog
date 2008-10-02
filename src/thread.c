#include "yog/yog.h"

YogThread*
YogThread_new(YogEnv* env) 
{
    YogThread* th = ALLOC_OBJ(env, GCOBJ_THREAD, YogThread);
    th->cur_frame = NULL;

    return th;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
