#include <setjmp.h>
#include "yog/yog.h"

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

void 
YogError_raise_type_error(YogEnv* env, const char* msg) 
{
    YogKlass* klass = ENV_VM(env)->eTypeError;
    YogVal self = OBJ2VAL(klass);

    YogString* s = YogString_new_str(env, msg);
    YogVal arg = OBJ2VAL(s);
    YogVal args[] = { arg, };

    YogVal val = YogThread_call_method(env, ENV_TH(env), self, "new", 1, args);

    YogError_raise(env, val);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
