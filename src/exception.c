#include "yog/config.h"
#include <errno.h>
#include <string.h>
#if defined(HAVE_WINDOWS_H)
#   include <windows.h>
#endif
#if defined(HAVE_WINERROR_H)
#   include <winerror.h>
#endif
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
#include "yog/sprintf.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct YogSystemError {
    struct YogException base;
    int_t errno_;
};

typedef struct YogSystemError YogSystemError;

DECL_AS_TYPE(YogSystemError_alloc);
#define TYPE_SYSTEM_ERROR TO_TYPE(YogSystemError_alloc)

struct YogWindowsError {
    struct YogSystemError base;
    uint_t err_code;
};

typedef struct YogWindowsError YogWindowsError;

DECL_AS_TYPE(YogWindowsError_alloc);
#define TYPE_WINDOWS_ERROR TO_TYPE(YogWindowsError_alloc)

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

static void
YogWindowsError_init(YogEnv* env, YogVal self, type_t type, YogVal klass)
{
    SAVE_ARGS2(env, self, klass);
    YogSystemError_init(env, self, type, klass);
    PTR_AS(YogWindowsError, self)->err_code = 0;
    RETURN_VOID(env);
}

static YogVal
YogWindowsError_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal exc = YUNDEF;
    PUSH_LOCAL(env, exc);

    exc = ALLOC_OBJ(env, YogException_keep_children, NULL, YogWindowsError);
    YogWindowsError_init(env, exc, TYPE_WINDOWS_ERROR, klass);

    RETURN(env, exc);
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
                YogVal f = PTR_AS(YogCFrame, frame)->f;
                ID class_name = PTR_AS(YogNativeFunction, f)->class_name;
                ID func_name = PTR_AS(YogNativeFunction, f)->func_name;
                PTR_AS(YogStackTraceEntry, ent)->lineno = 0;
                PTR_AS(YogStackTraceEntry, ent)->filename = YNIL;
                PTR_AS(YogStackTraceEntry, ent)->class_name = class_name;
                PTR_AS(YogStackTraceEntry, ent)->func_name = func_name;
                break;
            }
        case FRAME_METHOD:
        case FRAME_PKG:
        case FRAME_CLASS:
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
                break;
            }
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

