#include <stdarg.h>
#include <string.h>
#include "yog/array.h"
#include "yog/error.h"
#include "yog/yog.h"

static void 
extend_locals(YogEnv* env, YogFrame* frame, unsigned int n) 
{
    unsigned int capacity = frame->locals->size + n;
    YogValArray* locals = YogValArray_new(env, capacity);
    frame = CUR_FRAME(env);
    unsigned int size = frame->locals_size;
    memcpy(locals->items, frame->locals->items, sizeof(YogVal) * size);
    frame->locals = locals;
}

void 
YogFrame_add_locals(YogEnv* env, YogFrame* frame, unsigned int n, ...)
{
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

    extend_locals(env, frame, n);
}

unsigned int
YogFrame_add_local(YogEnv* env, YogFrame* frame, YogVal val) 
{
    YogValArray* locals = frame->locals;
    unsigned int index = frame->locals_size;
    locals->items[index] = val;
    frame->locals_size++;

    return index;
}

#define KEEP(member)    frame->member = (*keeper)(env, frame->member)

static void 
YogFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogFrame* frame = ptr;
    KEEP(prev);
    KEEP(locals);
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

#undef KEEP

static void 
YogFrame_init(YogFrame* frame, YogFrameType type)
{
    frame->prev = NULL;
    frame->type = type;
    frame->locals = NULL;
    frame->locals_size = 0;
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
    YogNameFrame* frame = ALLOC_OBJ(env, YogNameFrame_keep_children, NULL, YogNameFrame);
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
    YogMethodFrame* frame = ALLOC_OBJ(env, YogMethodFrame_keep_children, NULL, YogMethodFrame);
    YogMethodFrame_init(frame);

    return frame;
}

static void 
YogCFrame_init(YogEnv* env, YogCFrame* frame) 
{
    YogFrame_init(FRAME(frame), FRAME_C);

    frame->self = YUNDEF;
    frame->args = NULL;
}

YogCFrame* 
YogCFrame_new(YogEnv* env) 
{
    YogCFrame* frame = ALLOC_OBJ(env, YogCFrame_keep_children, NULL, YogCFrame);
    YogCFrame_init(env, frame);

    return frame;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
