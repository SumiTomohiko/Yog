#include <stdarg.h>
#include <string.h>
#include "yog/array.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/yog.h"

void
YogCFrame_return_multi_value(YogEnv* env, YogVal self, YogVal multi_val)
{
    SAVE_ARGS2(env, self, multi_val);
    YogGC_UPDATE_PTR(PTR_AS(YogCFrame, self), multi_val, multi_val);
    RETURN_VOID(env);
}

static void
YogFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogFrame* frame = PTR_AS(YogFrame, ptr);
    YogGC_keep(env, &frame->prev, keeper, heap);
}

#define KEEP(member)    YogGC_keep(env, &frame->member, keeper, heap)

static void
YogCFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogFrame_keep_children(env, ptr, keeper, heap);

    YogCFrame* frame = PTR_AS(YogCFrame, ptr);
    KEEP(f);
    KEEP(multi_val);
}

static void
YogScriptFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogFrame_keep_children(env, ptr, keeper, heap);

    YogScriptFrame* frame = PTR_AS(YogScriptFrame, ptr);
    KEEP(code);
    KEEP(stack);
    KEEP(globals);
    KEEP(outer_vars);
    KEEP(frame_to_long_return);
    KEEP(frame_to_long_break);
}

static void
YogNameFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogScriptFrame_keep_children(env, ptr, keeper, heap);

    YogNameFrame* frame = PTR_AS(YogNameFrame, ptr);
    KEEP(self);
    KEEP(vars);
}

static void
YogMethodFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogScriptFrame_keep_children(env, ptr, keeper, heap);

    YogMethodFrame* frame = PTR_AS(YogMethodFrame, ptr);
    KEEP(vars);
    KEEP(klass);
}

#undef KEEP

static void
YogFrame_init(YogVal frame, YogFrameType type)
{
    PTR_AS(YogFrame, frame)->prev = YNIL;
    PTR_AS(YogFrame, frame)->type = type;
}

void
YogScriptFrame_push_stack(YogEnv* env, YogVal frame, YogVal val)
{
    YOG_ASSERT(env, PTR_AS(YogFrame, frame)->type != FRAME_C, "invalid frame type (0x%x)", PTR_AS(YogFrame, frame)->type);
    SAVE_ARGS2(env, frame, val);
    YogVal stack = YUNDEF;
    PUSH_LOCAL(env, stack);

    stack = PTR_AS(YogScriptFrame, frame)->stack;
    uint_t capacity = YogValArray_size(env, stack);
    YOG_ASSERT(env, PTR_AS(YogScriptFrame, frame)->stack_size < capacity, "Stack is full.");

    uint_t n = PTR_AS(YogScriptFrame, frame)->stack_size;
    YogGC_UPDATE_PTR(PTR_AS(YogValArray, stack), items[n], val);
    PTR_AS(YogScriptFrame, frame)->stack_size++;

    RETURN_VOID(env);
}

YogVal
YogScriptFrame_pop_stack(YogEnv* env, YogScriptFrame* frame)
{
    YOG_ASSERT(env, 0 < frame->stack_size, "Stack is empty.");

    YogVal stack = frame->stack;
    uint_t index = frame->stack_size - 1;
    YogVal retval = YogValArray_at(env, stack, index);

    PTR_AS(YogValArray, stack)->items[index] = YUNDEF;
    frame->stack_size--;

    return retval;
}

static void
YogScriptFrame_init(YogVal frame, YogFrameType type)
{
    YogFrame_init(frame, type);
#define INIT(member, value)     PTR_AS(YogScriptFrame, frame)->member = value
    INIT(pc, 0);
    INIT(code, YUNDEF);
    INIT(stack_size, 0);
    INIT(stack, YUNDEF);
    INIT(globals, YUNDEF);
    INIT(outer_vars, YUNDEF);
    INIT(frame_to_long_return, YUNDEF);
    INIT(frame_to_long_break, YUNDEF);
    INIT(lhs_left_num, 0);
    INIT(lhs_middle_num, 0);
    INIT(lhs_right_num, 0);
#undef INIT
}

static void
YogNameFrame_init(YogVal frame, YogFrameType type)
{
    YogScriptFrame_init(frame, type);
    PTR_AS(YogNameFrame, frame)->self = YUNDEF;
    PTR_AS(YogNameFrame, frame)->vars = YUNDEF;
}

YogVal
YogClassFrame_new(YogEnv* env)
{
    YogVal frame = ALLOC_OBJ(env, YogNameFrame_keep_children, NULL, YogClassFrame);
    YogNameFrame_init(frame, FRAME_CLASS);

    return frame;
}

YogVal
YogPackageFrame_new(YogEnv* env)
{
    YogVal frame = ALLOC_OBJ(env, YogNameFrame_keep_children, NULL, YogPackageFrame);
    YogNameFrame_init(frame, FRAME_PKG);

    return frame;
}

static void
YogMethodFrame_init(YogVal frame)
{
    YogScriptFrame_init(frame, FRAME_METHOD);
    PTR_AS(YogMethodFrame, frame)->vars = YUNDEF;
    PTR_AS(YogMethodFrame, frame)->klass = YUNDEF;
}

YogVal
YogMethodFrame_new(YogEnv* env)
{
    YogVal frame = ALLOC_OBJ(env, YogMethodFrame_keep_children, NULL, YogMethodFrame);
    YogMethodFrame_init(frame);

    return frame;
}

static void
YogCFrame_init(YogEnv* env, YogVal frame)
{
    YogFrame_init(frame, FRAME_C);

    PTR_AS(YogCFrame, frame)->f = YUNDEF;
    PTR_AS(YogCFrame, frame)->multi_val = YUNDEF;
}

YogVal
YogCFrame_new(YogEnv* env)
{
    YogVal frame = ALLOC_OBJ(env, YogCFrame_keep_children, NULL, YogCFrame);
    YogCFrame_init(env, frame);

    return frame;
}

static void
YogOuterVars_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogOuterVars* vars = PTR_AS(YogOuterVars, ptr);

    uint_t size = vars->size;
    uint_t i = 0;
    for (i = 0; i < size; i++) {
        YogGC_keep(env, &vars->items[i], keeper, heap);
    }
}

YogVal
YogOuterVars_new(YogEnv* env, uint_t size)
{
    YogVal vars = ALLOC_OBJ_ITEM(env, YogOuterVars_keep_children, NULL, YogOuterVars, size, YogVal);
    PTR_AS(YogOuterVars, vars)->size = size;
    uint_t i = 0;
    for (i = 0; i < size; i++) {
        PTR_AS(YogOuterVars, vars)->items[i] = YUNDEF;
    }

    return vars;
}

static void
YogFinishFrame_init(YogEnv* env, YogVal self)
{
    YogScriptFrame_init(self, FRAME_FINISH);
    PTR_AS(YogScriptFrame, self)->lhs_left_num = 1;
}

YogVal
YogFinishFrame_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal frame = YUNDEF;
    PUSH_LOCAL(env, frame);

    frame = ALLOC_OBJ(env, YogScriptFrame_keep_children, NULL, YogScriptFrame);
    YogFinishFrame_init(env, frame);

    RETURN(env, frame);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
