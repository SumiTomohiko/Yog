#include "yog/array.h"
#include "yog/code.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/gc.h"
#include "yog/class.h"
#include "yog/thread.h"
#include "yog/yog.h"

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogException* exc = ptr;
#define KEEP(member)    YogGC_keep(env, &exc->member, keeper, heap)
    KEEP(stack_trace);
    KEEP(message);
#undef KEEP
}

static YogVal
allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal exc = ALLOC_OBJ(env, keep_children, NULL, YogException);
    YogBasicObj_init(env, exc, 0, klass);
    PTR_AS(YogException, exc)->stack_trace = YNIL;
    PTR_AS(YogException, exc)->message = YNIL;

    RETURN(env, exc);
}

static YogVal
skip_frame(YogEnv* env, YogVal frame, const char* func_name)
{
    SAVE_ARG(env, frame);

    ID name = YogVM_intern(env, env->vm, func_name);

    switch (PTR_AS(YogFrame, frame)->type) {
    case FRAME_C:
        {
            YogVal f = PTR_AS(YogCFrame, frame)->f;
            if (PTR_AS(YogNativeFunction, f)->func_name == name) {
                RETURN(env, PTR_AS(YogFrame, frame)->prev);
            }
            break;
        }
    case FRAME_METHOD:
    case FRAME_NAME:
        {
            YogVal code = PTR_AS(YogScriptFrame, frame)->code;
            if (PTR_AS(YogCode, code)->func_name == name) {
                RETURN(env, PTR_AS(YogFrame, frame)->prev);
            }
            break;
        }
    default:
        YOG_ASSERT(env, FALSE, "unknown frame type");
        break;
    }

    RETURN(env, frame);
}

static YogVal
initialize(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);

    YogVal message = YUNDEF;
    YogVal frame = YUNDEF;
    YogVal st = YUNDEF;
    PUSH_LOCALS3(env, message, frame, st);

    if (0 < YogArray_size(env, args)) {
        message = YogArray_at(env, args, 0);
    }
    else {
        message = YNIL;
    }

    frame = PTR_AS(YogThread, env->thread)->cur_frame;
    frame = skip_frame(env, frame, "initialize");
    frame = skip_frame(env, frame, "new");

    st = YNIL;
    while (IS_PTR(frame)) {
        if (PTR_AS(YogFrame, frame)->type == FRAME_FINISH) {
            frame = PTR_AS(YogFrame, frame)->prev;
            continue;
        }

        YogVal entry = YogStackTraceEntry_new(env);

        switch (PTR_AS(YogFrame, frame)->type) {
        case FRAME_C:
            {
                YogVal f = PTR_AS(YogCFrame, frame)->f;
                ID class_name = PTR_AS(YogNativeFunction, f)->class_name;
                ID func_name = PTR_AS(YogNativeFunction, f)->func_name;
                PTR_AS(YogStackTraceEntry, entry)->lineno = 0;
                PTR_AS(YogStackTraceEntry, entry)->filename = YNIL;
                PTR_AS(YogStackTraceEntry, entry)->class_name = class_name;
                PTR_AS(YogStackTraceEntry, entry)->func_name = func_name;
                break;
            }
        case FRAME_METHOD:
        case FRAME_NAME:
            {
                YogVal code = PTR_AS(YogScriptFrame, frame)->code;
                uint_t lineno = 0;
                pc_t pc = PTR_AS(YogScriptFrame, frame)->pc - 1;
                uint_t i = 0;
                for (i = 0; i < PTR_AS(YogCode, code)->lineno_tbl_size; i++) {
                    YogVal lineno_tbl = PTR_AS(YogCode, code)->lineno_tbl;
                    YogLinenoTableEntry* entry = &PTR_AS(YogLinenoTableEntry, lineno_tbl)[i];
                    if ((entry->pc_from <= pc) && (pc < entry->pc_to)) {
                        lineno = entry->lineno;
                        break;
                    }
                }

                YogVal filename = PTR_AS(YogCode, code)->filename;
                ID class_name = PTR_AS(YogCode, code)->class_name;
                ID func_name = PTR_AS(YogCode, code)->func_name;
                PTR_AS(YogStackTraceEntry, entry)->lineno = lineno;
                PTR_AS(YogStackTraceEntry, entry)->filename = filename;
                PTR_AS(YogStackTraceEntry, entry)->class_name = class_name;
                PTR_AS(YogStackTraceEntry, entry)->func_name = func_name;
                break;
            }
        case FRAME_FINISH:
        default:
            YOG_ASSERT(env, FALSE, "invalid frame type (0x%x)", PTR_AS(YogFrame, frame)->type);
            break;
        }

        PTR_AS(YogStackTraceEntry, entry)->lower = st;
        st = entry;

        frame = PTR_AS(YogFrame, frame)->prev;
    }

    PTR_AS(YogException, self)->stack_trace = st;
    if (IS_UNDEF(message)) {
        message = YNIL;
    }
    PTR_AS(YogException, self)->message = message;

    RETURN(env, YNIL);
}

static YogVal
to_s(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);

    YogException* exc = PTR_AS(YogException, self);
    YogVal msg = exc->message;
    YogVal retval = YogEval_call_method(env, msg, "to_s", 0, NULL);

    RETURN(env, retval);
}

static YogVal
get_message(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal message = YUNDEF;
    PUSH_LOCAL(env, message);

    message = PTR_AS(YogException, self)->message;

    RETURN(env, message);
}

YogVal
YogException_define_class(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "Exception", env->vm->cObject);
    YogClass_define_allocator(env, klass, allocate);
    YogClass_define_method(env, klass, "initialize", initialize);
    YogClass_define_method(env, klass, "to_s", to_s);
    YogClass_define_property(env, klass, "message", get_message, NULL);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
