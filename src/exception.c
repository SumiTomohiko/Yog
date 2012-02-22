#include "yog/config.h"
#include <errno.h>
#include <string.h>
#include "yog/array.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/code.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/handle.h"
#include "yog/sprintf.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct YogSystemError {
    struct YogException base;
    int_t errno_;
};

typedef struct YogSystemError YogSystemError;

#define TYPE_SYSTEM_ERROR TO_TYPE(YogSystemError_alloc)

static void
YogException_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogException* exc = PTR_AS(YogException, ptr);
#define KEEP(member)    YogGC_KEEP(env, exc, member, keeper, heap)
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

    exc = ALLOC_OBJ(env, YogException_keep_children, NULL, YogException);
    YogException_init(env, exc, TYPE_EXCEPTION, klass);

    RETURN(env, exc);
}

static void
YogSystemError_init(YogEnv* env, YogVal self, type_t type, YogVal klass)
{
    SAVE_ARGS2(env, self, klass);
    YogException_init(env, self, type, klass);
    PTR_AS(YogSystemError, self)->errno_ = 0;
    RETURN_VOID(env);
}

static YogVal
YogSystemError_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal exc = YUNDEF;
    PUSH_LOCAL(env, exc);

    exc = ALLOC_OBJ(env, YogException_keep_children, NULL, YogSystemError);
    YogSystemError_init(env, exc, TYPE_SYSTEM_ERROR, klass);

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
    case FRAME_SCRIPT:
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
get_names_in_c_frame(YogEnv* env, YogVal f, ID* class_name, ID* func_name)
{
    if (IS_PTR(f) && (BASIC_OBJ_TYPE(f) == TYPE_NATIVE_FUNCTION)) {
        *class_name = PTR_AS(YogNativeFunction, f)->class_name;
        *func_name = PTR_AS(YogNativeFunction, f)->func_name;
        return;
    }
    YogHandle* h_f = YogHandle_REGISTER(env, f);
    YogVal s = PTR_AS(YogNativeFunction2, HDL2VAL(h_f))->class_name;
    *class_name = IS_PTR(s) ? YogString_intern(env, s) : INVALID_ID;
    YogVal t = PTR_AS(YogNativeFunction2, HDL2VAL(h_f))->func_name;
    *func_name = IS_PTR(t) ? YogString_intern(env, t) : INVALID_ID;
}

