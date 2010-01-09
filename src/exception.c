#include <errno.h>
#include <string.h>
#include "yog/array.h"
#include "yog/class.h"
#include "yog/code.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct SystemCallError {
    struct YogException base;
    int errno_;
};

typedef struct SystemCallError SystemCallError;

#define TYPE_SYSTEM_CALL_ERROR  ((type_t)SystemCallError_alloc)

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogException* exc = PTR_AS(YogException, ptr);
#define KEEP(member)    YogGC_keep(env, &exc->member, keeper, heap)
    KEEP(stack_trace);
    KEEP(message);
#undef KEEP
}

static void
YogException_init(YogEnv* env, YogVal self, type_t type, YogVal klass)
{
    SAVE_ARGS2(env, self, klass);

    YogBasicObj_init(env, self, type, 0, klass);
    PTR_AS(YogException, self)->stack_trace = YUNDEF;
    PTR_AS(YogException, self)->message = YUNDEF;

    RETURN_VOID(env);
}

static YogVal
YogException_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal exc = YUNDEF;
    PUSH_LOCAL(env, exc);

    exc = ALLOC_OBJ(env, keep_children, NULL, YogException);
    YogException_init(env, exc, TYPE_EXCEPTION, klass);

    RETURN(env, exc);
}

static YogVal
SystemCallError_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal exc = YUNDEF;
    PUSH_LOCAL(env, exc);

    exc = ALLOC_OBJ(env, keep_children, NULL, SystemCallError);
    YogException_init(env, exc, TYPE_SYSTEM_CALL_ERROR, klass);
    PTR_AS(SystemCallError, exc)->errno_ = 0;

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
    case FRAME_PKG:
    case FRAME_CLASS:
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

static void
init_YogException(YogEnv* env, YogVal self, YogVal msg)
{
    SAVE_ARGS2(env, self, msg);
    YogVal frame = YUNDEF;
    YogVal st = YUNDEF;
    PUSH_LOCALS2(env, frame, st);

    frame = env->frame;
    frame = skip_frame(env, frame, "init");
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
        case FRAME_PKG:
        case FRAME_CLASS:
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
    PTR_AS(YogException, self)->message = msg;

    RETURN_VOID(env);
}

static YogVal
Exception_init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal msg = YNIL;
    PUSH_LOCAL(env, msg);

    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_EXCEPTION)) {
        YogError_raise_TypeError(env, "self must be Exception");
    }
    YogCArg params[] = { { "|", NULL }, { "message", &msg }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "init", params, args, kw);
    init_YogException(env, self, msg);

    RETURN(env, YNIL);
}

static void
init_SystemCallError(YogEnv* env, YogVal self, int errno_)
{
    SAVE_ARG(env, self);
    YogVal msg = YUNDEF;
    PUSH_LOCAL(env, msg);

    msg = YogString_from_str(env, strerror(errno_));
    init_YogException(env, self, msg);
    PTR_AS(SystemCallError, self)->errno_ = errno_;

    RETURN_VOID(env);
}

static YogVal
SystemCallError_init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal msg = YUNDEF;
    YogVal errno_ = YUNDEF;
    PUSH_LOCALS2(env, msg, errno_);

    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_SYSTEM_CALL_ERROR)) {
        YogError_raise_TypeError(env, "self must be SystemCallError");
    }
    YogCArg params[] = { { "errno", &errno_ }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "init", params, args, kw);

    int e = YogVal_to_signed_type(env, errno_, "errno");
    init_SystemCallError(env, self, e);

    RETURN(env, YNIL);
}

static YogVal
to_s(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal msg = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, msg, retval);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_s", params, args, kw);

    msg = PTR_AS(YogException, self)->message;
    retval = YogEval_call_method(env, msg, "to_s", 0, NULL);

    RETURN(env, retval);
}

static YogVal
get_message(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal message = YUNDEF;
    PUSH_LOCAL(env, message);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_message", params, args, kw);

    message = PTR_AS(YogException, self)->message;

    RETURN(env, message);
}

#if !defined(MINIYOG)
static void
construct_sys_call_err(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block, int errno_)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "init", params, args, kw);

    init_SystemCallError(env, self, errno_);

    RETURN_VOID(env);
}
#   include "errno_cons.inc"
#endif

void
YogException_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal eException = YUNDEF;
    YogVal eSystemCallError = YUNDEF;
    PUSH_LOCALS2(env, eException, eSystemCallError);
    YogVM* vm = env->vm;

    eException = YogClass_new(env, "Exception", vm->cObject);
    YogClass_define_allocator(env, eException, YogException_alloc);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, eException, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("init", Exception_init);
    DEFINE_METHOD("to_s", to_s);
#undef DEFINE_METHOD
#define DEFINE_PROP(name, getter, setter)   do { \
    YogClass_define_property(env, eException, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("message", get_message, NULL);
#undef DEFINE_PROP
    vm->eException = eException;

#define EXCEPTION_NEW(member, name)  do { \
    vm->member = YogClass_new(env, name, eException); \
} while (0)
    EXCEPTION_NEW(eArgumentError, "ArgumentError");
    EXCEPTION_NEW(eAttributeError, "AttributeError");
    EXCEPTION_NEW(eBugError, "BugError");
    EXCEPTION_NEW(eEOFError, "EOFError");
    EXCEPTION_NEW(eImportError, "ImportError");
    EXCEPTION_NEW(eIndexError, "IndexError");
    EXCEPTION_NEW(eKeyError, "KeyError");
    EXCEPTION_NEW(eLocalJumpError, "LocalJumpError");
    EXCEPTION_NEW(eNameError, "NameError");
    EXCEPTION_NEW(eSyntaxError, "SyntaxError");
    EXCEPTION_NEW(eTypeError, "TypeError");
    EXCEPTION_NEW(eValueError, "ValueError");
    EXCEPTION_NEW(eZeroDivisionError, "ZeroDivisionError");
#undef EXCEPTION_NEW

    eSystemCallError = YogClass_new(env, "SystemCallError", eException);
    YogClass_define_allocator(env, eSystemCallError, SystemCallError_alloc);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, eSystemCallError, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("init", SystemCallError_init);
#undef DEFINE_METHOD
#if !defined(MINIYOG)
#   define EXCEPTION_NEW(member, name, f)  do { \
    YogVal member; \
    PUSH_LOCAL(env, member); \
    member = YogClass_new(env, name, eSystemCallError); \
    YogClass_define_method(env, member, pkg, "init", (f)); \
    vm->member = member; \
    POP_LOCALS(env); \
} while (0)
#   include "errno_new.inc"
#   undef EXCEPTION_NEW
#endif

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
