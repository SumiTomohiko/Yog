#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yog/class.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/string.h"
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
YogError_out_of_memory(YogEnv* env)
{
    FILE* out = stderr;
    fprintf(out, "[ERROR]\nout of memory\n");
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
YogError_bug(YogEnv* env, const char* filename, uint_t lineno, const char* fmt, ...)
{
    PRINT_ERROR(env, "BUG", filename, lineno, fmt);
    fflush(stderr);
    abort();
}

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
    YogVal jmp_val = YUNDEF;
    if (IS_UNDEF(exc)) {
        jmp_val = PTR_AS(YogThread, thread)->jmp_val;
    }
    else {
        jmp_val = exc;
    }
    YOG_ASSERT(env, !IS_UNDEF(jmp_val), "jmp_val is undefined.");

    PTR_AS(YogThread, thread)->jmp_val = jmp_val;
    longjmp(PTR_AS(YogThread, thread)->jmp_buf_list->buf, JMP_RAISE);
}

static void
raise_error(YogEnv* env, YogVal klass, const char* msg)
{
    SAVE_ARG(env, klass);

    YogVal args[] = { YogString_new_str(env, msg), };
    PUSH_LOCALSX(env, 1, args);
    YogVal val = YogEval_call_method(env, klass, "new", 1, args);
    RESTORE_LOCALS(env);
    YogError_raise(env, val);
}

static void
raise_format(YogEnv* env, YogVal klass, const char* fmt, va_list ap)
{
    /**
     * Don't GC before making error message. Some arguments in ap are GC object.
     */
    char buffer[4096];
#if defined(_MSC_VER)
#   define vsnprintf(buffer, size, fmt, ap)     vsprintf(buffer, fmt, ap)
#endif
    vsnprintf(buffer, array_sizeof(buffer), fmt, ap);
    raise_error(env, klass, buffer);
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
    YogVal s = YUNDEF;
    YogVal t = YUNDEF;
    YogVal name = YUNDEF;
    YogVal exc = YUNDEF;
    YogVal st = YUNDEF;
    YogVal filename = YUNDEF;
    YogVal klass = YUNDEF;
    YogVal msg = YUNDEF;
    PUSH_LOCALS8(env, s, t, name, exc, st, filename, klass, msg);

    exc = PTR_AS(YogThread, env->thread)->jmp_val;
    st = PTR_AS(YogException, exc)->stack_trace;
    if (IS_PTR(st)) {
        fprintf(stderr, "Traceback (most recent call last):\n");
    }
#define ID2NAME(id)     YogVM_id2name(env, env->vm, id)
    while (IS_PTR(st)) {
        fprintf(stderr, "  File ");
        filename = PTR_AS(YogStackTraceEntry, st)->filename;
        if (IS_PTR(filename)) {
            const char* name = PTR_AS(YogCharArray, filename)->items;
            fprintf(stderr, "\"%s\"", name);
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
                s = ID2NAME(class_name);
                t = ID2NAME(func_name);
                fprintf(stderr, "%s#%s", STRING_CSTR(s), STRING_CSTR(t));
            }
            else {
                name = ID2NAME(class_name);
                fprintf(stderr, "<class %s>", STRING_CSTR(name));
            }
        }
        else {
            name = ID2NAME(func_name);
            fprintf(stderr, "%s", STRING_CSTR(name));
        }
        fprintf(stderr, "\n");

        st = PTR_AS(YogStackTraceEntry, st)->lower;
    }

    klass = YOGBASICOBJ(exc)->klass;
    ID id = PTR_AS(YogClass, klass)->name;
    name = ID2NAME(id);
#undef ID2NAME
    msg = YogEval_call_method(env, PTR_AS(YogException, exc)->message, "to_s", 0, NULL);
    fprintf(stderr, "%s: %s\n", STRING_CSTR(name), STRING_CSTR(msg));

    RETURN_VOID(env);
}

static void
raise_TypeError(YogEnv* env, const char* msg, YogVal left, YogVal right)
{
    SAVE_ARGS2(env, left, right);
    YogVal left_class = YUNDEF;
    YogVal right_class = YUNDEF;
    YogVal left_name = YUNDEF;
    YogVal right_name = YUNDEF;
    PUSH_LOCALS4(env, left_class, right_class, left_name, right_name);

    left_class = YogVal_get_class(env, left);
    right_class = YogVal_get_class(env, right);
    left_name = YogVM_id2name(env, env->vm, PTR_AS(YogClass, left_class)->name);
    right_name = YogVM_id2name(env, env->vm, PTR_AS(YogClass, right_class)->name);

    YogError_raise_TypeError(env, msg, STRING_CSTR(left_name), STRING_CSTR(right_name));

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

    char buffer[4096];
#if defined(_MSC_VER)
#   define snprintf(buffer, size, fmt, arg)    sprintf(buffer, fmt, arg)
#endif
    snprintf(buffer, array_sizeof(buffer), "unsupported operand type(s) for %s: '%%s' and '%%s'", escaped_opname);
    raise_TypeError(env, buffer, left, right);
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
YogError_raise_NameError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eNameError, fmt);
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
YogError_raise_comparison_type_error(YogEnv* env, YogVal left, YogVal right)
{
    raise_TypeError(env, "comparison of %s with %s failed", left, right);
}

void
YogError_raise_LocalJumpError(YogEnv* env, const char* fmt, ...)
{
    RAISE_FORMAT(env, eLocalJumpError, fmt);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
