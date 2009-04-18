#include <stdio.h>
#include "yog/array.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/package.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal 
raise(YogEnv* env)
{
    YogVal exc = ARG(env, 0);

    if (!YogVal_is_subklass_of(env, exc, env->vm->eException)) {
        YogVal receiver = env->vm->eException;
        YogVal args[] = { exc };
        PUSH_LOCALSX(env, array_sizeof(args), args);
        exc = YogEval_call_method(env, receiver, "new", 1, args);
        POP_LOCALS(env);
    }

    YogVal cur_frame = env->thread->cur_frame;
    env->thread->cur_frame = PTR_AS(YogFrame, cur_frame)->prev;

    YogError_raise(env, exc);

    /* NOTREACHED */
    return YNIL;
}

static YogVal 
puts_(YogEnv* env)
{
    YogVal vararg = ARG(env, 0);
    PUSH_LOCAL(env, vararg);

    unsigned int size = YogArray_size(env, vararg);
    if (0 < size) {
        unsigned int i = 0;
        for (i = 0; i < size; i++) {
            YogString* s = NULL;
            YogVal arg = YogArray_at(env, vararg, i);
            if (IS_OBJ_OF(cString, arg)) {
                s = PTR_AS(YogString, arg);
            }
            else {
                YogVal val = YogEval_call_method(env, arg, "to_s", 0, NULL);
                s = VAL2PTR(val);
            }
            printf("%s", PTR_AS(YogCharArray, s->body)->items);
            printf("\n");
        }
    }
    else {
        printf("\n");
    }

    POP_LOCALS(env);
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
    YogVal klass = env->vm->c; \
    YogObj_set_attr_id(env, bltins, PTR_AS(YogKlass, klass)->name, klass); \
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