YogVal
YogException_get_stacktrace(YogEnv* env, YogVal frame)
{
    SAVE_ARG(env, frame);
    YogVal st = YNIL;
    YogVal ent = YUNDEF;
    YogVal code = YUNDEF;
    PUSH_LOCALS3(env, st, ent, code);

    while (IS_PTR(frame)) {
        if (PTR_AS(YogFrame, frame)->type == FRAME_FINISH) {
            frame = PTR_AS(YogFrame, frame)->prev;
            continue;
        }

        ent = YogStackTraceEntry_new(env);

        switch (PTR_AS(YogFrame, frame)->type) {
        case FRAME_C:
            {
                ID class_name;
                ID func_name;
                YogVal f = PTR_AS(YogCFrame, frame)->f;
                get_names_in_c_frame(env, f, &class_name, &func_name);
                PTR_AS(YogStackTraceEntry, ent)->lineno = 0;
                PTR_AS(YogStackTraceEntry, ent)->filename = YNIL;
                PTR_AS(YogStackTraceEntry, ent)->class_name = class_name;
                PTR_AS(YogStackTraceEntry, ent)->func_name = func_name;
            }
            break;
        case FRAME_SCRIPT:
            {
                code = PTR_AS(YogScriptFrame, frame)->code;
                uint_t lineno = 0;
                pc_t pc = PTR_AS(YogScriptFrame, frame)->pc - 1;
                uint_t i = 0;
                for (i = 0; i < PTR_AS(YogCode, code)->lineno_tbl_size; i++) {
                    YogVal lineno_tbl = PTR_AS(YogCode, code)->lineno_tbl;
                    YogLinenoTableEntry* e = &PTR_AS(YogLinenoTableEntry, lineno_tbl)[i];
                    if ((e->pc_from <= pc) && (pc < e->pc_to)) {
                        lineno = e->lineno;
                        break;
                    }
                }

                YogVal filename = PTR_AS(YogCode, code)->filename;
                ID class_name = PTR_AS(YogCode, code)->class_name;
                ID func_name = PTR_AS(YogCode, code)->func_name;
                PTR_AS(YogStackTraceEntry, ent)->lineno = lineno;
                YogGC_UPDATE_PTR(env, PTR_AS(YogStackTraceEntry, ent), filename, filename);
                PTR_AS(YogStackTraceEntry, ent)->class_name = class_name;
                PTR_AS(YogStackTraceEntry, ent)->func_name = func_name;
            }
            break;
        case FRAME_FINISH:
        default:
            YOG_ASSERT(env, FALSE, "invalid frame type (0x%x)", PTR_AS(YogFrame, frame)->type);
            break;
        }

        YogGC_UPDATE_PTR(env, PTR_AS(YogStackTraceEntry, ent), lower, st);
        st = ent;

        frame = PTR_AS(YogFrame, frame)->prev;
    }

    RETURN(env, st);
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
    st = YogException_get_stacktrace(env, frame);

    YogGC_UPDATE_PTR(env, PTR_AS(YogException, self), stack_trace, st);
    YogGC_UPDATE_PTR(env, PTR_AS(YogException, self), message, msg);

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
init_SystemError(YogEnv* env, YogVal self, YogVal msg, int_t errno_)
{
    SAVE_ARGS2(env, msg, self);

    init_YogException(env, self, msg);
    PTR_AS(YogSystemError, self)->errno_ = errno_;

    RETURN_VOID(env);
}

static YogVal
join_err_msg(YogEnv* env, const char* msg, YogVal opt)
{
    SAVE_ARG(env, opt);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    if (IS_NIL(opt)) {
        s = YogString_from_string(env, msg);
        RETURN(env, s);
    }

    s = YogSprintf_sprintf(env, "%s - %S", msg, opt);
    RETURN(env, s);
}

static YogVal
SystemError_init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal msg = YUNDEF;
    YogVal errno_ = YUNDEF;
    YogVal opt = YNIL;
    PUSH_LOCALS3(env, msg, errno_, opt);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_SYSTEM_ERROR)) {
        YogError_raise_TypeError(env, "self must be SystemError");
    }
    YogCArg params[] = {
        { "errno", &errno_ },
        { "|", NULL },
        { "opt", &opt },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "init", params, args, kw);

    int_t e = YogVal_to_signed_type(env, errno_, "errno");
    msg = join_err_msg(env, strerror(e), opt);
    init_SystemError(env, self, msg, e);

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

static YogVal
get_errno(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_SYSTEM_ERROR)) {
        YogError_raise_TypeError(env, "self must be SystemError");
    }
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_errno", params, args, kw);

    int_t errno_ = PTR_AS(YogSystemError, self)->errno_;
    val = YogVal_from_int(env, errno_);

    RETURN(env, val);
}

void
YogException_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal eException = YUNDEF;
    YogVal eSystemError = YUNDEF;
    PUSH_LOCALS2(env, eException, eSystemError);
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
    EXCEPTION_NEW(eCoroutineError, "CoroutineError");
    EXCEPTION_NEW(eEOFError, "EOFError");
    EXCEPTION_NEW(eFFIError, "FFIError");
    EXCEPTION_NEW(eIOError, "IOError");
    EXCEPTION_NEW(eImportError, "ImportError");
    EXCEPTION_NEW(eIndexError, "IndexError");
    EXCEPTION_NEW(eKeyError, "KeyError");
    EXCEPTION_NEW(eLocalJumpError, "LocalJumpError");
    EXCEPTION_NEW(eNameError, "NameError");
    EXCEPTION_NEW(eNotImplementedError, "NotImplementedError");
    EXCEPTION_NEW(eOverflowError, "OverflowError");
    EXCEPTION_NEW(eSyntaxError, "SyntaxError");
    EXCEPTION_NEW(eSystemError, "SystemError");
    EXCEPTION_NEW(eTypeError, "TypeError");
    EXCEPTION_NEW(eUnboundLocalError, "UnboundLocalError");
    EXCEPTION_NEW(eValueError, "ValueError");
    EXCEPTION_NEW(eZeroDivisionError, "ZeroDivisionError");
#undef EXCEPTION_NEW

    eSystemError = YogClass_new(env, "SystemError", eException);
    YogClass_define_allocator(env, eSystemError, YogSystemError_alloc);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, eSystemError, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("init", SystemError_init);
#undef DEFINE_METHOD
    YogClass_define_property(env, eSystemError, pkg, "errno", get_errno, NULL);
    vm->eSystemError = eSystemError;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
