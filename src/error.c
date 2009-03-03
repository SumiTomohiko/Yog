#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include "yog/error.h"
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
    YogThread* th = ENV_TH(env);
    YogVal jmp_val = YUNDEF;
    if (IS_UNDEF(exc)) {
        jmp_val = th->jmp_val;
    }
    else {
        jmp_val = exc;
    }
    YOG_ASSERT(env, !IS_UNDEF(jmp_val), "jmp_val is undefined.");

    th->jmp_val = jmp_val;
    longjmp(th->jmp_buf_list->buf, JMP_RAISE);
}

static void 
raise_error(YogEnv* env, YogVal klass, const char* msg) 
{
    YogVal args[] = { YogString_new_str(env, msg), };
    YogVal val = YogThread_call_method(env, ENV_TH(env), klass, "new", 1, args);
    YogError_raise(env, val);
}

void 
YogError_raise_type_error(YogEnv* env, const char* msg) 
{
    raise_error(env, ENV_VM(env)->eTypeError, msg);
}

void 
YogError_raise_index_error(YogEnv* env, const char* msg) 
{
    raise_error(env, ENV_VM(env)->eIndexError, msg);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
