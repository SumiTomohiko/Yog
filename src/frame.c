#include "yog/yog.h"

static void 
gc_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogFrame* frame = ptr;
    PKG_VARS(frame) = do_gc(env, PKG_VARS(frame));
    frame->stack = do_gc(env, frame->stack);
}

YogFrame* 
YogFrame_new(YogEnv* env) 
{
    YogFrame* frame = ALLOC_OBJ(env, gc_children, YogFrame);
    PKG_VARS(frame) = NULL;
    frame->stack = NULL;

    return frame;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
