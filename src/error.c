#include "yog/config.h"
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yog/binary.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/sprintf.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define RAISE_FORMAT(env, type, fmt)  do { \
    va_list ap; \
    va_start(ap, fmt); \
    raise_format((env), (env)->vm->type, (fmt), ap); \
    va_end(ap); \
} while (0)

void
YogError_out_of_memory(YogEnv* env, size_t size)
{
    FILE* out = stderr;
    fprintf(out, "[ERROR]\n");
    fprintf(out, "Out of memory: requested size=%u\n", size);
    fflush(out);
    abort();
}

static void
print_error(YogEnv* env, const char* type, const char* filename, uint_t lineno, const char* fmt, va_list ap)
{
    FILE* stream = stderr;

    fprintf(stream, "[%s]\n", type);
    fprintf(stream, "at %s:%d\n", filename, lineno);
    vfprintf(stream, fmt, ap);
    fprintf(stream, "\n");
}

#define PRINT_ERROR(env, type, filename, lineno, fmt)   do { \
    va_list ap; \
    va_start(ap, fmt); \
    print_error(env, type, filename, lineno, fmt, ap); \
    va_end(ap); \
} while (0)

void
YogError_warn(YogEnv* env, const char* filename, uint_t lineno, const char* fmt, ...)
{
    PRINT_ERROR(env, "WARN", filename, lineno, fmt);
}

#undef PRINT_ERROR

void
YogError_raise(YogEnv* env, YogVal exc)
{
    YogVal thread = env->thread;
    YogVal jmp_val = IS_UNDEF(exc) ? PTR_AS(YogThread, thread)->jmp_val : exc;
    YOG_ASSERT(env, !IS_UNDEF(jmp_val), "jmp_val is undefined.");

    YogGC_UPDATE_PTR(env, PTR_AS(YogThread, thread), jmp_val, jmp_val);
    YogEval_longjmp(env, JMP_RAISE);
    /* NOTREACHED */
}

static void
print_stacktrace(YogEnv* env, YogVal st)
{
    SAVE_ARG(env, st);
    YogVal filename = YUNDEF;
    YogVal s = YUNDEF;
    YogVal t = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS4(env, filename, s, t, name);
    if (!IS_PTR(st)) {
        RETURN_VOID(env);
    }

    fprintf(stderr, "Traceback (most recent call last):\n");
#define ID2BIN(id) YogVM_id2bin(env, env->vm, id)
    while (IS_PTR(st)) {
        fprintf(stderr, "  File ");
        filename = PTR_AS(YogStackTraceEntry, st)->filename;
        if (IS_PTR(filename)) {
            YogHandle* h = YogHandle_REGISTER(env, filename);
            YogVal bin = YogString_to_bin_in_default_encoding(env, h);
            fprintf(stderr, "\"%s\"", BINARY_CSTR(bin));
        }
        else {
            fprintf(stderr, "builtin");
        }

        uint_t lineno = PTR_AS(YogStackTraceEntry, st)->lineno;
        if (0 < lineno) {
            fprintf(stderr, ", line %d", lineno);
        }

        fprintf(stderr, ", in ");
        ID class_name = PTR_AS(YogStackTraceEntry, st)->class_name;
        ID func_name = PTR_AS(YogStackTraceEntry, st)->func_name;
        if (class_name != INVALID_ID) {
            if (func_name != INVALID_ID) {
                s = ID2BIN(class_name);
                t = ID2BIN(func_name);
                fprintf(stderr, "%s#%s", BINARY_CSTR(s), BINARY_CSTR(t));
            }
            else {
                fprintf(stderr, "<class %s>", BINARY_CSTR(ID2BIN(class_name)));
            }
        }
        else {
            fprintf(stderr, "%s", BINARY_CSTR(ID2BIN(func_name)));
        }
        fprintf(stderr, "\n");

        st = PTR_AS(YogStackTraceEntry, st)->lower;
    }

    RETURN_VOID(env);
}

void
YogError_bug(YogEnv* env, const char* filename, uint_t lineno, const char* fmt, ...)
{
    if ((env != NULL) && (!env->vm->running_gc)) {
        SAVE_LOCALS(env);
        YogVal st = YUNDEF;
        PUSH_LOCAL(env, st);

        st = YogException_get_stacktrace(env, env->frame);
        print_stacktrace(env, st);

        RESTORE_LOCALS(env);
    }
    va_list ap;
    va_start(ap, fmt);
    print_error(env, "BUG", filename, lineno, fmt, ap);
    va_end(ap);
    abort();
    /* NOTREACHED */
}

static void
raise_error(YogEnv* env, YogVal klass, YogVal msg)
{
    SAVE_ARGS2(env, klass, msg);
    YogVal exc = YUNDEF;
    PUSH_LOCAL(env, exc);

    exc = YogEval_call_method1(env, klass, "new", msg);

    RESTORE_LOCALS(env);
    YogError_raise(env, exc);
}

