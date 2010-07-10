#include <stdarg.h>
#include <string.h>
#include "yog/array.h"
#include "yog/code.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/vm.h"
#include "yog/yog.h"

void
YogScriptFrame_push_stack(YogEnv* env, YogVal self, YogVal val)
{
    SAVE_ARGS2(env, self, val);

    uint_t size = PTR_AS(YogScriptFrame, self)->stack_size;
    YOG_ASSERT(env, size < PTR_AS(YogScriptFrame, self)->stack_capacity, "Full stack");
    YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, self), locals_etc[size], val);
    PTR_AS(YogScriptFrame, self)->stack_size++;

    RETURN_VOID(env);
}

void
YogCFrame_return_multi_value(YogEnv* env, YogVal self, YogVal multi_val)
{
    SAVE_ARGS2(env, self, multi_val);
    YogGC_UPDATE_PTR(env, PTR_AS(YogCFrame, self), multi_val, multi_val);
    RETURN_VOID(env);
}

static void
YogFrame_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogFrame* frame = PTR_AS(YogFrame, ptr);
    YogGC_KEEP(env, frame, prev, keeper, heap);
}

#define KEEP(member)    YogGC_KEEP(env, frame, member, keeper, heap)

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

    YogScriptFrame* frame = (YogScriptFrame*)ptr;
    KEEP(code);
    KEEP(globals);
    KEEP(frame_to_long_return);
    KEEP(frame_to_long_break);
    KEEP(klass);

    uint_t stack_size = frame->stack_size;
    uint_t i;
    for (i = 0; i < stack_size; i++) {
        KEEP(locals_etc[i]);
    }

    uint_t stack_capacity = frame->stack_capacity;
    uint_t locals_num = frame->locals_num;
    for (i = 0; i < locals_num; i++) {
        KEEP(locals_etc[stack_capacity + i]);
    }

    uint_t offset = stack_capacity + locals_num;
    uint_t frames_num = frame->outer_frames_num;
    for (i = 0; i < frames_num; i++) {
        KEEP(locals_etc[offset + i]);
    }
}

#undef KEEP

static void
YogFrame_init(YogVal frame, YogFrameType type)
{
    PTR_AS(YogFrame, frame)->prev = YNIL;
    PTR_AS(YogFrame, frame)->type = type;
}

static void
YogScriptFrame_init(YogEnv* env, YogVal self, YogFrameType type, YogVal code, uint_t locals_num, uint_t lhs_left_num)
{
    SAVE_ARG(env, code);
    uint_t stack_capacity = PTR_AS(YogCode, code)->stack_size;
    uint_t outer_depth = PTR_AS(YogCode, code)->outer_size;

    YogFrame_init(self, type);
#define INIT(member, value) PTR_AS(YogScriptFrame, self)->member = (value)
    INIT(pc, 0);
    INIT(code, YUNDEF);
    INIT(stack_capacity, stack_capacity);
    INIT(stack_size, 0);
    INIT(locals_num, locals_num);
    INIT(outer_frames_num, outer_depth);
    INIT(globals, YUNDEF);
    INIT(frame_to_long_return, YUNDEF);
    INIT(frame_to_long_break, YUNDEF);
    INIT(lhs_left_num, lhs_left_num);
    INIT(lhs_middle_num, 0);
    INIT(lhs_right_num, 0);
    INIT(klass, YUNDEF);
    INIT(name, INVALID_ID);

    uint_t locals_etc_size = stack_capacity + locals_num + outer_depth;
    uint_t i;
    for (i = 0; i < locals_etc_size; i++) {
        INIT(locals_etc[i], YUNDEF);
    }
#undef INIT

    YogGC_UPDATE_PTR(env, SCRIPT_FRAME(self), code, code);

    RETURN_VOID(env);
}

YogVal
YogScriptFrame_new(YogEnv* env, YogFrameType type, YogVal code, uint_t locals_num, uint_t lhs_left_num)
{
    SAVE_ARG(env, code);
    YogVal frame = YUNDEF;
    PUSH_LOCAL(env, frame);

    uint_t locals_etc_size = PTR_AS(YogCode, code)->stack_size + locals_num + PTR_AS(YogCode, code)->outer_size;
    frame = ALLOC_OBJ_ITEM(env, YogScriptFrame_keep_children, NULL, YogScriptFrame, locals_etc_size, YogVal);
    YogScriptFrame_init(env, frame, type, code, locals_num, lhs_left_num);

    RETURN(env, frame);
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

YogVal
YogFinishFrame_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal code = env->vm->finish_code;
    YogVal frame = YUNDEF;
    PUSH_LOCALS2(env, code, frame);

    uint_t locals_num = PTR_AS(YogCode, code)->local_vars_count;
    frame = YogScriptFrame_new(env, FRAME_FINISH, code, locals_num, 1);

    RETURN(env, frame);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
