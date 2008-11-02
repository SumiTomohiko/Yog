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

YogKlass* 
YogInt_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, ENV_VM(env)->obj_klass);
#define DEFINE_METHOD(name, f) do { \
    YogKlass_define_method(env, klass, name, f, 0, 0, 0, -1, "n", NULL); \
} while (0)
    DEFINE_METHOD("+", int_add);
    DEFINE_METHOD("<", int_less);
#undef DEFINE_METHOD
    YogKlass_define_method(env, klass, "to_s", int_to_s, 0, 0, 0, 0, NULL);

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
