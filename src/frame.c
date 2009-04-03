#include <stdarg.h>
#include <string.h>
#include "yog/array.h"
#include "yog/error.h"
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
YogFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogFrame* frame = ptr;
    frame->prev = YogVal_keep(env, frame->prev, keeper);
}

#define KEEP(member)        frame->member = (*keeper)(env, frame->member)
#define KEEP_VAL(member)    do { \
    frame->member = YogVal_keep(env, frame->member, keeper); \
} while (0)

static void 
YogCFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogFrame_keep_children(env, ptr, keeper);

    YogCFrame* frame = ptr;
    frame->self = YogVal_keep(env, frame->self, keeper);
    KEEP(f);
    KEEP(args);
#if 0
    KEEP_VAL(locals);
#endif
}

static void 
YogScriptFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogFrame_keep_children(env, ptr, keeper);

    YogScriptFrame* frame = ptr;
    frame->code = YogVal_keep(env, frame->code, keeper);
    KEEP_VAL(stack);
    KEEP_VAL(globals);
    KEEP_VAL(outer_vars);
}

static void 
YogNameFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogScriptFrame_keep_children(env, ptr, keeper);

    YogNameFrame* frame = ptr;
    frame->self = YogVal_keep(env, frame->self, keeper);
    KEEP_VAL(vars);
}

static void 
YogMethodFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogScriptFrame_keep_children(env, ptr, keeper);

    YogMethodFrame* frame = ptr;
    KEEP_VAL(vars);
}

#undef KEEP

static void 
YogFrame_init(YogFrame* frame, YogFrameType type)
{
    frame->prev = YNIL;
    frame->type = type;
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
YogScriptFrame_init(YogScriptFrame* frame, YogFrameType type)
{
    YogFrame_init((YogFrame*)frame, type);
    frame->pc = 0;
    frame->code = YUNDEF;
    frame->stack_size = 0;
    frame->stack = YUNDEF;
    frame->globals = YUNDEF;
    frame->outer_vars = YUNDEF;
}

static void 
YogNameFrame_init(YogNameFrame* frame)
{
    YogScriptFrame_init((YogScriptFrame*)frame, FRAME_NAME);
    frame->self = YUNDEF;
    frame->vars = YUNDEF;
}

YogVal 
YogNameFrame_new(YogEnv* env) 
{
    YogNameFrame* frame = ALLOC_OBJ(env, YogNameFrame_keep_children, NULL, YogNameFrame);
    YogNameFrame_init(frame);

    return PTR2VAL(frame);
}

static void 
YogMethodFrame_init(YogMethodFrame* frame) 
{
    YogScriptFrame_init((YogScriptFrame*)frame, FRAME_METHOD);
    frame->vars = YUNDEF;
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
    YogFrame_init((YogFrame*)frame, FRAME_C);

    frame->self = YUNDEF;
    frame->args = NULL;
#if 0
    frame->locals = YUNDEF;
    frame->locals_size = 0;
#endif
}

YogCFrame* 
YogCFrame_new(YogEnv* env) 
{
    YogCFrame* frame = ALLOC_OBJ(env, YogCFrame_keep_children, NULL, YogCFrame);
    YogCFrame_init(env, frame);

    return frame;
}

static void 
YogOuterVars_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogOuterVars* vars = ptr;

    unsigned int size = vars->size;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        vars->items[i] = YogVal_keep(env, vars->items[i], keeper);
    }
}

YogVal 
YogOuterVars_new(YogEnv* env, unsigned int size) 
{
    YogOuterVars* vars = ALLOC_OBJ_ITEM(env, YogOuterVars_keep_children, NULL, YogOuterVars, size, YogVal);
    vars->size = size;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        vars->items[i] = YUNDEF;
    }

    return PTR2VAL(vars);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
