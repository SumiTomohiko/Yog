#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

static void
print_error(YogEnv* env, const char* type, const char* filename, unsigned int lineno, const char* fmt, va_list ap)
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
YogError_bug(YogEnv* env, const char* filename, unsigned int lineno, const char* fmt, ...) 
{
    PRINT_ERROR(env, "BUG", filename, lineno, fmt);
    abort();
}

void
YogError_warn(YogEnv* env, const char* filename, unsigned int lineno, const char* fmt, ...)
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

void 
YogError_raise_TypeError(YogEnv* env, const char* msg) 
{
    raise_error(env, env->vm->eTypeError, msg);
}

void 
YogError_raise_IndexError(YogEnv* env, const char* msg) 
{
    raise_error(env, env->vm->eIndexError, msg);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
