#include <stdio.h>
#include "yog/yog.h"

static YogVal 
raise(YogEnv* env)
{
    YogVal exc = ARG(env, 0);

    if (!YogVal_is_subklass_of(env, exc, ENV_VM(env)->eException)) {
        YogVal receiver = OBJ2VAL(ENV_VM(env)->eException);
        YogVal args[1] = { exc };
        exc = YogThread_call_method(env, ENV_TH(env), receiver, "new", 1, args);
    }

    ENV_TH(env)->cur_frame = ENV_TH(env)->cur_frame->prev;

    YogError_raise(env, exc);

    /* NOTREACHED */
    return YNIL;
}

static YogVal 
puts_(YogEnv* env)
{
    YogArray* vararg = OBJ_AS(YogArray, ARG(env, 0));
    unsigned int size = YogArray_size(env, vararg);
    if (0 < size) {
        unsigned int i = 0;
        for (i = 0; i < size; i++) {
            YogString* s = NULL;
            YogVal arg = YogArray_at(env, vararg, i);
            if (IS_OBJ(arg) && (VAL2OBJ(arg)->klass == ENV_VM(env)->cString)) {
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

YogPackage* 
YogBuiltins_new(YogEnv* env) 
{
    YogPackage* bltins = YogPackage_new(env);
    YogPackage_define_method(env, bltins, "puts", puts_, 0, 1, 0, 0, NULL);
    YogPackage_define_method(env, bltins, "raise", raise, 0, 0, 0, 0, "exc", NULL);

#define REGISTER_KLASS(c)   do { \
    YogKlass* klass = ENV_VM(env)->c; \
    YogVal val = OBJ2VAL(klass); \
    YogObj_set_attr_id(env, YOGOBJ(bltins), klass->name, val); \
} while (0)
    REGISTER_KLASS(cObject);
    REGISTER_KLASS(eException);
#undef REGISTER_KLASS

    return bltins;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
