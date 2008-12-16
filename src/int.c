#include "yog/error.h"
#include "yog/yog.h"

#define CHECK_TYPE(v) do { \
    YOG_ASSERT(env, VAL_TYPE(v) == VAL_INT, "Value isn't int."); \
} while (0)

#define CHECK_ARGS(self, v) do { \
    CHECK_TYPE(self); \
    CHECK_TYPE(v); \
} while (0)

static YogVal 
to_s(YogEnv* env)
{
    YogVal self = SELF(env);

    CHECK_TYPE(self);

    YogString* s = YogString_new_format(env, "%d", VAL2INT(self));
    YogVal val = OBJ2VAL(s);

    return val;
}

static YogVal 
add(YogEnv* env)
{
    YogVal self = SELF(env);
    YogVal n = ARG(env, 0);

    CHECK_ARGS(self, n);

    int result = VAL2INT(self) + VAL2INT(n);

    return INT2VAL(result);
}

static YogVal 
less(YogEnv* env)
{
    YogVal self = SELF(env);
    YogVal n = ARG(env, 0);

    CHECK_ARGS(self, n);

    if (VAL2INT(self) < VAL2INT(n)) {
        return YTRUE;
    }
    else {
        return YFALSE;
    }
}

static YogVal 
times(YogEnv* env)
{
    YogVal self = SELF(env);
    int n = VAL2INT(self);

    unsigned int i = 0;
    unsigned int argc = 1;
    for (i = 0; i < n; i++) {
        YogVal args[argc];
        args[0] = INT2VAL(i);

        YogVal block = ARG(env, 0);

        YogThread_call_block(env, ENV_TH(env), block, argc, args);
    }

    return YNIL;
}

YogKlass* 
YogInt_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, "Int", ENV_VM(env)->cObject);
    FRAME_DECL_LOCAL(env, klass_idx, OBJ2VAL(klass));

#define DEFINE_METHOD(name, f)  do { \
    FRAME_LOCAL_OBJ(env, klass, YogKlass, klass_idx); \
    YogKlass_define_method(env, klass, name, f, 0, 0, 0, -1, "n", NULL); \
} while (0)
    DEFINE_METHOD("+", add);
    DEFINE_METHOD("<", less);
#undef DEFINE_METHOD

    FRAME_LOCAL_OBJ(env, klass, YogKlass, klass_idx);
    YogKlass_define_method(env, klass, "to_s", to_s, 0, 0, 0, 0, NULL);

    FRAME_LOCAL_OBJ(env, klass, YogKlass, klass_idx);
    YogKlass_define_method(env, klass, "times", times, 1, 0, 0, 0, "block", NULL);

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