static int_t
err_code2errno(uint_t err_code)
{
#if WINDOWS
    struct Entry {
        uint_t err_code;
        int_t errno_;
    };

    typedef struct Entry Entry;

    /* This table came from OCaml 3.11.2 (otherlibs/win32unix/unixsupport.c) */
    Entry table[] = {
        { ERROR_INVALID_FUNCTION, EINVAL },
        { ERROR_FILE_NOT_FOUND, ENOENT },
        { ERROR_PATH_NOT_FOUND, ENOENT },
        { ERROR_TOO_MANY_OPEN_FILES, EMFILE },
        { ERROR_ACCESS_DENIED, EACCES },
        { ERROR_INVALID_HANDLE, EBADF },
        { ERROR_ARENA_TRASHED, ENOMEM },
        { ERROR_NOT_ENOUGH_MEMORY, ENOMEM },
        { ERROR_INVALID_BLOCK, ENOMEM },
        { ERROR_BAD_ENVIRONMENT, E2BIG },
        { ERROR_BAD_FORMAT, ENOEXEC },
        { ERROR_INVALID_ACCESS, EINVAL },
        { ERROR_INVALID_DATA, EINVAL },
        { ERROR_INVALID_DRIVE, ENOENT },
        { ERROR_CURRENT_DIRECTORY, EACCES },
        { ERROR_NOT_SAME_DEVICE, EXDEV },
        { ERROR_NO_MORE_FILES, ENOENT },
        { ERROR_LOCK_VIOLATION, EACCES },
        { ERROR_BAD_NETPATH, ENOENT },
        { ERROR_NETWORK_ACCESS_DENIED, EACCES },
        { ERROR_BAD_NET_NAME, ENOENT },
        { ERROR_FILE_EXISTS, EEXIST },
        { ERROR_CANNOT_MAKE, EACCES },
        { ERROR_FAIL_I24, EACCES },
        { ERROR_INVALID_PARAMETER, EINVAL },
        { ERROR_NO_PROC_SLOTS, EAGAIN },
        { ERROR_DRIVE_LOCKED, EACCES },
        { ERROR_BROKEN_PIPE, EPIPE },
        { ERROR_NO_DATA, EPIPE },
        { ERROR_DISK_FULL, ENOSPC },
        { ERROR_INVALID_TARGET_HANDLE, EBADF },
        { ERROR_INVALID_HANDLE, EINVAL },
        { ERROR_WAIT_NO_CHILDREN, ECHILD },
        { ERROR_CHILD_NOT_COMPLETE, ECHILD },
        { ERROR_DIRECT_ACCESS_HANDLE, EBADF },
        { ERROR_NEGATIVE_SEEK, EINVAL },
        { ERROR_SEEK_ON_DEVICE, EACCES },
        { ERROR_DIR_NOT_EMPTY, ENOTEMPTY },
        { ERROR_NOT_LOCKED, EACCES },
        { ERROR_BAD_PATHNAME, ENOENT },
        { ERROR_MAX_THRDS_REACHED, EAGAIN },
        { ERROR_LOCK_FAILED, EACCES },
        { ERROR_ALREADY_EXISTS, EEXIST },
        { ERROR_FILENAME_EXCED_RANGE, ENOENT },
        { ERROR_NESTING_NOT_ALLOWED, EAGAIN },
        { ERROR_NOT_ENOUGH_QUOTA, ENOMEM },
        { ERROR_INVALID_STARTING_CODESEG, ENOEXEC },
        { ERROR_INVALID_STACKSEG, ENOEXEC },
        { ERROR_INVALID_MODULETYPE, ENOEXEC },
        { ERROR_INVALID_EXE_SIGNATURE, ENOEXEC },
        { ERROR_EXE_MARKED_INVALID, ENOEXEC },
        { ERROR_BAD_EXE_FORMAT, ENOEXEC },
        { ERROR_ITERATED_DATA_EXCEEDS_64k, ENOEXEC },
        { ERROR_INVALID_MINALLOCSIZE, ENOEXEC },
        { ERROR_DYNLINK_FROM_INVALID_RING, ENOEXEC },
        { ERROR_IOPL_NOT_ENABLED, ENOEXEC },
        { ERROR_INVALID_SEGDPL, ENOEXEC },
        { ERROR_AUTODATASEG_EXCEEDS_64k, ENOEXEC },
        { ERROR_RING2SEG_MUST_BE_MOVABLE, ENOEXEC },
        { ERROR_RELOC_CHAIN_XEEDS_SEGLIM, ENOEXEC },
        { ERROR_INFLOOP_IN_RELOC_CHAIN, ENOEXEC },
        { ERROR_WRITE_PROTECT, EACCES },
        { ERROR_BAD_UNIT, EACCES },
        { ERROR_NOT_READY, EACCES },
        { ERROR_BAD_COMMAND, EACCES },
        { ERROR_CRC, EACCES },
        { ERROR_BAD_LENGTH, EACCES },
        { ERROR_SEEK, EACCES },
        { ERROR_NOT_DOS_DISK, EACCES },
        { ERROR_SECTOR_NOT_FOUND, EACCES },
        { ERROR_OUT_OF_PAPER, EACCES },
        { ERROR_WRITE_FAULT, EACCES },
        { ERROR_READ_FAULT, EACCES },
        { ERROR_GEN_FAILURE, EACCES },
        { ERROR_SHARING_VIOLATION, EACCES },
        { ERROR_LOCK_VIOLATION, EACCES },
        { ERROR_WRONG_DISK, EACCES },
        { ERROR_SHARING_BUFFER_EXCEEDED, EACCES },
        { WSAEINVAL, EINVAL },
        { WSAEACCES, EACCES },
        { WSAEBADF, EBADF },
        { WSAEFAULT, EFAULT },
        { WSAEINTR, EINTR },
        { WSAEINVAL, EINVAL },
        { WSAEMFILE, EMFILE },
#   if defined(WSANAMETOOLONG)
        { WSANAMETOOLONG, ENAMETOOLONG },
#   endif
#   if defined(WSAENFILE)
        { WSAENFILE, ENFILE },
#   endif
        { WSAENOTEMPTY, ENOTEMPTY } };
    uint_t i;
    for (i = 0; i < array_sizeof(table); i++) {
        if (table[i].err_code == err_code) {
            return table[i].errno_;
        }
    }
#endif
    return EINVAL;
}

