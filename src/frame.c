#include "yog/yog.h"

#define GC(name)    DO_GC(env, do_gc, frame->name)

static void 
gc_pkg_frame_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogPkgFrame* frame = ptr;
    GC(vars);
}

static void 
YogFrame_init(YogFrame* frame, YogFrameType type) 
{
    frame->type = type;
}

static void 
YogScriptFrame_init(YogScriptFrame* frame, YogFrameType type) 
{
    YogFrame_init(FRAME(frame), type);
    frame->pc = 0;
    frame->stack = NULL;
}

YogPkgFrame* 
YogPkgFrame_new(YogEnv* env) 
{
    YogPkgFrame* frame = ALLOC_OBJ(env, gc_pkg_frame_children, YogPkgFrame);
    YogScriptFrame_init(SCRIPT_FRAME(frame), FT_PKG);
    frame->vars = NULL;

    return frame;
}

static void 
gc_method_frame_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogMethodFrame* frame = ptr;
    GC(vars);
}

YogMethodFrame* 
YogMethodFrame_new(YogEnv* env) 
{
    YogMethodFrame* frame = ALLOC_OBJ(env, gc_method_frame_children, YogMethodFrame);
    YogScriptFrame_init(SCRIPT_FRAME(frame), FT_METHOD);
    frame->vars = NULL;

    return frame;
}

YogCFrame* 
YogCFrame_new(YogEnv* env) 
{
    YogCFrame* frame = ALLOC_OBJ(env, NULL, YogCFrame);
    YogFrame_init(FRAME(frame), FT_C);

    return frame;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
