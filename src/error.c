#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/klass.h"
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
    fprintf(stderr, "[ERROR]\nout of memory\n");
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
    char buffer[4096];
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
#define PRINT(...)  fprintf(stderr, __VA_ARGS__)
    YogVal exc = PTR_AS(YogThread, env->thread)->jmp_val;
    YogVal st = PTR_AS(YogException, exc)->stack_trace;
    if (IS_PTR(st)) {
        PRINT("Traceback (most recent call last):\n");
    }
#define ID2NAME(id)     YogVM_id2name(env, env->vm, id)
    while (IS_PTR(st)) {
        PRINT("  File ");
        YogVal filename = PTR_AS(YogStackTraceEntry, st)->filename;
        if (IS_PTR(filename)) {
            const char* name = PTR_AS(YogCharArray, filename)->items;
            PRINT("\"%s\"", name);
        }
        else {
            PRINT("builtin");
        }

        uint_t lineno = PTR_AS(YogStackTraceEntry, st)->lineno;
        if (0 < lineno) {
            PRINT(", line %d", lineno);
        }

        PRINT(", in ");
        ID klass_name = PTR_AS(YogStackTraceEntry, st)->klass_name;
        ID func_name = PTR_AS(YogStackTraceEntry, st)->func_name;
        if (klass_name != INVALID_ID) {
            if (func_name != INVALID_ID) {
                const char* s = ID2NAME(klass_name);
                const char* t = ID2NAME(func_name);
                PRINT("%s#%s", s, t);
            }
            else {
                const char* name = ID2NAME(klass_name);
                PRINT("<class %s>", name);
            }
        }
        else {
            const char* name = ID2NAME(func_name);
            PRINT("%s", name);
        }
        PRINT("\n");

        st = PTR_AS(YogStackTraceEntry, st)->lower;
    }

    YogVal klass = YOGBASICOBJ(exc)->klass;
    const char* name = ID2NAME(PTR_AS(YogKlass, klass)->name);
    /* dirty hack */
    size_t len = strlen(name);
    char s[len + 1];
    strcpy(s, name);
#undef ID2NAME
    YogVal val = YogEval_call_method(env, PTR_AS(YogException, exc)->message, "to_s", 0, NULL);
    YogString* msg = PTR_AS(YogString, val);
    PRINT("%s: %s\n", s, PTR_AS(YogCharArray, msg->body)->items);
#undef PRINT
}

static void
raise_TypeError(YogEnv* env, const char* msg, YogVal left, YogVal right)
{
    SAVE_ARGS2(env, left, right);
    YogVal left_klass = YUNDEF;
    YogVal right_klass = YUNDEF;
    PUSH_LOCALS2(env, left_klass, right_klass);

    left_klass = YogVal_get_klass(env, left);
    right_klass = YogVal_get_klass(env, right);
    const char* left_name = YogVM_id2name(env, env->vm, PTR_AS(YogKlass, left_klass)->name);
    const char* right_name = YogVM_id2name(env, env->vm, PTR_AS(YogKlass, right_klass)->name);

    YogError_raise_TypeError(env, msg, left_name, right_name);

    RETURN_VOID(env);
}

void
YogError_raise_binop_type_error(YogEnv* env, YogVal left, YogVal right, const char* opname)
{
    char buffer[4096];
    snprintf(buffer, array_sizeof(buffer), "unsupported operand type(s) for %s: '%%s' and '%%s'", opname);
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
