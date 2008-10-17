#include <setjmp.h>
#include <stdio.h>
#include "yog/yog.h"

static YogVal 
bltins_raise(YogEnv* env, YogVal recv, int argc, YogVal* args) 
{
    YogThread* th = env->th;
    YogVal jmp_val = YogVal_undef();
    if (argc < 1) {
        jmp_val = th->jmp_val;
    }
    else {
        jmp_val = args[0];
    }
    Yog_assert(env, !IS_UNDEF(jmp_val), "jmp_val is undefined.");

    th->jmp_val = jmp_val;
    longjmp(th->jmp_buf_list->buf, JMP_RAISE);

    /* NOTREACHED */
    return YogVal_nil();
}

static YogVal 
bltins_puts(YogEnv* env, YogVal recv, int argc, YogVal* args) 
{
    if (0 < argc) {
        unsigned int i = 0;
        for (i = 0; i < argc; i++) {
            YogVal val = YogThread_call_method(env, args[i], "to_s", 0, NULL);
            YogString* s = YOGVAL_PTR(val);
            printf("%s", s->body->items);
            printf("\n");
        }
    }
    else {
        printf("\n");
    }

    return YogVal_nil();
}

YogObj* 
Yog_bltins_new(YogEnv* env) 
{
    YogObj* bltins = YogObj_new(env, ENV_VM(env)->pkg_klass);
    YogObj_define_method(env, bltins, "puts", bltins_puts);
    YogObj_define_method(env, bltins, "raise", bltins_raise);

    return bltins;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
