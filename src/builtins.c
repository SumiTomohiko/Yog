#include <setjmp.h>
#include <stdio.h>
#include "yog/yog.h"

static YogVal 
bltins_raise(YogEnv* env, YogVal self, YogVal exc)
{
    YogThread* th = env->th;
    YogVal jmp_val = YogVal_undef();
    if (IS_UNDEF(exc)) {
        jmp_val = th->jmp_val;
    }
    else {
        jmp_val = exc;
    }
    Yog_assert(env, !IS_UNDEF(jmp_val), "jmp_val is undefined.");

    th->jmp_val = jmp_val;
    longjmp(th->jmp_buf_list->buf, JMP_RAISE);

    /* NOTREACHED */
    return YogVal_nil();
}

static YogVal 
bltins_puts(YogEnv* env, YogVal self, YogArray* vararg)
{
    unsigned int size = YogArray_size(env, vararg);
    if (0 < size) {
        unsigned int i = 0;
        for (i = 0; i < size; i++) {
            YogString* s = NULL;
            YogVal arg = YogArray_at(env, vararg, i);
            if (IS_OBJ(arg) && (VAL2OBJ(arg)->klass == ENV_VM(env)->string_klass)) {
                s = (YogString*)VAL2OBJ(arg);
            }
            else {
                YogVal val = YogThread_call_method(env, ENV_TH(env), arg, "to_s", 0, NULL);
                s = VAL2PTR(val);
            }
            printf("%s", s->body->items);
            printf("\n");
        }
    }
    else {
        printf("\n");
    }

    return YogVal_nil();
}

YogPkg* 
Yog_bltins_new(YogEnv* env) 
{
    YogPkg* bltins = YogObj_new(env, ENV_VM(env)->pkg_klass);
    YogPkg_define_method(env, bltins, "puts", bltins_puts, 0, 1, 0, 0, NULL);
    YogPkg_define_method(env, bltins, "raise", bltins_raise, 0, 0, 0, 0, "exc", NULL);

    return bltins;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
