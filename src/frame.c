#include "yog/yog.h"

YogFrame* 
YogFrame_new(YogEnv* env) 
{
    YogFrame* frame = ALLOC_OBJ(env, GCOBJ_FRAME, YogFrame);
    frame->code = NULL;
    frame->pc = 0;
    PKG_VARS(frame) = NULL;
    frame->stack = NULL;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
