#include <stdarg.h>
#include <string.h>
#include "yog/array.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/thread.h"
#include "yog/yog.h"

#if 0
static void 
extend_locals(YogEnv* env, YogCFrame* frame, unsigned int n) 
{
    YogValArray* old_locals = frame->locals;
    unsigned int capacity = (old_locals != NULL ? old_locals->size : 0) + n;

    YogVal new_locals = YogValArray_new(env, capacity);
    if (old_locals != NULL) {
        unsigned int size = frame->locals_size;
        YogVal* dest = PTR_AS(YogValArray, new_locals)->items;
        memcpy(dest, old_locals->items, sizeof(YogVal) * size);
    }
    frame->locals = new_locals;
}

void 
YogFrame_add_locals(YogEnv* env, YogCFrame* frame, unsigned int n, ...)
{
    extend_locals(env, frame, n);

    YogValArray* locals = frame->locals;
    unsigned int locals_size = frame->locals_size;

    va_list ap;
    va_start(ap, n);
    unsigned int i = 0;
    for (i = 0; i < n; i++) {
        locals->items[locals_size + i] = va_arg(ap, YogVal);
    }
    va_end(ap);

    frame->locals_size = locals_size + n;
}
#endif

static void 
YogFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogFrame* frame = ptr;
    YogGC_keep(env, &frame->prev, keeper, heap);
}

#define KEEP(member)    YogGC_keep(env, &frame->member, keeper, heap)

static void 
YogCFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogFrame_keep_children(env, ptr, keeper, heap);

    YogCFrame* frame = ptr;
    KEEP(self);
    KEEP(f);
    KEEP(args);
#if 0
    KEEP(locals);
#endif
}

static void 
YogScriptFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogFrame_keep_children(env, ptr, keeper, heap);

    YogScriptFrame* frame = ptr;
    KEEP(code);
    KEEP(stack);
    KEEP(globals);
    KEEP(outer_vars);
}

static void 
YogNameFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogScriptFrame_keep_children(env, ptr, keeper, heap);

    YogNameFrame* frame = ptr;
    KEEP(self);
    KEEP(vars);
}

static void 
YogMethodFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogScriptFrame_keep_children(env, ptr, keeper, heap);

    YogMethodFrame* frame = ptr;
    KEEP(vars);
}

#undef KEEP

static void 
YogFrame_init(YogVal frame, YogFrameType type)
{
    PTR_AS(YogFrame, frame)->prev = YNIL;
    PTR_AS(YogFrame, frame)->type = type;
}

void 
YogScriptFrame_push_stack(YogEnv* env, YogScriptFrame* frame, YogVal val) 
{
    YogVal stack = frame->stack;
    unsigned int capacity = YogValArray_size(env, stack);
    YOG_ASSERT(env, frame->stack_size < capacity, "Stack is full.");

    unsigned int n = frame->stack_size;
    MODIFY(env, PTR_AS(YogValArray, stack)->items[n], val);
    frame->stack_size++;
}

YogVal 
YogScriptFrame_pop_stack(YogEnv* env, YogScriptFrame* frame) 
{
    YOG_ASSERT(env, 0 < frame->stack_size, "Stack is empty.");

    YogVal stack = frame->stack;
    unsigned int index = frame->stack_size - 1;
    YogVal retval = YogValArray_at(env, stack, index);

    PTR_AS(YogValArray, stack)->items[index] = YUNDEF;
    frame->stack_size--;

    return retval;
}

static void 
YogScriptFrame_init(YogVal frame, YogFrameType type)
{
    YogFrame_init(frame, type);
    PTR_AS(YogScriptFrame, frame)->pc = 0;
    PTR_AS(YogScriptFrame, frame)->code = YUNDEF;
    PTR_AS(YogScriptFrame, frame)->stack_size = 0;
    PTR_AS(YogScriptFrame, frame)->stack = YUNDEF;
    PTR_AS(YogScriptFrame, frame)->globals = YUNDEF;
    PTR_AS(YogScriptFrame, frame)->outer_vars = YUNDEF;
}

static void 
YogNameFrame_init(YogVal frame) 
{
    YogScriptFrame_init(frame, FRAME_NAME);
    PTR_AS(YogNameFrame, frame)->self = YUNDEF;
    PTR_AS(YogNameFrame, frame)->vars = YUNDEF;
}

YogVal 
YogNameFrame_new(YogEnv* env) 
{
    YogVal frame = ALLOC_OBJ(env, YogNameFrame_keep_children, NULL, YogNameFrame);
    YogNameFrame_init(frame);

    return frame;
}

static void 
YogMethodFrame_init(YogVal frame) 
{
    YogScriptFrame_init(frame, FRAME_METHOD);
    PTR_AS(YogMethodFrame, frame)->vars = YUNDEF;
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

    PTR_AS(YogCFrame, frame)->self = YUNDEF;
    PTR_AS(YogCFrame, frame)->args = YUNDEF;
    PTR_AS(YogCFrame, frame)->f = YUNDEF;
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
    YogOuterVars* vars = ptr;

    unsigned int size = vars->size;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogGC_keep(env, &vars->items[i], keeper, heap);
    }
}

YogVal 
YogOuterVars_new(YogEnv* env, unsigned int size) 
{
    YogVal vars = ALLOC_OBJ_ITEM(env, YogOuterVars_keep_children, NULL, YogOuterVars, size, YogVal);
    PTR_AS(YogOuterVars, vars)->size = size;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        PTR_AS(YogOuterVars, vars)->items[i] = YUNDEF;
    }

    return vars;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
