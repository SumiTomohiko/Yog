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

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
