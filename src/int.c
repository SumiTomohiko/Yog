#include "yog/yog.h"

#define CHECK_TYPE(v) do { \
    Yog_assert(env, YOGVAL_TYPE(v) == VAL_INT, "Value isn't int."); \
} while (0)

#define CHECK_ARGS(self, v) do { \
    CHECK_TYPE(self); \
    CHECK_TYPE(v); \
} while (0)

static YogVal 
int_to_s(YogEnv* env, YogVal self, YogVal n)
{
    CHECK_TYPE(self);

    YogString* s = YogString_new_format(env, "%d", YOGVAL_INT(self));
    YogVal val = YogVal_obj(YOGBASICOBJ(s));

    return val;
}

static YogVal 
int_add(YogEnv* env, YogVal self, YogVal n)
{
    CHECK_ARGS(self, n);

    int result = YOGVAL_INT(self) + YOGVAL_INT(n);

    return YogVal_int(result);
}

static YogVal 
int_less(YogEnv* env, YogVal self, YogVal n)
{
    CHECK_ARGS(self, n);

    if (YOGVAL_INT(self) < YOGVAL_INT(n)) {
        return YogVal_true();
    }
    else {
        return YogVal_false();
    }
}

static YogVal 
int_times(YogEnv* env, YogVal self, YogVal block) 
{
    int n = YOGVAL_INT(self);
    unsigned int i = 0;
    unsigned int argc = 1;
    for (i = 0; i < n; i++) {
        YogVal args[1];
        args[0] = YogVal_int(i);
        YogThread_call_block(env, ENV_TH(env), block, argc, args);
    }

    return YogVal_nil();
}

YogKlass* 
YogInt_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, NULL, "Int", ENV_VM(env)->obj_klass);
#define DEFINE_METHOD(name, f) do { \
    YogKlass_define_method(env, klass, name, f, 0, 0, 0, -1, "n", NULL); \
} while (0)
    DEFINE_METHOD("+", int_add);
    DEFINE_METHOD("<", int_less);
#undef DEFINE_METHOD
    YogKlass_define_method(env, klass, "to_s", int_to_s, 0, 0, 0, 0, NULL);
    YogKlass_define_method(env, klass, "times", int_times, 1, 0, 0, 0, "block", NULL);

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
