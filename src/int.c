#include "yog/yog.h"

static YogVal 
int_to_s(YogEnv* env, YogVal recv, int argc, YogVal* args) 
{
    Yog_assert(env, YOGVAL_TYPE(recv) == VAL_INT, "Receiver isn't int.");

    YogString* s = YogString_new_format(env, "%d", YOGVAL_INT(recv));
    YogVal val = YogVal_gcobj(YOGGCOBJ(s));

    return val;
}

static YogVal 
int_add(YogEnv* env, YogVal recv, int argc, YogVal* args) 
{
    Yog_assert(env, YOGVAL_TYPE(recv) == VAL_INT, "Receiver isn't int.");

    Yog_assert(env, argc == 1, "");
    unsigned int i = 0;
    for (i = 0; i < argc; i++) {
        Yog_assert(env, args[i].type == VAL_INT, "");
    }

    int result = YOGVAL_INT(recv) + YOGVAL_INT(args[0]);

    return YogVal_int(result);
}

YogKlass* 
YogInt_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, ENV_VM(env)->obj_klass);
    YogObj* obj = YOGOBJ(klass);
    YogObj_define_method(env, obj, "+", int_add);
    YogObj_define_method(env, obj, "to_s", int_to_s);

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
