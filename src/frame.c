#include "yog/yog.h"

YogFrame* 
YogFrame_new(YogEnv* env) 
{
    YogFrame* frame = ALLOC_OBJ(env, GCOBJ_FRAME, YogFrame);
    PKG_VARS(frame) = NULL;
    frame->stack = NULL;

    return frame;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
