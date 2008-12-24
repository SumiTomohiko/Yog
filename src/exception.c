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
    exc->stack_trace = (*keeper)(env, exc->stack_trace);
    exc->message = YogVal_keep(env, exc->message, keeper);
}

static YogBasicObj* 
allocate(YogEnv* env, YogKlass* klass) 
{
    YogBasicObj* obj = ALLOC_OBJ(env, keep_children, NULL, YogException);
    YogBasicObj_init(env, obj, 0, klass);

    return obj;
}

static YogFrame* 
skip_frame(YogEnv* env, YogFrame* frame, const char* func_name) 
{
    ID name = INTERN(func_name);

    switch (frame->type) {
    case FRAME_C:
        {
            YogCFrame* f = C_FRAME(frame);
            if (f->f->func_name == name) {
                return frame->prev;
            }
            break;
        }
    case FRAME_SCRIPT:
        {
            YogScriptFrame* f = SCRIPT_FRAME(frame);
            if (f->code->func_name == name) {
                return frame->prev;
            }
            break;
        }
    default:
        YOG_ASSERT(env, FALSE, "unknown frame type");
        break;
    }

    return frame;
}

static YogVal 
initialize(YogEnv* env)
{
    YogVal self = SELF(env);
    YogVal message = ARG(env, 0);

    YogStackTraceEntry* st = NULL;
    YogFrame* frame = ENV_TH(env)->cur_frame;
    frame = skip_frame(env, frame, "initialize");
    frame = skip_frame(env, frame, "new");

    while (frame != NULL) {
        YogStackTraceEntry* entry = YogStackTraceEntry_new(env);

        switch (frame->type) {
        case FRAME_C:
            {
                YogCFrame* f = C_FRAME(frame);
                entry->lineno = 0;
                entry->filename = NULL;
                entry->klass_name = f->f->klass_name;
                entry->func_name = f->f->func_name;
                break;
            }
        case FRAME_SCRIPT:
            {
                YogScriptFrame* f = SCRIPT_FRAME(frame);
                YogCode* code = f->code;
                unsigned int lineno = 0;
                pc_t pc = f->pc - 1;
                unsigned int i = 0;
                for (i = 0; i < code->lineno_tbl_size; i++) {
                    YogLinenoTableEntry* entry = &code->lineno_tbl[i];
                    if ((entry->pc_from <= pc) && (pc < entry->pc_to)) {
                        lineno = entry->lineno;
                        break;
                    }
                }

                entry->lineno = lineno;
                entry->filename = code->filename;
                entry->klass_name = code->klass_name;
                entry->func_name = code->func_name;
                break;
            }
        default:
            YOG_ASSERT(env, FALSE, "Unkown frame type.");
            break;
        }

        entry->lower = st;
        st = entry;

        frame = frame->prev;
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

YogKlass* 
YogException_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, "Exception", ENV_VM(env)->cObject);
    FRAME_DECL_LOCAL(env, klass_idx, OBJ2VAL(klass));
#define UPDATE_PTR  FRAME_LOCAL_OBJ(env, klass, YogKlass, klass_idx)
    UPDATE_PTR;
    YogKlass_define_allocator(env, klass, allocate);

    UPDATE_PTR;
    YogKlass_define_method(env, klass, "initialize", initialize, 0, 0, 0, 0, "message", NULL);
    UPDATE_PTR;
    YogKlass_define_method(env, klass, "to_s", to_s, 0, 0, 0, 0, NULL);

    UPDATE_PTR;
    return klass;
#undef UPDATE_PTR
}

YogException* 
YogBugException_new(YogEnv* env) 
{
    YogKlass* klass = ENV_VM(env)->eBugException;
    YogVal self = OBJ2VAL(klass);
    YogVal obj = YogThread_call_method(env, ENV_TH(env), self, "new", 0, NULL);
    YogException* exc = OBJ_AS(YogException, obj);

    return exc;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
