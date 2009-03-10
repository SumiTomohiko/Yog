#include <stdio.h>
#include "yog/error.h"
#include "yog/yog.h"

static YogVal 
raise(YogEnv* env)
{
    YogVal exc = ARG(env, 0);

    if (!YogVal_is_subklass_of(env, exc, ENV_VM(env)->eException)) {
        YogVal receiver = ENV_VM(env)->eException;
        YogVal args[] = { exc };
        exc = YogThread_call_method(env, ENV_TH(env), receiver, "new", 1, args);
    }

    ENV_TH(env)->cur_frame = PTR_AS(YogFrame, ENV_TH(env)->cur_frame)->prev;

    YogError_raise(env, exc);

    /* NOTREACHED */
    return YNIL;
}

static YogVal 
puts_(YogEnv* env)
{
    YogVal vararg = ARG(env, 0);
    unsigned int size = YogArray_size(env, vararg);
    if (0 < size) {
        unsigned int i = 0;
        for (i = 0; i < size; i++) {
            YogString* s = NULL;
            YogVal arg = YogArray_at(env, vararg, i);
            if (IS_OBJ(arg) && (VAL2PTR(VAL2OBJ(arg)->klass) == VAL2PTR(ENV_VM(env)->cString))) {
                s = OBJ_AS(YogString, arg);
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

    return YNIL;
}

#include "yog/st.h"

YogVal 
YogBuiltins_new(YogEnv* env) 
{
    YogVal bltins = YogPackage_new(env);
    PUSH_LOCAL(env, bltins);

    YogPackage_define_method(env, bltins, "puts", puts_, 0, 1, 0, 0, NULL);
    YogPackage_define_method(env, bltins, "raise", raise, 0, 0, 0, 0, "exc", NULL);

#define REGISTER_KLASS(c)   do { \
    YogVal klass = ENV_VM(env)->c; \
    YogObj_set_attr_id(env, bltins, OBJ_AS(YogKlass, klass)->name, klass); \
} while (0)
    REGISTER_KLASS(cObject);
    REGISTER_KLASS(eException);
#undef REGISTER_KLASS

    POP_LOCALS(env);
    return bltins;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
