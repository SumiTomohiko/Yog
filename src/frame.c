#include "yog/error.h"
#include "yog/yog.h"

#define KEEP(member)    frame->member = (*keeper)(env, frame->member)

static void 
YogFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogFrame* frame = ptr;
    KEEP(prev);
}

static void 
YogCFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogFrame_keep_children(env, ptr, keeper);

    YogCFrame* frame = ptr;
    frame->self = YogVal_keep(env, frame->self, keeper);
    KEEP(args);
}

static void 
YogScriptFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogFrame_keep_children(env, ptr, keeper);

    YogScriptFrame* frame = ptr;
    KEEP(code);
    KEEP(stack);
}

static void 
YogNameFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogScriptFrame_keep_children(env, ptr, keeper);

    YogNameFrame* frame = ptr;
    frame->self = YogVal_keep(env, frame->self, keeper);
    KEEP(vars);
}

static void 
YogMethodFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogScriptFrame_keep_children(env, ptr, keeper);

    YogMethodFrame* frame = ptr;
    KEEP(vars);
}

static void 
YogFrame_init(YogFrame* frame, YogFrameType type)
{
    frame->prev = NULL;
    frame->type = type;
}

void 
YogScriptFrame_push_stack(YogEnv* env, YogScriptFrame* frame, YogVal val) 
{
    YogValArray* stack = frame->stack;
    unsigned int capacity = YogValArray_size(env, stack);
    YOG_ASSERT(env, frame->stack_size < capacity, "Stack is full.");

    stack->items[frame->stack_size] = val;
    frame->stack_size++;
}

YogVal 
YogScriptFrame_pop_stack(YogEnv* env, YogScriptFrame* frame) 
{
    YOG_ASSERT(env, 0 < frame->stack_size, "Stack is empty.");

    unsigned int index = frame->stack_size - 1;
    YogVal retval = YogValArray_at(env, frame->stack, index);

    frame->stack->items[index] = YUNDEF;
    frame->stack_size--;

    return retval;
}

static void 
YogScriptFrame_init(YogScriptFrame* frame)
{
    YogFrame_init(FRAME(frame), FRAME_SCRIPT);
    frame->pc = 0;
    frame->code = NULL;
    frame->stack_size = 0;
    frame->stack = NULL;
}

static void 
YogNameFrame_init(YogNameFrame* frame)
{
    YogScriptFrame_init(SCRIPT_FRAME(frame));
    frame->vars = NULL;
}

YogNameFrame* 
YogNameFrame_new(YogEnv* env) 
{
    YogNameFrame* frame = ALLOC_OBJ(env, YogNameFrame_keep_children, YogNameFrame);
    YogNameFrame_init(frame);

    return frame;
}

static void 
YogMethodFrame_init(YogMethodFrame* frame) 
{
    YogScriptFrame_init(SCRIPT_FRAME(frame));
    frame->vars = NULL;
}

YogMethodFrame* 
YogMethodFrame_new(YogEnv* env) 
{
    YogMethodFrame* frame = ALLOC_OBJ(env, YogMethodFrame_keep_children, YogMethodFrame);
    YogMethodFrame_init(frame);

    return frame;
}

static void 
YogCFrame_init(YogCFrame* frame) 
{
    YogFrame_init(FRAME(frame), FRAME_C);

    frame->self = YUNDEF;
    frame->args = NULL;
}

YogCFrame* 
YogCFrame_new(YogEnv* env) 
{
    YogCFrame* frame = ALLOC_OBJ(env, YogCFrame_keep_children, YogCFrame);
    YogCFrame_init(frame);

    return frame;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
