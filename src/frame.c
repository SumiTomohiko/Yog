#include "yog/yog.h"

#define GC(name)    DO_GC(env, do_gc, frame->name)

static void 
gc_pkg_frame_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
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
    frame->code = NULL;
    frame->stack = NULL;
}

static void 
YogNameFrame_init(YogNameFrame* frame, YogFrameType type) 
{
    YogScriptFrame_init(SCRIPT_FRAME(frame), type);
    frame->vars = NULL;
}

YogNameFrame* 
YogNameFrame_new(YogEnv* env) 
{
    YogNameFrame* frame = ALLOC_OBJ(env, NULL, YogNameFrame);
    YogNameFrame_init(frame, FT_KLASS);

    return frame;
}

static void 
YogPkgFrame_init(YogPkgFrame* frame) 
{
    YogNameFrame_init(NAME_FRAME(frame), FT_PKG);
}

YogPkgFrame* 
YogPkgFrame_new(YogEnv* env) 
{
    YogPkgFrame* frame = ALLOC_OBJ(env, gc_pkg_frame_children, YogPkgFrame);
    YogPkgFrame_init(frame);

    return frame;
}

static void 
gc_method_frame_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogMethodFrame* frame = ptr;
    GC(vars);
}

static void 
YogMethodFrame_init(YogMethodFrame* frame) 
{
    YogScriptFrame_init(SCRIPT_FRAME(frame), FT_METHOD);
    frame->vars = NULL;
}

YogMethodFrame* 
YogMethodFrame_new(YogEnv* env) 
{
    YogMethodFrame* frame = ALLOC_OBJ(env, gc_method_frame_children, YogMethodFrame);
    YogMethodFrame_init(frame);

    return frame;
}

static void 
YogCFrame_init(YogCFrame* frame) 
{
    YogFrame_init(FRAME(frame), FT_C);
}

YogCFrame* 
YogCFrame_new(YogEnv* env) 
{
    YogCFrame* frame = ALLOC_OBJ(env, NULL, YogCFrame);
    YogCFrame_init(frame);

    return frame;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