static YogVal
join_err_msg(YogEnv* env, const char* msg, YogVal opt)
{
    SAVE_ARG(env, opt);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    if (IS_NIL(opt)) {
        s = YogString_from_str(env, msg);
        RETURN(env, s);
    }

    s = YogSprintf_sprintf(env, "%s - %S", msg, opt);
    RETURN(env, s);
}

static void
init_WindowsError(YogEnv* env, YogVal self, uint_t err_code, YogVal opt)
{
    SAVE_ARGS2(env, self, opt);
    YogVal msg = YUNDEF;
    PUSH_LOCAL(env, msg);

    int_t errno_ = err_code2errno(err_code);
#if WINDOWS
    char* buf = NULL;
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err_code, 0, (PTSTR)&buf, 0, NULL) != 0) {
        msg = join_err_msg(env, buf, opt);
        LocalFree((HLOCAL)buf);
    }
    else {
        msg = opt;
    }
#else
    msg = join_err_msg(env, strerror(errno_), opt);
#endif
    init_SystemError(env, self, msg, errno_);
    PTR_AS(YogWindowsError, self)->err_code = err_code;

    RETURN_VOID(env);
}

static YogVal
WindowsError_init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal err_code = YUNDEF;
    YogVal opt = YNIL;
    PUSH_LOCALS2(env, err_code, opt);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_WINDOWS_ERROR)) {
        YogError_raise_TypeError(env, "self must be WindowsError");
    }
    YogCArg params[] = {
        { "err_code", &err_code },
        { "|", NULL },
        { "opt", &opt },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "init", params, args, kw);

    uint_t e = (uint_t)YogVal_to_signed_type(env, err_code, "err_code");
    init_WindowsError(env, self, e, opt);

    RETURN(env, YNIL);
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
    if (!IS_PTR(self) || ((BASIC_OBJ_TYPE(self) != TYPE_SYSTEM_ERROR) && (BASIC_OBJ_TYPE(self) != TYPE_WINDOWS_ERROR))) {
        YogError_raise_TypeError(env, "self must be SystemError or WindowsError");
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
    YogVal eWindowsError = YUNDEF;
    YogVal exc = YUNDEF;
    PUSH_LOCALS4(env, eException, eSystemError, eWindowsError, exc);
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
    EXCEPTION_NEW(eIOError, "IOError");
    EXCEPTION_NEW(eImportError, "ImportError");
    EXCEPTION_NEW(eIndexError, "IndexError");
    EXCEPTION_NEW(eKeyError, "KeyError");
    EXCEPTION_NEW(eLocalJumpError, "LocalJumpError");
    EXCEPTION_NEW(eNameError, "NameError");
    EXCEPTION_NEW(eSyntaxError, "SyntaxError");
    EXCEPTION_NEW(eSystemError, "SystemError");
    EXCEPTION_NEW(eTypeError, "TypeError");
    EXCEPTION_NEW(eValueError, "ValueError");
    EXCEPTION_NEW(eWindowsError, "WindowsError");
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

    eWindowsError = YogClass_new(env, "WindowsError", eSystemError);
    YogClass_define_allocator(env, eWindowsError, YogWindowsError_alloc);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, eWindowsError, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("init", WindowsError_init);
#undef DEFINE_METHOD
    vm->eWindowsError = eWindowsError;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
