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

void 
YogError_bug(YogEnv* env, const char* filename, unsigned int lineno, const char* fmt, ...) 
{
    FILE* stream = stderr;

    fprintf(stream, "[BUG]\n");
    fprintf(stream, "at %s:%d\n", filename, lineno);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stream, fmt, ap);
    va_end(ap);

    fprintf(stream, "\n");

    abort();
}

void 
YogError_raise(YogEnv* env, YogVal exc) 
{
    YogThreadCtx* thread_ctx = env->thread_ctx;
    YogVal jmp_val = YUNDEF;
    if (IS_UNDEF(exc)) {
        jmp_val = thread_ctx->jmp_val;
    }
    else {
        jmp_val = exc;
    }
    YOG_ASSERT(env, !IS_UNDEF(jmp_val), "jmp_val is undefined.");

    thread_ctx->jmp_val = jmp_val;
    longjmp(thread_ctx->jmp_buf_list->buf, JMP_RAISE);
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
YogError_raise_type_error(YogEnv* env, const char* msg) 
{
    raise_error(env, env->vm->eTypeError, msg);
}

void 
YogError_raise_index_error(YogEnv* env, const char* msg) 
{
    raise_error(env, env->vm->eIndexError, msg);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
