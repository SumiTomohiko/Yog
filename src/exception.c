#include "yog/code.h"
#include "yog/error.h"
#include "yog/exception.h"
#include "yog/function.h"
#include "yog/yog.h"

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogBasicObj_keep_children(env, ptr, keeper);

    YogException* exc = ptr;
    exc->stack_trace = YogVal_keep(env, exc->stack_trace, keeper);
    exc->message = YogVal_keep(env, exc->message, keeper);
}

static YogVal 
allocate(YogEnv* env, YogVal klass) 
{
    SAVE_ARG(env, klass);

    YogException* exc = ALLOC_OBJ(env, keep_children, NULL, YogException);
    YogBasicObj_init(env, (YogBasicObj*)exc, 0, klass);
    exc->stack_trace = YNIL;
    exc->message = YNIL;

    RETURN(env, OBJ2VAL(exc));
}

static YogVal 
skip_frame(YogEnv* env, YogVal frame, const char* func_name) 
{
    SAVE_ARG(env, frame);

    ID name = INTERN(func_name);

    switch (PTR_AS(YogFrame, frame)->type) {
    case FRAME_C:
        {
            if (PTR_AS(YogCFrame, frame)->f->func_name == name) {
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
initialize(YogEnv* env)
{
    YogVal self = SELF(env);
    YogVal message = ARG(env, 0);
    YogVal frame = YUNDEF;
    YogVal st = YUNDEF;
    PUSH_LOCALS4(env, self, message, frame, st);

    frame = ENV_TH(env)->cur_frame;
    frame = skip_frame(env, frame, "initialize");
    frame = skip_frame(env, frame, "new");

    st = YNIL;
    while (IS_PTR(frame)) {
        YogVal entry = YogStackTraceEntry_new(env);

        switch (PTR_AS(YogFrame, frame)->type) {
        case FRAME_C:
            {
                ID klass_name = PTR_AS(YogCFrame, frame)->f->klass_name;
                ID func_name = PTR_AS(YogCFrame, frame)->f->func_name;
                PTR_AS(YogStackTraceEntry, entry)->lineno = 0;
                PTR_AS(YogStackTraceEntry, entry)->filename = NULL;
                PTR_AS(YogStackTraceEntry, entry)->klass_name = klass_name;
                PTR_AS(YogStackTraceEntry, entry)->func_name = func_name;
                break;
            }
        case FRAME_METHOD:
        case FRAME_NAME:
            {
                YogVal code = PTR_AS(YogScriptFrame, frame)->code;
                unsigned int lineno = 0;
                pc_t pc = PTR_AS(YogScriptFrame, frame)->pc - 1;
                unsigned int i = 0;
                for (i = 0; i < PTR_AS(YogCode, code)->lineno_tbl_size; i++) {
                    YogVal lineno_tbl = PTR_AS(YogCode, code)->lineno_tbl;
                    YogLinenoTableEntry* entry = &PTR_AS(YogLinenoTableEntry, lineno_tbl)[i];
                    if ((entry->pc_from <= pc) && (pc < entry->pc_to)) {
                        lineno = entry->lineno;
                        break;
                    }
                }

                const char* filename = PTR_AS(YogCode, code)->filename;
                ID klass_name = PTR_AS(YogCode, code)->klass_name;
                ID func_name = PTR_AS(YogCode, code)->func_name;
                PTR_AS(YogStackTraceEntry, entry)->lineno = lineno;
                PTR_AS(YogStackTraceEntry, entry)->filename = filename;
                PTR_AS(YogStackTraceEntry, entry)->klass_name = klass_name;
                PTR_AS(YogStackTraceEntry, entry)->func_name = func_name;
                break;
            }
        default:
            YOG_ASSERT(env, FALSE, "Unkown frame type.");
            break;
        }

        MODIFY(env, PTR_AS(YogStackTraceEntry, entry)->lower, st);
        st = entry;

        frame = PTR_AS(YogFrame, frame)->prev;
    }

    YogException* exc = OBJ_AS(YogException, self);
    exc->stack_trace = st;
    if (IS_UNDEF(message)) {
        message = YNIL;
    }
    exc->message = message;

    return YNIL;
}

static YogVal 
to_s(YogEnv* env)
{
    YogVal self = SELF(env);

    YogException* exc = OBJ_AS(YogException, self);
    YogVal msg = exc->message;
    YogVal retval = YogThread_call_method(env, ENV_TH(env), msg, "to_s", 0, NULL);

    return retval;
}

YogVal 
YogException_klass_new(YogEnv* env) 
{
    YogVal klass = YogKlass_new(env, "Exception", ENV_VM(env)->cObject);
    PUSH_LOCAL(env, klass);

    YogKlass_define_allocator(env, klass, allocate);
    YogKlass_define_method(env, klass, "initialize", initialize, 0, 0, 0, 0, "message", NULL);
    YogKlass_define_method(env, klass, "to_s", to_s, 0, 0, 0, 0, NULL);

    POP_LOCALS(env);
    return klass;
#undef UPDATE_PTR
}

YogVal 
YogBugException_new(YogEnv* env) 
{
    YogVal self = ENV_VM(env)->eBugException;
    return YogThread_call_method(env, ENV_TH(env), self, "new", 0, NULL);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