static void
raise_format(YogEnv* env, YogVal klass, const char* fmt, va_list ap)
{
    SAVE_ARG(env, klass);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    s = YogSprintf_vsprintf(env, fmt, ap);
    raise_error(env, klass, s);

    RETURN_VOID(env);
}

void
YogError_raise_UnboundLocalError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eUnboundLocalError, fmt);
}

void
YogError_raise_TypeError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eTypeError, fmt);
}

void
YogError_raise_IndexError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eIndexError, fmt);
}

void
YogError_raise_IOError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eIOError, fmt);
}

void
YogError_raise_SyntaxError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eSyntaxError, fmt);
}

void
YogError_raise_ValueError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eValueError, fmt);
}

void
YogError_print_stacktrace(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal name = YUNDEF;
    YogVal exc = YUNDEF;
    YogVal st = YUNDEF;
    YogVal klass = YUNDEF;
    YogVal msg_str = YUNDEF;
    PUSH_LOCALS5(env, name, exc, st, klass, msg_str);

    exc = PTR_AS(YogThread, env->thread)->jmp_val;
    st = PTR_AS(YogException, exc)->stack_trace;
    print_stacktrace(env, st);

    klass = BASIC_OBJ(exc)->klass;
    ID id = PTR_AS(YogClass, klass)->name;
    name = ID2BIN(id);
#undef ID2BIN
    YogVal msg = PTR_AS(YogException, exc)->message;
    YogHandle* h = VAL2HDL(env, YogEval_call_method0(env, msg, "to_s"));
    YogVal bin = YogString_to_bin_in_default_encoding(env, h);
    fprintf(stderr, "%s: %s\n", BINARY_CSTR(name), BINARY_CSTR(bin));

    RETURN_VOID(env);
}

void
YogError_raise_binop_type_error(YogEnv* env, YogVal left, YogVal right, const char* opname)
{
    const char* escaped_opname;
    if (strcmp("%", opname) == 0) {
        escaped_opname = "%%";
    }
    else {
        escaped_opname = opname;
    }

    char buffer[128];
    YogSysdeps_snprintf(buffer, array_sizeof(buffer), "unsupported operand type(s) for %s: %%C and %%C", escaped_opname);
    YogError_raise_TypeError(env, buffer, left, right);
}

static void
raise_TypeError(YogEnv* env, const char* mark, const char* needed, YogVal actual)
{
    const char* fmt = "Argument after %s must be %s, not %C";
    YogError_raise_TypeError(env, fmt, mark, needed, actual);
}

void
YogError_raise_TypeError_for_vararg(YogEnv* env, YogVal actual)
{
    raise_TypeError(env, "*", "an Array", actual);
}

void
YogError_raise_TypeError_for_varkwarg(YogEnv* env, YogVal actual)
{
    raise_TypeError(env, "**", "a Dict", actual);
}

void
YogError_raise_ZeroDivisionError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eZeroDivisionError, fmt);
}

void
YogError_raise_ArgumentError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eArgumentError, fmt);
}

void
YogError_raise_NameError(YogEnv* env, ID name)
{
    SAVE_LOCALS(env);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    s = YogSprintf_sprintf(env, "Name \"%I\" is not defined", name);
    raise_error(env, env->vm->eNameError, s);

    RETURN_VOID(env);
}

void
YogError_raise_ImportError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eImportError, fmt);
}

void
YogError_raise_AttributeError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eAttributeError, fmt);
}

void
YogError_raise_FFIError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eFFIError, fmt);
}

void
YogError_raise_KeyError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eKeyError, fmt);
}

void
YogError_raise_EOFError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eEOFError, fmt);
}

void
YogError_raise_CoroutineError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eCoroutineError, fmt);
}

void
YogError_raise_comparison_type_error(YogEnv* env, YogVal left, YogVal right)
{
    YogError_raise_TypeError(env, "comparison of %C with %C failed", left, right);
}

void
YogError_raise_LocalJumpError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eLocalJumpError, fmt);
}

static void
raise_sys_err(YogEnv* env, YogVal klass, int_t errno_, YogVal opt)
{
    SAVE_ARGS2(env, klass, opt);
    YogVal exc = YUNDEF;
    PUSH_LOCAL(env, exc);
    YogVal args[] = { YUNDEF, YUNDEF };
    PUSH_LOCALSX(env, array_sizeof(args), args);

    args[0] = YogVal_from_int(env, errno_);
    args[1] = opt;
    exc = YogEval_call_method(env, klass, "new", array_sizeof(args), args);

    RESTORE_LOCALS(env);
    YogError_raise(env, exc);
    /* NOTREACHED */
}

void
YogError_raise_sys_err(YogEnv* env, int_t errno_, YogVal opt)
{
    SAVE_ARG(env, opt);
    raise_sys_err(env, env->vm->eSystemError, errno_, opt);
    RETURN_VOID(env);
}

void
YogError_raise_sys_err2(YogEnv* env, int_t err_code, YogVal opt)
{
    SAVE_ARG(env, opt);
#if WINDOWS
    raise_sys_err(env, env->vm->eWindowsError, err_code, opt);
#else
    YogError_raise_sys_err(env, err_code, opt);
#endif
    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
