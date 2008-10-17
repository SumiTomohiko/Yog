#include "yog/yog.h"

#define CHECK_TYPE(v) do { \
    Yog_assert(env, YOGVAL_TYPE(recv) == VAL_INT, "Value isn't int."); \
} while (0)

#define CHECK_ARGS(argc, args) do { \
    unsigned int i = 0; \
    for (i = 0; i < argc; i++) { \
        CHECK_TYPE(args[i]); \
    } \
} while (0)

#define CHECK_ARGC(expected, argc) do { \
    Yog_assert(env, expected == argc, ""); \
} while (0)

#define CHECK_BIN_ARGS(recv, argc, args) do { \
    CHECK_TYPE(recv); \
    CHECK_ARGC(1, argc); \
    CHECK_ARGS(argc, args); \
} while (0)

static YogVal 
int_to_s(YogEnv* env, YogVal recv, int argc, YogVal* args) 
{
    CHECK_TYPE(recv);

    YogString* s = YogString_new_format(env, "%d", YOGVAL_INT(recv));
    YogVal val = YogVal_obj(YOGBASICOBJ(s));

    return val;
}

static YogVal 
int_add(YogEnv* env, YogVal recv, int argc, YogVal* args) 
{
    CHECK_BIN_ARGS(recv, argc, args);

    int result = YOGVAL_INT(recv) + YOGVAL_INT(args[0]);

    return YogVal_int(result);
}

static YogVal 
int_less(YogEnv* env, YogVal recv, int argc, YogVal* args) 
{
    CHECK_BIN_ARGS(recv, argc, args);

    if (YOGVAL_INT(recv) < YOGVAL_INT(args[0])) {
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
    YogObj* obj = YOGOBJ(klass);
    YogObj_define_method(env, obj, "+", int_add);
    YogObj_define_method(env, obj, "<", int_less);
    YogObj_define_method(env, obj, "to_s", int_to_s);

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
