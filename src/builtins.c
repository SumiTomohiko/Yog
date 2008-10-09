#include <stdio.h>
#include "yog/yog.h"

static YogVal 
bltins_p(YogEnv* env, YogVal recv, int argc, YogVal* args) 
{
    /* TODO */
    return YogVal_nil();
}

static YogVal 
bltins_puts(YogEnv* env, YogVal recv, int argc, YogVal* args) 
{
    if (0 < argc) {
        unsigned int i = 0;
        for (i = 0; i < argc; i++) {
            YogVal val = YogThread_call_method(env, args[i], "to_s", 0, NULL);
            Yog_assert(env, YOGVAL_TYPE(val) == VAL_GCOBJ, "Can't get string by to_s.");
            YogGCObj* gcobj = YOGVAL_GCOBJ(val);
            Yog_assert(env, gcobj->type == GCOBJ_STRING, "Object isn't string.");
            YogString* s = (YogString*)gcobj;
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
    YogObj_define_method(env, bltins, "p", bltins_p);

    return bltins;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
