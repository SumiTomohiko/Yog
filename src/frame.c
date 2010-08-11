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
    uint_t size = PTR_AS(YogScriptFrame, self)->stack_size;
    YOG_ASSERT(env, size < PTR_AS(YogScriptFrame, self)->stack_capacity, "Full stack");
    YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, self), locals_etc[size], val);
    PTR_AS(YogScriptFrame, self)->stack_size++;
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
YogFrame_clean(YogEnv* env, YogVal self)
{
    PTR_AS(YogFrame, self)->prev = YNIL;
}

static void
YogFrame_init(YogEnv* env, YogVal self, YogFrameType type)
{
    YogFrame_clean(env, self);
    PTR_AS(YogFrame, self)->type = type;
}

static void
cleanup_locals(YogEnv* env, YogVal self)
{
    uint_t stack_capacity = PTR_AS(YogScriptFrame, self)->stack_capacity;
    uint_t locals_num = PTR_AS(YogScriptFrame, self)->locals_num;
    uint_t outer_depth = PTR_AS(YogScriptFrame, self)->outer_frames_num;
    uint_t locals_etc_size = stack_capacity + locals_num + outer_depth;
    uint_t i;
    for (i = 0; i < locals_etc_size; i++) {
        PTR_AS(YogScriptFrame, self)->locals_etc[i] = YUNDEF;
    }
}

void
YogScriptFrame_cleanup(YogEnv* env, YogVal self)
{
    YogFrame_clean(env, self);
#define CLEAN(member, val) PTR_AS(YogScriptFrame, self)->member = (val)
    CLEAN(pc, 0);
    CLEAN(code, YUNDEF);
    CLEAN(stack_size, 0);
    CLEAN(globals, YUNDEF);
    CLEAN(frame_to_long_return, YUNDEF);
    CLEAN(frame_to_long_break, YUNDEF);
    CLEAN(klass, YUNDEF);
#undef CLEAN
    cleanup_locals(env, self);
}

static void
YogScriptFrame_init(YogEnv* env, YogVal self, YogFrameType type, YogVal code, uint_t locals_num, uint_t lhs_left_num)
{
    SAVE_ARGS2(env, self, code);

    YogFrame_init(env, self, type);
#define INIT(member, value) PTR_AS(YogScriptFrame, self)->member = (value)
    INIT(pc, 0);
    INIT(code, YUNDEF);
    INIT(stack_capacity, PTR_AS(YogCode, code)->stack_size);
    INIT(stack_size, 0);
    INIT(locals_num, locals_num);
    INIT(outer_frames_num, PTR_AS(YogCode, code)->outer_size);
    INIT(globals, YUNDEF);
    INIT(frame_to_long_return, YUNDEF);
    INIT(frame_to_long_break, YUNDEF);
    INIT(lhs_left_num, lhs_left_num);
    INIT(lhs_middle_num, 0);
    INIT(lhs_right_num, 0);
    INIT(used_by_func, FALSE);
    INIT(klass, YUNDEF);
    INIT(name, INVALID_ID);
#undef INIT
    cleanup_locals(env, self);
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
    YogFrame_init(env, frame, FRAME_C);

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

void
YogFinishFrame_clean(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogFrame_clean(env, self);
#define CLEAN(member, val) PTR_AS(YogScriptFrame, self)->member = (val)
    CLEAN(pc, 0);
    CLEAN(stack_size, 0);
#undef CLEAN
    RETURN_VOID(env);
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

YogVal
YogFrame_get_script_frame(YogEnv* env, YogVal code, uint_t locals_num)
{
    YogVal frame = YogThread_get_script_frame(env, env->thread);
    if (IS_PTR(frame)) {
        uint_t actual = PTR_AS(YogScriptFrame, frame)->stack_capacity + PTR_AS(YogScriptFrame, frame)->locals_num + PTR_AS(YogScriptFrame, frame)->outer_frames_num;
        uint_t stack_size = PTR_AS(YogCode, code)->stack_size;
        uint_t needed = stack_size + locals_num + PTR_AS(YogCode, code)->outer_size;
        if (needed <= actual) {
            PTR_AS(YogScriptFrame, frame)->stack_capacity = stack_size;
            PTR_AS(YogScriptFrame, frame)->locals_num = locals_num;
            uint_t outer_frames_num = actual - stack_size - locals_num;
            PTR_AS(YogScriptFrame, frame)->outer_frames_num = outer_frames_num;
            YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, frame), code, code);
            return frame;
        }
    }

    return YogScriptFrame_new(env, FRAME_SCRIPT, code, locals_num, 0);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
