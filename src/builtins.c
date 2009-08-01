#include <stdio.h>
#include <string.h>
#include "yog/array.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/package.h"
#include "yog/property.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal 
raise(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal exc = YogArray_at(env, args, 0);

    if (!YogVal_is_subklass_of(env, exc, env->vm->eException)) {
        YogVal receiver = env->vm->eException;
        YogVal args[] = { exc };
        PUSH_LOCALSX(env, array_sizeof(args), args);
        exc = YogEval_call_method(env, receiver, "new", 1, args);
        POP_LOCALS(env);
    }

    YogVal cur_frame = PTR_AS(YogThread, env->thread)->cur_frame;
    YogVal prev_frame = PTR_AS(YogFrame, cur_frame)->prev;
    PTR_AS(YogThread, env->thread)->cur_frame = prev_frame;

    YogError_raise(env, exc);

    /* NOTREACHED */
    RETURN(env, YNIL);
}

static YogVal 
puts_(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);

    uint_t size = YogArray_size(env, args);
    if (0 < size) {
        uint_t i = 0;
        for (i = 0; i < size; i++) {
            YogString* s = NULL;
            YogVal arg = YogArray_at(env, args, i);
            if (IS_PTR(arg) && IS_OBJ_OF(env, arg, cString)) {
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

    RETURN(env, YNIL);
}

static YogVal
import_package(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YogVal name = YogArray_at(env, args, 0);
    return YogVM_import_package(env, env->vm, VAL2ID(name));
}

static YogVal
property(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal getter = YUNDEF;
    YogVal setter = YUNDEF;
    YogVal prop = YUNDEF;
    PUSH_LOCALS3(env, getter, setter, prop);

    getter = YogArray_at(env, args, 0);
    if (1 < YogArray_size(env, args)) {
        setter = YogArray_at(env, args, 1);
    }

    prop = YogProperty_new(env);
    PTR_AS(YogProperty, prop)->getter = getter;
    PTR_AS(YogProperty, prop)->setter = setter;

    RETURN(env, prop);
}

YogVal 
YogBuiltins_new(YogEnv* env) 
{
    YogVal bltins = YogPackage_new(env);
    PUSH_LOCAL(env, bltins);

    YogPackage_define_method(env, bltins, "puts", puts_, 0, 1, 0, 0, NULL);
    YogPackage_define_method(env, bltins, "raise", raise, 0, 0, 0, 0, "exc", NULL);
    YogPackage_define_method(env, bltins, "import_package", import_package, 0, 0, 0, 0, "package", NULL);
    YogPackage_define_method(env, bltins, "property", property, 0, 0, 0, 0, NULL);

#define REGISTER_KLASS(c)   do { \
    YogVal klass = env->vm->c; \
    YogObj_set_attr_id(env, bltins, PTR_AS(YogKlass, klass)->name, klass); \
} while (0)
    REGISTER_KLASS(cObject);
    REGISTER_KLASS(cThread);
    REGISTER_KLASS(eException);
#undef REGISTER_KLASS

    POP_LOCALS(env);
    return bltins;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
